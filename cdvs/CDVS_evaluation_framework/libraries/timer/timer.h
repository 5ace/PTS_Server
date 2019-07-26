/* timer.h -- high-precision timer */

#ifndef _TIMER_H_
#define _TIMER_H_

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Structure used by the high precision timer. 
 */
typedef struct {
  long long freq;
} TM_STATE;

/**
 * Structure used by the high precision timer. 
 */
typedef union {
  struct {
    time_t tv_sec;        
    long   tv_nsec;       
  } tmspec;
  struct {
      time_t  tv_sec;
      long  tv_usec;
  } osxtmspec;
  long long counter;
} TM_COUNTER;

/*
 * Start/stop timer functions:
 */
void start_timer (TM_STATE *state, TM_COUNTER *start);
void stop_timer (TM_COUNTER *end);

/*
 * Compute time difference between start and end points (in seconds):
 */
double elapsed_time (TM_STATE *state, TM_COUNTER *start, TM_COUNTER *end);

#ifdef __cplusplus
}
#endif

#endif
