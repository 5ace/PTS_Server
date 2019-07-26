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

namespace mpeg7cdvs
{

/**
 * @class Parameters
 * Container for all encoding/decoding parameters associated to each target bitrate defined by MPEG CDVS. 
 * The actual value of each parameter is read from a text file, although default values are provided in this class.
 * Each set of parameters is associated to a profile, and a single instance of this class may only contain parameters for a single profile. 
 * Please note that changing any parameter in the parameters file may break compatibility between encoder and decoder.
 * @author Giovanni Cordara, Massimo Balestri 
 * @date 2011
 */
class Parameters
{
private:
	unsigned int modeId;			///< coding mode identifier (0..nModes)

public:
	Parameters(void);
	~Parameters(void);

	static const int nBits  = 8;	///< number of bits to represent the mode ID
	static const int nModes = 7; 	///< the max number of processing modes

	int descLength;					///< length in bytes of the CDVS descriptor (i.e. 512, 1024, 2048)
	int resizeMaxSize;				///< maximum size of one side of the image 
	int blockWidth;					///< coordinate coding: spatial resolution of the coordinates (max error = blockWidth/2)
	int ctxTableIdx;				///< coordinate coding: index of the context table to use

	char modeExt[40];				///< descriptor extension
	unsigned int selectMaxPoints;	///< feature extraction: max number of points used to describe an image
	unsigned int numRelevantPoints;	///< feature extraction: number of points considered relevant in the retrieval process
	int numberOfElementGroups;		///< feature compression: number of element groups in a compressed local feature descriptor

	float ratioThreshold;			///< DISTRAT: threshold for descriptor matching 
	unsigned int minNumInliers;		///< DISTRAT: min number of inliers after the geometric check
	double wmThreshold;				///< Weighted matching threshold
	double wmThreshold2Way;			///< Two way matching weighted threshold

	double wmMixed;					///< Weighted matching threshold for mixed cases
	double wmMixed2Way;				///< Two way weighted matching threshold for mixed cases

	// execution parameters
	int debugLevel;					///< 0 = off, 1 = on (quiet), 2 = on (verbose), 3 = verbose + dump files 

	int ransacNumTests; 			///< RANSAC: number of iterations in RANSAC 
	float ransacThreshold;			///< RANSAC: distortion threshold to be used by RANSAC

	unsigned int chiSquarePercentile;///< percentile used in DISTRAT for Chi-square computation
    int retrievalLoops;				///< number of loops performed in the final stage of the retrieval process
	double wmRetrieval;				///< Weighted matching threshold for retrieval
	double wmRetrieval2Way;			///< Two way weighted matching threshold for retrieval

	int retrievalMaxPoints;			///< max number of points used in the retrieval experiment
	int queryExpansionLoops;		///< number of query expansion loops to perform in the retrieval experiment

	float scfvThreshold;			///< threshold value to control the sparsity of scfv vector -- add by linjie
	bool hasVar;					///< indicates if using the gradient vector w.r.t the variance of Gaussian function -- add by linjie
	float locationBits;				///< average bits per key point to encode location information;
	bool hasBitSelection;			///< indicates if the Global Descriptor uses the bit selection algorithm to reduce its size

	float gdThreshold;				///< global descriptor threshold
	float gdThresholdMixed;			///< global descriptor threshold for mixed cases

#ifdef USE_MBIT
	int MBIT_threshold;				///< threshold use to control the retrieval speed up (if the optional flag useMBIT is set)
#endif


	/**
	 * Read all modes form a text file into the given vector of parameters.
	 * @param filename filename pathname of the file containing the specific parameter values. If NULL it is ignored.
	 * @param params the vector of parameters to fill.
	 */
	static void readAll(const char *filename, Parameters params[]);

	/**
	 * Load all modes using default parameters into the given vector of parameters.
	 * @param params the vector of parameters to fill.
	 */
	static void readAll(Parameters params[]);

	/**
	 * Read text file and load parameters related to a specified mode.
	 * A maximum of nModes are supported.
	 * @param filename pathname of the file containing the specific parameter values. If NULL it is ignored.
	 * @param mode one of the MPEG CDVS supported modes, from 0 to 6.
	 * @return 0 if successful, an error code otherwise.
	 */
	int readParameters (const char *filename, int mode);

	/**
	 * Load hard-coded parameters related to a specified mode.
	 * A maximum of nModes are supported.
	 * @param mode one of the MPEG CDVS supported modes, from 0 to 6.
	 * @return 0 if successful, an error code otherwise.
	 */
	int readParameters (int mode);

	/**
	 * Get the modeID of this set of parameters.
	 * The mode id cannot be changed; it is read from the parameters file.
	 * @return the mode id value
	 */
	unsigned int getModeID() const;

private:
	int setParameter(const char *paramName, const char *paramValue);	// set a parameter value
};

// define a type for the complete set of parameters (i.e. parameters for all modes)

typedef Parameters ParameterSet[Parameters::nModes];

}  // end of namespace
