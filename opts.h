#ifndef OPTS_H
#define OPTS_H

#define BOILERD_OPTS_HOST_SIZE 32

struct boilerd_daemon_opts {
  int gpio;
  int iio;
};
struct boilerd_common_opts {
  char host[BOILERD_OPTS_HOST_SIZE];
  int port;
};
struct boilerd_runtime_opts {
  int sp;
  int kp;
  int ki;
  int kd;
  int max_temp;
};

int boilerd_opts_parse(int argc, char **argv, struct boilerd_daemon_opts *dopts,
                       struct boilerd_common_opts *copts,
                       struct boilerd_runtime_opts *ropts);

#endif // OPTS_H
