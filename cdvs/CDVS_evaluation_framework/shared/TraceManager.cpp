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
 * TraceManager.cpp
 *
 *  Created on: 24/lug/2012
 *      Author: massimo
 */

#include "TraceManager.h"
#include "CdvsException.h"
#include <string>

using namespace std;
using namespace mpeg7cdvs;

TraceManager::TraceManager() {
	trace = NULL;
	xmlTrace = false;
}

TraceManager::~TraceManager() {
	if (trace != NULL)
		fclose(trace);
}

void TraceManager::openXml(const char* fname) {
	xmlTrace = true;
	if (trace != NULL)
	{
		fclose(trace);
		trace = NULL;
	}

	if ((trace = fopen (fname, "wt+")) == NULL)
	{
		string msg("Can't open/create trace file: ");
		msg.append(fname);
		throw CdvsException(msg);
	}
	fprintf(trace,"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
}

bool TraceManager::isEnabled() const {
	return (trace != NULL);
}

void TraceManager::openTxt(const char* fname) {
	xmlTrace = false;
	if (trace != NULL)
	{
		fclose(trace);
		trace = NULL;
	}

	if ((trace = fopen (fname, "wt+")) == NULL)
	{
		string msg("Can't open/create trace file: ");
		msg.append(fname);
		throw CdvsException(msg);
	}
	fprintf(trace,"==== trace starting ====\n");
}

void TraceManager::start(const char * section) {
	if ((trace != NULL) && xmlTrace)
  	    fprintf(trace,"<%s>\n", section);
}

void TraceManager::stop(const char * section) {
	if ((trace != NULL) && xmlTrace)
	    fprintf(trace,"</%s>\n", section);
}


void TraceManager::matchResults(const PointPairs & pairs, CDVSPOINT *proj_bbox, int loopCounter) {

	// log data into XML trace file
	if ((trace != NULL) && xmlTrace){
			fprintf(trace,"<keypoints image=\"1\" status=\"all\">");
			for (int l=0;l<pairs.nMatched;l++){
				fprintf(trace,"<keypoint seq=\"%d\" x=\"%f\" y=\"%f\"/>\n",l,pairs.x1[l],pairs.x2[l]);
				}
			fprintf(trace,"</keypoints>\n");
			fprintf(trace,"<keypoints image=\"2\" status=\"all\">");
			for (int l=0;l<pairs.nMatched;l++){
				fprintf(trace,"<keypoint seq=\"%d\" x=\"%f\" y=\"%f\"/>\n",l,pairs.y1[l],pairs.y2[l]);
				}
			fprintf(trace,"</keypoints>\n");
		}

	// log data into text trace file
	if ((trace != NULL) && !xmlTrace)
	{
		if (loopCounter == 0)
		{
			fprintf(trace, "index, nmatched, inliers,    weight,  w-thresh,  g-thresh,   g-score,     score, [bounding box]\n");
		}

		fprintf(trace, "%5d %9d %8d %10.6f %10.6f %10.6f %10.6f %10.6f", loopCounter + 1, pairs.nMatched, pairs.nInliers, pairs.getInlierWeight(), pairs.local_threshold, pairs.global_threshold, pairs.global_score, pairs.score);

		if (proj_bbox != NULL)
			fprintf(trace, "  (%.1f %.1f)(%.1f %.1f)(%.1f %.1f)(%.1f %.1f)\n",
			proj_bbox[0].x, proj_bbox[0].y,
			proj_bbox[1].x, proj_bbox[1].y,
			proj_bbox[2].x, proj_bbox[2].y,
			proj_bbox[3].x, proj_bbox[3].y
			);	// log data into trace file
		else
			fprintf(trace, "\n");
	}

	// log data to XML trace file
	if ((trace != NULL) && xmlTrace)
	{
		if (proj_bbox != NULL)
			fprintf(trace, "<matching_info seq=\"%5d\" nmatched=\"%9d\" nInliers=\"%8d\" score=\"%6.2f\" bbox0=\"(%.1f %.1f)\" bbox1=\"(%.1f %.1f)\" bbox2=\"(%.1f %.1f)\" bbox3=\"(%.1f %.1f)\" />\n", loopCounter + 1, pairs.nMatched, pairs.nInliers, pairs.score,
			proj_bbox[0].x, proj_bbox[0].y,
			proj_bbox[1].x, proj_bbox[1].y,
			proj_bbox[2].x, proj_bbox[2].y,
			proj_bbox[3].x, proj_bbox[3].y
			);	// log data into trace file
		else
			fprintf(trace, "<matching_info seq=\"%5d\" nmatched=\"%9d\" nInliers=\"%8d\" score=\"%6.2f\" />\n", loopCounter + 1, pairs.nMatched, pairs.nInliers, pairs.score);

		const int* indices = pairs.inlierIndexes;

		fprintf (trace,"<keypoints image=\"1\" status=\"selected\">\n");
		for (int k=0;k<pairs.nInliers;k++){
			fprintf(trace,"<keypoint seq=\"%d\" x=\"%f\" y=\"%f\" />\n",k,pairs.x1[indices[k]],pairs.x2[indices[k]]);
		}
		fprintf (trace,"</keypoints>\n");
		fprintf (trace,"<graphics>\n");
		for (int k=0;k<pairs.nInliers;k++){
			fprintf(trace," -draw 'circle %d,%d %d,%d' ", ((int)pairs.x1[indices[k]])-2,((int)pairs.x2[indices[k]])-2,((int)pairs.x1[indices[k]])+2,((int)pairs.x2[indices[k]])+2);
		}
		fprintf (trace,"</graphics>\n");
		fprintf (trace,"<keypoints image=\"2\" status=\"selected\">\n");
		for (int k=0;k<pairs.nInliers;k++){
			fprintf(trace,"<keypoint seq=\"%d\" x=\"%f\" y=\"%f\" />\n",k,pairs.y1[indices[k]],pairs.y2[indices[k]]);
		}
		fprintf (trace,"</keypoints>\n");
                fprintf (trace,"<graphics>\n");
		for (int k=0;k<pairs.nInliers;k++){
			fprintf(trace," -draw 'circle %d,%d %d,%d' ", ((int)pairs.y1[indices[k]])-2,((int)pairs.y2[indices[k]])-2,((int)pairs.y1[indices[k]])+2,((int)pairs.y2[indices[k]])+2);
		}
		fprintf(trace,"</graphics>\n");
	}
}

void TraceManager::matchPair(const char * q_fname, const char *  r_fname)
{
	if ((trace != NULL) && xmlTrace)
	{
		fprintf(trace,"<image role=\"query\">%s</image><image role=\"reference\">%s</image>",q_fname,r_fname);
	}
}




