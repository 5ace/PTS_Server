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

#include "Projective2D.h"

#include <eigen3/Eigen/SVD>

using namespace Eigen;

void Projective2D::directLinearTransform(Matrix3f &H, HomPoint2DArray const& fromPoints, HomPoint2DArray const& toPoints)
{
	/*
	 * Compute H using the direct linear transform
	 * (see Hartley & Zisserman, 2nd edition, Alg 4.2 page 109)
	*/
	int nPoints = fromPoints.rows();

	Matrix<float, Dynamic, 9> A(2 * nPoints, 9);
	Matrix<float, 2, 9> subA;
	Matrix<float, 3, 9> rightFactor;

	for (int n = 0; n < nPoints; ++n)
	{
		rightFactor.block(0,0,3,3).fill(0);
		rightFactor.block(0,3,3,3) = - Matrix3f::Identity();
		rightFactor.block(0,6,3,3) = toPoints(n,1) * Matrix3f::Identity();
		subA.topRows(1) = fromPoints.row(n) * rightFactor;

		rightFactor.block(0,0,3,3).setIdentity();
		rightFactor.block(0,3,3,3).fill(0);
		rightFactor.block(0,6,3,3) = -toPoints(n,0) * Matrix3f::Identity();
		subA.bottomRows(1) =  fromPoints.row(n) * rightFactor;

		A.block(n*2, 0, 2, 9) = subA;
	}


	JacobiSVD<MatrixXf> Asvd(A.transpose(), ComputeFullU);   // because ComputeFullV does not seem to work
	Matrix<float, 9, 9> V = Asvd.matrixU();  // since Asvd is the svd for A'


	VectorXf nullVec = V.rightCols(1);
	for (int n = 0; n < 3; ++n)
		H.row(n) = nullVec.segment(n*3, 3);
}


Projective2D::Projective2D()
{
	 m_Homography.setIdentity();
}


bool Projective2D::isIdentity() const
{
	Matrix3f normHomography = m_Homography / m_Homography(0,0);
	return normHomography.isIdentity();
}


// moveByHomography //
void Projective2D::moveByHomography(Point2DArray &outCoordinates,
									Point2DArray const& inCoordinates,
									const Matrix3f &H) const
{
	HomPoint2DArray inHomogeneous;
	homogeneousPoints(inHomogeneous, inCoordinates);

	HomPoint2DArray outHomogeneous = inHomogeneous * H.transpose();
	inhomogeneousPoints(outCoordinates, outHomogeneous);
}

// moveByHomography //
void Projective2D::moveByHomography(Point2DArray &outCoordinates, Point2DArray const& inCoordinates) const
{
	moveByHomography(outCoordinates, inCoordinates, m_Homography);
}


// modelFit //
void Projective2D::modelFit(VectorXf &distances, Point2DArray const& fromPoints,
							Point2DArray const& toPoints, Matrix3f const& H) const
{
	Point2DArray approxToX(fromPoints.rows(), 2);
	moveByHomography(approxToX, fromPoints, H);

	ArrayXXf sqDiffTo = (approxToX-toPoints).array().square();
	VectorXf sqDistTo = sqDiffTo.matrix().rowwise().sum();

	distances = sqDistTo.array().sqrt();
}


// makeHomography //
void Projective2D::makeHomography(Matrix3f &H, Point2DArray const& fromX,
											   Point2DArray const& toX) const
{
	Matrix3f Tfrom;
	preconditionTransform(Tfrom, fromX);
	HomPoint2DArray fromXh;
	homogeneousPoints(fromXh, fromX);
	HomPoint2DArray fromXprec = fromXh * Tfrom.transpose();

	Matrix3f Tto;
	preconditionTransform(Tto, toX);
	HomPoint2DArray toXh;
	homogeneousPoints(toXh, toX);
	HomPoint2DArray toXprec = toXh * Tto.transpose();

	Matrix3f uncondH;
	directLinearTransform(uncondH, fromXprec, toXprec);

	// denormalization
	H = Tto.inverse() * uncondH * Tfrom;
}

