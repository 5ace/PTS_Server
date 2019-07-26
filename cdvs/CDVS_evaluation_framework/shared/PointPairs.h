/*
 * This software module was originally developed by:
 *
 *   Politecnico di Milano/ST Microelectronics
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
 * Politecnico di Milano/ST Microelectronics retain full rights to modify and use the code for their own
 * purposes, assign or donate the code to a third party and to inhibit third parties
 * from using the code for products that do not conform to MPEG-related
 * ITU Recommendations and/or ISO/IEC International Standards.
 *
 * This copyright notice must be included in all copies or derivative works.
 * Copyright (c) ISO/IEC 2012.
 */

#pragma once


namespace mpeg7cdvs
{

enum {
	match_2way_INTERSECTION = 0,		///< indicates 2-direction matching pair
	match_2way_DISJOINT1 = 1,			///< indicates 1-direction matching pair: direction A=>B
	match_2way_DISJOINT2 = 2,			///< indicates 1-direction matching pair: direction B=>A
};

/**

 * @class PointPairs
 * Parameter class, used to pass around matched point coordinates.
 * Used by the matching methods in the Feature*List* classes.
 * @author Emanuele Plebani
 * @date 2012
 */
class PointPairs{
public:

	double local_score;			///< matching score provided by local descriptors
	double global_score;		///< matching score provided by global descriptor
	double local_threshold;		///< local threshold used for weighted matching of keypoints
	double global_threshold;	///< global threshold used for matching the global descriptor
	double score;				///< final normalized score (in a range from 0 to 1)

	int nMatched;	///< actual number of matched points
	int size;		///< size of all buffers

	float * x1;		///< x-coordinates of matching points of the first image
	float * x2;		///< y-coordinates of matching points of the first image
	float * y1;		///< x-coordinates of matching points of the second image
	float * y2;		///< y-coordinates of matching points of the second image
	double * weights;	///< weights of each match
	int * match_dirs;	///< indicates the direction of matching (A<=>B, A=>B, B=>A) in 2-way matching (not used in 1-way matching)

	int nInliers;		///< indicates the number of pairs which actually match according to the geometric verification
	int * inlierIndexes;	///< indicates the indices of the pairs that have passed the geometric verification


	PointPairs();							///< default constructor
	PointPairs(int maxPairs);				///< alternate constructor
	PointPairs(const PointPairs& other );	///< copy constructor

	PointPairs& operator=( PointPairs other );		///< assignment operator

	virtual ~PointPairs();			///< destructor

	/**
	 * Return true if the PointPairs instance contains localization information.
	 */
	bool hasLocalizationInliers() const;


	/**
	 * Add a new pair to the matched pairs and increment the nMatched count.
	 * @param x_1 x-coordinate of matching point of the first image
	 * @param x_2 y-coordinate of matching point of the first image
	 * @param y_1 x-coordinate of matching point of the second image
	 * @param y_2 y-coordinate of matching point of the second image
	 * @param weight weight of matching (defaults is zero if not used)
	 * @param mtype direction of matching for 2-way matching (A<=>B, A=>B or B=>A (defaults is A=>B if not used)
	 */
	void addPair(float x_1, float x_2, float y_1, float y_2, double weight = 0, int mtype = match_2way_DISJOINT1);

	/**
	 * Convert matched coordinates to the full resolution of the original image.
	 * This method may change the scale of the coordinates of matching points stored in x1, x2, y1, y2.
	 * @param query_maxres the greater dimension of the (possibly scaled) query image.
	 * @param query_fullres the greater dimension of the original query image.
	 * @param ref_maxres the greater dimension of the (possibly scaled) reference image.
	 * @param ref_fullres the greater dimension of the original reference image.
	 */
	void toFullResolution(int query_maxres, int query_fullres, int ref_maxres, int ref_fullres);

	/**
	 * Get the total weight of all matched points.
	 * @return the total weight of all matched points.
	 */
	double getTotalWeight() const;

	/**
	 * Get the weight of inlier points (these are a subset of all points, which passed the geometric verification test).
	 * @return the weight of inlier points.
	 */
	double getInlierWeight() const;
};

}	// end of namespace
