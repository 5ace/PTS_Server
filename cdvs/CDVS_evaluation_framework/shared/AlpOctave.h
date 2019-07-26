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
 * Copyright (c) ISO/IEC 2014.
 *
 */

#pragma once
#include "FeatureList.h"
#include "Parameters.h"
#include <ostream>

namespace mpeg7cdvs
{

/**
 *  @class FeatureAlp
 *  Definition of keypoint for Alp.
 *  This is smaller than Feature because it does not include the SIFT descriptor.
 */

class FeatureAlp
{
public:

	float x;				  ///< the X coordinate of the ALP keypoint
	float y;				  ///< the Y coordinate of the ALP keypoint
	float sigma;			  ///< the sigma of the Gaussian filter used to detect this point
	float orientation;		  ///< the orientation of the ALP keypoint
	float peak;				  ///< the peak of the ALP keypoint
	float curvRatio;		  ///< the ratio of the curvatures
	float curvSigma;		  ///< the curvature at sigma
	float pdf;				  ///< probability of this point to be matched
	int spatialIndex;		  ///< indicates the order of transmission of this point
	int octave;				  ///< octave of this feature
	int iscale;				  ///< int scale
	int ix;					  ///< original X coordinate
	int iy;					  ///< original Y coordinate

	/**
	 * Print the Feature to cout.
	 */
	friend std::ostream & operator << (std::ostream & outstr, const FeatureAlp & f);

	/**
	 * Compute distance from another FeatureAlp;
	 * @param other the other feature
	 * @return the L1 distance
	 */
	float distL1(const FeatureAlp & other) const;
};

/**
 * @class Filter
 * A class containing a separable Gaussian filter kernel.
 * The filter must be applied in the two spatial directions (horizontally and vertically) in whatever order.
 */
class Filter
{
public:
	static const int maxsize = 27;		///< max size of the filter kernel

	/**
	 * Create a separable Gaussian filter kernel using the given sigma.
	 * @param sigma the sigma of the Gaussian filter
	 */
    Filter(double sigma);

	int ntaps;		///< the number of filter taps
	double sigma;	///< the sigma value corresponding to this Gaussian filter
	float kernel[maxsize];	///< the filter kernel values

	/**
	 * Print the filter data.
	 */
	void print() const;
};

/**
 * @class AlpOctave
 * A container class for a single octave of an image, at a given scale, used
 * to detect and extract ALP key points.
 * @author Massimo Balestri
 * @date 2013
 */
class AlpOctave
{
private:
	bool fast_mode;// // boolean parameter indicating whether partial gradient will be used or not (Added by ETRI)
	static const int minSize = 20;					// minimum size of the octave
	static const int nsigmas = 4;					// number of different filtered images
	static const int nlevels = nsigmas - 2;		// intermediate levels between filtered images
	static const double magnification;			// feature size magnification factor
	static const double sigma0;					// base sigma
	static const double sigman;					// sigma of the original image
	static const double sigmak;					// ratio between sigmas
	static const double dsigma0;					// base differential sigma
	static const float sigmas[];					// sigma values for G1,G2,G3,G4
	static const float inter_sigmas[];			// intermediate sigma values

	static const float peakThreshold;				// minimum peak value
	static const Filter o1g1_filter;				// filter for G1 of octave 1
	static const Filter g2_filter;					// filter for G2 of all octaves
	static const Filter g3_filter;					// filter for G3 of all octaves
	static const Filter g4_filter;					// filter for G4 of all octaves

	static const float lowSigma;					// min allowed sigma
	static const float highSigma;					// max allowed sigma
	static const float sigmaThreshold;			// threshold for the second derivative
	static const float curvRatioThreshold;		// curvature ratio threshold
	static const float sigmaMerge;				// sigma merge threshold
	static const float radiusMerge;				// radius merge threshold
	static const float maxDisplacement;			// maximum displacement of the x and y coordinate
	static const float qcoeff[4][54];				// coefficients of the qcoeff matrix for all sigmas
	static const int   overlapLines;				// how many overlap lines are needed when splitting the source image into blocks

	AlpOctave (const AlpOctave&);					// disallow copy
	AlpOctave & operator= (const AlpOctave&);		// disallow assignment

