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


#include "Points.h"

#include <string>
#include <sstream>
#include <iostream>

using namespace Eigen;
using std::vector;
using std::string;
using std::istream;


namespace
{
	inline int randint(int start, int end)
	{
		return rand() % (end - start) + start;
	}


	void removeSeparators(string &line, string const& separators)
	{
		size_t found = line.find_first_of(separators);
		while (found != string::npos)
		{
			line[found] = ' ';
			found = line.find_first_of(separators, found + 1);
		}
	}

}


void read(istream & in, Point2DArray & points)
{
	string line;
	int i = 0;

	while(getline(in, line))
	{
		removeSeparators(line, ",");
		std::istringstream iss(line);
		float point_x, point_y;

		if (iss >> point_x && iss >> point_y)
		{
			points.conservativeResize(i + 1, Eigen::NoChange);
			points.row(i) << point_x, point_y;
			++i;
		}
		else
			break;
	}
}


istream & operator >> (istream & in, Point2DArray & points)
{
	read(in, points);
	return in;
}


Ball2D spread(Point2DArray const& points)
{
	VectorXf ones(points.rows());
	ones.fill(1);

	Point2D center(baricenter(points));

	Point2DArray mDiff = points - ones * center.transpose();
	Point2DArray sqDiff = mDiff.cwiseProduct(mDiff);
	VectorXf sqDistTo = sqDiff.rowwise().sum();

	float radius = sqDistTo.array().cwiseSqrt().mean();

	return Ball2D(center, radius);
}

void inhomogeneousPoints(Point2DArray & inhPoints, HomPoint2DArray const& hPoints)
{
	inhPoints.resize(hPoints.rows(), 2);
	inhPoints.col(0) = (hPoints.col(0).array() / hPoints.col(2).array()).matrix();
	inhPoints.col(1) = (hPoints.col(1).array() / hPoints.col(2).array()).matrix();
}

void homogeneousPoints(HomPoint2DArray & hPoints, Point2DArray const& inhPoints)
{
	hPoints.resize(inhPoints.rows(), 3);
	hPoints.leftCols(2) << inhPoints;
	hPoints.rightCols(1).fill(1.0f);
}


void preconditionTransform(Eigen::Matrix3f & transform, Point2DArray const& points)
{
	Ball2D ball = spread(points);

	transform.setIdentity();
	transform.block(0,0,2,2) =   sqrt(2.0f) * Matrix2f::Identity() / ball.radius;
	transform.block(0,2,2,1) = - sqrt(2.0f) * ball.center / ball.radius;
}

void randomSubset(vector<int> & indices, int nPoints, int subsetSize)
{
	indices.resize(nPoints);
	for (int i = 0; i < nPoints; ++i)
		indices[i] = i;

	for (int i = 0; i < subsetSize; ++i)
	{
		int index = randint(i, nPoints);
		std::swap(indices[i], indices[index]);
	}
	indices.resize(subsetSize);
}
