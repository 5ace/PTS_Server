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
#include "AlpOctave.h"

namespace mpeg7cdvs
{

/**
 * @class AlpDetector
 * Implementation of the ALP keypoint detector.
 * @author  Gianluca Francini, Massimo Balestri
 * @date 2013
 */
class AlpDetector: public ImageBuffer
{
public:
	AlpDetector();
	virtual ~AlpDetector();

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


	/**
	 * Sort the given ALP features in descending order of importance.
	 * @param featurelist the detected keypoints
	 */
	static void sortPdf(FeatureList & featurelist);

	/**
	 * Sort the given ALP features in descending order of importance.
	 * @param alpFeatures the detected keypoints (without descriptor information and/or storage)
	 * @param imageWidth the width of the (possibly resampled) image
	 * @param imageHeight the height of the (possibly resampled) image
	 */
	static void sortPdf(std::vector<FeatureAlp> & alpFeatures, float imageWidth, float imageHeight);

private:
	static const int maxNumberOctaves = 8;
	AlpOctave octaves[maxNumberOctaves];		// keep one instance per octave
	int verbose;
	std::vector<FeatureAlp> alpKeypoints;

	static bool sortAlpPredicate(const FeatureAlp &f1, const FeatureAlp &f2);

	// constants used in sortPdf()
	static const float DistC[];
	static const float DistP[];
	static const float ScaleC[];
	static const float ScaleP[];
	static const float PeakC[];
	static const float PeakP[];
	static const float DetHessianC[];
	static const float DetHessianP[];
	static const float CurvRatioC[];
	static const float CurvRatioP[];
	static const float CurvSigmaC[];
	static const float CurvSigmaP[];
};

}	// end of namespace
