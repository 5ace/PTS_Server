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
 * Copyright (c) ISO/IEC 2013.
 */

#pragma once
#include "ImageBuffer.h"

/**
 * @class SiftDetector
 * Implementation of the SIFT keypoint detector.
 * @author  Giovanni Cordara, Massimo Balestri
 * @date 2013
 */
class SiftDetector: public mpeg7cdvs::ImageBuffer
{
public:

	SiftDetector();

	virtual ~SiftDetector();

	/**
	 * Detect all SIFT keypoints from this image.
	 * @param featurelist the ouput list of keypoints with their associated features.
	 * @param params the running parameters.
	 * @throws CdvsException in case of error
	 */
	virtual void detect(mpeg7cdvs::FeatureList & featurelist, const mpeg7cdvs::Parameters &params);


	/**
	 * Extract the SIFT descriptor of each keypoint and store it back in featurelist.
	 * @param featurelist the detected keypoints
	 * @param num the absolute maximum number of features to be extracted from this image
	 */
	virtual void extract(mpeg7cdvs::FeatureList & featurelist, size_t num) const;

protected:

	/**
	 * Sort the given SIFT features in descending order of importance.
	 * @param featurelist the detected keypoints
	 */
	void sortPdf(mpeg7cdvs::FeatureList & featurelist) const;

	static void printC();

private:
	static const int nFilters = 8;
	void * filters[nFilters];		// keep one filter per octave

	void * vl_save_gradient (const void *fin);

	// function to prepare descriptor in the final format
	static void transpose_descriptor (float* dst, float* src);

	int verbose;

	// constants used in sortPdf()
	static const float DistC[];
	static const float DistP[];
	static const float ScaleC[];
	static const float ScaleP[];
	static const float OrientC[];
	static const float OrientP[];
	static const float PeakC[];
	static const float PeakP[];
};

