/*
 * This software module was originally developed by:
 *
 *   Telecom Italia/Visual Atoms
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
 * Telecom Italia/Visual Atoms retain full rights to modify and use the code for their own
 * purposes, assign or donate the code to a third party and to inhibit third parties
 * from using the code for products that do not conform to MPEG-related
 * ITU Recommendations and/or ISO/IEC International Standards.
 *
 * This copyright notice must be included in all copies or derivative works.
 * Copyright (c) ISO/IEC 2011.
 *
 */

#pragma once
#include <vector>
#include <cstdio>
#include <ostream>
#include "Parameters.h"
#include "Feature.h"
#include "PointPairs.h"
#include "BitOutputStream.h"
#include "BitInputStream.h"

namespace mpeg7cdvs
{

/**
 * @class FeatureList
 * Container class for all features of an image.
 * @author Gianluca Francini
 * @date 2011
 */
class FeatureList
{
public:
	FeatureList();

	/**
	 * Clear all memory
	 */
	void clear();
	
	/**
	 * Store the resolution of the image from which the SIFT points were extracted.
	 * @param imgWidth the (possibly scaled) image width.
	 * @param imgHeight the (possibly scaled) image height.
	 * @param originalWidth the width of the original image.
	 * @param originalHeight the height the original image.
	 */
	void setResolution(int imgWidth, int imgHeight, int originalWidth, int originalHeight);

	/**
	 * Get the number of features of the image.
	 * @return the number of features currently stored in the features vector.
	 */
	int nFeatures() const;
		
	/**
	 * Add a feature to the current list of features of the image.
	 * @param f the feature to be added.
	 */
	void addFeature(const Feature & f);

	/**
	 * Sort the list of features on the basis of the spatial position of the points, used in the compression of coordinates.
	 */
	void sortSpatialIndex();

	/**
	 * Sort the list of features on the basis of the relevance.
	 */
	void sortRelevance();

	/**
	 * Compare the descriptor contained in this  
	 * FeatureList with the one contained in otherList,
	 * and return the number of different values.
	 * This is mainly used for debugging. 
	 * @param otherList the other list to compare.
	 * @param compressed indicates if both descriptors are compressed
	 * @return the number of different values in the two descriptors.
	 */
	int compareDescriptors(const FeatureList &otherList, bool compressed = false) const;
	
	/**
	 * Compare the coordinates contained in this
	 * FeatureList with the one contained in otherList,
	 * and return the number of different values.
	 * This is mainly used for debugging.
	 * @param otherList the other list to compare.
	 * @param compressed indicates if both descriptors are compressed
	 * @param blockWidth if compressed, indicates the quantization block width (in pixels)
	 * @return the number of different values in the two descriptors.
	 */
	int compareCoordinates(const FeatureList &otherList, bool compressed = false, int blockWidth = 1) const;

	/**
	 * Compare the key points properties contained in this
	 * FeatureList with the one contained in otherList,
	 * and return the number of different values.
	 * This is mainly used for debugging.
	 * @param otherList the other list to compare.
	 * @return the number of different values in the two lists.
	 */
	int compareKeypoints(const FeatureList &otherList) const;

	/**
	 * Write the entire FeatureList into a file.
	 * @param file the output file.
	 */
	void toFile(FILE * file) const;

	/**
	 * Write the entire FeatureList into a file.
	 * @param filename the output filename.
	 */
	void toFile(const char * filename) const;

	/**
	 * Read the entire FeatureList from a file.
	 * @param file the input file.
	 */
	void fromFile(FILE * file);
	
	/**
	 * Read the entire FeatureList from a file.
	 * @param filename the input filename.
	 */
	void fromFile(const char * filename);
	
	/**
	 * Select a subset of features on the basis of the given indices; all other elements are discarded.
	 * @param indices indices of elements to keep.
	 */
	void select(const std::vector<int> &indices);	

	/**
	 * Select a subset of features on the basis of the given range; all other elements are discarded.
	 * @param startInd first elements to keep.
	 * @param endInd last elements to keep.
	 */
	void selectFromTo(int startInd, int endInd);

	/**
	 * Select the first n features; all other elements are discarded.
	 * @param n the number of elements to keep.
	 */
	void selectFirst(int n);

	/**
	 * Performs the compression of the SIFT descriptor. 
	 * @param numberOfElementGroups the number of element groups of this descriptor
	 */
	void compress(int numberOfElementGroups);

	/**
	 * Serialize FeatureList into a stream of bits.
	 * @param writer the bitstream writer object.
	 * @param writeRelevance write also the relevance value, used for the higher querylengths.
	 * @param numFeatures the number of features to encode.
	 */
	void toBinary(BitOutputStream &writer, bool writeRelevance, int numFeatures);

	/**
	 * De-serialize FeatureList from a stream of bits.
	 * @param reader the bitstream reader object.
	 * @param readRelevance read also the relevance value, used for the higher querylengths
	 */
	void fromBinary(BitInputStream &reader, bool readRelevance);
	
	/**
	 * Computes the maximum number of points to be added to the descriptor for a given bitrate.
	 * This method does not assume any pre-computed statistics, just try to encode the features and discover how many bits are used.
	 * @param params the current running parameters
	 * @param targetBits the target number of bits to fill
	 * @return the number of points
	 */
	int computeMaxPoints(const Parameters & params, int targetBits);

	/**
	 * Set the first n points as more relevant.
	 * @param num the number of releval points.
	 */
	void setRelevantPoints(int num);

