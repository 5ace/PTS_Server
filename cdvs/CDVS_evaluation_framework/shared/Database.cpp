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

#include "Database.h"
#include "CdvsException.h"
#include <sstream>
#include <fstream>
#include <string>
#include <algorithm>

using namespace std;
using namespace mpeg7cdvs;

Database::Database():recallGraph()
{
	modeId = 0;
}


size_t Database::addImage(const FeatureList & features, const char *filename)
{
	CompressedFeatureList compressed(features);
	compressed.setFilename(filename);
	size_t pos = images.size();
	images.push_back(compressed);
	return pos;
}

size_t Database::replaceImage(size_t index, const FeatureList & features, const char * filename)
{
	CompressedFeatureList compressed(features);
	compressed.setFilename(filename);
	images[index] = compressed;		// replace image
	return index;
}


void Database::merge(const Database &otherDB)
{
	if(modeId != otherDB.modeId)
	{
		std::ostringstream oss;
		oss << "Database::merge, Error: modeId =  " << modeId << " != otherDB.modeId = " << otherDB.modeId;
		throw CdvsException(oss.str());
	}

	images.reserve(images.size()+otherDB.images.size());

	// Update the list of images
	for(std::vector<CompressedFeatureList>::const_iterator i=otherDB.images.begin(); i<otherDB.images.end(); ++i)
	{
		images.push_back(*i);
	}
}


int Database::matchCompressedDescriptors_twoWay(PointPairs &pairs, const CompressedFeatureList &query, int imageDBindex, float ratioThreshold) const
{
	return images[imageDBindex].matchDescriptors_twoWay(pairs, query, ratioThreshold);
}

int Database::matchCompressedDescriptors_oneWay(PointPairs &pairs, const CompressedFeatureList &query, int imageDBindex, float ratioThreshold) const
{
	return images[imageDBindex].matchDescriptors_oneWay(pairs, query, ratioThreshold);
}

//
// if the DB is not empty, expand it.
// If the DB is empty, create it.
//
std::streamoff Database::read(std::istream& sin)
{
	std::streamoff position = sin.tellg();

	unsigned int newmodeId;
	sin.read((char*)&newmodeId, sizeof(unsigned int));

	int nImages;
	sin.read((char*)&nImages, sizeof(int));

	if (images.size() > 0)
	{
		if(modeId != newmodeId)		// different mode IDs in the same DB are not safe
		{
			std::ostringstream oss;
			oss << "Database::read, modeId mismatch in DB:  " << modeId << " != " << newmodeId;
			throw CdvsException(oss.str());
		}
	}
	else
		modeId = newmodeId;

	size_t currentSize = images.size();
	images.resize(currentSize + nImages);
	for(std::vector<CompressedFeatureList>::iterator i=images.begin() + currentSize; i<images.end(); ++i)
	{
		i->read(sin);
	}

	currentSize = recallGraph.size();	// current size of recall graph (may be different from images.size())
	size_t graphSize = 0;
	sin.read((char *) &graphSize, sizeof(size_t));
	if ((graphSize > 0) && (!sin.eof()))
	{
		recallGraph.resize(currentSize + graphSize);
		for (size_t idx=currentSize; idx < currentSize + graphSize; idx++)
		{
			size_t nodeSize = 0;
			sin.read((char *) &nodeSize, sizeof(size_t));
			recallGraph[idx].resize(nodeSize);

			for (size_t i=0; i < nodeSize; i++)
				sin.read((char *) &recallGraph[idx][i], sizeof (unsigned int));
		}
	}

	return (sin.tellg() - position);
}


std::streamoff Database::readHeader(const char *filename)
{
	std::ifstream fin(filename, std::ios::binary);

	if(fin.fail())
	{
		std::ostringstream oss;
		oss << "Database::readHeader, Error reading " << filename;
		throw CdvsException(oss.str());
	}

	std::streamoff position = fin.tellg();

	fin.read((char*)&modeId, sizeof(unsigned int));

	std::streamoff t = (fin.tellg() - position);

	fin.close();

	return t;
}


std::streamoff Database::readFromFile(const char *filename)
{
	std::ifstream fin(filename, std::ios::binary);

	if(fin.fail())
	{
		std::ostringstream oss;
		oss << "Database::readFromFile, Error reading " << filename;
		throw CdvsException(oss.str());
	}

	std::streamoff t = read(fin);

	fin.close();

	return t;
}


std::streamoff Database::writeToFile(const char *filename) const
{
	std::ofstream fout(filename, std::ios::binary);

	if(fout.fail())
	{
		std::ostringstream oss;
		oss << "Database::WriteToFile, Error writing " << filename;
		throw CdvsException(oss.str());
	}

	std::streamoff t = write(fout);

	fout.close();

	return t;
}


std::streamoff Database::write(std::ostream &sout) const
{
	std::streamoff position = sout.tellp();

	sout.write((const char*)&modeId, sizeof(unsigned int));

	int nImages = (int) images.size();
	sout.write((const char*)&nImages, sizeof(int));

	for(std::vector<CompressedFeatureList>::const_iterator i=images.begin(); i<images.end(); ++i)
	{
		i->write(sout);
	}

	// now write the recall graph - the size is either zero or the same as the images size
	size_t graphSize = recallGraph.size();
	sout.write((const char *) &graphSize, sizeof(size_t));

	for (size_t idx=0; idx < graphSize; idx++)
	{
		size_t nodeSize = recallGraph[idx].size();
		sout.write((const char *) &nodeSize, sizeof(size_t));
		for (size_t i=0; i < nodeSize; i++)
			sout.write((const char *) &recallGraph[idx][i], sizeof (unsigned int));
	}

	return (sout.tellp() - position);
}

void Database::copyImageName(char * output, unsigned int i, size_t maxlen) const
{
	size_t len = std::min(images[i].imagefile.size(), maxlen);
	if (len == maxlen)
	{
		std::ostringstream oss;
		oss << "warning: maximum reference file name length reached: " << maxlen
				<< "; actual size is: " << images[i].imagefile.size() << endl;
		throw CdvsException(oss.str());
	}

	images[i].imagefile.copy(output, len);
}

size_t Database::find(const char * filename) const
{
	if (filename == NULL)
		return NOT_FOUND;		// not found

	for (size_t i = 0; i < images.size(); ++i)
	{
		// each string must have at least one character; we check it before further processing to speed-up the search
		if (images[i].imagefile[0] == filename[0])
		{
			if (images[i].imagefile.compare(filename) == 0)
				return i;	// found!
		}
	}

	return NOT_FOUND;		// not found
}


void Database::clear(void)
{
	images.clear();
	recallGraph.clear();
}

Database::~Database(void)
{
	clear();
}
