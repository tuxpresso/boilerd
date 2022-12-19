#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
  struct boilerd_opts opts;
  if (boilerd_opts_parse(argc, argv, &opts)) {
    fprintf(stderr,
            "%s -g gpio -i iio -sp setpoint [-kp pgain -ki igain -kd dgain -max"
            "max_temp]\n",
            argv[0]);
    return 1;
  }
  fprintf(stderr,
          "INFO  - gpio %d, iio %d, sp %d, kp %d, ki %d, kd %d, max_temp %d\n",
          opts.gpio, opts.iio, opts.sp, opts.kp, opts.ki, opts.kd,
          opts.max_temp);

  char path[255];
  int gpio_fd, iio_fd;
  sprintf(path, "/sys/class/gpio/gpio%d/value", opts.gpio);
  if ((gpio_fd = open(path, O_WRONLY)) == -1) {
    fprintf(stderr, "FATAL - failed to open gpio %d\n", opts.gpio);
    return 1;
  }
  sprintf(path, "/sys/bus/iio/devices/iio:device%d/in_temp_raw", opts.iio);
  if ((iio_fd = open(path, O_RDONLY)) == -1) {
    fprintf(stderr, "FATAL - failed to open iio %d\n", opts.iio);
    return 1;
  }

  pidc_t *pidc;
  pidc_init(&pidc, opts.kp, opts.ki, opts.kd);

  struct boilerd_pwm pwm = {
      .period_ms = 1000, .pulse_ms = 0, .min_pulse_ms = 50};

  struct boilerd_timer period_timer = {0};
  struct boilerd_timer pulse_timer = {0};
  struct timespec start;
  clock_gettime(CLOCK_MONOTONIC, &start);
  int is_on = 0;
  while (1) {
    int now_ms = get_elapsed_ms(&start);
    boilerd_timer_update(&period_timer, now_ms);
    boilerd_timer_update(&pulse_timer, now_ms);

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

      if (temp > opts.max_temp) {
        fprintf(stderr, "WARN  - temperature exceeds %d, not pulsing\n",
                opts.max_temp);
        pwm.pulse_ms = 0;
        goto schedule;
      }

      // use error to calculate gain, then translate to pulse width
      int e = opts.sp - temp;
      int g = pidc_update(pidc, e) >> 4;
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