	void conv2(const float * imagein, const Filter & filter, float * imageout) const;	 // same as MATLAB conv2 function
	static void laplacian(const float * imagein, float sigma, float * imageout, int w, int h);		// compute the Laplacian on an image
	static void print(const Feature & f, float ratio);
	bool isMax(int k, float * R) const;
	bool isMin(int k, float * R) const;
	static int getClosestIndex(float scale);
	bool computeCurvRatioAndCoordinates(FeatureAlp & d) const;
	void allocate(int width, int height, bool split); 
	void computeResponse(int startLineSrc, int startLineDst, int nLines);
	void shiftUp();
	static bool isDuplicate(const Feature & a);		// is the keypoint marked as duplicate?

	// member variables

	int height;				// height of this octave
	int width;				// width of this octave
	int octave;				// octave number
	int extraTopLines;		// extra top lines to allow overlap filtering of split images
	int extraBottomLines;	// extra bottom lines to allow overlap filtering of split images
	int capacity;			// the size of the allocated memory buffers

	float * G1;		// first Gaussian
	float * G2;		// second Gaussian
	float * G3;		// third Gaussian
	float * G4;		// fourth Gaussian

	float * L1;		// first Laplacian
	float * L2;		// second Laplacian
	float * L3;		// third Laplacian
	float * L4;		// fourth Laplacian

	float * A;		// first coefficient
	float * B;		// second coefficient
	float * C;		// third coefficient
	float * D;		// fourth coefficient

	float * tmp;			// temporary buffer
	float * nextG1;			// temporary buffer

	float * minResponse;	// temporary buffer
	float * maxResponse;	// temporary buffer
	float * minScale; 		// temporary buffer
	float * maxScale;		// temporary buffer


	float * gradTheta[4];	// will contain 4 pointers to gradients angle
	float * gradMod[4];		// will contain 4 pointers to gradients module

	// std::vector<FeatureAlp> keypoints;					///< raw key points detected in this octave (without orientation)

	/**
	 * initialize the octave using raw data from itself (the previous octave).
	 * @return true if successful
	 */
	bool init();

	/**
	 * Subsample the given source image 2 to 1 into the destination image.
	 * @param src the source image
	 * @param dst the destination image
	 * @param src_width the source image width
	 * @param src_height the source image height
	 * @param skip_first_line if true, the first line is dropped; else, the second line is dropped.
	 * @return pointer to the next destination address.
	 */
	static float * subsampleImage(const float * src, float * dst, int src_width, int src_height, bool skip_first_line = false);

	
public:

	AlpOctave();					///< constructor

	virtual ~AlpOctave();					///< destructor

	std::vector<FeatureAlp> keypoints;					///< raw key points detected in this octave (without orientation) // Keundong Lee, moved to public

	/**
	 * initialize the octave using the given image.
	 * If the image size is above a given limit, it can be split horizontally into n parts to reduce memory allocation.
	 * @param data the image data
	 * @param width the image width
	 * @param height the image height
	 * @param extra_top_lines extra top lines of the split image (images are split to reduce memory usage if needed)
	 * @param extra_bottom_lines extra bottom lines of the split image (images are split to reduce memory usage if needed)
	 * @param fast the boolean parameter indicating whether partial gradient computation will be used or not (Added by ETRI)
	 * @return true if successful
	 */
	bool init(unsigned char * data, int width, int height, int extra_top_lines = 0, int extra_bottom_lines = 0,bool fast=false); 

	/**
	 * initialize the octave using the previous octave.
	 * @param previous the previous octave
	 * @return true if successful
	 */
	bool init(const AlpOctave & previous);

	bool empty () const;				///< return true if the AlpOctave is empty

	void clear ();						///< clear the AlpOctave

	/**
	 * Detect all ALP keypoints from this octave.
	 * @throws CdvsException in case of error
	 */
	void detect();

	/**
	 * Drop duplicate points between this and the previous octave.
	 * This method assumes that the octave given as parameter is the previous octave.
	 * @param previous the previous octave.
	 */
	void detectDuplicates(AlpOctave & previous);

	/**
	 * Low-memory version of detect duplicates.
	 * @param featurelist the list of keypoints already detected in the previous octaves.
	 */
	void detectDuplicates(FeatureList& featurelist);