	/**
	 * Get the number of relevant points.
	 * @return the number of relevant points (generally smaller than the total number of key points).
	 */
	int getRelevantPoints() const;

	/**
	 * Print a summary of the featurelist content.
	 */
	void print() const;

	unsigned int qdescr_size;				///< The number of quantized elements in the key-point features (qdescr)

	int imageHeight;						///< the (possibly scaled) image height.
	int imageWidth;							///< the (possibly scaled) image width.
	int originalHeight;						///< the original image height.
	int originalWidth;						///< the original image width.
	std::vector<Feature> features;			///< the vector of features extracted from the image.
	static const int MAX_NUM_FEATURES = 65536;		///< theoretical limit set by the CDVS syntax

private:
	friend class CompressedFeatureList;

	static const int histogram_groups[4][4];
	static const int priority_list[32][2];
	static const int hm_ter_quant_table[Feature::descrLength][2];

	static bool sortSpatialIndexPredicate(const Feature &, const Feature &);
	static bool sortRelevancePredicate(const Feature &, const Feature &);
};


/**
 * @class CompressedFeatureList
 * Container class for all compressed features of an image.
 * This is only used in the database in order to minimize the memory usage.
 * @date 2014
 */
class CompressedFeatureList
{
private:
	void allocate(int nFeatures, int descLen);
	void clear();

protected:
	int numFeatures;					///< number of features of this image
	int nDescLength;					///< descriptor length in bytes.

public:
	unsigned short *Ycoord;				  	///< the X coordinate of the ALP keypoint
	unsigned short *Xcoord;				  	///< the Y coordinate of the ALP keypoint
	unsigned char * features;				///< all compressed features
	std::string imagefile;					///< pathname of the image file.

	int imageHeight;						///< the (possibly scaled) image height.
	int imageWidth;							///< the (possibly scaled) image width.
	int originalHeight;						///< the original image height.
	int originalWidth;						///< the original image width.

	CompressedFeatureList();			///< default constructor
	CompressedFeatureList(int nFeatures, int descLen);			///< parametric constructor (allocates memory)
	CompressedFeatureList(const CompressedFeatureList & a);		///< copy constructor

	virtual ~CompressedFeatureList();

	/**
	 * Copy constructor from FeatureList, optionally including relevance sorting.
	 * @param other the other FeatureList instance to copy
	 * @param relevantOnly if true, this method copies from a FeatureList instance only the features having the highest relevance
	 */
	CompressedFeatureList(const FeatureList & other, bool relevantOnly = false);

	/**
	 * Assignment operator.
	 * @param other the other CompressedFeatureList instance
	 * @return a CompressedFeatureList instance
	 */
	CompressedFeatureList & operator=(CompressedFeatureList other);

	/**
	 * Swap this instance with another.
	 * @param other the other instance to swap with
	 */
	void swap(CompressedFeatureList & other);

	/**
	 * Get the number of features
	 */
	int nFeatures() const {
		return numFeatures;
	}

	/**
	 * Get the size (number of bytes) of each feature stored in this compressed feature list.
	 * @return the size in bytes of the features.
	 */
	int descrBytes() const {
		return nDescLength;
	}

	/**
	 * Set the name of the image (to be stored for subsequent retrieval).
	 * @param filename pathname of the image.
	 */
	void setFilename(const char *filename);

	/**
	 * Match the features of the current list with the ones contained in otherList in a one way fashion.
	 * The coordinates of the matching points are stored in the pairs parameter.
	 * @param pairs computed matching pairs of points
	 * @param otherList the other list.
	 * @param ratioThreshold the threshold used in the ratio test.
	 * @return the number of matched features.
	 */
	int matchDescriptors_oneWay(PointPairs &pairs, const CompressedFeatureList &otherList, float ratioThreshold) const;

	/**
	 * Match the features of the current list with the ones contained in otherList in a two way fashion.
	 * The coordinates of the matching points are stored in the pairs parameter.
	 * @param pairs computed matching pairs of points
	 * @param otherList the other list.
	 * @param ratioThreshold the threshold used in the ratio test.
	 * @return the number of matched features.
	 */
	int matchDescriptors_twoWay(PointPairs &pairs, const CompressedFeatureList &otherList, float ratioThreshold) const;

	/**
	 * Get the distance of one feature from another feature.
	 * @param mine my feature
	 * @param other the other feature
	 * @param nbytes the number of bytes to use as input data
	 * @return the distance
	 */
    static int getDistance(const unsigned char * mine, const unsigned char * other, int nbytes);

	/**
	 * Read an entire feature list from the given file.
	 * @param filename the pathname of the file containing the feature list.
	 * @return the number of bytes that have been read from the file.
	 */
	std::streamoff readFromFile(char *filename);

	/**
	 * Read an entire feature list from the given input stream.
	 * @param sin the input stream.
	 * @return the number of bytes that have been read from the input stream.
	 */
	std::streamoff read(std::istream& sin);

	/**
	 * Write an entire feature list into the given file.
	 * @param filename the pathname of the file where to store the feature list.
	 * @return the number of bytes that have been written into the file.
	 */
	std::streamoff writeToFile(char *filename) const;

	/**
	 * Write an entire feature list into the given output stream.
	 * @param sout the output stream.
	 * @return the number of bytes that have been written from the input stream.
	 */
	std::streamoff write(std::ostream &sout) const;


	/**
	 * Print a summary of the content.
	 */
	void print() const;
};



} 	// end of namespace
