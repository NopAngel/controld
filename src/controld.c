/* controld.c - v1.0
   Copyright (C) 2026 <NopAngel> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>
#include <arpa/inet.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>

#define SOCKET_PATH "/tmp/controld.sock"
#define UNITS_DIR "units/"
#define LOGS_DIR "logs/"
#define MAX_SERVICES 10

struct unit {
  char name[64];
  char exec_path[256];
  char arg[64];
  char after[64];
  int timer_sec;
  time_t last_run;
  pid_t pid;
  bool is_active;
  bool enabled;
};

static struct unit registry[MAX_SERVICES];
static int service_count = 0;
static volatile bool keep_running = true;

/* --- UTILS --- */
static void trim(char *s) {
  size_t len = strlen(s);
  while (len > 0 && (s[len-1] == '\n' || s[len-1] == '\r' || s[len-1] == ' ')) s[--len] = '\0';
}

static struct unit *find_unit(const char *name) {
  for (int i = 0; i < service_count; i++)
    if (strcmp(registry[i].name, name) == 0) return &registry[i];
  return NULL;
}

/* --- NETWORK --- */
static void set_interface_state(const char *ifname, bool up) {
  int nlsock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
  if (nlsock < 0) return;
  struct { struct nlmsghdr nlh; struct ifinfomsg ifi; } req;
  memset(&req, 0, sizeof(req));
  req.nlh.nlmsg_len = sizeof(req);
  req.nlh.nlmsg_type = RTM_NEWLINK;
  req.nlh.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
  req.ifi.ifi_family = AF_UNSPEC;
  req.ifi.ifi_index = if_nametoindex(ifname);
  req.ifi.ifi_change = IFF_UP;
  req.ifi.ifi_flags = up ? IFF_UP : 0;
  send(nlsock, &req, req.nlh.nlmsg_len, 0);
  printf("[network] %s is now %s\n", ifname, up ? "UP" : "DOWN");
  close(nlsock);
}

static void set_interface_ip(const char *ifname, const char *ip_str, int prefix) {
  int nlsock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
  if (nlsock < 0) return;
  struct { struct nlmsghdr nlh; struct ifaddrmsg ifa; char attr[64]; } req;
  memset(&req, 0, sizeof(req));
  req.nlh.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
  req.nlh.nlmsg_type = RTM_NEWADDR;
  req.nlh.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_ACK;
  req.ifa.ifa_family = AF_INET;
  req.ifa.ifa_prefixlen = prefix;
  req.ifa.ifa_index = if_nametoindex(ifname);
  struct rtattr *rta = (struct rtattr *)(((char *)&req) + NLMSG_ALIGN(req.nlh.nlmsg_len));
  rta->rta_type = IFA_LOCAL; rta->rta_len = RTA_LENGTH(4);
  inet_pton(AF_INET, ip_str, RTA_DATA(rta));
  req.nlh.nlmsg_len = NLMSG_ALIGN(req.nlh.nlmsg_len) + RTA_LENGTH(4);
  send(nlsock, &req, req.nlh.nlmsg_len, 0);
  printf("[network] Assigned %s/%d to %s\n", ip_str, prefix, ifname);
  close(nlsock);
}

/* --- SYSTEM LOGIC --- */
static void handle_signal(int sig) { keep_running = false; }

static void load_units(void) {
  DIR *d = opendir(UNITS_DIR); struct dirent *dir; if (!d) return;
  service_count = 0;
  while ((dir = readdir(d)) != NULL) {
    if (strstr(dir->d_name, ".service") && service_count < MAX_SERVICES) {
      char path[512]; snprintf(path, 512, "%s/%s", UNITS_DIR, dir->d_name);
      FILE *f = fopen(path, "r");
      if (f) {
        struct unit *u = &registry[service_count]; memset(u, 0, sizeof(struct unit));
        char line[256];
        while (fgets(line, 256, f)) {
          if (strncmp(line, "Name=", 5) == 0) { strncpy(u->name, line+5, 63); trim(u->name); }
          else if (strncmp(line, "Exec=", 5) == 0) { strncpy(u->exec_path, line+5, 255); trim(u->exec_path); }
          else if (strncmp(line, "Timer=", 6) == 0) u->timer_sec = atoi(line+6);
          else if (strncmp(line, "After=", 6) == 0) { strncpy(u->after, line+6, 63); trim(u->after); }
        }
        u->pid = -1; u->enabled = true; fclose(f); service_count++;
      }
    }
  }
  closedir(d);
}

int main(void) {
  signal(SIGINT, handle_signal); signal(SIGTERM, handle_signal);
  printf("controld v1.0 - Full Network Stack\n");
  mkdir(LOGS_DIR, 0777); load_units();
  int server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
  fcntl(server_sock, F_SETFL, O_NONBLOCK);
  struct sockaddr_un addr = { .sun_family = AF_UNIX };
  strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path)-1);
  unlink(SOCKET_PATH); bind(server_sock, (struct sockaddr *)&addr, sizeof(addr));
  listen(server_sock, 5);

  while (keep_running) {
    int status; pid_t dead = waitpid(-1, &status, WNOHANG);
    if (dead > 0) {
      for (int i=0; i<service_count; i++) if (registry[i].pid == dead) { registry[i].is_active = false; registry[i].pid = -1; }
    }
    for (int i=0; i<service_count; i++) {
      struct unit *u = &registry[i];
      if (u->enabled && !u->is_active) {
        if (u->timer_sec > 0 && (time(NULL) - u->last_run < u->timer_sec)) continue;
        pid_t p = fork();
        if (p == 0) {
          char log[512]; snprintf(log, 512, "%s/%s.log", LOGS_DIR, u->name);
          int fd = open(log, O_WRONLY|O_CREAT|O_APPEND, 0666);
          dup2(fd, 1); dup2(fd, 2);
          execv(u->exec_path, (char*[]){u->exec_path, NULL}); exit(1);
        } else if (p > 0) { u->pid = p; u->is_active = true; u->last_run = time(NULL); }
      }
    }
    int cfd = accept(server_sock, NULL, NULL);
    if (cfd >= 0) {
      char b[256] = {0}; read(cfd, b, 256);
      if (strncmp(b, "net-set-ip", 10) == 0) {
        char ifa[16], ip[16]; int pfx; sscanf(b+11, "%s %s %d", ifa, ip, &pfx);
        set_interface_ip(ifa, ip, pfx);
      } else if (strncmp(b, "list", 4) == 0) {
        char r[1024] = "UNIT\tPID\tSTATUS\n";
        for(int i=0; i<service_count; i++) {
          char l[128]; snprintf(l, 128, "%s\t%d\t%s\n", registry[i].name, registry[i].pid, registry[i].is_active?"ACT":"IN");
          strcat(r, l);
        }
        write(cfd, r, strlen(r));
      }
      close(cfd);
    }
    usleep(300000);
  }
  unlink(SOCKET_PATH);
  return 0;
}