#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <zmq.h>

struct boilerd_zmq_opts {
  char topic[16];
  int sp;
  int kp;
  int ki;
  int kd;
};

struct client_cli_opts {
  int n;
  int delay;
  int sp;
  int kp;
  int ki;
  int kd;
  const char *zbind;
};

int parse_opts(int argc, char **argv, struct client_cli_opts *opts) {
  opts->n = -1;
  opts->delay = -1;
  opts->sp = -1;
  opts->kp = 0;
  opts->ki = 0;
  opts->kd = 0;
  opts->zbind = NULL;
  for (int i = 1; i + 1 < argc; i += 2) {
    if (!strcmp(argv[i], "-n")) {
      opts->n = atoi(argv[i + 1]);
    } else if (!strcmp(argv[i], "-d")) {
      opts->delay = atoi(argv[i + 1]);
    } else if (!strcmp(argv[i], "-sp")) {
      opts->sp = atoi(argv[i + 1]);
    } else if (!strcmp(argv[i], "-kp")) {
      opts->kp = atoi(argv[i + 1]);
    } else if (!strcmp(argv[i], "-ki")) {
      opts->ki = atoi(argv[i + 1]);
    } else if (!strcmp(argv[i], "-kd")) {
      opts->kd = atoi(argv[i + 1]);
    } else if (!strcmp(argv[i], "-z")) {
      opts->zbind = argv[i + 1];
    } else {
      return 1;
    }
  }
  return (opts->n < 0) || (opts->delay < 0) || (opts->zbind == NULL);
}
int main(int argc, char **argv) {
  struct client_cli_opts opts;
  if (parse_opts(argc, argv, &opts)) {
    fprintf(stderr,
            "%s -n number -d delay (ms) [-kp pgain -ki igain -kd dgain -z "
            "zmq_bind]\n",
            argv[0]);
    return 1;
  }
  fprintf(stderr, "INFO  - n %d, delay %d, kp %d, ki %d, kd %d, zbind %s\n",
          opts.n, opts.delay, opts.kp, opts.ki, opts.kd, opts.zbind);

  struct boilerd_zmq_opts to_send = {
      .sp = opts.sp, .kp = opts.kp, .ki = opts.ki, .kd = opts.kd};
  char *topic = "settings";
  memcpy(to_send.topic, topic, strlen(topic));

  void *zctx = zmq_ctx_new();
  void *zpub = zmq_socket(zctx, ZMQ_PUB);
  int ret = zmq_bind(zpub, opts.zbind);
  if (ret) {
    fprintf(stderr, "FATAL - failed to bind to %s\n", opts.zbind);
    return 1;
  }
  for (int i = 0; i < opts.n; ++i) {
    zmq_send(zpub, &to_send, sizeof(to_send), 0);
    fprintf(stderr, "INFO  - published %d of %d\n", i, opts.n);

    usleep(opts.delay * 1000);
  }

  return 0;
}
