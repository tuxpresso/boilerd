#include "opts.h"

#include <stdlib.h>
#include <string.h>
int pwmd_opts_parse(int argc, char **argv, struct pwmd_daemon_opts *dopts,
                    struct pwmd_common_opts *copts,
                    struct pwmd_runtime_opts *ropts) {
  if (dopts) {
    dopts->gpio = -1;
    dopts->min_pulse_ms = -1;
    dopts->period_ms = -1;
  }
  if (copts == NULL) {
    return 1;
  }
  copts->host[0] = '\0';
  copts->port[0] = '\0';

  if (ropts == NULL) {
    return 1;
  }
  ropts->pulse_ms = 0;

  for (int i = 1; i + 1 < argc; i += 2) {
    if (dopts && !strcmp(argv[i], "-g")) {
      dopts->gpio = atoi(argv[i + 1]);
    } else if (dopts && !strcmp(argv[i], "-min")) {
      dopts->min_pulse_ms = atoi(argv[i + 1]);
    } else if (!strcmp(argv[i], "-h")) {
      strncpy(copts->host, argv[i + 1], PWMD_OPTS_HOST_SIZE - 1);
      copts->host[PWMD_OPTS_HOST_SIZE - 1] = '\0';
    } else if (!strcmp(argv[i], "-p")) {
      strncpy(copts->port, argv[i + 1], PWMD_OPTS_PORT_SIZE - 1);
      copts->port[PWMD_OPTS_PORT_SIZE - 1] = '\0';
    } else if (!strcmp(argv[i], "-period")) {
      dopts->period_ms = atoi(argv[i + 1]);
    } else if (!strcmp(argv[i], "-pulse")) {
      ropts->pulse_ms = atoi(argv[i + 1]);
    } else {
      return 1;
    }
  }

  int dopts_err = dopts && ((dopts->gpio < 0) || (dopts->period_ms < 0) ||
                            (dopts->min_pulse_ms < 0));
  return dopts_err || (copts->host[0] == '\0') || (copts->port[0] == '\0');
}
