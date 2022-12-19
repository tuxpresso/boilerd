#include "opts.h"

#include <stdlib.h>
#include <string.h>

int boilerd_opts_parse(int argc, char **argv, struct boilerd_opts *opts) {
  opts->gpio = -1;
  opts->iio = -1;
  opts->sp = -1;
  opts->kp = 0;
  opts->ki = 0;
  opts->kd = 0;
  opts->max_temp = 0;
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
    } else if (!strcmp(argv[i], "-max")) {
      opts->max_temp = atoi(argv[i + 1]);
    } else {
      return 1;
    }
  }
  return (opts->gpio < 0) || (opts->iio < 0) || (opts->sp < 0);
}
