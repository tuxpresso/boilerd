#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "opts.h"

int main(int argc, char **argv) {
  struct boilerd_opts opts;
  if (boilerd_opts_parse(argc, argv, &opts)) {
    fprintf(stderr,
            "%s -p port -sp setpoint [-kp pgain -ki igain -kd dgain -max "
            "max_temp]\n",
            argv[0]);
    return 1;
  }
  fprintf(stderr, "INFO  - port %d\n", opts.port);
  fprintf(stderr, "INFO  - sp %d, kp %d, ki %d, kd %d, max_temp %d\n", opts.sp,
          opts.kp, opts.ki, opts.kd, opts.max_temp);

  int udp_fd;
  struct addrinfo hints, *res, *targ;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;
  getaddrinfo(NULL, "0", &hints, &res);
  if ((udp_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) <
      0) {
    fprintf(stderr, "FATAL - failed to open udp socket\n");
    return 1;
  }
  char port_buffer[7] = {0};
  snprintf(port_buffer, 6, "%d", opts.port);
  getaddrinfo(opts.host, port_buffer, &hints, &targ);
  if (sendto(udp_fd, &opts, sizeof(opts), 0, targ->ai_addr, targ->ai_addrlen) <
      0) {
    fprintf(stderr, "FATAL - failed to sendto udp port %d\n", opts.port);
    return 1;
  }
}
