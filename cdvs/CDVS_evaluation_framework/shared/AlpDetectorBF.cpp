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
 * Copyright (c) ISO/IEC 2014.
 *
 */

#include "AlpDetectorBF.h"
#include "AlpDetector.h"
#include <vector>
#include <iostream>
#include <algorithm>

// exceptions
#include "CdvsException.h"


using namespace std;
using namespace mpeg7cdvs;

AlpDetectorBF::AlpDetectorBF():frequencyfilter()
{
	verbose = 0;
}

/*
 * Detect keypoints using the ALP algorithm.
 *
 * ALP: A Low-degree Polynomial detector
 *
 * keypoint information includes:
 * - Horizontal coordinate
 * - Vertical coordinate
 * - Scale
 * - Orientation
 * - Peak
 * - Second derivative in sigma
 * - Ratio of the spatial curvatures
 */
void AlpDetectorBF::detect(FeatureList & featurelist, const Parameters &params)
{
	verbose = params.debugLevel;	// import the debugging level

	AlpOctaveBF octave;				// keep only one instance of AlpOctave
	octave.filters = &frequencyfilter;
	if (octave.processOctave(featurelist, buffer.data(), width, height))		// process first octave
	{
		while (octave.processOctave(featurelist))			// process next octaves
		{

		}
	}
	AlpDetector::sortPdf(featurelist);	// sort keypoints in descending order of importance
}

void AlpDetectorBF::extract(FeatureList & featurelist, size_t num) const
{
	// extract is already done in this low-memory version of the ALP detector

	if (verbose > 0)
	{
		printHeader("AlpDetectorBF.cpp", featurelist.features.size());
		for(std::vector<Feature>::const_iterator d=featurelist.features.begin(); d<featurelist.features.end(); ++d)
			printDescr(*d);
	}
}

