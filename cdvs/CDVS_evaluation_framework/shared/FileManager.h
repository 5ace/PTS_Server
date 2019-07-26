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
 * Copyright (c) ISO/IEC 2012.
 *
 */

/*
 * FileManager.h
 *
 *  Created on: 18/lug/2012
 *      Author: massimo
 */

#ifndef FILEMANAGER_H_
#define FILEMANAGER_H_

#include <string>
#include <vector>

/**
 * @class FileManager
 * Helper class to manage lists of file names.
 * @author Massimo Balestri
 * @date 2012
 */
class FileManager {
private:
	std::vector<std::string> lines;
	std::string datasetBaseDir;
	std::string annotationBaseDir;

public:
	FileManager();
	virtual ~FileManager();

	/**
	 * Set the dataset base directory.
	 * @param basedir the base directory.
	 */
	void setDatasetPath(const char * basedir);

	/**
	 * Set the annotation base directory.
	 * @param basedir the base directory.
	 */
	void setAnnotationPath(const char * basedir);

	/**
	 * Read the list of images from the given annotation file.
	 * @param filename the name of the annotation text file containing the list of images.
	 * @return the number of lines read from the filename.
	 */
	size_t readAnnotation(const char * filename);

	/**
	 * Get the first image name found at the i-th position in the annotation file.
	 * The image name is provided as an absolute pathname.
	 * @param i the index of the image in the annotation file.
	 * @return the absolute pathname of the image.
	 */
	std::string getAbsolutePathname(size_t i) const;

	/**
	 * Get the first image name found at the i-th position in the annotation file.
	 * The image name is provided as a relative pathname.
	 * @param i the index of the image in the annotation file.
	 * @return the relative pathname of the image.
	 */
	std::string getRelativePathname(size_t i) const;

	/**
	 * Convert a pathname into a pathname with the given extension.
	 * @param imageName the original image name;
	 * @param ext new extension;
	 * @return the modified pathname.
	 */
	std::string replaceExt(const std::string & imageName, const char * ext) const;

	/**
	 * Get the first image name found at the i-th position in the annotation file.
	 * The image name is provided as an absolute pathname.
	 * @param i the index of the image in the annotation file.
	 * @return the absolute pathname of the image.
	 */
	std::string getQueryName(size_t i) const;

	/**
	 * Get the second image name found at the i-th position in the annotation file.
	 * The image name is provided as an absolute pathname.
	 * @param i the index of the image in the annotation file.
	 * @return the absolute pathname of the image.
	 */
	std::string getReferenceName(size_t i) const;
};

#endif /* FILEMANAGER_H_ */
