/*
 * This software module was originally developed by:
 *
 *   Stanford University, Huawei Technologies Co., Ltd.
 *
 * in the course of development of ISO/IEC <number> (Compact Descriptors for Visual 
 * Search) standard for reference purposes and its performance may not have been 
 * optimized. This software module includes implementation of one or more tools as 
 * specified by the ISO/IEC <number> standard.
 *
 * ISO/IEC gives you a royalty-free, worldwide, non-exclusive, copyright license to copy, 
 * distribute, and make derivative works of this software module or modifications thereof 
 * for use in implementations of the ISO/IEC <number> standard in products that satisfy 
 * conformance criteria (if any).
 *
 * Those intending to use this software module in products are advised that its use may 
 * infringe existing patents. ISO/IEC have no liability for use of this software module 
 * or modifications thereof.
 *
 * Copyright is not released for products that do not conform to audiovisual and image-
 * coding related ITU Recommendations and/or ISO/IEC International Standards.
 *
 * Stanford University and Huawei Technologies Co., Ltd. retain full rights to modify and use the code for their own 
 * purposes, assign or donate the code to a third party and to inhibit third parties 
 * from using the code for products that do not conform to MPEG-related 
 * ITU Recommendations and/or ISO/IEC International Standards.
 *
 * This copyright notice must be included in all copies or derivative works.
 * Copyright (c) ISO/IEC 2012.
 *
 */

#pragma once

#include "BitOutputStream.h"
#include "BitInputStream.h"
#include "FeatureList.h"
#include "Parameters.h"

namespace mpeg7cdvs
{

/**
 * @class CsscCoordinateCoding
 * Class that converts the coordinates of all descriptors of an image into a bitstream, and vice versa.
 * @date 2012
 */
class CsscCoordinateCoding
{
public:
	static const int	SUM_HIST_COUNT_SIZE = 64;										///< Histogram count context lenght
	static const int	CONTEXT_RANGE = 5;												///< Sum-based context range					
	static const int 	MAXIMUM_SUM_CONTEXT = (2*CONTEXT_RANGE*CONTEXT_RANGE + CONTEXT_RANGE) ;	///< Maximum value of sum context
	
	/**
	 * @struct CircularSumContext
	 * Basic structure for Cssc
	 */
	struct CircularSumContext {
		long	 vCount[SUM_HIST_COUNT_SIZE];		///< Histogram Count Arithmetic Coding model initialization data
		long	 vInitialMap[2];					///< Circular scan map Arithmetic Coding model initialization data (initial)
		long	 vMap[MAXIMUM_SUM_CONTEXT+1][2];	///< Circular scan map Arithmetic Coding model initialization data (following)
	} ;

private:

	static const int CONTEXT_TABLE[12][6];
	static const int HIST_COUNT_TABLE[12][3];

	CsscCoordinateCoding(const CsscCoordinateCoding& p);									// copy constructor - set private to avoid its usage.
	CsscCoordinateCoding &operator=(const CsscCoordinateCoding&);							// assignment operator - set private to avoid its usage.
	static int compareArray(int * a, int * b, int size);									// compare two int arrays
	static int compareArray(int * a, int * b, int iWidth, int iHeight, bool bTranspose);	// compare two int arrays
	static int compareArray(bool * a, bool * b, int size);									// compare two bool arrays

	// Input parameters
	int 	m_debugLevel;		// If (debugLevel > 0) some debugging info will be printed
	int 	m_blockWidth;		// Size of the grid used to build the histogram map
	int		m_queryLength;		// To identify operating point

	// Histogram count data structure
	int* 	m_hCount; 			// Pointer to the histogram count 
	int 	m_hCountSize;		// Histogram count size

	// Histogram map data structure
	int* 	m_hMap;				// Pointer to histogram map
	int 	m_hMapSizeX;		// X Dimension of histogram map
	int 	m_hMapSizeY;		// Y Dimension of histogram map
	bool 	m_bIsTransposed;    // Indicating whether the circular scanned matrix needs to be transposed

	// For context entropy coding
	bool 	m_bTrainingMode;
	CircularSumContext	m_csc;	

	// Utility functions
	void 	TransposeMatrix( int* &pMatrix, int &iRows, int &iCols);


public:	

	/**
	 * Constructor using the given parameters to set the CsscCoordinateCoding behaviour.
	 * @param param the set of parameters to initialize this object.
	 */
	CsscCoordinateCoding(const Parameters & param);

	virtual ~CsscCoordinateCoding();							// destructor

	/**
	 * Convert the stored information into a binary stream.
	 * @param writer the bitstream writer object.
	 */
	void toBinary(BitOutputStream & writer);

	/**
	 * Convert a binary stream into the stored information.
	 * @param reader the bitstream reader object.
	 */
	void fromBinary(BitInputStream & reader);
	 
	/**
	 * Compare this instance with another one. 
	 * @param other the other instance.
	 * @return 0 if equal, or the number of different values if different.
	 */
	int compare(const CsscCoordinateCoding & other);

	/**
	 * Export the value of the histogram count and size.
	 * @param histogramCountSize size of histogram count
	 * @param histogramMapSizeX	size of histogram map (X)
	 * @param histogramMapSizeY size of histogram map (Y)
	 */
	void exportVars(unsigned int & histogramCountSize, unsigned int & histogramMapSizeX, unsigned int & histogramMapSizeY) const;


	/**
	 * Generation of new matrix representation based on circular scanning. 
	 * @param featurelist list of keypoints.
	 * @param numPoints the number of features to encode (only the first numPoint features in featurelist will be encoded)
	 */
	void 	generateHistogramMap(FeatureList & featurelist, int numPoints);

	/**
	 * Reconstruction of the original histogram map starting from circular scanning representation. 
	 * @param descriptors reconstructed list of keypoints and descriptors
	 */
	void 	generateFeatureList(FeatureList & descriptors);


	// Functions for context training
	void 	StartTrainingMode();
	void 	EndTrainingMode();
	int 	AddImageSample(FeatureList& featurelist);

	// Functions for context I/O
	int     writeSeparateContext(char *filename);
	static int		readSeparateContext( char *filename, CircularSumContext &cCsc);
	int		readSeparateContext( char *filename);
};

} 	// end of namespace
