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
 * FileManager.cpp
 *
 *  Created on: 18/lug/2012
 *      Author: massimo
 */

#include "FileManager.h"
#include "CdvsException.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstring>

using namespace std;

FileManager::FileManager():lines(),datasetBaseDir("."),annotationBaseDir(".") {
}

FileManager::~FileManager() {
}

void FileManager::setDatasetPath(const char * basedir)
{
	datasetBaseDir = basedir;
}

void FileManager::setAnnotationPath(const char * basedir)
{
	annotationBaseDir = basedir;
}

size_t FileManager::readAnnotation(const char *filename)
{
   ifstream file((annotationBaseDir + '/' + filename).c_str());
   string line ;
   if (! file.good())
   {
	   string msg("readAnnotation, file not found: ");
	   msg.append((annotationBaseDir + '/' + filename));
	   throw mpeg7cdvs::CdvsException(msg);
   }

   lines.clear();

   while( getline( file, line ) )
   {
	   //fprintf (stderr, "lineeee: %s", line.c_str());
	   lines.push_back( line ) ;
   }
   return lines.size();
}

string FileManager::replaceExt(const string & imageName, const char * ext) const
{
	size_t found = imageName.find_last_of('.');
	//fprintf (stderr, "founddddd: %d", found);
	//fprintf (stderr, "Heyyyyyy: %s", imageName.c_str());
	return imageName.substr(0,found) + ext;
}

string FileManager::getAbsolutePathname(size_t i) const
{
	return getQueryName(i);
}

string FileManager::getRelativePathname(size_t i) const
{
	if (i >= lines.size())
		mpeg7cdvs::CdvsException("FileManager::getRelativePathname index out of range");

	istringstream iss(lines[i]);
	string sub;
	iss >> sub;										// the query is the first name
	return  sub;			// return the relative path
}

string FileManager::getQueryName(size_t i) const
{
	if (i >= lines.size())
		mpeg7cdvs::CdvsException("FileManager::getQueryName index out of range");

	istringstream iss(lines[i]);
	string sub;
	//myself
	getline(iss, sub);
	sub = sub.substr(0, sub.find_first_of(".")+4);
	//iss >> sub;			// the query is the first name
	//fprintf (stderr, "linesssss: %s", sub.c_str());
	return (sub);			// return the absolute path
}

string FileManager::getReferenceName(size_t i) const
{
	if (i >= lines.size())
		mpeg7cdvs::CdvsException("FileManager::getReferenceName index out of range");

	istringstream iss(lines[i]);
	string sub;
	iss >> sub;										// the query is the first name
	iss >> sub;										// the reference is the second name
	return (datasetBaseDir + "/" + sub);			// return the absolute path
}


