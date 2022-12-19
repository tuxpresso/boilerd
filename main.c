#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <pidc.h>

struct boilerd_opts {
  int gpio;
  int iio;
  int sp;
  int kp;
  int ki;
  int kd;
};

struct boilerd_pwm {
  int period_ms;
  int pulse_ms;
  int min_pulse_ms;
};

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

int parse_opts(int argc, char **argv, struct boilerd_opts *opts) {
  opts->gpio = -1;
  opts->iio = -1;
  opts->sp = -1;
  opts->kp = 0;
  opts->ki = 0;
  opts->kd = 0;
  for (int i = 1; i + 1 < argc; i += 2) {
    if (!strcmp(argv[i], "-g")) {
      opts->gpio = atoi(argv[i + 1]);
    } else if (!strcmp(argv[i], "-i")) {
      opts->iio = atoi(argv[i + 1]);
    } else if (!strcmp(argv[i], "-sp")) {
      opts->sp = atoi(argv[i + 1]);
    } else if (!strcmp(argv[i], "-kp")) {
      opts->kp = atoi(argv[i + 1]);
    } else if (!strcmp(argv[i], "-ki")) {
      opts->ki = atoi(argv[i + 1]);
    } else if (!strcmp(argv[i], "-kd")) {
      opts->kd = atoi(argv[i + 1]);
    } else {
      return 1;
    }
  }
  return (opts->gpio < 0) || (opts->iio < 0) || (opts->sp < 0);
}

int main(int argc, char **argv) {
  struct boilerd_opts opts;
  if (parse_opts(argc, argv, &opts)) {
    fprintf(stderr, "%s -g gpio -i iio [-kp pgain -ki igain -kd dgain]\n",
            argv[0]);
    return 1;
  }
  fprintf(stderr, "INFO  - gpio %d, iio %d, kp %d, ki %d, kd %d\n", opts.gpio,
          opts.iio, opts.kp, opts.ki, opts.kd);

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
  int max_temp = 2000;

  struct timespec start;
  clock_gettime(CLOCK_MONOTONIC, &start);
  int now_ms = 0;
  int is_on = 0;
  int pulse_end_ms = 0;
  int period_end_ms = 0;
  while (1) {
    now_ms = get_elapsed_ms(&start);

    // if boiler on and it is not before the pulse deadline, disable boiler
    if (is_on && !(now_ms < pulse_end_ms)) {
      fprintf(stderr, "TRACE - boiler off at %d\n", now_ms);
      write(gpio_fd, "0", 1);
      is_on = 0;
    }

    // if it is not before the period deadline
    if (!(now_ms < period_end_ms)) {
      fprintf(stderr, "TRACE - it is now %d\n", now_ms);

      // read the temperature
      char temp_buf[255];
      ssize_t ret;
      int temp;
      if ((ret = read(iio_fd, temp_buf, 255)) > 0) {
        temp = atoi(temp_buf);
        fprintf(stderr, "DEBUG - temperature is %d\n", temp);
      } else {
        fprintf(stderr, "ERROR - iio read returned %zd\n", ret);
      }
      lseek(iio_fd, 0, SEEK_SET);

      // if over max temperature, turn off boiler and wait
      if (temp > max_temp) {
        write(gpio_fd, "0", 1);
        is_on = 0;
        period_end_ms += 1000;
        fprintf(stderr, "ERROR - temperature exceeds %d\n", max_temp);
        continue;
      }

      // use error to calculate gain, then translate to pulse width
      int e = opts.sp - temp;
      int g = pidc_update(pidc, e) >> 4;
      int ms = max(g, 0);          // map gain (if positive) to pulse width
      ms = min(ms, pwm.period_ms); // pulse can't be longer than period
      pwm.pulse_ms = (ms == 0) ? 0 : max(ms, pwm.min_pulse_ms); // or < min
      fprintf(stderr, "TRACE - error is %d\n", e);
      fprintf(stderr, "TRACE - gain is %d\n", g);
      fprintf(stderr, "TRACE - pulse_ms is %d\n", pwm.pulse_ms);

      // schedule next pulse and period deadlines
      period_end_ms = now_ms + pwm.period_ms;
      pulse_end_ms = now_ms + pwm.pulse_ms;
      fprintf(stderr, "TRACE - pulse deadline is %d\n", pulse_end_ms);
      fprintf(stderr, "TRACE - period deadline is %d\n", period_end_ms);

      // if it is before the pulse deadline, enable boiler
      if (now_ms < pulse_end_ms) {
        fprintf(stderr, "TRACE - boiler on at %d\n", now_ms);
        write(gpio_fd, "1", 1);
        is_on = 1;
      }
    }

    usleep(500); // .5 ms tick
  }
}
