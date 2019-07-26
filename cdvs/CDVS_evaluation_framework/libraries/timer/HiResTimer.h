 /*
 * This software module was originally developed by:
 *
 *   Telecom Italia
 *
 * in the course of development of ISO/IEC 15938-13 Compact Descriptors for Visual
 * Search standard for reference purposes and its performance may not have been
 * optimized. This software module includes implementation of one or more tools as
 * specified by the ISO/IEC 15938-13 standard.
 *
 * ISO/IEC gives you a royalty-free, worldwide, non-exclusive, copyright license to copy,
 * distribute, and make derivative works of this software module or modifications thereof
 * for use in implementations of the ISO/IEC 15938-13 standard in products that satisfy
 * conformance criteria (if any).
 *
 * Those intending to use this software module in products are advised that its use may
 * infringe existing patents. ISO/IEC have no liability for use of this software module
 * or modifications thereof.
 *
 * Copyright is not released for products that do not conform to audiovisual and image-
 * coding related ITU Recommendations and/or ISO/IEC International Standards.
 *
 * Telecom Italia retain full rights to modify and use the code for their own
 * purposes, assign or donate the code to a third party and to inhibit third parties
 * from using the code for products that do not conform to MPEG-related
 * ITU Recommendations and/or ISO/IEC International Standards.
 *
 * This copyright notice must be included in all copies or derivative works.
 * Copyright (c) ISO/IEC 2011.
 *
 */

#pragma once


#ifdef _OPENMP

#include <omp.h>

/**
 * @class HiResTimer
 * C++ wrapper class for the high resolution timer.
 * @author Massimo Balestri
 * @date 2012
 */
class HiResTimer {
private:
	double m_start;
	double m_end;

public:
	/**
	 * Start the timer.
	 */
	void start(){
		m_start = omp_get_wtime();		// start performance timer
	}

	/**
	 * Stop the timer.
	 */
	void stop() {
		m_end = omp_get_wtime();		// stop timer and accumulate duration
	}

	/**
	 * Get the elapsed time (in seconds) from start to stop.
	 * @return the elapsed time in seconds.
	 */
	double elapsed() {
		return (m_end - m_start); 
	}
};



#else

#include "timer.h"

/**
 * @class HiResTimer
 * C++ wrapper class for the C functions implementing the high resolution timer.
 * @author Massimo Balestri
 * @date 2012
 */
class HiResTimer {
private:
	TM_STATE m_timer;						// time structures
	TM_COUNTER m_start;
	TM_COUNTER m_end;

public:
	/**
	 * Start the timer.
	 */
	void start(){
		start_timer (&m_timer, &m_start);		// start performance timer
	}

	/**
	 * Stop the timer.
	 */
	void stop() {
		stop_timer (&m_end);					// stop timer and accumulate duration
	}

	/**
	 * Get the elapsed time (in seconds) from start to stop.
	 * @return the elapsed time in seconds.
	 */
	double elapsed() {
		return elapsed_time (&m_timer, &m_start, &m_end);
	}
};

#endif
