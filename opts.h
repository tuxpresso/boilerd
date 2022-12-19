#ifndef OPTS_H
#define OPTS_H

#define BOILERD_OPTS_HOST_SIZE 32

struct boilerd_opts {
  int gpio;
  int iio;
  char host[BOILERD_OPTS_HOST_SIZE];
  int port;
  int sp;
  int kp;
  int ki;
  int kd;
  int max_temp;
};

int boilerd_opts_parse(int argc, char **argv, struct boilerd_opts *opts);

#endif // OPTS_H
