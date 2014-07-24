#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <resolv.h>
#include <netdb.h>

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

int parse_args(int argc, const char **argv);

int setnonblock(int sock);
int resolve_dns_servers();

const char *default_ip_list_file = "iplist.txt";
char *ip_list_file = NULL;
ip_list_t ip_list;
int parse_ip_list();

int dns_init_sockets();
void dns_handle_local();
void dns_handle_remote();

int should_filter_query(ns_msg msg);

void queue_add(id_addr_t id_addr);
id_addr_t *queue_lookup(uint16_t id);

#define MAX_QUEUE_LEN 128
// use a queue instead of hash here since it's not long
id_addr_t id_addr_queue[MAX_QUEUE_LEN];
int id_addr_queue_pos = 0;

int local_sock;
int remote_sock;

int main(int argc, const char **argv) {
  int r;
  fd_set readset, errorset;
  int max_fd;

  memset(&id_addr_queue, 0, sizeof(id_addr_queue));
  if (0 != parse_args(argc, argv))
    return 1;
  if (0 != parse_ip_list())
    return 1;
  if (0 != resolve_dns_servers())
    return 1;
  r = dns_init_sockets();
  if (r != 0)
    return 1;
  max_fd = MAX(local_sock, remote_sock) + 1;

  while (1)
  {
    FD_ZERO(&readset);
    FD_ZERO(&errorset);
    FD_SET(local_sock, &readset);
    FD_SET(local_sock, &errorset);
    FD_SET(remote_sock, &readset);
    FD_SET(remote_sock, &errorset);
    r = select(max_fd, &readset, NULL, &errorset, NULL);
    if (r == -1)
    {
      perror("select");
      return 1;
    }
    if (FD_ISSET(local_sock, &errorset))
    {
      // TODO getsockopt(..., SO_ERROR, ...);
      printf("local_sock error\n");
      return 1;
    }
    if (FD_ISSET(remote_sock, &errorset))
    {
      // TODO getsockopt(..., SO_ERROR, ...);
      printf("remote_sock error\n");
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
  if(flags == -1)
  {
    perror("fcntl");
    return 1;
  }
  fcntl(local_sock, F_SETFL, flags | O_NONBLOCK);
  if(flags == -1)
  {
    perror("fcntl");
    return 1;
  }
  return 0;
}

int parse_args(int argc, const char **argv) {
  dns_servers = strdup(default_dns_servers);
  ip_list_file = strdup(default_ip_list_file);
  return 0;
}

int resolve_dns_servers() {
  struct addrinfo hints;
  struct addrinfo *addr_ip;
  char* token;
  int i = 0;
  char *pch = strchr(dns_servers, ',');
  dns_servers_len = 1;
  while (pch != NULL) {
    dns_servers_len++;
    pch=strchr(pch + 1, ',');
  }
  dns_server_addrs = calloc(dns_servers_len, sizeof(id_addr_t));

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
  token = strtok(dns_servers, ",");
  while (token)
  {
    if (getaddrinfo(token, "53", &hints, &addr_ip) != 0)
    {
      perror("getaddrinfo");
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
  if (fp == NULL)
  {
    perror("can not open ip list: fopen");
    return 1;
  }
  while ((read = getline(&line, &len, fp)) != -1)
  {
    ip_list.entries++;
  }
  if (line)
    free(line);
  line = NULL;

  ip_list.ips = calloc(ip_list.entries, sizeof(struct in_addr));
  fseek(fp, 0, SEEK_SET);
  while ((read = getline(&line, &len, fp)) != -1)
  {
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
  int r;
  struct addrinfo addr;
  struct addrinfo *addr_ip;

  local_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  r = setnonblock(local_sock);
  if (r != 0)
    return 1;

  memset(&addr, 0, sizeof(addr));
  r = getaddrinfo("0.0.0.0", "53", &addr, &addr_ip);
  if (r != 0)
  {
    perror("getaddrinfo");
    return 1;
  }
  r = bind(local_sock, addr_ip->ai_addr, addr_ip->ai_addrlen);
  if (r != 0)
  {
    perror("bind");
    return 1;
  }
  freeaddrinfo(addr_ip);
  remote_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  r = setnonblock(remote_sock);
  if (r != 0)
    return 1;

  return 0;
}

void dns_handle_local() {
  struct sockaddr *src_addr = malloc(sizeof(struct sockaddr));
  socklen_t src_addrlen = sizeof(src_addr);
  uint16_t query_id;
  ssize_t len;
  int i;
  ns_msg msg;
  len = recvfrom(local_sock, global_buf, BUF_SIZE, 0, src_addr, &src_addrlen);
  if (len > 0)
  {
    if (ns_initparse((const u_char *)global_buf, len, &msg) < 0)
    {
      perror("ns_initparse");
      free(src_addr);
      return;
    }
    // parse DNS query id
    // TODO generate id for each request to avoid conflicts
    query_id = ns_msg_id(msg);
    id_addr_t id_addr;
    id_addr.id = query_id;
    id_addr.addr = src_addr;
    id_addr.addrlen = src_addrlen;
    queue_add(id_addr);
    for (i = 0; i < dns_servers_len; i++)
    {
      if (-1 == sendto(remote_sock, global_buf, len, 0, dns_server_addrs[i].addr,
                       dns_server_addrs[i].addrlen))
        perror("sendto");
    }
  }
  else
    perror("recvfrom");
}

void dns_handle_remote() {
  struct sockaddr *src_addr = malloc(sizeof(struct sockaddr));
  socklen_t src_len = sizeof(src_addr);
  uint16_t query_id;
  ssize_t len;
  ns_msg msg;
  len = recvfrom(remote_sock, global_buf, BUF_SIZE, 0, src_addr, &src_len);
  if (len > 0)
  {
    if (ns_initparse((const u_char *)global_buf, len, &msg) < 0)
    {
      perror("ns_initparse");
      free(src_addr);
      return;
    }
    // parse DNS query id
    // TODO assign new id instead of using id from clients
    query_id = ns_msg_id(msg);
    id_addr_t *id_addr = queue_lookup(query_id);
    id_addr->addr->sa_family = AF_INET;
    if (id_addr && !should_filter_query(msg))
    {
      if (-1 == sendto(local_sock, global_buf, len, 0, id_addr->addr,
                       id_addr->addrlen))
        perror("sendto");
    }
  }
  else
    perror("recvfrom");

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
  for (i = 0; i < MAX_QUEUE_LEN; i++)
  {
    if (id_addr_queue[i].id == id)
      return id_addr_queue + i;
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
  printf("got response: ");
  for (rrnum = 0; rrnum < rrmax; rrnum++)
  {
    if (ns_parserr(&msg, ns_s_an, rrnum, &rr))
    {
      perror("ns_parserr");
      return 0;
    }
    u_int type;
    const u_char *rd;
    type = ns_rr_type(rr);
    rd = ns_rr_rdata(rr);
    if (type == ns_t_a)
    {
      printf("%s, ", inet_ntoa(*(struct in_addr *)rd));
      r = bsearch(rd, ip_list.ips, ip_list.entries, sizeof(struct in_addr),
              cmp_in_addr);
      if (r)
      {
        printf("filter\n");
        return 1;
      }
    }
  }
  printf("pass\n");
  return 0;
}
