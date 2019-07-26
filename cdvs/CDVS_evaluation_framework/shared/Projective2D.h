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
 */

#ifndef PROJECTIVE2D_H_
#define PROJECTIVE2D_H_

#include "Points.h"
#include <eigen3/Eigen/Dense>

/**
 * @class Projective2D
 * Computes and performs homographies between two images. 
 * @author Skjalg Lepsoy
 * @date 2011
 */
class Projective2D
{
public:
	Projective2D();

	/**
	 * Check if the projectivity is the identity.
	 * @return true if the projectivity is the identity.
	 */
	bool isIdentity() const;
	
	/**
	 * Transforms input coordinates into output coordinates by a homography.
	 * @param outCoordinates output coordinates.
	 * @param inCoordinates input coordinates.
	 * @param H homography to be used.
	 */
	void moveByHomography(Point2DArray &outCoordinates,
						  Point2DArray const& inCoordinates,
						  Eigen::Matrix3f const& H) const;
		
	/**
	 * Transforms input coordinates into output coordinates using the homography stored in the object.
	 * @param outCoordinates output coordinates.
	 * @param inCoordinates input coordinates.
	 */	
	void moveByHomography(Point2DArray & outCoordinates, Point2DArray const& inCoordinates) const;

	/**
	 * Computes the error when approximating a target set of points by transforming another set of points through a homography.
	 * @param distances output approximation errors.
	 * @param fromPoints input list of points to be tranformed.
	 * @param toPoints input target list of points.
	 * @param H homography to be used.
	 */
	void modelFit(Eigen::VectorXf &distances, Point2DArray const& fromPoints,
				  Point2DArray const& toPoints, const Eigen::Matrix3f &H) const;

	/**
	 * Computes a homography that approximates one set of points by transforming another set of points.
	 * Uses the direct linear transformation method with normalization described by Hartley & Zisserman: Multiview Geometry 2nd edition, Alg. 4.2 page 109.
	 * @param H output homography.
	 * @param fromX input points to be transformed.
	 * @param toX input points to be approximated.
	 */
	void makeHomography(Eigen::Matrix3f &H, Point2DArray const& fromX, Point2DArray const& toX) const;

	/**
	 * Computes a homography that approximates one set of points by transforming another set of points, and stores it internally.
	 * Uses the direct linear transformation method with normalization described by Hartley & Zisserman: Multiview Geometry 2nd edition, Alg. 4.2 page 109.
	 * @param fromX input points to be transformed.
	 * @param toX input points to be approximated.	 
	 */
	void makeHomography(Point2DArray const& fromX, Point2DArray const& toX);

	/**
	 * Identifies correct matches as a subset of matches represented by two sets of points using the RANSAC algorithm.
	 * The transformation between the two sets is a homography.
	 * @param consensusSet output binary vector indicating the correct matches.
	 * @param fromPoints input list of matched points of the first image.
	 * @param toPoints input list of matched points of the second image.
	 * @param nTests input number of iterations to use in RANSAC (typically 10).
	 * @param threshold input maximum distance in order to judge a match as correct.
	 */
	void ransac(Eigen::VectorXi &consensusSet,
				Point2DArray const& fromPoints, Point2DArray const& toPoints,
				int nTests, float threshold);
	
	
	/**
	 * Builds a new homography through the RANSAC algorithm and saves it internally.
	 * @param fromPoints input list of matched points of the first image.
	 * @param toPoints input list of matched points of the second image.
	 * @param nTests input number of iterations to use in RANSAC (typically 10).
	 * @param threshold input maximum distance in order to judge a match as correct.
	 */	
	void ransac(Point2DArray const& fromPoints, Point2DArray const& toPoints,
				int nTests, float threshold);

	
private:
	static const int kMinimalSetSize = 4;

	Eigen::Matrix3f m_Homography;	///< 3x3 matrix defining a homography.

	bool unconditionedRansac(Eigen::VectorXi & consensusSet,
							 HomPoint2DArray const& fromPoints, HomPoint2DArray const& toPoints,
							 int nTests, float threshold);

	static void directLinearTransform(Eigen::Matrix3f &H, HomPoint2DArray const& fromPoints, HomPoint2DArray const& toPoints);
};

#endif // PROJECTIVE2D_H_
