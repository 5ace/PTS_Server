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
 * Copyright (c) ISO/IEC 2013.
 *
 */

#pragma once
#include "Buffer.h"
#include "Parameters.h"
#include "FeatureList.h"
#include "AbstractDetector.h"

namespace mpeg7cdvs
{

/**
 * @class ImageBuffer
 * A container class for a bidimensional image; it's the base class of all keypoint detector classes.
 * This class properly deallocates memory when an exception is thrown.
 * @author  Giovanni Cordara, Massimo Balestri
 * @date 2013
 */
class ImageBuffer : public AbstractDetector
{
public:
	Buffer buffer;			///< buffer containing the image data

	ImageBuffer();

	virtual ~ImageBuffer();

	/**
	 *  swap the content of two ImageBuffer(s)
	 *  @param other the other ImageBuffer instance
	 */
	void swap (ImageBuffer & other);

	/**
	 * Read a planar luminance image from a buffer.
	 * This method can be used only if the image is already available as an 8-bit planar luminance buffer.
	 * @param width width of the image
	 * @param height height of the image
	 * @param buffer the buffer containing the luminance component of the image
	 */
	void read(int width, int height, const unsigned char * buffer);

	/**
	 * Convert this image into a destination image having a different resolution by filtering and sampling the original image.
	 * @param dest the destination image
	 * @throws CdvsException in case or error
	 */
	void resample(ImageBuffer & dest) const;

	/**
	 * Resample this image using the given reduction factor (the original image is discarded).
	 * @param rfactor the reduction factor (must be < 1)
	 * @throws CdvsException in case or error
	 */
	void resample(double rfactor);

	/**
	 * Resample this image if either the horizontal or the vertical dimension of the image is greater that the given maximum size.
	 * @param maxSize the maximum size to set
	 * @throws CdvsException in case or error
	 */
	void resampleIfGreater(int maxSize);

	/**
	 * Print the given list of features.
	 * @param f a vector of features
	 * @param source the name of the keypoint detector that produced the given list of features
	 */
	static void print(const std::vector<Feature> & f, const char * source);


	/**
	 * Print the header of printDescr().
	 * @param source the name of the detector under test
	 * @param npoints the number of points that will be printed
	 */
	static void printHeader(const char * source, size_t npoints);

	/**
	 * Print the descriptor of the given feature.
	 * @param d the feature
	 */
	static void printDescr(const Feature & d);

	/**
	 * write a BMP file containing the given image (luminance only).
	 */
	static void writeBMP(const char * filename, const float * source, int w, int h);

	/**
	 * write a raw data file containing h, w and the source float matrix.
	 */
	static void writeRawData(const char * filename, const float * source, int w, int h);


protected:


	/**
	 * Resize the current image
	 * @param newheight the new height
	 * @param newwidth the new width
	 * @return true if successful
	 */
	bool resize(int newheight, int newwidth);
	
	/**
	 * Perform scalar quantization on given data.
	 * @param value the value to quantize
	 * @param data the quantization centroids
	 * @param size the size of the quantization centroid array
	 * @return the index such that the distance between value and data[index] is minimum
	 */
	static unsigned int scalarQuantize(float value, const float *data, size_t size);

	/**
	 * Perform scalar quantization on given data and return the corresponding output.
	 * @param value the value to quantize
	 * @param data the quantization centroids
	 * @param output the corresponding output values (probabilities)
	 * @param size the size of the quantization centroid array
	 * @return the probability corresponding to the given value
	 */
	static float fastScalarQuantize(float value, const float *data, const float *output, size_t size);

	/**
	 * Perform scalar quantization then interpolate the output.
	 * @param value the value to quantize
	 * @param data the quantization centroids
	 * @param output the corresponding output values (probabilities)
	 * @param size the size of the quantization centroid array
	 * @return the interpolated probability corresponding to the given value
	 */
	static float fastInterpolate(float value, const float *data, const float *output, size_t size);


	/**
	 * Predicate used to order features.
	 * @param a first element to compare
	 * @param b second element to compare
	 * @return true if a > b
	 */
	static bool sortPdfPredicate(const Feature & a, const Feature & b);

	/**
	 * Predicate used to order float values.
	 * @param a first element to compare
	 * @param b second element to compare
	 * @return true if a > b
	 */
	static bool sortPredicate(const float & a, const float &b); 

private:

	/**
	 * Change the image resolution by filtering and sampling the original image.
	 * @param pSrc_image the source image
	 * @param src_width the width of the source image
	 * @param src_height the height of the source image
	 * @param n number of components of the source image
	 * @param dst_image the destination image
	 * @param dst_width the width of the destination image
	 * @param dst_height the height of the destination image
	 * @param pFilter the resampling filter to use
	 * @param debugLevel the debug level
	 * @throws CdvsException in case or error
	 */
	static void resampleImage(
	   const unsigned char* pSrc_image,
	   int src_width,
	   int src_height,
	   int n,			// num of components
	   unsigned char* dst_image,
	   int dst_width,
	   int dst_height,
	   const char* pFilter,
	   int debugLevel);

	/**
	 * Predicate used to sort the given ALP keypoints in ascending order of sigma,
	 * just to print them in some predefined order.
	 */
	static bool sortBySigmaPredicate(const Feature &f1, const Feature &f2);
};

}  // end of namespace