// makeHomography //
void Projective2D::makeHomography(Point2DArray const& fromX, Point2DArray const& toX)
{
	makeHomography(m_Homography,  fromX,  toX);
}


bool Projective2D::unconditionedRansac(VectorXi & outConsensusSet,
									   HomPoint2DArray const& fromPoints, HomPoint2DArray const& toPoints,
									   int nTests, float threshold)
{
	int biggestYet = -1;

	Point2DArray inhFromPoints;
	inhomogeneousPoints(inhFromPoints, fromPoints);
	Point2DArray inhToPoints;
	inhomogeneousPoints(inhToPoints, toPoints);

	std::vector<int> subset;
	Matrix<float, kMinimalSetSize, 3> sampleMatrixFrom;
	Matrix<float, kMinimalSetSize, 3> sampleMatrixTo;

	Matrix3f Hhyp;

	int nPoints = fromPoints.rows();
	VectorXf distances(nPoints);
	VectorXi hypoteticConsensusSet(nPoints);
	outConsensusSet.resize(nPoints);

	for (int sampleNo = 0; sampleNo < nTests; ++sampleNo)
	{
		randomSubset(subset, nPoints, kMinimalSetSize);
		for (int k = 0; k < kMinimalSetSize; ++k)
		{
			sampleMatrixFrom.row(k) = fromPoints.row(subset[k]);
			sampleMatrixTo.row(k)	= toPoints.row(subset[k]);
		}

		// estimation of model parameters
		directLinearTransform(Hhyp, sampleMatrixFrom,  sampleMatrixTo);

		// get the consensus set
		modelFit(distances, inhFromPoints, inhToPoints, Hhyp);
		for (int k = 0; k < nPoints; ++k)
		{
			hypoteticConsensusSet(k) = (distances(k) < threshold) ? 1 : 0;
		}


		// is it the biggest so far?
		if (hypoteticConsensusSet.sum() > biggestYet)
		{
			outConsensusSet = hypoteticConsensusSet;
			biggestYet		= hypoteticConsensusSet.sum();
		}
	}


	// re-estimation of model parameters
	int consensusSize = biggestYet;
	if (consensusSize > 0)
	{
		HomPoint2DArray consensusFrom(consensusSize, 3);
		HomPoint2DArray consensusTo(consensusSize, 3);
		int ck = 0;
		for (int k = 0; k < nPoints; ++k)
		{
			if (outConsensusSet(k))
			{
				consensusFrom.row(ck) = fromPoints.row(k);
				consensusTo.row(ck++) = toPoints.row(k);
			}
		}

		directLinearTransform(m_Homography, consensusFrom,  consensusTo);
		modelFit(distances, inhFromPoints, inhToPoints, m_Homography);

		for (int k = 0; k < nPoints; ++k)
			outConsensusSet(k) = (distances(k) < threshold) ? 1 : 0;

		return true;
	}
	else
	{
		return false;		// not found
	}
}

// ransac //
void Projective2D::ransac(VectorXi &outConsensusSet,
						  Point2DArray const& fromPoints, Point2DArray const& toPoints,
						  int nTests, float threshold)
{
	Matrix3f Tfrom;
	preconditionTransform(Tfrom, fromPoints);
	HomPoint2DArray fromXh;
	homogeneousPoints(fromXh, fromPoints);
	HomPoint2DArray fromXprec = fromXh * Tfrom.transpose();

	Matrix3f Tto;
	preconditionTransform(Tto, toPoints);
	HomPoint2DArray toXh;
	homogeneousPoints(toXh, toPoints);
	HomPoint2DArray toXprec = toXh * Tto.transpose();

	if (unconditionedRansac(outConsensusSet, fromXprec, toXprec, nTests, threshold))
		m_Homography = Tto.inverse() * m_Homography * Tfrom;
	else
		m_Homography.setIdentity();
}


void Projective2D::ransac(Point2DArray const& fromPoints, Point2DArray const& toPoints, int nTests, float threshold)
{
	VectorXi consensusSet;
	ransac(consensusSet, fromPoints, toPoints, nTests, threshold);
}
