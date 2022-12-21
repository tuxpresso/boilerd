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
  struct boilerd_common_opts copts;
  struct boilerd_runtime_opts ropts;
  if (boilerd_opts_parse(argc, argv, NULL, &copts, &ropts)) {
    fprintf(stderr,
            "%s "
            "-h host -p port "
            "[-sp setpoint -kp pgain -ki igain -kd dgain -max max_temp]"
            "\n",
            argv[0]);
    return 1;
  }
  fprintf(stderr, "INFO  - host %s, port %s\n", copts.host, copts.port);
  fprintf(stderr, "INFO  - sp %d, kp %d, ki %d, kd %d, max_temp %d\n", ropts.sp,
          ropts.kp, ropts.ki, ropts.kd, ropts.max_temp);

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
  getaddrinfo(copts.host, copts.port, &hints, &targ);
  if (sendto(udp_fd, &ropts, sizeof(ropts), 0, targ->ai_addr,
             targ->ai_addrlen) < 0) {
    fprintf(stderr, "FATAL - failed to sendto %s:%s\n", copts.host, copts.port);
    return 1;
  }
}
