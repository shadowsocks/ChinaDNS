#include <fcntl.h>
#include <netdb.h>
#include <resolv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>

typedef struct {
  size_t size;
  char *data;
} buf_t;

typedef struct {
  uint16_t id;
  struct sockaddr *addr;
  socklen_t addrlen;
} id_addr_t;

typedef struct {
  int entries;
  struct in_addr *ips;
} ip_list_t;

// avoid malloc and free
#define BUF_SIZE 2048
char global_buf[BUF_SIZE];

const char *default_dns_servers = "114.114.114.114,8.8.8.8,208.67.222.222";
char *dns_servers = NULL;
int dns_servers_len;
id_addr_t *dns_server_addrs;

int parse_args(int argc, char **argv);

int setnonblock(int sock);
int resolve_dns_servers();

const char *default_listen_addr = "0.0.0.0";
const char *default_listen_port = "53";

char *listen_addr = NULL;
char *listen_port = NULL;

const char *default_ip_list_file = "iplist.txt";
char *ip_list_file = NULL;
ip_list_t ip_list;
int parse_ip_list();

int dns_init_sockets();
void dns_handle_local();
void dns_handle_remote();

const char *hostname_from_question(ns_msg msg);
int should_filter_query(ns_msg msg);

void queue_add(id_addr_t id_addr);
id_addr_t *queue_lookup(uint16_t id);

#define MAX_QUEUE_LEN 128
// use a queue instead of hash here since it's not long
id_addr_t id_addr_queue[MAX_QUEUE_LEN];
int id_addr_queue_pos = 0;

int local_sock;
int remote_sock;

const char *help_message = 
  "usage: chinadns [-h] [-b BIND_ADDR] [-p BIND_PORT] [-s DNS]\n"
  "Forward DNS requests.\n"
  "\n"
  "  -h, --help            show this help message and exit\n"
  "  -b BIND_ADDR\n"
  "                        address that listens, default: 127.0.0.1\n"
  "  -p BIND_PORT\n"
  "                        port that listens, default: 53\n"
  "  -s DNS     DNS server to use, default:\n"
  "                        114.114.114.114,208.67.222.222,8.8.8.8\n"
  "\n"
  "Online help: <https://github.com/clowwindy/ChinaDNS-C>\n";

#define __LOG(t, v, s...) do {                                      \
  time_t now;                                                       \
  time(&now);                                                       \
  char *time_str = ctime(&now);                                     \
  time_str[strlen(time_str) - 1] = '\0';                            \
  if (t == 0) {                                                     \
    fprintf(stdout, "%s ", time_str);                               \
    printf(s);                                                      \
  } else if (t == 1) {                                              \
    fprintf(stderr, "%s %s:%d ", time_str, __FILE__, __LINE__);     \
    perror(v);                                                      \
  }                                                                 \
} while (0);

#define LOG(s...) __LOG(0, NULL, s)
#define ERR(s) __LOG(1, s, NULL)

int main(int argc, char **argv) {
  fd_set readset, errorset;
  int max_fd;

  memset(&id_addr_queue, 0, sizeof(id_addr_queue));
  if (0 != parse_args(argc, argv))
    return 1;
  if (0 != parse_ip_list())
    return 1;
  if (0 != resolve_dns_servers())
    return 1;
  if (0 != dns_init_sockets())
    return 1;
  max_fd = MAX(local_sock, remote_sock) + 1;

  while (1) {
    FD_ZERO(&readset);
    FD_ZERO(&errorset);
    FD_SET(local_sock, &readset);
    FD_SET(local_sock, &errorset);
    FD_SET(remote_sock, &readset);
    FD_SET(remote_sock, &errorset);
    if (-1 == select(max_fd, &readset, NULL, &errorset, NULL)) {
      ERR("select");
      return 1;
    }
    if (FD_ISSET(local_sock, &errorset)) {
      // TODO getsockopt(..., SO_ERROR, ...);
      LOG("local_sock error\n");
      return 1;
    }
    if (FD_ISSET(remote_sock, &errorset)) {
      // TODO getsockopt(..., SO_ERROR, ...);
      LOG("remote_sock error\n");
      return 1;
    }
    if (FD_ISSET(local_sock, &readset))
      dns_handle_local();
    if (FD_ISSET(remote_sock, &readset))
      dns_handle_remote();
  }

  return 0;
}