	/**
	 * Get Alp keypoints.
	 * These do not include orientation information and descriptor data.
	 * This must be done after init(), detect() and detectDuplicates() because it reuses the same memory buffers.
	 * @param outKeypoints the output vector of keypoints
	 * @param rescale rescale the keypoints to their absolute value
	 */
	void getAlpKeypoints(std::vector<FeatureAlp> & outKeypoints, bool rescale = true);


	/**
	 * Get the final keypoints.
	 * This must be done after init(), detect() and detectDuplicates() because it reuses the same memory buffers.
	 * @param outKeypoints the output vector of keypoints
	 * @param calcDescriptor optionally compute the descriptor of each keypoint
	 * @param rescale rescale the keypoints to their absolute value
	 * @param base_Y displace all Y coordinates using the given base Y
	 */
	void getKeypoints(std::vector<Feature> & outKeypoints, bool calcDescriptor = false, bool rescale = true, int base_Y = 0); 

	/**
	 * Get the final keypoints (fast mode). This function was added by ETRI.
	 * This function computes orientation only for keypoints having a pdf greater than minpdf.
	 * Partial gradient computation was adopted in this function.
	 * @param outKeypoints the output vector of keypoints
	 * @param minpdf is added for preliminary feature selection
	 * @param calcDescriptor optionally compute the descriptor of each keypoint
	 * @param rescale rescale the keypoints to their absolute value
	 * @param base_Y displace all Y coordinates using the given base Y
	 */
	void getKeypoints_fast_mode(std::vector<Feature> & outKeypoints,float minpdf = 0.0, bool calcDescriptor = false, bool rescale = true, int base_Y = 0); 

	/**
	 * Compute the descriptor of the given keypoint.
	 * @param keypoint the input/output keypoint instance
	 * @param rescale if true, the keypoint x and y coordinates are scaled to match the octave of the keypoint
	 */
	void computeDescriptor(Feature & keypoint, bool rescale) const;

	/**
	 * Compute orientation and descriptor of the given ALP keypoint and store it in featurelist.
	 * Each ALP keypoint can produce up to four output keypoints in featurelist, having different orientation.
	 * @param featurelist the output list of keypoints
	 * @param keypoint the input ALP keypoint instance
	 * @param rescale if true, the keypoint x and y coordinates are scaled to match the octave of the keypoint
	 * @param missing how many missing points are currently required to be computed
	 */
	void computeDescriptor(FeatureList & featurelist, const FeatureAlp & keypoint, bool rescale, size_t missing) const;

	/**
	 * This function was added by ETRI which is fast mode of computeDescriptor
	 * Partial gradient computation was adopted in this function
	 */
	void computeDescriptor_fast_mode(Feature & keypoint, bool rescale) const; 

	/**
	 * Process the first octave of the given image (low memory version).
	 * If the image size is above a given limit, it can be split horizontally into n parts (slices) to reduce memory allocation.
	 * Moreover, this implementation extract all keypoints before applying the feature selection.
	 * It must be followed by multiple processNext() calls.
	 * @param data the image data
	 * @param width the image width
	 * @param height the image height
	 * @param featurelist the output list of features
	 * @return true if successful
	 */
	bool processFirst(unsigned char * data, int width, int height, FeatureList & featurelist);

	/**
	 * Process the next octave (low memory version).
	 * This implementation extract all keypoints before applying the feature selection.
	 * @param featurelist the output list of features
	 * @return true if successful, false if the next octave is too small.
	 */
	bool processNext(FeatureList & featurelist);

	/**
	 * Export the height value (read only)
	 * @return the height of this octave
	 */
	int getHeight() const {
		return height;
	}

	/**
	 * Export the width value (read only)
	 * @return the width of this octave
	 */
	int getWidth() const {
		return width;
	}

	/**
	 * Export the octave value (read only), the meaning of which is defined in the vl_sift library:
	 * By convention, the octave of index 0
	 * starts with the image full resolution. Specifying an index greater
	 * than 0 starts the scale space at a lower resolution (e.g. 1 halves the resolution).
	 * @return the octave number
	 */
	int getOctave() const {
		return octave;
	}

};

} 	// end of namespace
