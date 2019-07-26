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
#include <stdio.h>
#include <vector>

namespace mpeg7cdvs
{

// Arithmetic coding is not used in the TM and this switch is ONLY for debugging purposes - DO NOT CHANGE IT.
//#define USE_DESCR_ARITHMETIC_CODER			// If defined arithmetic coder is used for encoding of descriptor elements, otherwise prefix coding is used.


/**
 * @class Feature
 * Container class for the features of a single point (storing coordinates, scale, orientation, peak and descriptor of a point). 
 * @author Gianluca Francini
 * @date 2011
 */
class Feature
{
public:
	Feature(void);

	// default copy constructor, assignment op, and destructor;
	
	/**
	 * Write the feature into a file.
	 * @param file the output file.
	 */
	void toFile(FILE * file) const;

	/**
	 * Write the feature into a file.
	 * @param file the output file.
	 */
	void fromFile(FILE * file);

	static const unsigned int descrLength = 128;		///< the size of a feature (key point)

	float x;				  ///< the X coordinate of the ALP keypoint
	float y;				  ///< the Y coordinate of the ALP keypoint
	float scale;			  ///< the scale of the ALP keypoint
	float orientation;		  ///< the orientation of the ALP keypoint
	float peak;				  ///< the peak of the ALP keypoint
	float curvRatio;		  ///< the ratio of the curvatures
	float curvSigma;		  ///< the curvature at sigma
	float descr[descrLength]; ///< the SIFT descriptor of the ALP keypoint
	float pdf;				  ///< probability of this point to be matched
	int spatialIndex;		  ///< indicates the order of transmission of this point
	unsigned short relevance; ///< relevance of the keypoint, computed on the basis of his characteristics
	int qdescr[descrLength];  ///<  the quantized (ternarized) descriptor values.
	int octave;				  ///< octave of this feature
	int iscale;				  ///< int scale
};

}  	// end of namespace