int setnonblock(int sock) {
  int flags;
  flags = fcntl(local_sock, F_GETFL, 0);
  if(flags == -1) {
    ERR("fcntl");
    return 1;
  }
  fcntl(local_sock, F_SETFL, flags | O_NONBLOCK);
  if(flags == -1) {
    ERR("fcntl");
    return 1;
  }
  return 0;
}

int parse_args(int argc, char **argv) {
  int ch;

  dns_servers = strdup(default_dns_servers);
  ip_list_file = strdup(default_ip_list_file);
  listen_addr = strdup(default_listen_addr);
  listen_port = strdup(default_listen_port);

  while ((ch = getopt(argc, argv, "hb:p:s:")) != -1) {
    switch (ch) {
    case 'h':
      printf("%s", help_message);
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
    default:
      printf("%s", help_message);
      exit(1);
    }
  }
  argc -= optind;
  argv += optind;
  return 0;
}

int resolve_dns_servers() {
  struct addrinfo hints;
  struct addrinfo *addr_ip;
  char* token;
  int r;
  int i = 0;
  char *pch = strchr(dns_servers, ',');
  dns_servers_len = 1;
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
    if (0 != (r = getaddrinfo(token, "53", &hints, &addr_ip))) {
      LOG("%s:%s\n", gai_strerror(r), token);
      return 1;
    }
    dns_server_addrs[i].addr = addr_ip->ai_addr;
    dns_server_addrs[i].addrlen = addr_ip->ai_addrlen;
    i++;
    token = strtok(0, ",");
  }
  return 0;
}

int cmp_in_addr(const void *a, const void *b) {
  struct in_addr *ina = (struct in_addr *)a;
  struct in_addr *inb = (struct in_addr *)b;
  return ina->s_addr - inb->s_addr;
}

int parse_ip_list() {
  FILE * fp;
  char * line = NULL;
  size_t len = 0;
  ssize_t read;
  ip_list.entries = 0;
  int i = 0;

  fp = fopen(ip_list_file, "rb");
  if (fp == NULL) {
    ERR("Can't open ip list: fopen");
    return 1;
  }
  while ((read = getline(&line, &len, fp)) != -1) {
    ip_list.entries++;
  }
  if (line)
    free(line);
  line = NULL;

  ip_list.ips = calloc(ip_list.entries, sizeof(struct in_addr));
  fseek(fp, 0, SEEK_SET);
  while ((read = getline(&line, &len, fp)) != -1) {
    inet_aton(line, &ip_list.ips[i]);
    i++;
  }
  if (line)
    free(line);

  qsort(ip_list.ips, ip_list.entries, sizeof(struct in_addr), cmp_in_addr);

  fclose(fp);
  return 0;
}

int dns_init_sockets() {
  struct addrinfo addr;
  struct addrinfo *addr_ip;
  int r;

  local_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (0 != setnonblock(local_sock))
    return 1;

  memset(&addr, 0, sizeof(addr));
  if (0 != (r = getaddrinfo(listen_addr, listen_port, &addr, &addr_ip))) {
    LOG("%s:%s:%s\n", gai_strerror(r), listen_addr, listen_port);
    return 1;
  }
  if (0 != bind(local_sock, addr_ip->ai_addr, addr_ip->ai_addrlen)) {
    LOG("Can't bind address %s:%s\n", listen_addr, listen_port);
    ERR("bind");
    return 1;
  }
  freeaddrinfo(addr_ip);
  remote_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (0 != setnonblock(remote_sock))
    return 1;

  return 0;
}

