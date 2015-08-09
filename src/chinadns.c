/*  ChinaDNS
    Copyright (C) 2015 clowwindy

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <fcntl.h>
#include <netdb.h>
#include <resolv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/param.h>

#include "local_ns_parser.h"

#include "config.h"

typedef struct {
  uint16_t id;
  struct timeval ts;
  char *buf;
  size_t buflen;
  struct sockaddr *addr;
  socklen_t addrlen;
} delay_buf_t;

typedef struct {
  uint16_t id;
  uint16_t old_id;
  struct sockaddr *addr;
  socklen_t addrlen;
} id_addr_t;

typedef struct {
  int entries;
  struct in_addr *ips;
} ip_list_t;

typedef struct {
  struct in_addr net;
  in_addr_t mask;
} net_mask_t;

typedef struct {
  int entries;
  net_mask_t *nets;
} net_list_t;


// avoid malloc and free
#define BUF_SIZE 512
static char global_buf[BUF_SIZE];
static char compression_buf[BUF_SIZE];
static int verbose = 0;
static int compression = 0;
static int bidirectional = 0;

static const char *default_dns_servers =
"114.114.114.114,223.5.5.5,8.8.8.8,8.8.4.4,208.67.222.222:443,208.67.222.222:5353";
static char *dns_servers = NULL;
static int dns_servers_len;
static int has_chn_dns;
static id_addr_t *dns_server_addrs;

static int parse_args(int argc, char **argv);

static int setnonblock(int sock);
static int resolve_dns_servers();

static const char *default_listen_addr = "0.0.0.0";
static const char *default_listen_port = "53";

static char *listen_addr = NULL;
static char *listen_port = NULL;

static char *ip_list_file = NULL;
static ip_list_t ip_list;
static int parse_ip_list();

static char *chnroute_file = NULL;
static net_list_t chnroute_list;
static int parse_chnroute();
static int test_ip_in_list(struct in_addr ip, const net_list_t *netlist);

static int dns_init_sockets();
static void dns_handle_local();
static void dns_handle_remote();

static const char *hostname_from_question(ns_msg msg);
static int should_filter_query(ns_msg msg, struct in_addr dns_addr);

static void queue_add(id_addr_t id_addr);
static id_addr_t *queue_lookup(uint16_t id);

#define ID_ADDR_QUEUE_LEN 128
// use a queue instead of hash here since it's not long
static id_addr_t id_addr_queue[ID_ADDR_QUEUE_LEN];
static int id_addr_queue_pos = 0;

#define EMPTY_RESULT_DELAY 0.3f
#define DELAY_QUEUE_LEN 128
static delay_buf_t delay_queue[DELAY_QUEUE_LEN];
static void schedule_delay(uint16_t query_id, const char *buf, size_t buflen,
                           struct sockaddr *addr, socklen_t addrlen);
static void check_and_send_delay();
static void free_delay(int pos);
// next position for first, not used
static int delay_queue_first = 0;
// current position for last, used
static int delay_queue_last = 0;
static float empty_result_delay = EMPTY_RESULT_DELAY;

static int local_sock;
static int remote_sock;

static void usage(void);

#define __LOG(o, t, v, s...) do {                                   \
  time_t now;                                                       \
  time(&now);                                                       \
  char *time_str = ctime(&now);                                     \
  time_str[strlen(time_str) - 1] = '\0';                            \
  if (t == 0) {                                                     \
    if (stdout != o || verbose) {                                   \
      fprintf(o, "%s ", time_str);                                  \
      fprintf(o, s);                                                \
      fflush(o);                                                    \
    }                                                               \
  } else if (t == 1) {                                              \
    fprintf(o, "%s %s:%d ", time_str, __FILE__, __LINE__);          \
    perror(v);                                                      \
  }                                                                 \
} while (0)

#define LOG(s...) __LOG(stdout, 0, "_", s)
#define ERR(s) __LOG(stderr, 1, s, "_")
#define VERR(s...) __LOG(stderr, 0, "_", s)

#ifdef DEBUG
#define DLOG(s...) LOG(s)
void __gcov_flush(void);
static void gcov_handler(int signum)
{
  __gcov_flush();
  exit(1);
}
#else
#define DLOG(s...)
#endif

int main(int argc, char **argv) {
  fd_set readset, errorset;
  int max_fd;

#ifdef DEBUG
  signal(SIGTERM, gcov_handler);
#endif

  memset(&id_addr_queue, 0, sizeof(id_addr_queue));
  if (0 != parse_args(argc, argv))
    return EXIT_FAILURE;
  if (!compression)
    memset(&delay_queue, 0, sizeof(delay_queue));
  if (0 != parse_ip_list())
    return EXIT_FAILURE;
  if (0 != parse_chnroute())
    return EXIT_FAILURE;
  if (0 != resolve_dns_servers())
    return EXIT_FAILURE;
  if (0 != dns_init_sockets())
    return EXIT_FAILURE;

  max_fd = MAX(local_sock, remote_sock) + 1;
  while (1) {
    FD_ZERO(&readset);
    FD_ZERO(&errorset);
    FD_SET(local_sock, &readset);
    FD_SET(local_sock, &errorset);
    FD_SET(remote_sock, &readset);
    FD_SET(remote_sock, &errorset);
    struct timeval timeout = {
      .tv_sec = 0,
      .tv_usec = 50 * 1000,
    };
    if (-1 == select(max_fd, &readset, NULL, &errorset, &timeout)) {
      ERR("select");
      return EXIT_FAILURE;
    }
    check_and_send_delay();
    if (FD_ISSET(local_sock, &errorset)) {
      // TODO getsockopt(..., SO_ERROR, ...);
      VERR("local_sock error\n");
      return EXIT_FAILURE;
    }
    if (FD_ISSET(remote_sock, &errorset)) {
      // TODO getsockopt(..., SO_ERROR, ...);
      VERR("remote_sock error\n");
      return EXIT_FAILURE;
    }
    if (FD_ISSET(local_sock, &readset))
      dns_handle_local();
    if (FD_ISSET(remote_sock, &readset))
      dns_handle_remote();
  }
  return EXIT_SUCCESS;
}

static int setnonblock(int sock) {
  int flags;
  flags = fcntl(sock, F_GETFL, 0);
  if (flags == -1) {
    ERR("fcntl");
    return -1;
  }
  if (-1 == fcntl(sock, F_SETFL, flags | O_NONBLOCK)) {
    ERR("fcntl");
    return -1;
  }
  return 0;
}

static int parse_args(int argc, char **argv) {
  int ch;
  while ((ch = getopt(argc, argv, "hb:p:s:l:c:y:dmvV")) != -1) {
    switch (ch) {
      case 'h':
        usage();
        exit(0);
      case 'b':
        listen_addr = strdup(optarg);
        break;
      case 'p':
        listen_port = strdup(optarg);
        break;
      case 's':
        dns_servers = strdup(optarg);
        break;
      case 'c':
        chnroute_file = strdup(optarg);
        break;
      case 'l':
        ip_list_file = strdup(optarg);
        break;
      case 'y':
        empty_result_delay = atof(optarg);
        break;
      case 'd':
        bidirectional = 1;
        break;
      case 'm':
        compression = 1;
        break;
      case 'v':
        verbose = 1;
        break;
      case 'V':
        printf("ChinaDNS %s\n", PACKAGE_VERSION);
        exit(0);
      default:
        usage();
        exit(1);
    }
  }
  if (dns_servers == NULL) {
    dns_servers = strdup(default_dns_servers);
  }
  if (listen_addr == NULL) {
    listen_addr = strdup(default_listen_addr);
  }
  if (listen_port == NULL) {
    listen_port = strdup(default_listen_port);
  }
  argc -= optind;
  argv += optind;
  return 0;
}

static int resolve_dns_servers() {
  struct addrinfo hints;
  struct addrinfo *addr_ip;
  char* token;
  int r;
  int i = 0;
  char *pch = strchr(dns_servers, ',');
  has_chn_dns = 0;
  int has_foreign_dns = 0;
  dns_servers_len = 1;
  if (compression) {
    if (!chnroute_file) {
      VERR("Chnroutes are necessary when using DNS compression pointer mutation\n");
      return -1;
    }
  }
  while (pch != NULL) {
    dns_servers_len++;
    pch = strchr(pch + 1, ',');
  }
  dns_server_addrs = calloc(dns_servers_len, sizeof(id_addr_t));

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
  token = strtok(dns_servers, ",");
  while (token) {
    char *port;
    memset(global_buf, 0, BUF_SIZE);
    strncpy(global_buf, token, BUF_SIZE - 1);
    port = (strrchr(global_buf, ':'));
    if (port) {
      *port = '\0';
      port++;
    } else {
      port = "53";
    }
    if (0 != (r = getaddrinfo(global_buf, port, &hints, &addr_ip))) {
      VERR("%s:%s\n", gai_strerror(r), token);
      return -1;
    }
    if (compression) {
      if (test_ip_in_list(((struct sockaddr_in *)addr_ip->ai_addr)->sin_addr,
                          &chnroute_list)) {
        dns_server_addrs[has_chn_dns].addr = addr_ip->ai_addr;
        dns_server_addrs[has_chn_dns].addrlen = addr_ip->ai_addrlen;
        has_chn_dns++;
      } else {
        has_foreign_dns++;
        dns_server_addrs[dns_servers_len - has_foreign_dns].addr = addr_ip->ai_addr;
        dns_server_addrs[dns_servers_len - has_foreign_dns].addrlen = addr_ip->ai_addrlen;
      }
      token = strtok(0, ",");
    } else {
      dns_server_addrs[i].addr = addr_ip->ai_addr;
      dns_server_addrs[i].addrlen = addr_ip->ai_addrlen;
      i++;
      token = strtok(0, ",");
      if (chnroute_file) {
        if (test_ip_in_list(((struct sockaddr_in *)addr_ip->ai_addr)->sin_addr,
                            &chnroute_list)) {
          has_chn_dns = 1;
        } else {
          has_foreign_dns = 1;
        }
      }
    }
  }
  if (chnroute_file) {
    if (!(has_chn_dns && has_foreign_dns)) {
      if (compression) {
        VERR("You should have at least one Chinese DNS and one foreign DNS when "
             "using DNS compression pointer mutation\n");
        return -1;
      } else {
        VERR("You should have at least one Chinese DNS and one foreign DNS when "
             "chnroutes is enabled\n");
        return 0;
      }
    }
  }
  return 0;
}

static int cmp_in_addr(const void *a, const void *b) {
  struct in_addr *ina = (struct in_addr *)a;
  struct in_addr *inb = (struct in_addr *)b;
  if (ina->s_addr == inb->s_addr)
    return 0;
  if (ina->s_addr > inb->s_addr)
    return 1;
  return -1;
}

static int parse_ip_list() {
  FILE *fp;
  char line_buf[32];
  char *line = NULL;
  size_t len = sizeof(line_buf);
  ssize_t read;
  ip_list.entries = 0;
  int i = 0;

  if (ip_list_file == NULL)
    return 0;

  fp = fopen(ip_list_file, "rb");
  if (fp == NULL) {
    ERR("fopen");
    VERR("Can't open ip list: %s\n", ip_list_file);
    return -1;
  }
  while ((line = fgets(line_buf, len, fp))) {
    ip_list.entries++;
  }

  ip_list.ips = calloc(ip_list.entries, sizeof(struct in_addr));
  if (0 != fseek(fp, 0, SEEK_SET)) {
    VERR("fseek");
    return -1;
  }
  while ((line = fgets(line_buf, len, fp))) {
    char *sp_pos;
    sp_pos = strchr(line, '\r');
    if (sp_pos) *sp_pos = 0;
    sp_pos = strchr(line, '\n');
    if (sp_pos) *sp_pos = 0;
    inet_aton(line, &ip_list.ips[i]);
    i++;
  }

  qsort(ip_list.ips, ip_list.entries, sizeof(struct in_addr), cmp_in_addr);
  fclose(fp);
  return 0;
}

static int cmp_net_mask(const void *a, const void *b) {
  net_mask_t *neta = (net_mask_t *)a;
  net_mask_t *netb = (net_mask_t *)b;
  if (neta->net.s_addr == netb->net.s_addr)
    return 0;
  // TODO: pre ntohl
  if (ntohl(neta->net.s_addr) > ntohl(netb->net.s_addr))
    return 1;
  return -1;
}

static int parse_chnroute() {
  FILE *fp;
  char line_buf[32];
  char *line;
  size_t len = sizeof(line_buf);
  ssize_t read;
  char net[32];
  chnroute_list.entries = 0;
  int i = 0;

  if (chnroute_file == NULL) {
    VERR("CHNROUTE_FILE not specified, CHNRoute is disabled\n");
    return 0;
  }

  fp = fopen(chnroute_file, "rb");
  if (fp == NULL) {
    ERR("fopen");
    VERR("Can't open chnroute: %s\n", chnroute_file);
    return -1;
  }
  while ((line = fgets(line_buf, len, fp))) {
    chnroute_list.entries++;
  }

  chnroute_list.nets = calloc(chnroute_list.entries, sizeof(net_mask_t));
  if (0 != fseek(fp, 0, SEEK_SET)) {
    VERR("fseek");
    return -1;
  }
  while ((line = fgets(line_buf, len, fp))) {
    char *sp_pos;
    sp_pos = strchr(line, '\r');
    if (sp_pos) *sp_pos = 0;
    sp_pos = strchr(line, '\n');
    if (sp_pos) *sp_pos = 0;
    sp_pos = strchr(line, '/');
    if (sp_pos) {
      *sp_pos = 0;
      chnroute_list.nets[i].mask = (1 << (32 - atoi(sp_pos + 1))) - 1;
    } else {
      chnroute_list.nets[i].mask = UINT32_MAX;
    }
    if (0 == inet_aton(line, &chnroute_list.nets[i].net)) {
      VERR("invalid addr %s in %s:%d\n", line, chnroute_file, i + 1);
      return 1;
    }
    i++;
  }

  qsort(chnroute_list.nets, chnroute_list.entries, sizeof(net_mask_t),
        cmp_net_mask);

  fclose(fp);
  return 0;
}

static int test_ip_in_list(struct in_addr ip, const net_list_t *netlist) {
  // binary search
  int l = 0, r = netlist->entries - 1;
  int m, cmp;
  if (netlist->entries == 0)
    return 0;
  net_mask_t ip_net;
  ip_net.net = ip;
  while (l != r) {
    m = (l + r) / 2;
    cmp = cmp_net_mask(&ip_net, &netlist->nets[m]);
    if (cmp == -1) {
      if (r != m)
        r = m;
      else
        break;
    } else {
      if (l != m)
        l = m;
      else
        break;
    }
    DLOG("l=%d, r=%d\n", l, r);
    DLOG("%s, %d\n", inet_ntoa(netlist->nets[m].net),
         netlist->nets[m].mask);
  }
  DLOG("result: %x\n",
       (ntohl(netlist->nets[l].net.s_addr) ^ ntohl(ip.s_addr)));
  DLOG("mask: %x\n", (UINT32_MAX - netlist->nets[l].mask));
  if ((ntohl(netlist->nets[l].net.s_addr) ^ ntohl(ip.s_addr)) &
      (UINT32_MAX ^ netlist->nets[l].mask)) {
    return 0;
  }
  return 1;
}

static int dns_init_sockets() {
  struct addrinfo hints;
  struct addrinfo *addr_ip;
  int r;

  local_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (0 != setnonblock(local_sock))
    return -1;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  if (0 != (r = getaddrinfo(listen_addr, listen_port, &hints, &addr_ip))) {
    VERR("%s:%s:%s\n", gai_strerror(r), listen_addr, listen_port);
    return -1;
  }
  if (0 != bind(local_sock, addr_ip->ai_addr, addr_ip->ai_addrlen)) {
    ERR("bind");
    VERR("Can't bind address %s:%s\n", listen_addr, listen_port);
    return -1;
  }
  freeaddrinfo(addr_ip);
  remote_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (0 != setnonblock(remote_sock))
    return -1;
  return 0;
}

static void dns_handle_local() {
  struct sockaddr *src_addr = malloc(sizeof(struct sockaddr));
  socklen_t src_addrlen = sizeof(struct sockaddr);
  uint16_t query_id;
  ssize_t len;
  int i;
  int sended = 0;
  const char *question_hostname;
  ns_msg msg;
  len = recvfrom(local_sock, global_buf, BUF_SIZE, 0, src_addr, &src_addrlen);
  if (len > 0) {
    if (local_ns_initparse((const u_char *)global_buf, len, &msg) < 0) {
      ERR("local_ns_initparse");
      free(src_addr);
      return;
    }
    // parse DNS query id
    // TODO generate id for each request to avoid conflicts
    query_id = ns_msg_id(msg);
    question_hostname = hostname_from_question(msg);
    LOG("request %s\n", question_hostname);

    // assign a new id
    uint16_t new_id;
    do {
      struct timeval tv;
      gettimeofday(&tv, 0);
      int randombits = (tv.tv_sec << 8) ^ tv.tv_usec;
      new_id = randombits & 0xffff;
    } while (queue_lookup(new_id));

    uint16_t ns_new_id = htons(new_id);
    memcpy(global_buf, &ns_new_id, 2);

    id_addr_t id_addr;
    id_addr.id = new_id;
    id_addr.old_id = query_id;

    id_addr.addr = src_addr;
    id_addr.addrlen = src_addrlen;
    queue_add(id_addr);
    if (compression) {
      if (len > 16) {
        size_t off = 12;
        int ended = 0;
        while (off < len - 4) {
          if (global_buf[off] & 0xc0)
            break;
          if (global_buf[off] == 0) {
            ended = 1;
            off ++;
            break;
          }
          off += 1 + global_buf[off];
        }
        if (ended) {
          memcpy(compression_buf, global_buf, off-1);
          memcpy(compression_buf + off + 1, global_buf + off, len - off);
          compression_buf[off-1] = '\xc0';
          compression_buf[off] = '\x04';
          for (i = 0; i < has_chn_dns; i++) {
            if (-1 == sendto(remote_sock, global_buf, len, 0,
                             dns_server_addrs[i].addr,
                             dns_server_addrs[i].addrlen))
              ERR("sendto");
          }
          for (i =  has_chn_dns; i < dns_servers_len; i++) {
            if (-1 == sendto(remote_sock, compression_buf, len + 1, 0,
                             dns_server_addrs[i].addr,
                             dns_server_addrs[i].addrlen))
              ERR("sendto");
            sended = 1;
          }
        }
      }
    }
    if (!sended) {
      for (i = 0; i < dns_servers_len; i++) {
        if (-1 == sendto(remote_sock, global_buf, len, 0,
                         dns_server_addrs[i].addr,
                         dns_server_addrs[i].addrlen))
          ERR("sendto");
      }
    }
  }
  else
    ERR("recvfrom");
}

static void dns_handle_remote() {
  struct sockaddr *src_addr = malloc(sizeof(struct sockaddr));
  socklen_t src_len = sizeof(struct sockaddr);
  uint16_t query_id;
  ssize_t len;
  const char *question_hostname;
  int r;
  ns_msg msg;
  len = recvfrom(remote_sock, global_buf, BUF_SIZE, 0, src_addr, &src_len);
  if (len > 0) {
    if (local_ns_initparse((const u_char *)global_buf, len, &msg) < 0) {
      ERR("local_ns_initparse");
      free(src_addr);
      return;
    }
    // parse DNS query id
    query_id = ns_msg_id(msg);
    question_hostname = hostname_from_question(msg);
    if (question_hostname) {
      LOG("response %s from %s:%d - ", question_hostname,
          inet_ntoa(((struct sockaddr_in *)src_addr)->sin_addr),
          htons(((struct sockaddr_in *)src_addr)->sin_port));
    }
    id_addr_t *id_addr = queue_lookup(query_id);
    if (id_addr) {
      id_addr->addr->sa_family = AF_INET;
      uint16_t ns_old_id = htons(id_addr->old_id);
      memcpy(global_buf, &ns_old_id, 2);
      r = should_filter_query(msg, ((struct sockaddr_in *)src_addr)->sin_addr);
      if (r == 0) {
        if (verbose)
          printf("pass\n");
        if (-1 == sendto(local_sock, global_buf, len, 0, id_addr->addr,
                         id_addr->addrlen))
          ERR("sendto");
      } else if (r == -1) {
        schedule_delay(query_id, global_buf, len, id_addr->addr,
                       id_addr->addrlen);
        if (verbose)
          printf("delay\n");
      } else {
        if (verbose)
          printf("filter\n");
      }
    } else {
      if (verbose)
        printf("skip\n");
    }
    free(src_addr);
  }
  else
    ERR("recvfrom");
}

static void queue_add(id_addr_t id_addr) {
  id_addr_queue_pos = (id_addr_queue_pos + 1) % ID_ADDR_QUEUE_LEN;
  // free next hole
  id_addr_t old_id_addr = id_addr_queue[id_addr_queue_pos];
  free(old_id_addr.addr);
  id_addr_queue[id_addr_queue_pos] = id_addr;
}

static id_addr_t *queue_lookup(uint16_t id) {
  int i;
  for (i = 0; i < ID_ADDR_QUEUE_LEN; i++) {
    if (id_addr_queue[i].id == id)
      return id_addr_queue + i;
  }
  return NULL;
}

static char *hostname_buf = NULL;
static size_t hostname_buflen = 0;
static const char *hostname_from_question(ns_msg msg) {
  ns_rr rr;
  int rrnum, rrmax;
  const char *result;
  int result_len;
  rrmax = ns_msg_count(msg, ns_s_qd);
  if (rrmax == 0)
    return NULL;
  for (rrnum = 0; rrnum < rrmax; rrnum++) {
    if (local_ns_parserr(&msg, ns_s_qd, rrnum, &rr)) {
      ERR("local_ns_parserr");
      return NULL;
    }
    result = ns_rr_name(rr);
    result_len = strlen(result) + 1;
    if (result_len > hostname_buflen) {
      hostname_buflen = result_len << 1;
      hostname_buf = realloc(hostname_buf, hostname_buflen);
    }
    memcpy(hostname_buf, result, result_len);
    return hostname_buf;
  }
  return NULL;
}

static int should_filter_query(ns_msg msg, struct in_addr dns_addr) {
  ns_rr rr;
  int rrnum, rrmax;
  void *r;
  // TODO cache result for each dns server
  int dns_is_chn = 0;
  int dns_is_foreign = 0;
  if (chnroute_file && (dns_servers_len > 1)) {
    dns_is_chn = test_ip_in_list(dns_addr, &chnroute_list);
    dns_is_foreign = !dns_is_chn;
  }
  rrmax = ns_msg_count(msg, ns_s_an);
  if (rrmax == 0) {
    if (compression) {
      // Wait for foreign dns
      if (dns_is_chn) {
        return 1;
      } else {
        return 0;
      }
    }
    return -1;
  }
  for (rrnum = 0; rrnum < rrmax; rrnum++) {
    if (local_ns_parserr(&msg, ns_s_an, rrnum, &rr)) {
      ERR("local_ns_parserr");
      return 0;
    }
    u_int type;
    const u_char *rd;
    type = ns_rr_type(rr);
    rd = ns_rr_rdata(rr);
    if (type == ns_t_a) {
      if (verbose)
        printf("%s, ", inet_ntoa(*(struct in_addr *)rd));
      if (!compression) {
        r = bsearch(rd, ip_list.ips, ip_list.entries, sizeof(struct in_addr),
                    cmp_in_addr);
        if (r) {
          return 1;
        }
      }
      if (test_ip_in_list(*(struct in_addr *)rd, &chnroute_list)) {
        // result is chn
        if (dns_is_foreign) {
          if (bidirectional) {
            // filter DNS result from foreign dns if result is inside chn
            return 1;
          }
        }
      } else {
        // result is foreign
        if (dns_is_chn) {
          // filter DNS result from chn dns if result is outside chn
          return 1;
        }
      }
    } else if (type == ns_t_aaaa || type == ns_t_ptr) {
      // if we've got an IPv6 result or a PTR result, pass
      return 0;
    }
  }
  if (rrmax == 1) {
    if (compression) {
      return 0;
    } else {
      return -1;
    }
  }
  return 0;
}

static void schedule_delay(uint16_t query_id, const char *buf, size_t buflen,
                           struct sockaddr *addr, socklen_t addrlen) {
  int i;
  int found = 0;
  struct timeval now;
  gettimeofday(&now, 0);

  delay_buf_t *delay_buf = &delay_queue[delay_queue_last];

  // first search for existed item with query_id and replace it
  for (i = delay_queue_first;
       i != delay_queue_last;
       i = (i + 1) % DELAY_QUEUE_LEN) {
    delay_buf_t *delay_buf2 = &delay_queue[i];
    if (delay_buf2->id == query_id) {
      free_delay(i);
      delay_buf = &delay_queue[i];
      found = 1;
    }
  }

  delay_buf->id = query_id;
  delay_buf->ts = now;
  delay_buf->buf = malloc(buflen);
  memcpy(delay_buf->buf, buf, buflen);
  delay_buf->buflen = buflen;
  delay_buf->addr = malloc(addrlen);
  memcpy(delay_buf->addr, addr, addrlen);
  delay_buf->addrlen = addrlen;

  // then append to queue
  if (!found) {
    delay_queue_last = (delay_queue_last + 1) % DELAY_QUEUE_LEN;
    if (delay_queue_last == delay_queue_first) {
      free_delay(delay_queue_first);
      delay_queue_first = (delay_queue_first + 1) % DELAY_QUEUE_LEN;
    }
  }
}

float time_diff(struct timeval t0, struct timeval t1) {
  return (t1.tv_sec - t0.tv_sec) +
      (t1.tv_usec - t0.tv_usec) / 1000000.0f;
}

static void check_and_send_delay() {
  struct timeval now;
  int i;
  gettimeofday(&now, 0);
  for (i = delay_queue_first;
       i != delay_queue_last;
       i = (i + 1) % DELAY_QUEUE_LEN) {
    delay_buf_t *delay_buf = &delay_queue[i];
    if (time_diff(delay_buf->ts, now) > empty_result_delay) {
      if (-1 == sendto(local_sock, delay_buf->buf, delay_buf->buflen, 0,
                       delay_buf->addr, delay_buf->addrlen))
        ERR("sendto");
      free_delay(i);
      delay_queue_first = (delay_queue_first + 1) % DELAY_QUEUE_LEN;
    } else {
      break;
    }
  }
}

static void free_delay(int pos) {
  free(delay_queue[pos].buf);
  free(delay_queue[pos].addr);
}

static void usage() {
  printf("%s\n", "\
usage: chinadns [-h] [-l IPLIST_FILE] [-b BIND_ADDR] [-p BIND_PORT]\n\
       [-c CHNROUTE_FILE] [-s DNS] [-m] [-v] [-V]\n\
Forward DNS requests.\n\
\n\
  -l IPLIST_FILE        path to ip blacklist file\n\
  -c CHNROUTE_FILE      path to china route file\n\
                        if not specified, CHNRoute will be turned\n\
  -d                    off enable bi-directional CHNRoute filter\n\
  -y                    delay time for suspects, default: 0.3\n\
  -b BIND_ADDR          address that listens, default: 0.0.0.0\n\
  -p BIND_PORT          port that listens, default: 53\n\
  -s DNS                DNS servers to use, default:\n\
                        114.114.114.114,208.67.222.222:443,8.8.8.8\n\
  -m                    use DNS compression pointer mutation\n\
                        (backlist and delaying would be disabled)\n\
  -v                    verbose logging\n\
  -h                    show this help message and exit\n\
  -V                    print version and exit\n\
\n\
Online help: <https://github.com/clowwindy/ChinaDNS>\n");
}


