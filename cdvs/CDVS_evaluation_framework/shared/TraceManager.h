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
 * Copyright (c) ISO/IEC 2012.
 */


/*
 * TraceManager.h
 *
 *  Created on: 24/lug/2012
 *      Author: massimo
 */

#ifndef TRACEMANAGER_H_
#define TRACEMANAGER_H_

#include "PointPairs.h"
#include "CdvsPoint.h"
#include <cstdio>

/**
 * @class TraceManager
 * Helper class to manage trace files in text or XML format.
 * @author Massimo Balestri, Alberto Messina
 * @date 2012
 */
class TraceManager {
private:
	FILE * trace;
	bool xmlTrace;

public:
	TraceManager();
	virtual ~TraceManager();

	/**
	 * Indicates if tracing has been enabled.
	 * @return true if tracing is enabled; false if no tracing data is currently being written into the trace file.
	 */
	bool isEnabled() const;

	/**
	 * Open a text trace file (alternative to openXml).
	 * @param fname text trace file name.
	 */
	void openTxt(const char * fname);

	/**
	 * Open an XML trace file (alternative to openTxt).
	 * @param fname XML trace file name.
	 */
	void openXml(const char * fname);

	/**
	 * Start the given section in the trace file.
	 * @param section the section name.
	 */
	void start(const char * section);

	/**
	 * Stop the given section in the trace file.
	 * @param section the section name.
	 */
	void stop(const char * section);

	/**
	 * Trace information on the match results.
	 * @param pairs the number and coordinates of matched features
	 * @param proj_bbox the projected bounding box
	 * @param loopCounter the index of this match in the list of matches
	 */
	void matchResults(const mpeg7cdvs::PointPairs & pairs, mpeg7cdvs::CDVSPOINT *proj_bbox, int loopCounter);

	/**
	 * Trace information on the names of the files that constitute the matching pair.
	 * @param q_fname query file name
	 * @param r_fname reference file name
	 */
	void matchPair(const char * q_fname, const char *  r_fname);
};

#endif /* TRACEMANAGER_H_ */
