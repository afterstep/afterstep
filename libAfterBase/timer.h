#ifndef TIMER_H
#define TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * To use:
 * 1. call timer_handle() in event loop; use timer_delay_till_next_alarm()
 *    to determine how long app should wait (so the app can block with
 *    select())
 *
 * Notes:
 *  o this timer code is not intended to handle large numbers of timeout
 *    events; if large numbers of events are needed, timer should be
 *    rewritten to be a priority queue
 *  o timers may be created (with timer_new()) at any point, including
 *    inside a timer handler
 *  o example of use:
 *    {
 *      struct timeval tv;
 *      ...
 *      timer_new(1000, my_handler, NULL);
 *      ...
 *      timer_delay_till_next_alarm(&tv.tv_sec, &tv.tv_usec);
 *      select(..., &tv); // block until timer expires
 *      timer_handle(); // handle any timeout events
 *      ...
 *    }
 */

typedef struct Timer
{
  struct Timer *next;
  void *data;
  time_t sec;
  time_t usec;
  void (*handler) (void *timer);
}
Timer;

void timer_new (time_t msec, void (*handler) (void *), void *data);
Bool timer_delay_till_next_alarm (time_t * sec, time_t * usec);
Bool timer_handle (void);
Bool timer_remove_by_data (void *data);
Bool timer_find_by_data (void *data);

#ifdef __cplusplus
}
#endif


#endif /* TIMER_H */
