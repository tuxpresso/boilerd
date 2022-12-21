#include "timer.h"

void boilerd_timer_update(struct boilerd_timer *timer, int now_ms) {
  timer->now_ms = now_ms;
}
void boilerd_timer_schedule(struct boilerd_timer *timer, int duration_ms) {
  timer->deadline_ms = timer->now_ms + duration_ms;
}
int boilerd_timer_is_expired(struct boilerd_timer *timer) {
  return !(timer->now_ms < timer->deadline_ms);
}
