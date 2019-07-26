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

#include "Parameters.h"
#include <algorithm>
#include <cstring>
#include "CdvsException.h"

/**
 * @class Match
 * Helper class to sort images according to a pair of values.
 * It is used in retrieval to take into account the number of matched points before and after geometric verification.
 * @author Gianluca Francini
 * @date 2011
 */
class Match{
public:
	int featureInd;				///< secondary feature
	int otherFeatureInd;		///< primary feature
	double weight;				///< weight of this match
	
	/**
	 * Predicate used by the STL algoritms sort() and stable_sort() to rank the images according to 
	 * the number of common features.
	 * @param m1 first image values.
	 * @param m2 second image values.
	 * @return true if m1 is strictly greater than m2.
	 */
	static bool sortMatchPredicate(const Match &m1, const Match &m2)
	{
		if(m1.otherFeatureInd == m2.otherFeatureInd)
			return m1.featureInd < m2.featureInd;

		return m1.otherFeatureInd < m2.otherFeatureInd;
	}

	/**
	 * Predicate used by the STL algoritms sort() and stable_sort() to rank the images according to
	 * the number of common features.
	 * @param m1 first image values.
	 * @param m2 second image values.
	 * @return true if m1 is strictly greater than m2.
	 */
	static bool sortMatchByWeight(const Match &m1, const Match &m2)
	{
		if(m1.otherFeatureInd == m2.otherFeatureInd)
			return m1.weight > m2.weight;

		return m1.otherFeatureInd < m2.otherFeatureInd;
	}

};
