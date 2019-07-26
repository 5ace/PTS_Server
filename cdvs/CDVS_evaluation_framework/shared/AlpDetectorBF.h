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

#pragma once
#include "ImageBuffer.h"
#include "AlpOctaveBF.h"

namespace mpeg7cdvs
{

/**
 * @class AlpDetectorBF
 * implementation of the ALP keypoint detector by employing block-based processing and frequency domain filtering.
 * @author Jie Chen
 * @date 2014
 */
class AlpDetectorBF: public ImageBuffer
{
public:
	AlpDetectorBF();

	/**
	 * Detect all ALP keypoints from this image.
	 * @param featurelist the ouput list of keypoints with their associated features.
	 * @param params the running parameters.
	 * @throws CdvsException in case of error
	 */
	void detect(FeatureList & featurelist, const Parameters &params);

	/**
	 * Extract the SIFT descriptor of each keypoint and store it back in featurelist.
	 * @param featurelist the detected keypoints
	 * @param num the absolute maximum number of features to be extracted from this image
	 */
	void extract(FeatureList & featurelist, size_t num) const;


private:
	FrequencyFilter frequencyfilter;
	int verbose;
};

} // end of namespace
