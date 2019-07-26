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

namespace mpeg7cdvs
{

/**
 * A structure containing the x and y coordinate of a point in the image.
 */
typedef struct {
	float x;	///< the X coordinate
	float y;	///< the Y coordinate
} CDVSPOINT;

/**
 * A structure containing the output of a retrieval operation.
 */
typedef struct {
  unsigned int nMatched;	///< number of matched points
  unsigned int nInliers;	///< number of inliers points
  unsigned int index;		///< index of this image in the image DB
  float gScore;				///< score assigned by the global descriptor matching
  float fScore;				///< score assigned by the local descriptors matching
} RetrievalData;


/**
 * Type of matching
 */
enum {
	MATCH_TYPE_DEFAULT = 0,		///< ignore global if local match
	MATCH_TYPE_BOTH = 1,		///< compute both local and global matching scores
	MATCH_TYPE_LOCAL = 2,		///< compute only local matching score
	MATCH_TYPE_GLOBAL = 3		///< compute only global matching score
};


}	// end of namespace
