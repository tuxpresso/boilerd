#include "opts.h"

#include <stdlib.h>
#include <string.h>
int boilerd_opts_parse(int argc, char **argv, struct boilerd_daemon_opts *dopts,
                       struct boilerd_common_opts *copts,
                       struct boilerd_runtime_opts *ropts) {
  if (dopts) {
    dopts->gpio = -1;
    dopts->iio = -1;
  }
  if (copts == NULL) {
    return 1;
  }
  copts->host[0] = '\0';
  copts->port = 0;

  if (ropts == NULL) {
    return 1;
  }
  ropts->sp = 0;
  ropts->kp = 0;
  ropts->ki = 0;
  ropts->kd = 0;
  ropts->max_temp = 0;

  for (int i = 1; i + 1 < argc; i += 2) {
    if (dopts && !strcmp(argv[i], "-g")) {
      dopts->gpio = atoi(argv[i + 1]);
    } else if (dopts && !strcmp(argv[i], "-i")) {
      dopts->iio = atoi(argv[i + 1]);
    } else if (!strcmp(argv[i], "-h")) {
      strncpy(copts->host, argv[i + 1], BOILERD_OPTS_HOST_SIZE - 1);
      copts->host[BOILERD_OPTS_HOST_SIZE - 1] = '\0';
    } else if (!strcmp(argv[i], "-p")) {
      copts->port = atoi(argv[i + 1]);
    } else if (!strcmp(argv[i], "-sp")) {
      ropts->sp = atoi(argv[i + 1]);
    } else if (!strcmp(argv[i], "-kp")) {
      ropts->kp = atoi(argv[i + 1]);
    } else if (!strcmp(argv[i], "-ki")) {
      ropts->ki = atoi(argv[i + 1]);
    } else if (!strcmp(argv[i], "-kd")) {
      ropts->kd = atoi(argv[i + 1]);
    } else if (!strcmp(argv[i], "-max")) {
      ropts->max_temp = atoi(argv[i + 1]);
    } else {
      return 1;
    }
  }

  int dopts_err = dopts && ((dopts->gpio < 0) || (dopts->iio < 0));
  return dopts_err || (copts->host[0] == '\0') || (copts->port < 1);
}
