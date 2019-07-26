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

#include <eigen3/Eigen/Dense>

using namespace Eigen;

/**
 * @class DistratEigen
 * Simplified and faster version of DISTRAT, based on the Eigen library.
 * The DISTRAT algorithm performs a geometric consistency check with better performances than RANSAC.
 * This DISTRAT implementation is based on the Eigen C++ library and does not depend on any other class/lib.
 * @author Massimo Balestri
 * @date 13 Jan 2012
 */
class DistratEigen {

public:
	virtual ~DistratEigen();

	/**
	 * Parametric constructor.
	 * @param x1 a vector containing the first coordinate of all the points belonging to the first image
	 * @param x2 contains the second coordinate on the first image
	 * @param y1 contains the first coordinate on the second image
	 * @param y2 contains the second coordinate on the second image
	 * @param size the number of elements of all (x1, x2, y1, y2) vectors
	 */
	DistratEigen(const float *x1, const float *x2, const float *y1, const float *y2, int size);

	/**
	 * Function computing the estimation of the number of inliers (DISTRAT core).
	 * @param useParametric if true the parametric version of Distrat is used, instead of the non-parametric one
	 * @param computeInliers if true the index of inliers is produced
	 * @param percentile acceptable values: 99, 98, 97, 96, 95
	 * @param inlierIndexes the output indexes of inlier points (if the pointer is NULL these values are not provided).
	 * @return the number of inliers
	 */
	int estimateInliers(bool useParametric = false,  bool computeInliers = true, unsigned int percentile=99, int * inlierIndexes = NULL);

	/// result of Goodness of Fit
	bool m_bFitIsGood;

	/// output produced by Goodness of Fit
	float m_c;

	/// Goodness of Fit threshold
	float m_GoFThreshold;


private:
	// constants
	static const float	samplingStep; 				// Sampling step for histogram computation
	static const float 	maxScaling;					// Max scaling for histogram computation
	static const float 	minScaling;					// Min scaling for histogram computation
	static const float  logImageDiagSize;			// logarithm of the maximum image diagonal size
	static const int 	maxNumBins			= 26;	// must be (maxScaling-minScaling)/samplingStep)+1;
	static const int 	gaussianFilterDim 	=  3;	// dimension of Gaussian filter

	// Look-up tables
	static const float LUTchiSquare99[];
	static const float LUTchiSquare98[];
	static const float LUTchiSquare97[];
	static const float LUTchiSquare96[];
	static const float LUTchiSquare95[];
	static const float LUTchiSquare40[];
	static const float LUTchiSquare50[];
	static const float LUTchiSquare60[];
	static const float LUTchiSquare70[];
	static const float LUTchiSquare80[];
	static const float LUTchiSquare90[];

	// gaussian filter taps
	static const float m_GaussianKernel[];

	// variables with known dimensions
	float m_Bins[maxNumBins];						// Vector Storing histogram bins
	float m_Fvalues[maxNumBins];					// Vector Storing F-distribution values
	float m_Edges[maxNumBins + 1];					// Edges for computing histogram
	float m_Hist[maxNumBins + 1];					// Histogram values
	float m_Np[maxNumBins];							// Model functions
	float m_differenceCurve[maxNumBins];			// Difference curve

	// Input vector of matching points from image1
	const float* m_x1;
	const float* m_x2;

	// Input vector of matching points from image2
	const float* m_y1;
	const float* m_y2;

	//number of matchings
	int m_nPoints;

	// Matrix of distances computed from points of image1
	MatrixXf DA;
	// Matrix of distances computed from points of image2
	MatrixXf DB;
	// Log Distance Ratio matrix
	float *LDR;

	//Initial values for image dimensions
	float m_stdA;
	float m_stdB;
	float m_diagScaling;

	//number of bins
	int m_nBins;

	//number of samples in the log distance ration matrix
	long m_nSamples;

	// Threshold for the minimum amount of points needed for computing Distrat
	int minNumPoints;

	float m_nFeatures;

	/* Private methods */
	static void eigPowIteration (const MatrixXf &G, VectorXf &u, float &lambda, int maxIterations);

	/**
	 * Computes the convolution of arrays x and h and produces the result in y.
	 * @param x input array
	 * @param y output array
	 * @param h input array
	 * @param sampleCount output lenght
	 * @param kernelCount h array lenght
	 */
	static void convolution(const float *x, float *y, const float *h, int sampleCount, int kernelCount);

	/**
	 * Computes the distances among a set of bidimensional points.
	 * @param v1 first coordinate (X)
	 * @param v2 second coordinate (Y)
	 * @param nPoints number of points
	 * @param result a matrix containing the resulting distances
	 * @param squared input parameter, if true the square of distances are provided (instead of the plain distances)
	 */
	static void coord2dist(const float *v1, const float *v2, int nPoints, MatrixXf & result, bool squared);

	/**
	 * Computes the average and standard deviation of an array of float.
	 * @param arr the input array of data
	 * @param n the size of the array
	 * @param average output average value
	 * @param std_dev output standard deviation
	 */
	static void vectorStatistics(const float *arr, int n, float *average, float *std_dev);

	/**
	 * Computation of F-distribution
	 * @param input the array of input values
	 * @param output the array of output values
	 * @param nValues the number of elements in the input/ouput arrays
	 * @param scalingFactor the scaling factor to use
	 */
	static void logRootF(const float *input, float*output, int nValues, float scalingFactor);

	/**
	 * Computation of histograms for a vector.
	 * @param hist output histogram of input values, where (nEdges - 1) values will be returned
	 * @param input input array
	 * @param edges limits of histogram bins
	 * @param nValues length of input array
	 * @param nEdges lenght of edges array
	 */
	static void computeHist(float *hist, float *input, float *edges, long nValues, int nEdges);

	/**
	 * Uniform quantization
	 * @param distRatios input array to be quantized
	 * @param samplingGrid quantization centroids array
	 * @param nElem length of input and output array
	 * @param nEdges number of quantization levels
	 * @param results quantized output array
	 */
	static void uniformQuantize(float *distRatios,float *samplingGrid, long nElem,  int nEdges, float *results);

	/**
	 * Stand-alone quicksort, returns a sorted array of values and indexes.
	 * @param vector input/output values
	 * @param beg index of the first element to sort
	 * @param end index of the last element to sort
	 * @param indexes input/output indexes of sorted values
	 */
	static void quicksort(VectorXf & vector, int beg, int end, int *indexes);

	//Initialization
	void prepareParametric();
	void prepareNonParametric();

	//GoodnessOfFit
	bool goodnessOfFit(unsigned int percentile);

	//MLCoherence
	int MLCoherence(int * inliersIndexes);

};
