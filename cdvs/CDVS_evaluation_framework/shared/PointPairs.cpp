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


#include <cstring>
#include "PointPairs.h"
#include "CdvsException.h"

using namespace mpeg7cdvs;

PointPairs::PointPairs() : nMatched(0), nInliers(0), local_score(0), global_score(0), score(0), local_threshold(0), global_threshold(0)
{
	size = 0;
	x1 = x2 = y1 = y2 = NULL;
	weights = NULL;
	match_dirs = NULL;
	inlierIndexes = NULL;
}


PointPairs::PointPairs(int maxPairs) : nMatched(0), nInliers(0), local_score(0), global_score(0), score(0), local_threshold(0), global_threshold(0)
{
	size = maxPairs;
	x1 = new float[size];
	x2 = new float[size];
	y1 = new float[size];
	y2 = new float[size];
	weights = new double[size];
	match_dirs = new int[size];
	inlierIndexes = new int[size];
}

PointPairs::PointPairs(const PointPairs& other )	// copy constructor
{
	local_score = other.local_score;
	global_score = other.global_score;
	score = other.score;
	local_threshold = other.local_threshold;
	global_threshold = other.global_threshold;
	nMatched = other.nMatched;
	nInliers = other.nInliers;

	size = other.size;
	if (size > 0)
	{
		x1 = new float[size];
		x2 = new float[size];
		y1 = new float[size];
		y2 = new float[size];
		weights = new double[size];
		match_dirs = new int[size];
		inlierIndexes = new int[size];
		memcpy(x1, other.x1, sizeof(float) * size);
		memcpy(x2, other.x2, sizeof(float) * size);
		memcpy(y1, other.y1, sizeof(float) * size);
		memcpy(y2, other.y2, sizeof(float) * size);
		memcpy(weights, other.weights, sizeof(double) * size);
		memcpy(match_dirs, other.match_dirs, sizeof(int) * size);
		memcpy(inlierIndexes, other.inlierIndexes, sizeof(int) * size);
	}
	else
	{
		x1 = x2 = y1 = y2 = NULL;
		weights = NULL;
		match_dirs = NULL;
		inlierIndexes = NULL;
	}
}

PointPairs& PointPairs::operator=( PointPairs other )		// assignment operator
{
	//  copy-swap idiom
	// "other" is a local copy (passed by value), so it can be swapped safely

	std::swap(local_score, other.local_score);
	std::swap(global_score, other.global_score);
	std::swap(score, other.score);
	std::swap(local_threshold, other.local_threshold);
	std::swap(global_threshold, other.global_threshold);
	std::swap(nMatched, other.nMatched);
	std::swap(nInliers, other.nInliers);
	std::swap(size, other.size);
	std::swap(x1, other.x1);
	std::swap(x2, other.x2);
	std::swap(y1, other.y1);
	std::swap(y2, other.y2);
	std::swap(weights, other.weights);
	std::swap(match_dirs, other.match_dirs);
	std::swap(inlierIndexes, other.inlierIndexes);
	return *this;
}

PointPairs::~PointPairs()
{
	delete[] x1;
	delete[] x2;
	delete[] y1;
	delete[] y2;
	delete[] weights;
	delete[] match_dirs;
	delete[] inlierIndexes;
}


bool PointPairs::hasLocalizationInliers() const {
	return (nInliers >= 4);
}


void PointPairs::addPair(float x_1, float x_2, float y_1, float y_2, double weight, int mtype)
{
	if (nMatched >= size)
	{
		throw CdvsException("Maximum size reached in PointPairs.addPair");
	}
	else
	{
		x1[nMatched]   = x_1;
		x2[nMatched]   = x_2;
		y1[nMatched]   = y_1;
		y2[nMatched]   = y_2;
		weights[nMatched] = weight;
		match_dirs[nMatched++] = mtype;
	}
}


void PointPairs::toFullResolution(int query_maxres, int query_fullres, int ref_maxres, int ref_fullres)
{
	if (query_fullres > query_maxres)
	{
		float zoom = (float) query_fullres / (float) query_maxres;

		// rescale all query points
		for(int k=0; k<nMatched; k++)
		{
			x1[k] = zoom * x1[k];
			x2[k] = zoom * x2[k];
		}
	}

	if (ref_fullres > ref_maxres)
	{
		float zoom = (float) ref_fullres / (float) ref_maxres;

		// rescale all reference points
		for(int k=0; k<nMatched; k++)
		{
			y1[k] = zoom * y1[k];
			y2[k] = zoom * y2[k];
		}
	}
}


double PointPairs::getTotalWeight() const
{
	double total = 0.0;
	for (int i=0; i<nMatched; ++i)
		total += weights[i];

	return total;
}


double PointPairs::getInlierWeight() const
{
	double weight = 0.0;
	for (int i=0; i<nInliers; ++i)
	{
		weight += weights[inlierIndexes[i]];
	}

	return weight;
}

