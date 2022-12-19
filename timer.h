#ifndef TIMER_H
#define TIMER_H

struct boilerd_timer {
  int now_ms;
  int deadline_ms;
};

void boilerd_timer_update(struct boilerd_timer *timer, int now_ms);
void boilerd_timer_schedule(struct boilerd_timer *timer, int duration_ms);
int boilerd_timer_is_expired(struct boilerd_timer *timer);

#endif // TIMER_H
