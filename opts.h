#ifndef OPTS_H
#define OPTS_H

struct boilerd_opts {
  int gpio;
  int iio;
  int sp;
  int kp;
  int ki;
  int kd;
  int max_temp;
};

int boilerd_opts_parse(int argc, char **argv, struct boilerd_opts *opts);

#endif // OPTS_H
