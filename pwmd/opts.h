#ifndef PWMD_OPTS_H
#define PWMD_OPTS_H

#define PWMD_OPTS_HOST_SIZE 32
#define PWMD_OPTS_PORT_SIZE 8

struct pwmd_daemon_opts {
  int gpio;
  int period_ms;
  int min_pulse_ms;
};
struct pwmd_common_opts {
  char host[PWMD_OPTS_HOST_SIZE];
  char port[PWMD_OPTS_PORT_SIZE];
};
struct pwmd_runtime_opts {
  int pulse_ms;
};

int pwmd_opts_parse(int argc, char **argv, struct pwmd_daemon_opts *dopts,
                    struct pwmd_common_opts *copts,
                    struct pwmd_runtime_opts *ropts);

#endif // PWMD_OPTS_H
