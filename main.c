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

#include <pidc.h>

#include "opts.h"
#include "timer.h"

struct boilerd_pwm {
  int period_ms;
  int pulse_ms;
  int min_pulse_ms;
};

int boilerd_read_temp(int iio_fd, int *temp) {
  char temp_buf[255];
  int ret;
  if ((ret = read(iio_fd, temp_buf, 255)) > 0) {
    *temp = atoi(temp_buf);
    ret = 0;
  } else {
    ret = 1;
  }
  lseek(iio_fd, 0, SEEK_SET);
  return ret;
}

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
  struct boilerd_daemon_opts dopts;
  struct boilerd_common_opts copts;
  struct boilerd_runtime_opts ropts;
  if (boilerd_opts_parse(argc, argv, &dopts, &copts, &ropts)) {
    fprintf(stderr,
            "%s "
            "-g gpio -i iio "
            "-h host -p port "
            "[-sp setpoint -kp pgain -ki igain -kd dgain -max max_temp]"
            "\n",
            argv[0]);
    return 1;
  }
  fprintf(stderr, "INFO  - gpio %d, iio %d\n", dopts.gpio, dopts.iio);
  fprintf(stderr, "INFO  - host %s, port %s\n", copts.host, copts.port);
  fprintf(stderr, "INFO  - sp %d, kp %d, ki %d, kd %d, max_temp %d\n", ropts.sp,
          ropts.kp, ropts.ki, ropts.kd, ropts.max_temp);

  char path[255];
  int gpio_fd, iio_fd;
  sprintf(path, "/sys/class/gpio/gpio%d/value", dopts.gpio);
  if ((gpio_fd = open(path, O_WRONLY)) == -1) {
    fprintf(stderr, "FATAL - failed to open gpio %d\n", dopts.gpio);
    return 1;
  }
  sprintf(path, "/sys/bus/iio/devices/iio:device%d/in_temp_raw", dopts.iio);
  if ((iio_fd = open(path, O_RDONLY)) == -1) {
    fprintf(stderr, "FATAL - failed to open iio %d\n", dopts.iio);
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

  pidc_t *pidc;
  pidc_init(&pidc, ropts.kp, ropts.ki, ropts.kd);

  struct boilerd_pwm pwm = {
      .period_ms = 1000, .pulse_ms = 0, .min_pulse_ms = 20};

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
        if (ret == sizeof(struct boilerd_runtime_opts)) {
          fprintf(stderr, "INFO  - received settings via udp\n");
          memcpy(&ropts, udp_buffer, sizeof(ropts));
          fprintf(stderr, "INFO  - sp %d\n", ropts.sp);
          fprintf(stderr, "INFO  - kp %d\n", ropts.kp);
          fprintf(stderr, "INFO  - ki %d\n", ropts.ki);
          fprintf(stderr, "INFO  - kd %d\n", ropts.kd);
          fprintf(stderr, "INFO  - max_temp %d\n", ropts.max_temp);
          pidc_destroy(pidc);
          pidc_init(&pidc, ropts.kp, ropts.ki, ropts.kd);
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

      int temp;
      if (boilerd_read_temp(iio_fd, &temp)) {
        fprintf(stderr, "ERROR - failed to read temperature, not pulsing\n");
        pwm.pulse_ms = 0;
        goto schedule;
      }
      fprintf(stderr, "INFO  - temperature is %d\n", temp);

      if (temp > ropts.max_temp) {
        fprintf(stderr, "WARN  - temperature exceeds %d, not pulsing\n",
                ropts.max_temp);
        pwm.pulse_ms = 0;
        goto schedule;
      }

      // use error to calculate gain, then translate to pulse width
      int e = ropts.sp - temp;
      int g = pidc_update(pidc, e) >> 8;
      int ms = max(g, 0);          // map gain (if positive) to pulse width
      ms = min(ms, pwm.period_ms); // pulse can't be longer than period
      pwm.pulse_ms = (ms == 0) ? 0 : max(ms, pwm.min_pulse_ms); // or < min
      fprintf(stderr, "TRACE - error is %d\n", e);
      fprintf(stderr, "TRACE - gain is %d\n", g);
      fprintf(stderr, "DEBUG - pulse_ms is %d\n", pwm.pulse_ms);

      // schedule next pulse and period deadlines
    schedule:
      boilerd_timer_schedule(&period_timer, pwm.period_ms);
      boilerd_timer_schedule(&pulse_timer, pwm.pulse_ms);
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
