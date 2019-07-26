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

#include "FeatureList.h"
#include <vector>
#include <limits>       // std::numeric_limits

namespace mpeg7cdvs
{

typedef std::vector<unsigned int> recallGraphNode_t;
typedef std::vector<recallGraphNode_t> recallGraph_t;

static const size_t NOT_FOUND = std::numeric_limits<size_t>::max();

/**
 * @class Database
 * The image database implementation containing helper methods for image retrieval.
 * @author Gianluca Francini
 * @date 2011
 */
class Database
{
public:
	Database();

	/**
	 * get the mode shared by all descriptors in the DB.
	 * @return the mode id.
	 */
	unsigned int getMode() const {
		return modeId;
	}


	/**
	 * Add an image to the database.
	 * @param featList features of the image being added.
	 * @param filename pathname of the image being added.
	 * @return the position (index) of the image in the database. 
	 */
	size_t addImage(const FeatureList & featList, const char *filename);

	/**
	 * Replace an image to the database.
	 * @param index index of the image to replace.
	 * @param features features of the new image.
	 * @param filename pathname of the new image.
	 * @return the position (index) of the replaced image in the database. 
	 */
	size_t replaceImage(size_t index, const FeatureList & features, const char * filename);

	/**
	 * Merge two databases. 
	 * The databases must have an index with the same number of codewords (if this is not empty).
	 * @param otherDB the other db to merge to the current one.
	 */
	void merge(const Database &otherDB);

	/**
	 * Euclidean match of a query against an image contained in the DB, with index imageDBindex in a one way fashion.
	 * The coordinates of the matched points are stored in the PointPairs container class.
	 * @param pairs computed matching pairs of points
	 * @param query features of the query image.
	 * @param imageDBindex index of the image contained in the database that will be compared to the query.
	 * @param ratioThreshold the threshold used in the ratio test.
	 * @return number of matched points.
	 */
	int matchCompressedDescriptors_oneWay(PointPairs &pairs, const CompressedFeatureList &query, int imageDBindex, float ratioThreshold) const;

	/**
	 * Euclidean match of a query against an image contained in the DB, with index imageDBindex in a two way fashion.
	 * The coordinates of the matched points are stored in the PointPairs container class.
	 * @param pairs computed matching pairs of points
	 * @param query features of the query image.
	 * @param imageDBindex index of the image contained in the database that will be compared to the query.
	 * @param ratioThreshold the threshold used in the ratio test.
	 * @return number of matched points.
	 */
	int matchCompressedDescriptors_twoWay(PointPairs &pairs, const CompressedFeatureList &query, int imageDBindex, float ratioThreshold) const;

	/** 
	 * Read an entire database from the given file.
	 * @param filename the pathname of the file containing the database.
	 * @return the number of bytes that have been read from the file.
	 */
	std::streamoff readFromFile(const char *filename);

	/**
	 * Read an entire database from the given input stream.
	 * @param sin the input stream.
	 * @return the number of bytes that have been read from the input stream.
	 */
	std::streamoff read(std::istream& sin);

	/**
	 * Read only the modeId and the hardwareMode information from a database.
	 * @param filename the pathname of the file containing the database.
	 * @return the number of bytes that have been read from the input stream.
	 */
	std::streamoff readHeader(const char *filename);
	
	/** 
	 * Write an entire database into the given file.
	 * @param filename the pathname of the file where to store the database.
	 * @return the number of bytes that have been written into the file.
	 */
	std::streamoff writeToFile(const char *filename) const;

	/**
	 * Write an entire database into the given output stream.
	 * @param sout the output stream.
	 * @return the number of bytes that have been written from the input stream.
	 */
	std::streamoff write(std::ostream &sout) const;

	/**
	 * Copy the i-th image name into the given output.
	 * @param output the output buffer
	 * @param index index of the image in the database
	 * @param maxlen maximum length of the name (if longer, the name will be clipped).
	 */
	void copyImageName(char * output, unsigned int index, size_t maxlen) const;

	/**
	 * Get the name of the image at position i in the database.
	 * @param index index of the image in the database
	 * @return the image name
	 */
	const std::string & getImageName(unsigned int index) const {
		return images[index].imagefile;
	}

	/**
	 * Find the index of the given image in the database.
	 * @param filename the name of the image
	 * @return the index of the image in the DB, or -1 if not found.
	 */
	 size_t find(const char * filename) const;

	/**
	 * Get the size of the DB (number of images in the index).
	 * @return the size of the DB
	 */
	size_t size()  const {
		return images.size();
	}

	/**
	 * Check if a recall graph for this DB exists.
	 * The recall graph indicates the relationships among images in the DB.
	 * @return true if the recall graph is present.
	 */
	bool hasRecallGraph() const {
		return (recallGraph.size() > 0);
	}

	/**
	 * Get the recall graph node of a specific image.
	 * @param index the index of the image in the database.
	 * @return the recall graph node of the i-th image.
	 */
	const recallGraphNode_t & getRecallGraph(unsigned int index) const {
		return recallGraph[index];
	}


	~Database();

	void clear();							///< free allocated resources.

	std::vector<CompressedFeatureList> images;	///< vector containing the features of all images in the database.

	recallGraph_t recallGraph;				///< a graph of db images that have relationships with other db images.

	unsigned int modeId;					///< modeId used to build the database.

};

}	// end of namespace
