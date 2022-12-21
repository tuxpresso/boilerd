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
#include "timer.h"

int max(int a, int b) {
  if (a > b) {
    return a;
  }
  return b;
}

int min(int a, int b) {
  if (a < b) {
    return a;
  }
  return b;
}

int ts_to_ms(struct timespec *ts) {
  return ts->tv_sec * 1000 + ts->tv_nsec / 1000000;
}

int get_elapsed_ms(struct timespec *then) {
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  return ts_to_ms(&now) - ts_to_ms(then);
}

int main(int argc, char **argv) {
  struct pwmd_daemon_opts dopts;
  struct pwmd_common_opts copts;
  struct pwmd_runtime_opts ropts;
  if (pwmd_opts_parse(argc, argv, &dopts, &copts, &ropts)) {
    fprintf(stderr,
            "%s "
            "-g gpio -period ms -min ms "
            "-h host -p port "
            "[-pulse ms]"
            "\n",
            argv[0]);
    return 1;
  }
  fprintf(stderr, "INFO  - gpio %d, period_ms %d, min_pulse_ms %d\n",
          dopts.gpio, dopts.period_ms, dopts.min_pulse_ms);
  fprintf(stderr, "INFO  - host %s, port %s\n", copts.host, copts.port);
  fprintf(stderr, "INFO  - pulse_ms %d\n", ropts.pulse_ms);

  char path[255];
  int gpio_fd, iio_fd;
  sprintf(path, "/sys/class/gpio/gpio%d/value", dopts.gpio);
  if ((gpio_fd = open(path, O_WRONLY)) == -1) {
    fprintf(stderr, "FATAL - failed to open gpio %d\n", dopts.gpio);
    return 1;
  }

  int udp_fd;
  struct addrinfo hints = {0}, *res;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  int ret;
  if ((ret = getaddrinfo(copts.host, copts.port, &hints, &res))) {
    fprintf(stderr, "FATAL - failed to getaddrinfo: %s\n", gai_strerror(ret));
    return 1;
  }
  if ((udp_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) <
      0) {
    fprintf(stderr, "FATAL - failed to open udp socket\n");
    return 1;
  }
  if (bind(udp_fd, res->ai_addr, res->ai_addrlen) < 0) {
    fprintf(stderr, "FATAL - failed to bind to %s:%s\n", copts.host,
            copts.port);
    return 1;
  }
  struct pollfd poll_set[1];
  poll_set[0].fd = udp_fd;
  poll_set[0].events = POLLIN;
  poll_set[0].revents = 0;

  struct boilerd_timer period_timer = {0};
  struct boilerd_timer pulse_timer = {0};
  struct timespec start;
  clock_gettime(CLOCK_MONOTONIC, &start);
  int is_on = 0;
  while (1) {
    int now_ms = get_elapsed_ms(&start);
    boilerd_timer_update(&period_timer, now_ms);
    boilerd_timer_update(&pulse_timer, now_ms);

    int ret;
    char udp_buffer[255];
    if ((ret = poll(poll_set, 1, 0)) < 0) {
      fprintf(stderr, "ERROR - poll returned %d\n", ret);
    } else if (poll_set[0].revents & POLLIN) {
      fprintf(stderr, "TRACE - poll found events\n");
      if ((ret = recv(udp_fd, udp_buffer, 255, 0)) < 0) {
        fprintf(stderr, "ERROR - recv returned %d\n", ret);
      } else {
        fprintf(stderr, "TRACE - recv %d\n", ret);
        if (ret == sizeof(struct pwmd_runtime_opts)) {
          fprintf(stderr, "INFO  - received settings via udp\n");
          memcpy(&ropts, udp_buffer, sizeof(ropts));
          fprintf(stderr, "INFO  - pulse_ms %d\n", ropts.pulse_ms);
        }
      }
    }

    if (is_on && boilerd_timer_is_expired(&pulse_timer)) {
      fprintf(stderr, "DEBUG - boiler off at %d\n", now_ms);
      write(gpio_fd, "0", 1);
      is_on = 0;
    }

    if (boilerd_timer_is_expired(&period_timer)) {
      fprintf(stderr, "DEBUG - it is now %d\n", now_ms);

      int ms = ropts.pulse_ms;

      // the pulse must be no longer than the period
      ms = min(ms, dopts.period_ms);
      // and not between (0, min_pulse_ms)
      ms = (ms == 0) ? 0 : max(ms, dopts.min_pulse_ms);

      ropts.pulse_ms = ms;

      // schedule next pulse and period deadlines
      boilerd_timer_schedule(&period_timer, dopts.period_ms);
      boilerd_timer_schedule(&pulse_timer, ropts.pulse_ms);
      fprintf(stderr, "DEBUG - pulse deadline is %d\n",
              pulse_timer.deadline_ms);
      fprintf(stderr, "DEBUG - period deadline is %d\n",
              period_timer.deadline_ms);

      if (!boilerd_timer_is_expired(&pulse_timer)) {
        fprintf(stderr, "DEBUG - boiler on at %d\n", now_ms);
        write(gpio_fd, "1", 1);
        is_on = 1;
      }
    }

    usleep(500); // .5 ms tick
  }
}
