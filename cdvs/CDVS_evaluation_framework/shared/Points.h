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


#ifndef POINTS_H_
#define POINTS_H_

#include <eigen3/Eigen/Dense>
#include <vector>
#include <iosfwd>


typedef Eigen::Vector2f Point2D;
typedef Eigen::Vector3f HomPoint2D;
typedef Eigen::Matrix<float, Eigen::Dynamic, 2, Eigen::RowMajor> Point2DArray;
typedef Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor> HomPoint2DArray;


struct Ball2D
{
	Point2D center;
	float radius;

	Ball2D(Point2D const& center = Point2D(0.0, 0.0),
			float radius = 1.0) :
		center(center), radius(radius) {}
};


void read(std::istream & in, Point2DArray & points);

std::istream & operator >> (std::istream & in, Point2DArray & points);


inline Point2D baricenter(Point2DArray const& points)
{
	return Point2D(points.col(0).mean(),
					points.col(1).mean());
}


Ball2D spread(Point2DArray const& points);


void inhomogeneousPoints(Point2DArray & inhPoints,
							HomPoint2DArray const& hPoints);

inline Point2DArray inhomogeneousPoints(HomPoint2DArray const& hPoints)
{
	Point2DArray res;
	inhomogeneousPoints(res, hPoints);
	return res;
}


void homogeneousPoints(HomPoint2DArray & hPoints,
						Point2DArray const& inhPoints);

inline HomPoint2DArray homogeneousPoints(Point2DArray const& inhPoints)
{
	HomPoint2DArray res;
	homogeneousPoints(res, inhPoints);
	return res;
}


void preconditionTransform(Eigen::Matrix3f & transform, Point2DArray const& points);

inline Eigen::Matrix3f preconditionTransform(Point2DArray const& points)
{
	Eigen::Matrix3f transform;
	preconditionTransform(transform, points);
	return transform;
}


void randomSubset(std::vector<int> & indices, int nPoints, int subsetSize);


#endif // POINTS_H_