void dns_handle_local() {
  struct sockaddr *src_addr = malloc(sizeof(struct sockaddr));
  socklen_t src_addrlen = sizeof(src_addr);
  uint16_t query_id;
  ssize_t len;
  int i;
  const char *question_hostname;
  ns_msg msg;
  len = recvfrom(local_sock, global_buf, BUF_SIZE, 0, src_addr, &src_addrlen);
  if (len > 0) {
    if (ns_initparse((const u_char *)global_buf, len, &msg) < 0) {
      ERR("ns_initparse");
      free(src_addr);
      return;
    }
    // parse DNS query id
    // TODO generate id for each request to avoid conflicts
    query_id = ns_msg_id(msg);
    question_hostname = hostname_from_question(msg);
    LOG("request: %s\n", question_hostname);
    id_addr_t id_addr;
    id_addr.id = query_id;
    id_addr.addr = src_addr;
    id_addr.addrlen = src_addrlen;
    queue_add(id_addr);
    for (i = 0; i < dns_servers_len; i++) {
      if (-1 == sendto(remote_sock, global_buf, len, 0,
                       dns_server_addrs[i].addr, dns_server_addrs[i].addrlen))
        ERR("sendto");
    }
  }
  else
    ERR("recvfrom");
}

void dns_handle_remote() {
  struct sockaddr *src_addr = malloc(sizeof(struct sockaddr));
  socklen_t src_len = sizeof(src_addr);
  uint16_t query_id;
  ssize_t len;
  const char *question_hostname;
  ns_msg msg;
  len = recvfrom(remote_sock, global_buf, BUF_SIZE, 0, src_addr, &src_len);
  if (len > 0) {
    if (ns_initparse((const u_char *)global_buf, len, &msg) < 0) {
      ERR("ns_initparse");
      free(src_addr);
      return;
    }
    // parse DNS query id
    // TODO assign new id instead of using id from clients
    query_id = ns_msg_id(msg);
    id_addr_t *id_addr = queue_lookup(query_id);
    id_addr->addr->sa_family = AF_INET;
    question_hostname = hostname_from_question(msg);
    LOG("response %s from %s: ", question_hostname,
           inet_ntoa(((struct sockaddr_in *)src_addr)->sin_addr));
    if (id_addr && !should_filter_query(msg)) {
      printf("pass\n");
      if (-1 == sendto(local_sock, global_buf, len, 0, id_addr->addr,
                       id_addr->addrlen))
        ERR("sendto");
    } else {
      printf("filter\n");
    }
  }
  else
    ERR("recvfrom");

}

void queue_add(id_addr_t id_addr) {
  id_addr_queue_pos++;
  if (id_addr_queue_pos > MAX_QUEUE_LEN)
    id_addr_queue_pos = 0;
  // free next hole
  id_addr_t old_id_addr = id_addr_queue[id_addr_queue_pos];
  free(old_id_addr.addr);
  id_addr_queue[id_addr_queue_pos] = id_addr;
}

id_addr_t *queue_lookup(uint16_t id) {
  int i;
  // TODO assign new id instead of using id from clients
  for (i = 0; i < MAX_QUEUE_LEN; i++) {
    if (id_addr_queue[i].id == id)
      return id_addr_queue + i;
  }
  return NULL;
}

static char *hostname_buf = NULL;
size_t hostname_buflen = 0;
const char *hostname_from_question(ns_msg msg) {
  ns_rr rr;
  int rrnum, rrmax;
  const char *result;
  int result_len;
  rrmax = ns_msg_count(msg, ns_s_qd);
  if (rrmax == 0)
    return NULL;
  for (rrnum = 0; rrnum < rrmax; rrnum++) {
    if (ns_parserr(&msg, ns_s_qd, rrnum, &rr)) {
      ERR("ns_parserr");
      return NULL;
    }
    u_int type;
    const u_char *rd;
    type = ns_rr_type(rr);
    rd = ns_rr_rdata(rr);
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

int should_filter_query(ns_msg msg) {
  ns_rr rr;
  int rrnum, rrmax;
  void *r;
  rrmax = ns_msg_count(msg, ns_s_an);
  if (rrmax == 0)
    return 0;
  for (rrnum = 0; rrnum < rrmax; rrnum++) {
    if (ns_parserr(&msg, ns_s_an, rrnum, &rr)) {
      ERR("ns_parserr");
      return 0;
    }
    u_int type;
    const u_char *rd;
    type = ns_rr_type(rr);
    rd = ns_rr_rdata(rr);
    if (type == ns_t_a) {
      printf("%s, ", inet_ntoa(*(struct in_addr *)rd));
      r = bsearch(rd, ip_list.ips, ip_list.entries, sizeof(struct in_addr),
                  cmp_in_addr);
      if (r)
        return 1;
    }
  }
  return 0;
}
