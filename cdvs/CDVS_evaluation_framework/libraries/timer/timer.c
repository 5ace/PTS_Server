/*
 * MPEG Compact Desctiptors for Visual Search (CDVS) evaluation framework.
 *
 * timer.c -- high-precision timer
 *
 ***********************************
 *
 * This software module was originally developed by:
 *
 *   <list of original developers>
 *
 * in the course of development of ISO/IEC <number> (Compact Descriptors for Visual 
 * Search) standard for reference purposes and its performance may not have been 
 * optimized. This software module includes implementation of one or more tools as 
 * specified by the ISO/IEC <number> standard.
 *
 * ISO/IEC gives you a royalty-free, worldwide, non-exclusive, copyright license to copy, 
 * distribute, and make derivative works of this software module or modifications thereof 
 * for use in implementations of the ISO/IEC <number> standard in products that satisfy 
 * conformance criteria (if any).
 *
 * Those intending to use this software module in products are advised that its use may 
 * infringe existing patents. ISO/IEC have no liability for use of this software module 
 * or modifications thereof.
 *
 * Copyright is not released for products that do not conform to audiovisual and image-
 * coding related ITU Recommendations and/or ISO/IEC International Standards.
 *
 ****** Section to be removed when the standard is published **************************
 *
 * Assurance that the originally developed software module can be used
 *  (1) in the ISO/IEC <number> standard once this standard has been adopted; and
 *  (2) to develop the ISO/IEC <number> standard:
 *
 * <Original developers> grant ISO/IEC all rights necessary to include the originally 
 * developed software module or modifications thereof in the ISO/IEC <number> standard 
 * and to permit ISO/IEC to offer You a royalty-free, worldwide, non-exclusive, copyright 
 * license to copy, distribute, and make derivative works for use in implementations of 
 * the ISO/IEC <number> standard in products that satisfy conformance criteria (if any), 
 * and to the extent that such originally developed software module or portions of it 
 * are included in the ISO/IEC <number> standard.
 *
 * To the extent that <original developers> own patent rights that would be required 
 * to make, use, or sell the originally developed software module or portions thereof 
 * included in the ISO/IEC <number> standard in a conforming product, the <original 
 * developers> will assure the ISO/IEC that they are willing to negotiate licenses under 
 * reasonable and non-discriminatory terms and conditions with applicants throughout 
 * the world.
 *
 * ISO/IEC gives You a free license to this software module or modifications thereof 
 * for the sole purpose of developing the ISO/IEC <number> standard.
 *
 ****** End of section to be removed when the standard is published *******************
 *
 * <original developers> retain full rights to modify and use the code for their own 
 * purposes, assign or donate the code to a third party and to inhibit third parties 
 * from using the code for products that do not conform to MPEG-related 
 * ITU Recommendations and/or ISO/IEC International Standards.
 *
 * This copyright notice must be included in all copies or derivative works.
 * Copyright (c) ISO/IEC 2011.
 *
 */



#include "timer.h"

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>  

/*!
 * \brief Start measuring execution time 
 *        Win32/64 timer version (essentially calls to __asm rdtsc).
 * \param TM_STATE *state - state to be set to signaled when the specified due time arrives
 * \param TM_COUNTER *start - get start time
 * \return void
 */

void start_timer (TM_STATE *state, TM_COUNTER *start)
{
  SetThreadAffinityMask (GetCurrentThread(), 1);
  QueryPerformanceFrequency ((PLARGE_INTEGER)&state->freq);
  QueryPerformanceCounter ((PLARGE_INTEGER)&start->counter);
}

/*!
 * \brief Stop measuring time
 *        Win32/64 timer version (essentially calls to __asm rdtsc).
 * \param TM_COUNTER *end - get end time
 * \return void
 */

void stop_timer (TM_COUNTER *end)
{
  QueryPerformanceCounter ((PLARGE_INTEGER)&end->counter);
}

/*!
 * \brief Compute elapsed time
 *        Win32/64 timer version (essentially calls to __asm rdtsc).
 * \param TM_STATE *state - state to be set to signaled when the specified due time arrives
 * \param TM_COUNTER *start - get start time
 * \param TM_COUNTER *end - get end time
 * \return void
 */

double elapsed_time (TM_STATE *state, TM_COUNTER *start, TM_COUNTER *end)
{
  return (double) (end->counter - start->counter) / (double) state->freq;
}
#elif __MACH__

#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>

void start_timer (TM_STATE *state, TM_COUNTER *start)
{
  gettimeofday((struct timeval*)&start->osxtmspec, NULL);
}

void stop_timer (TM_COUNTER *end)
{
  gettimeofday((struct timeval*)&end->osxtmspec, NULL);
}

double elapsed_time (TM_STATE *state, TM_COUNTER *start, TM_COUNTER *end)
{
  time_t sec  = end->osxtmspec.tv_sec - start->osxtmspec.tv_sec;
  long   usec = end->osxtmspec.tv_usec - start->osxtmspec.tv_usec;
  if (usec < 0) {sec --; usec += 1000000;}
  return (double)sec + (double)usec / 1000000.;
}

#else // !windows

#include <time.h>
#include <unistd.h>

/*!
 * \brief Start measuring execution time 
 *        Linux version, relying on POSIX clock_gettime() function
 * \param TM_STATE *state - state to be set to signaled when the specified due time arrives
 * \param TM_COUNTER *start - get start time
 * \return void
 */

void start_timer (TM_STATE *state, TM_COUNTER *start)
{
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, (struct timespec*)&start->tmspec);
}

/*!
 * \brief End measuring execution time 
 *        Linux version, relying on POSIX clock_gettime() function
 * \param TM_COUNTER *end - get end time
 * \return void
 */
void stop_timer (TM_COUNTER *end)
{
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, (struct timespec*)&end->tmspec);
}

/*!
 * \brief Measure execution/elapsed time 
 *        Linux version, relying on POSIX clock_gettime() function
 * \param TM_STATE *state - state to be set to signaled when the specified due time arrives
 * \param TM_COUNTER *start - get start time
 * \param TM_COUNTER *end - get end time
 * \return void
 */
double elapsed_time (TM_STATE *state, TM_COUNTER *start, TM_COUNTER *end)
{
  time_t sec  = end->tmspec.tv_sec - start->tmspec.tv_sec;
  long   nsec = end->tmspec.tv_nsec - start->tmspec.tv_nsec;
  if (nsec < 0) {sec --; nsec += 1000000000;} 
  return (double)sec + (double)nsec / 1000000000.;
}

#endif
