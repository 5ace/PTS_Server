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

#include "FeatureList.h"
#include "Match.h"
#include "CsscCoordinateCoding.h"
#include "CdvsException.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <cfloat>
#include <cassert>
#include <eigen3/Eigen/Dense>

using namespace Eigen;
using namespace mpeg7cdvs;

#define HALF_PI (1.570796326794897)

#ifdef USE_DESCR_ARITHMETIC_CODER
	#include "ArithmeticCoding.h"
	#define NUM_SYMBOLS    3
#endif


///////////////////////////// FeatureList methods ///////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Ternarization thresholds.
/////////////////////////////////////////////////////////////////////////////

// Integers are used here for convenience in arithmetic operations.
// There are 128 elements, each with two thresholds of limited range, each of
// which may be represented in one byte. The total memory required for
// storage of the quantisation thresholds is 128x2x1 = 256 bytes.
const int FeatureList::hm_ter_quant_table[Feature::descrLength][2] = {
		{-4, 0},	{-7, 0},	{-1, 8},	{-1, 0},	{-1, 0},	{-4, 0},	{-1, 13},	{-1, 3},
		{3, 37},	{-1, 11},	{-25, 0},	{-1, 8},	{-1, 0},	{-1, 0},	{-7, 6},	{-2, 12},
		{-1, 3},	{-10, 0},	{-1, 22},	{-1, 1},	{-1, 0},	{-7, 0},	{0, 22},	{-1, 4},
		{-2, 12},	{-1, 8},	{-9, 0},	{-1, 3},	{-1, 0},	{-1, 0},	{-5, 5},	{-1, 10},
		{-3, 19},	{-4, 5},	{-14, 0},	{-1, 3},	{-4, 0},	{-1, 1},	{-6, 5},	{-6, 7},
		{-3, 1},	{-15, -2},	{13, 39},	{-1, 0},	{-1, 1},	{-11, -2},	{13, 31},	{1, 7},
		{21, 59},	{2, 16},	{-41, -14},	{2, 12},	{-2, 0},	{-1, 0},	{-7, 5},	{4, 17},
		{-1, 3},	{-5, 4},	{-1, 12},	{-2, 0},	{-1, 4},	{-4, 0},	{0, 19},	{-1, 4},
		{-1, 3},	{-6, 3},	{-1, 13},	{-2, 0},	{-1, 3},	{-4, 0},	{1, 19},	{-1, 4},
		{21, 59},	{1, 14},	{-40, -14},	{1, 10},	{-2, 0},	{-1, 0},	{-8, 3},	{3, 16},
		{-4, 1},	{-17, -3},	{13, 40},	{-1, 0},	{-1, 1},	{-13, -3},	{13, 30},	{1, 7},
		{-4, 19},	{-5, 4},	{-13, 0},	{-1, 3},	{-5, 0},	{-1, 1},	{-6, 5},	{-8, 7},
		{-2, 11},	{-1, 6},	{-9, 0},	{-1, 3},	{-1, 0},	{-1, 0},	{-5, 4},	{-2, 9},
		{-2, 2},	{-12, 0},	{-1, 24},	{-1, 0},	{-1, 0},	{-9, 0},	{1, 23},	{-1, 5},
		{3, 38},	{-1, 9},	{-23, 0},	{-1, 6},	{-1, 0},	{-2, 0},	{-8, 5},	{-4, 11},
		{-5, 0},	{-9, 0},	{-1, 8},	{-1, 0},	{-1, 0},	{-4, 0},	{-1, 13},	{-1, 3}
};	// soglie locali no limits


// Groups of histograms.
const int FeatureList::histogram_groups[4][4] = {
	{0, 15, 3, 12},
	{7, 8, 1, 14},
	{2, 13, 4, 11},
	{5, 10, 6, 9}
};

// Priority list for local descriptor encoding.
// Integers are used here for convenience in arithmetic operations.
// The priority list size is 32*(2+3)=160 bits=20 bytes
const int FeatureList::priority_list[32][2] = {
	{3, 0},	{1, 0},	{2, 0},	{0, 0},	{3, 6},
	{3, 1},	{1, 1},	{2, 1},	{0, 1},	{3, 2},
	{1, 2},	{2, 2},	{0, 2},	{1, 6},	{2, 6},	{0, 6},
	{3, 7},	{1, 7},	{2, 7},	{0, 7},
	{3, 3},	{1, 3},	{2, 3},	{0, 3},	{3, 4},	{1, 4},	{2, 4},	{0, 4},	{3, 5},	{1, 5},	{2, 5},	{0, 5}
};


FeatureList::FeatureList():features(),qdescr_size(0)
{
	originalWidth = 0;
	originalHeight = 0;
	imageHeight = 0;
	imageWidth = 0;
}

void FeatureList::clear()
{
	features.clear();
	qdescr_size = 0;
	originalWidth = 0;
	originalHeight = 0;
	imageHeight = 0;
	imageWidth = 0;
}

// default destructor, copy-constructor, assignment operator are OK: no implementation needed.

void FeatureList::setResolution(int imgWidth, int imgHeight, int orWidth, int orHeight)
{
	imageWidth = imgWidth;
	imageHeight = imgHeight;
	originalWidth = orWidth;
	originalHeight = orHeight;
}


int FeatureList::nFeatures() const
{
	return (int) features.size();
}


bool FeatureList::sortSpatialIndexPredicate(const Feature &f1, const Feature &f2)
{
	 return f1.spatialIndex < f2.spatialIndex;
}




bool FeatureList::sortRelevancePredicate(const Feature &f1, const Feature &f2)
{
	return f1.relevance > f2.relevance;
}



void FeatureList::sortRelevance()
{
	std::stable_sort(features.begin(), features.end(), sortRelevancePredicate);
}


void FeatureList::sortSpatialIndex()
{
	std::stable_sort(features.begin(), features.end(), sortSpatialIndexPredicate);
}

int FeatureList::compareKeypoints(const FeatureList &otherList) const
{
	size_t minsize = std::min(features.size(), otherList.features.size());
	int diffsize = std::abs((int) (features.size()-otherList.features.size()));
	int nDifferent = 0;
	if (minsize > 0)
	{
		for(unsigned int i=0; i<minsize; ++i)
		{
			/*
				float x;				  ///< the X coordinate of the ALP keypoint
				float y;				  ///< the Y coordinate of the ALP keypoint
				float scale;			  ///< the scale of the ALP keypoint
				float orientation;		  ///< the orientation of the ALP keypoint
				float peak;				  ///< the peak o
			*/

			if (features[i].x != otherList.features[i].x)
				++nDifferent;
			if (features[i].y != otherList.features[i].y)
				++nDifferent;
			if (features[i].scale != otherList.features[i].scale)
				++nDifferent;
			if (features[i].orientation != otherList.features[i].orientation)
				++nDifferent;
			if (features[i].peak != otherList.features[i].peak)
				++nDifferent;
		}
	}
	return nDifferent + diffsize;
}

void FeatureList::print() const
{
	std::cout << "LD:qdescrSize            = " << qdescr_size << std::endl;
	for (int k=0; k<(int) features.size(); ++k)
	{
		std::cout << "LD:qdescr[" << features[k].x << "," << features[k].y <<"] = " << features[k].qdescr[0];
		for (int h=1; h<qdescr_size; ++h)
			std::cout << ", " << features[k].qdescr[h];
		std::cout << std::endl;
	}
}

int FeatureList::compareCoordinates(const FeatureList &otherList, bool compressed, int blockWidth) const
{
	int minsize = (int) std::min(features.size(), otherList.features.size());
	int nDifferent = std::abs((int) (features.size() - otherList.features.size()));
	if (compressed)
	{
		float halfbw = (float) blockWidth / 2;
		for (int k=0; k<minsize; ++k)
		{
			if (std::abs(features[k].x - otherList.features[k].x) > halfbw)
				++nDifferent;
			if (std::abs(features[k].y - otherList.features[k].y) > halfbw)
				++nDifferent;
		}
	}
	else
	{
		for(unsigned int i=0; i<minsize; ++i)
		{
			if (features[i].x != otherList.features[i].x)
				++nDifferent;
			if (features[i].y != otherList.features[i].y)
				++nDifferent;
		}
	}

	return nDifferent;
}

int FeatureList::compareDescriptors(const FeatureList &otherList, bool compressed) const
{
	int nDifferent = 0;

	if(features.size() != otherList.features.size())
	{
		nDifferent = std::abs((int) (features.size() - otherList.features.size()));
	}
	else
	{
		if (compressed)
		{
			for (int k=0; k<(int) features.size(); ++k)
			{
				for (int h=0; h<qdescr_size; ++h)
				{
					if (features[k].qdescr[h] != otherList.features[k].qdescr[h])
					{
						++nDifferent;
						break;
					}
				}
			}
		}
		else
		{
			const float *descr, *otherDescr;
			for(unsigned int i=0; i<features.size(); ++i)
			{
				descr = features[i].descr;
				otherDescr = otherList.features[i].descr;

				for(unsigned int j=0; j<Feature::descrLength; ++j)
				{
					if(descr[j] != otherDescr[j])
					{
						++nDifferent;
						break;
					}
				}
			}
		}
	}

	return nDifferent;
}



void FeatureList::select(const std::vector<int> &indices)
{
	if(indices.size() > 0)
	{
		std::vector<Feature> ltemp;

		for(std::vector<int>::const_iterator p=indices.begin(); p<indices.end(); ++p)
		{
			ltemp.push_back(features[*p]);
		}

		features = ltemp;
		ltemp.clear();
	}
	else
	{
		features.clear();
	}
}


void FeatureList::selectFromTo(int startInd, int endInd)
{
	if(startInd < 0)
	{
		throw CdvsException("FeatureList::SelectFromTo, Error: startInd < 0");
	}

	if(startInd > endInd)
	{
		throw CdvsException("FeatureList::SelectFromTo, Error: startInd > endInd");
	}

	if(startInd >= nFeatures())
	{
		throw CdvsException("FeatureList::SelectFromTo, Error: endInd >= nFeatures()");
	}

	std::vector<int> v(endInd-startInd+1);
	for(std::vector<int>::iterator p=v.begin(); p<v.end(); ++p)
	{
		*p = startInd++;
	}

	select(v);

	v.clear();
}

void FeatureList::selectFirst(int n)
{
	if (n < nFeatures())
		features.erase(features.begin() + n, features.end());

	// else nothing to do
}

void FeatureList::addFeature(const Feature & f)
{
	features.push_back(f);
}


void FeatureList::toFile(const char *filename) const
{
	FILE * fp = fopen(filename, "wb");
	toFile(fp);
	fclose(fp);
}


void FeatureList::toFile(FILE * file) const
{	
	size_t fout;
	fout = fwrite(&imageWidth, sizeof(imageWidth), 1, file);
	fout += fwrite(&imageHeight, sizeof(imageHeight), 1, file);
	fout += fwrite(&originalWidth, sizeof(originalWidth), 1, file);
	fout += fwrite(&originalHeight, sizeof(originalHeight), 1, file);

	// now save the vector of features
	
	/* change size_t to unsigned int by linjie */
	unsigned int size = features.size();
	if (size > MAX_NUM_FEATURES)
		throw CdvsException("FeatureList.toFile: number of features out of range");

	fout += fwrite(&size, sizeof(size), 1, file);

	for(unsigned int i=0; i<size; i++)
	{
		features[i].toFile(file);
	}

	assert(fout == 5);
}


void FeatureList::fromFile(const char *filename)
{
	FILE * fp = fopen(filename, "rb");
	fromFile(fp);
	fclose(fp);
}


void FeatureList::fromFile(FILE * file) 
{
	size_t fout;
	fout = fread(&imageWidth, sizeof(imageWidth), 1, file);
	fout += fread(&imageHeight, sizeof(imageHeight), 1, file);
	fout += fread(&originalWidth, sizeof(originalWidth), 1, file);
	fout += fread(&originalHeight, sizeof(originalHeight), 1, file);
	
	// now read the vector of features

	features.clear();
	/* change size_t to unsigned int by linjie */
	unsigned int size;
	fout += fread(&size, sizeof(size), 1, file);

	if (size > MAX_NUM_FEATURES)
		throw CdvsException("FeatureList.fromFile: number of features out of range");

	for(unsigned int i=0; i<size; i++)
	{
		Feature feature;
		feature.fromFile(file);
		features.push_back(feature);
	}

	assert(fout == 5);
}


void FeatureList::compress(int numberOfElementGroups)
{
	qdescr_size = numberOfElementGroups << 2;
	unsigned int N = numberOfElementGroups;
	for(std::vector<Feature>::iterator f=features.begin(); f<features.end(); ++f)
	{
		int i, j;

		// quick check
		if (f->descr[0] == -1.0f)
			throw CdvsException("FeatureList::compress error: data not valid");

		j = 0;
		for( i = 0; i < N; i++ )
		{
			int group_id = priority_list[i][0];
			int element_id = priority_list[i][1];
			int v0, v1, v2, v3;

			int hist_pos0 = histogram_groups[group_id][0] << 3;
			int hist_pos1 = histogram_groups[group_id][1] << 3;
			int hist_pos2 = histogram_groups[group_id][2] << 3;
			int hist_pos3 = histogram_groups[group_id][3] << 3;

			float *d0 = f->descr + hist_pos0;
			float *d1 = f->descr + hist_pos1;
			float *d2 = f->descr + hist_pos2;
			float *d3 = f->descr + hist_pos3;

			switch(element_id)
			{
			case 0:
				// transform A
				v0 = (int)(d0[2] - d0[6])/2;
				v1 = (int)(d1[2] - d1[6])/2;
				// transform B
				v2 = (int)(d2[0] - d2[4])/2;
				v3 = (int)(d3[0] - d3[4])/2;
				break;
			case 1:
				// transform A
				v0 = (int)(d0[3] - d0[7])/2;
				v1 = (int)(d1[3] - d1[7])/2;
				// transform B
				v2 = (int)(d2[1] - d2[5])/2;
				v3 = (int)(d3[1] - d3[5])/2;
				break;
			case 2:
				// transform A
				v0 = (int)(d0[0] - d0[1])/2;
				v1 = (int)(d1[0] - d1[1])/2;
				// transform B
				v2 = (int)(d2[7] - d2[0])/2;
				v3 = (int)(d3[7] - d3[0])/2;
				break;
			case 3:
				// transform A
				v0 = (int)(d0[2] - d0[3])/2;
				v1 = (int)(d1[2] - d1[3])/2;
				// transform B
				v2 = (int)(d2[1] - d2[2])/2;
				v3 = (int)(d3[1] - d3[2])/2;
				break;
			case 4:
				// transform A
				v0 = (int)(d0[4] - d0[5])/2;
				v1 = (int)(d1[4] - d1[5])/2;
				// transform B
				v2 = (int)(d2[3] - d2[4])/2;
				v3 = (int)(d3[3] - d3[4])/2;
				break;
			case 5:
				// transform A
				v0 = (int)(d0[6] - d0[7])/2;
				v1 = (int)(d1[6] - d1[7])/2;
				// transform B
				v2 = (int)(d2[5] - d2[6])/2;
				v3 = (int)(d3[5] - d3[6])/2;
				break;
			case 6:
				// transform A
				v0 = (int)(d0[0] + d0[4] - d0[2] - d0[6])/4;
				v1 = (int)(d1[0] + d1[4] - d1[2] - d1[6])/4;
				// transform B
				v2 = (int)(d2[1] + d2[5] - d2[3] - d2[7])/4;
				v3 = (int)(d3[1] + d3[5] - d3[3] - d3[7])/4;
				break;
			case 7:
				// transform A
				v0 = (int)(d0[0] + d0[2]  + d0[4] + d0[6] - d0[1] - d0[3] - d0[5] - d0[7])/8;
				v1 = (int)(d1[0] + d1[2]  + d1[4] + d1[6] - d1[1] - d1[3] - d1[5] - d1[7])/8;
				// transform B
				v2 = (int)(d2[0] + d2[1]  + d2[2] + d2[3] - d2[4] - d2[5] - d2[6] - d2[7])/8;
				v3 = (int)(d3[0] + d3[1]  + d3[2] + d3[3] - d3[4] - d3[5] - d3[6] - d3[7])/8;
				break;
			}

			int val = 0;
			if(v0 > hm_ter_quant_table[hist_pos0 + element_id][1])
				val = 2;
			else if(v0 > hm_ter_quant_table[hist_pos0 + element_id][0])
				val = 1;
			f->qdescr[j++] = val;

			if(v1 > hm_ter_quant_table[hist_pos1 + element_id][1])
				val = 2;
			else if(v1 > hm_ter_quant_table[hist_pos1 + element_id][0])
				val = 1;
			else
				val = 0;
			f->qdescr[j++] = val;

			if(v2 > hm_ter_quant_table[hist_pos2 + element_id][1])
				val = 2;
			else if(v2 > hm_ter_quant_table[hist_pos2 + element_id][0])
				val = 1;
			else
				val = 0;
			f->qdescr[j++] = val;

			if(v3 > hm_ter_quant_table[hist_pos3 + element_id][1])
				val = 2;
			else if(v3 > hm_ter_quant_table[hist_pos3 + element_id][0])
				val = 1;
			else
				val = 0;
			f->qdescr[j++] = val;
		}
	}
}

void FeatureList::toBinary(BitOutputStream &writer, bool writeRelevance, int numFeatures)
{
	if (numFeatures > features.size())
		numFeatures = features.size();		// cannot encode non-existing features!

	if(qdescr_size <= 0 || qdescr_size > 128)
	{
		throw CdvsException("FeatureList::toBinary, Error: (qdescr_size <= 0 || qdescr_size > 128)");
	}

	unsigned int descr_bytes = qdescr_size >> 2;

	// write the number of bytes in the compressed descriptor (assuming 4 elements per byte)
	writer.write(descr_bytes, 6);

#ifdef USE_DESCR_ARITHMETIC_CODER
	AC_encoder ace;
	AC_model acm1,acm2,acm3,acm4;

	ace.init(writer);
	acm1.init(NUM_SYMBOLS, 0, 1);
	acm2.init(NUM_SYMBOLS, 0, 1);
	acm3.init(NUM_SYMBOLS, 0, 1);
	acm4.init(NUM_SYMBOLS, 0, 1);

	for(std::vector<Feature>::iterator f=features.begin(); f<features.begin() + numFeatures; ++f)
	{
		int i=0;
		while (i<qdescr_size)
		{
			ace.encode_symbol(acm1, f->qdescr[i++]);
			ace.encode_symbol(acm2, f->qdescr[i++]);
			ace.encode_symbol(acm3, f->qdescr[i++]);
			ace.encode_symbol(acm4, f->qdescr[i++]);
		}
	}
	ace.done();

#else
	for(std::vector<Feature>::iterator f=features.begin(); f<features.begin() + numFeatures; ++f)
	{
		for(unsigned int i=0; i<qdescr_size; i++)
		{
			if(f->qdescr[i] == 1)
			{
				 // 1 => '0'
				writer.write(0);
			}
			else
			{
				writer.write(1);
				if(f->qdescr[i] == 0)
				{
					// 0 => '10'
					writer.write(0);
				}
				else
				{
					// 2 => '11'
					writer.write(1);
				}
			}
		}
	}
#endif // USE_DESCR_ARITHMETIC_CODER

	if(writeRelevance)
	{
		for(std::vector<Feature>::iterator f=features.begin(); f<features.begin() + numFeatures; ++f)
			writer.write(f->relevance>0);
	}
}

void FeatureList::fromBinary(BitInputStream &reader, bool readRelevance)
{
	int i;
	int nof_elements;

	unsigned int descr_bytes = reader.read(6);
	nof_elements = descr_bytes * 4;
	qdescr_size = nof_elements;

	if(features.size() == 0)
		return;


#ifdef USE_DESCR_ARITHMETIC_CODER
	AC_decoder acd;
	AC_model acm1,acm2,acm3,acm4;
	acd.init(reader);
	acm1.init(NUM_SYMBOLS, 0, 1);
	acm2.init(NUM_SYMBOLS, 0, 1);
	acm3.init(NUM_SYMBOLS, 0, 1);
	acm4.init(NUM_SYMBOLS, 0, 1);

	for(std::vector<Feature>::iterator f=features.begin(); f<features.end(); ++f)
	{
		int i = 0;

		while (i<qdescr_size)
		{
			f->qdescr[i++] = acd.decode_symbol(acm1);
			f->qdescr[i++] = acd.decode_symbol(acm2);
			f->qdescr[i++] = acd.decode_symbol(acm3);
			f->qdescr[i++] = acd.decode_symbol(acm4);
		}

		for(i = 0; i < 128; i++)
		{
			f->descr[i] = 0;
		}
	}
	acd.done();
#else
	for(std::vector<Feature>::iterator f=features.begin(); f<features.end(); ++f)
	{
		unsigned char uc;
		int read_elems = nof_elements;
		int byte_offset;

		i = 0;
		while(read_elems)
		{
			int v = reader.read();
			if(v == 0)
			{
				f->qdescr[i] = 1;
			}
			else
			{
				v = reader.read();
				if(v == 0)
				{
					f->qdescr[i] = 0;
				}
				else
				{
					f->qdescr[i] = 2;
				}
			}
			i++;
			read_elems--;
		}
	}
#endif // USE_DESCR_ARITHMETIC_CODER

	if(readRelevance)
	{
		for(std::vector<Feature>::iterator f=features.begin(); f<features.end(); ++f)
		{
			f->relevance = reader.read(1);
		}
	}
}

void FeatureList::setRelevantPoints(int num)
{
	if (num == 0)
		return;

	int numNotRelevantPoints = features.size() - num;
	if(numNotRelevantPoints<0)
		numNotRelevantPoints = 0;

	// Assignment of the relevance
	for(std::vector<Feature>::iterator p=features.begin(); p<features.end()-numNotRelevantPoints; ++p)
	{
		p->relevance = 1;
	}

}

int FeatureList::getRelevantPoints() const
{
	int counter = 0;
	for(std::vector<Feature>::const_iterator f=features.begin(); f<features.end(); ++f)
	{
		if (f->relevance)
			++counter;
	}
	return counter;
}

//
// compute the number of keypoints without assuming any statistics (which may be misleading)
//
int FeatureList::computeMaxPoints(const Parameters & params, int targetBits)
{
	// compute a rough estimate keypoint number (num1)

	double availableBits = targetBits;				// compute the available bits
	qdescr_size = params.numberOfElementGroups << 2;		// on average, each value requires 1.66666 bits to be transmitted
	double estimate_bits = qdescr_size * 1.66666 + params.locationBits;	// locationBits = average bits per key point to encode location information
	int num1 = (int)(availableBits / estimate_bits + 0.5);	// first approximation of the number of keypoints
	if (num1 > nFeatures())
		num1 = nFeatures();		// the estimate number of key points cannot be greater than the number of available key points

	// compute a precise estimate keypoint number (num2)

	int num2 = 0;
	if (num1 > 0)
	{
		static const int MAX_DSC_LENGTH = (32 * 1024);			// Maximum size of descriptor buffer
		unsigned char tempbuf[MAX_DSC_LENGTH];
		BitOutputStream out2(tempbuf, MAX_DSC_LENGTH);			// attach temporary output buffer
		CsscCoordinateCoding cc(params);
		cc.generateHistogramMap(*this, num1);
		cc.toBinary(out2);			// write bitstream
		toBinary(out2, params.numRelevantPoints > 0, num1); // write features
		size_t numbits = out2.produced();						// ignore header bits
		double points = (availableBits * num1) / numbits;
		num2 = (int) (points + 0.25);		// round with a conservative approach (better less than more)
	}

	if (num2 > nFeatures())
		num2 = nFeatures();		// the estimate number of key points cannot be greater than the number of available key points

	if(params.selectMaxPoints > 0 && num2 > params.selectMaxPoints)
		return params.selectMaxPoints;

	return num2;
}


/////////////////////////////// CompressedFeatureList //////////////////////////////////

void CompressedFeatureList::allocate(int nFeatures, int descLen)
{
	if ((nFeatures <= 0) || (nFeatures > 16000))
		throw CdvsException("invalid parameter nFeatures passed to CompressedFeatureList::allocate");

	if ((descLen <= 0) || (descLen > 32))
		throw CdvsException("invalid parameter descLen passed to CompressedFeatureList::allocate");

	numFeatures = nFeatures;
	nDescLength = descLen;
	features = new unsigned char [nFeatures * descLen];


	// allocate array for X and Y coordinates of key points

	Xcoord = new unsigned short [nFeatures];
	Ycoord = new unsigned short [nFeatures];
}

void CompressedFeatureList::clear()
{
	if (features != NULL)
	{
		delete [] features;
	}
	if (Xcoord != NULL)
		delete [] Xcoord;

	if (Ycoord != NULL)
		delete [] Ycoord;
}

CompressedFeatureList::CompressedFeatureList():imagefile(), features(NULL), numFeatures(0), nDescLength(0),Xcoord(NULL),Ycoord(NULL)
{
	originalWidth = 0;
	originalHeight = 0;
	imageHeight = 0;
	imageWidth = 0;
}

CompressedFeatureList::CompressedFeatureList(int nFeatures, int descLen):imagefile()
{
	allocate(nFeatures, descLen);

	originalWidth = 0;
	originalHeight = 0;
	imageHeight = 0;
	imageWidth = 0;
}

CompressedFeatureList::~CompressedFeatureList()
{
	clear();
}

// copy constructor
CompressedFeatureList::CompressedFeatureList(const CompressedFeatureList & a):imagefile(), features(NULL), numFeatures(0), nDescLength(0),Xcoord(NULL),Ycoord(NULL)
{
	originalWidth = a.originalWidth;
	originalHeight = a.originalHeight;
	imageHeight = a.imageHeight;
	imageWidth = a.imageWidth;

	if (a.numFeatures > 0)	// really copy only if not empty
	{
		imagefile = a.imagefile;
		allocate(a.numFeatures, a.nDescLength);
		memcpy(Xcoord, a.Xcoord, numFeatures * sizeof(unsigned short));
		memcpy(Ycoord, a.Ycoord, numFeatures * sizeof(unsigned short));
		if (features != NULL)
		{
			memcpy(features, a.features, numFeatures * nDescLength);
		}
	}
}


void CompressedFeatureList::swap(CompressedFeatureList & a)
{
	std::swap(imagefile, a.imagefile);
	std::swap(numFeatures, a.numFeatures);
	std::swap(nDescLength, a.nDescLength);
	std::swap(features, a.features);
	std::swap(Xcoord, a.Xcoord);
	std::swap(Ycoord, a.Ycoord);
	std::swap(originalWidth, a.originalWidth);
	std::swap(originalHeight, a.originalHeight);
	std::swap(imageWidth, a.imageWidth);
	std::swap(imageHeight, a.imageHeight);
}


// copy and swap idiom
CompressedFeatureList & CompressedFeatureList::operator=(CompressedFeatureList a)
{
	a.swap(*this);
	return *this;
}


/*
 * Copy constructor from FeatureList including relevance sorting.
 * Converts a FeatureList instance into a CompressedFeatureList.
 */
CompressedFeatureList::CompressedFeatureList(const FeatureList & a, bool relevantOnly)
{
	originalWidth = a.originalWidth;
	originalHeight = a.originalHeight;
	imageHeight = a.imageHeight;
	imageWidth = a.imageWidth;

	int nBytes = a.qdescr_size >> 2;
	int nFeatures = relevantOnly? a.getRelevantPoints() : a.nFeatures();

	allocate(nFeatures, nBytes);
	int k = 0;

	for(std::vector<Feature>::const_iterator f=a.features.begin(); f<a.features.end(); ++f)
	{
		if ((relevantOnly) && (f->relevance == 0))
			continue;		// skip all features having low relevance

		Xcoord[k] = (unsigned short) (f->x + 0.5f);			// round coordinates
		Ycoord[k] = (unsigned short) (f->y + 0.5f);			// round coordinates

		// Pack descriptor elements into bytes: 4 elements per byte.
		unsigned char uc;
		int nof_elements = a.qdescr_size;
		int byte_offset = 0;
		int descr_bytes = 0;
		for(int i = 0; i < nof_elements; i++)
		{
			if(byte_offset == 0)
			{
				uc = 0;
			}
			int val = f->qdescr[i];
			if(val == 2) val = 3;
			uc |= (val & 3) << byte_offset;
			byte_offset += 2;
			if(byte_offset == 8)
			{
				features[k*nDescLength + descr_bytes] = uc;
				byte_offset = 0;
				// go to next byte;
				descr_bytes++;
			}
		}

		++k;	// go to the next compressed feature
	}
}

void CompressedFeatureList::setFilename(const char *filename)
{
	imagefile = filename;
}

void CompressedFeatureList::print() const
{
	std::cout << "nDescLength   = " << nDescLength << std::endl;
	std::cout << "numFeatures   = " << numFeatures << std::endl;
	for (int k=0; k<numFeatures; ++k)
	{
		std::cout << "X,Y,data  = " << Xcoord[k] << ", "<< Ycoord[k];
		for (int h=0; h<nDescLength; ++h)
			std::cout << ", " << (unsigned int) (features[k*nDescLength + h]);
		std::cout << std::endl;
	}
}

std::streamoff CompressedFeatureList::read(std::istream& sin)
{
	clear();

	std::streamoff position = sin.tellg();

	int filenameLength;
	sin.read((char*)&filenameLength, sizeof(int));
	assert(filenameLength>0 && filenameLength<256);

	imagefile.resize(filenameLength);
	sin.read((char*)imagefile.data(), filenameLength*sizeof(char));

	sin.read((char*)&numFeatures, sizeof(numFeatures));
	sin.read((char*)&nDescLength, sizeof(nDescLength));

	allocate(numFeatures, nDescLength);			// allocate memory for the features

	sin.read((char*) Xcoord, numFeatures * sizeof(unsigned short));
	sin.read((char*) Ycoord, numFeatures * sizeof(unsigned short));

	sin.read((char*) features, numFeatures * nDescLength);

	sin.read((char *) &originalWidth, sizeof(originalWidth));
	sin.read((char *) &originalHeight, sizeof(originalHeight));
	sin.read((char *) &imageHeight, sizeof(imageHeight));
	sin.read((char *) &imageWidth, sizeof(imageWidth));

	return (sin.tellg() - position);
}


std::streamoff CompressedFeatureList::readFromFile(char *filename)
{
	std::ifstream fin(filename, std::ios::binary);

	if(fin.fail())
	{
		throw CdvsException(std::string("FeatureDBList::readFromFile, Error reading ").append(filename));
	}

	std::streamoff t = read(fin);

	fin.close();

	return t;
}


std::streamoff CompressedFeatureList::writeToFile(char *filename) const
{
	std::ofstream fout(filename, std::ios::binary);

	if(fout.fail())
	{
		throw CdvsException(std::string("FeatureDBList::WriteToFile - Error writing ").append(filename));
	}

	std::streamoff t = write(fout);

	fout.close();

	return t;
}


std::streamoff CompressedFeatureList::write(std::ostream &sout) const
{
	std::streamoff position = sout.tellp();

	int filenameLength = imagefile.size();
	sout.write((const char*)&filenameLength, sizeof(int));
	sout.write((const char*)imagefile.data(), filenameLength*sizeof(char));

	sout.write((const char*)&numFeatures, sizeof(numFeatures));
	sout.write((const char*)&nDescLength, sizeof(nDescLength));

	sout.write((const char*) Xcoord, numFeatures * sizeof(unsigned short));
	sout.write((const char*) Ycoord, numFeatures * sizeof(unsigned short));

	sout.write((const char*) features, numFeatures * nDescLength);

	sout.write((const char *) &originalWidth, sizeof(originalWidth));
	sout.write((const char *) &originalHeight, sizeof(originalHeight));
	sout.write((const char *) &imageHeight, sizeof(imageHeight));
	sout.write((const char *) &imageWidth, sizeof(imageWidth));

	return (sout.tellp() - position);
}



int CompressedFeatureList::matchDescriptors_oneWay(PointPairs &pairs, const CompressedFeatureList & otherFeatureList, float ratioThreshold) const
{
	pairs.nMatched = 0;

	if((numFeatures > 1) && (otherFeatureList.numFeatures > 1))
	{
		ratioThreshold = ratioThreshold*ratioThreshold;

		int distance;
		int minDistance, secondMinDistance;
		int minDistanceInd;

		Match match;
		std::vector<Match> matches;
		std::vector<int> featureMatches;

		// Check sizes of the descriptors.
		int matched_bytes = std::min(nDescLength, otherFeatureList.nDescLength);

		//// Select the two nearest descriptors
		for(int featureInd=0; featureInd < numFeatures; ++featureInd)
		{
			// Find the two nearest descriptors contained in otherFeatureList and the relative distances between f

			minDistance = 65536;
			secondMinDistance = 65536;
			minDistanceInd = 0;

			for(int otherFeatureInd = 0; otherFeatureInd < otherFeatureList.numFeatures; ++otherFeatureInd)
			{
				distance = getDistance(features + featureInd*nDescLength, otherFeatureList.features + otherFeatureInd*otherFeatureList.nDescLength, matched_bytes);

				if(distance<minDistance)
				{
					secondMinDistance = minDistance;
					minDistance = distance;
					minDistanceInd = otherFeatureInd;
				}
				else
				{
					if(distance<secondMinDistance)
					{
						secondMinDistance = distance;
					}
				}
			}	// end for otherFeatureInd

			// If the ratio test is passed the indices of the features are saved
			if ((minDistance <= ratioThreshold*secondMinDistance) && (secondMinDistance > 0))
			{
				match.featureInd = featureInd;
				match.otherFeatureInd = minDistanceInd;
				match.weight = std::cos(HALF_PI * std::sqrt((double)minDistance/(double)secondMinDistance));
				matches.push_back(match);
			}
		}	// end for featureInd

		// Creation of a list on biunique matchings deleting possible repetitions in otherFeatureInd
		if(matches.size()>0)
		{
			// Ordering matches in accordance to otherFeatureInd
			std::sort(matches.begin(), matches.end(), Match::sortMatchByWeight);
			int lastFeatureIndex = -1;

			// Look for repetitions
			for(std::vector<Match>::const_iterator m=matches.begin(); m<matches.end(); ++m)
			{
				if(lastFeatureIndex != m->otherFeatureInd)
				{
					lastFeatureIndex = m->otherFeatureInd;
					pairs.addPair(Xcoord[m->featureInd],
						Ycoord[m->featureInd],
						otherFeatureList.Xcoord[m->otherFeatureInd],
						otherFeatureList.Ycoord[m->otherFeatureInd],
						m->weight);
				}
			}
		}
	}

	return pairs.nMatched;
}


int CompressedFeatureList::matchDescriptors_twoWay(PointPairs &pairs, const CompressedFeatureList &otherFeatureList, float ratioThreshold) const
{
	pairs.nMatched = 0;
	std::vector<Match> matches;
	Match match;
	std::vector<Match> matches1;
	std::vector<Match> matches1clean;
	std::vector<Match> matches2;
	std::vector<Match> matches2clean;

	if(numFeatures>1 && otherFeatureList.numFeatures>1)
	{
		// There are at least two features in each featureList

		// We work with the square of the distances to optimize the computational time
		// This requires to raise ratioThreshold to a power of two
		ratioThreshold *= ratioThreshold;

		int distance;
		int minDistance, secondMinDistance;
		int minDistanceInd;

		int** distanceMatrix = new int*[numFeatures];
		if (distanceMatrix != NULL)
		{
			for (unsigned int i=0; i < numFeatures; i++)
				distanceMatrix[i] = new int[otherFeatureList.numFeatures];
		}

		std::vector<int> featureMatches;

		// Check sizes of the descriptors.
		int matched_bytes = std::min(nDescLength, otherFeatureList.nDescLength);

		// Select the two nearest descriptors

		for(int featureInd = 0; featureInd < numFeatures; ++featureInd)
		{
			// Find the two nearest descriptors contained in otherFeatureList and the relative distances between f
			minDistance = 65536;
			secondMinDistance = 65536;
			minDistanceInd = 0;

			for(int otherFeatureInd = 0; otherFeatureInd < otherFeatureList.numFeatures; ++otherFeatureInd)
			{
				distance = getDistance(features + featureInd*nDescLength, otherFeatureList.features + otherFeatureInd*otherFeatureList.nDescLength, matched_bytes);
				distanceMatrix[featureInd][otherFeatureInd] = distance;

				if(distance<minDistance)
				{
					secondMinDistance = minDistance;
					minDistance = distance;
					minDistanceInd = otherFeatureInd;
				}
				else
				{
					if(distance<secondMinDistance)
					{
						secondMinDistance = distance;
					}
				}
			}	// end for otherFeatureInd

			// If the ratio test is passed the indices of the features are saved
			if ((minDistance <= ratioThreshold*secondMinDistance) && (secondMinDistance>0))
			{
				match.featureInd = featureInd;
				match.otherFeatureInd = minDistanceInd;
				match.weight = std::cos(HALF_PI * std::sqrt((float)minDistance/((float)secondMinDistance)));
				matches1.push_back(match);
			}
		}	// end for featureind

		// Creation of a list on biunique matchings deleting possible repetitions in otherFeatureInd
		if(matches1.size()>0)
		{
			// Ordering matches in accordance to otherFeatureInd
			std::sort(matches1.begin(), matches1.end(), Match::sortMatchByWeight);
			int lastFeatureIndex = -1;

			// Look for repetitions
			for(std::vector<Match>::const_iterator m=matches1.begin(); m<matches1.end(); ++m)
			{
				if(lastFeatureIndex != m->otherFeatureInd)
				{
					lastFeatureIndex = m->otherFeatureInd;
					Match tempMatch;
					tempMatch.featureInd = m->featureInd;
					tempMatch.otherFeatureInd = m->otherFeatureInd;
					tempMatch.weight = m->weight;
					matches1clean.push_back(tempMatch);
				}
			}
		}
		matches1.clear();

		// Select the two nearest descriptors

		for (int otherFeatureInd = 0; otherFeatureInd < otherFeatureList.numFeatures; ++otherFeatureInd)
		{
			// Find the two nearest descriptors contained in otherFeatureList and the relative distances between f
			minDistance = 65536;
			secondMinDistance = 65536;
			minDistanceInd = 0;

			for(int featureInd = 0; featureInd < numFeatures; ++featureInd)
			{
				distance = distanceMatrix[featureInd][otherFeatureInd];

				if(distance<minDistance)
				{
					secondMinDistance = minDistance;
					minDistance = distance;
					minDistanceInd = featureInd;
				}
				else
				{
					if(distance<secondMinDistance)
					{
						secondMinDistance = distance;
					}
				}
			}	// end for featureInd

			// If the ratio test is passed the indices of the features are saved
			if ((minDistance < ratioThreshold*secondMinDistance) && (secondMinDistance > 0))
			{
				match.featureInd = minDistanceInd;
				match.otherFeatureInd = otherFeatureInd;
				match.weight = std::cos(HALF_PI * std::sqrt((float)minDistance/((float)secondMinDistance)));
				matches2.push_back(match);
			}
		}	// end for otherFeatureInd

		// Creations of a list on biunique matchings deleting possible repetitions in otherFeatureInd
		if(matches2.size()>0)
		{
			// Ordering matches in accordance to otherFeatureInd
			std::sort(matches2.begin(), matches2.end(), Match::sortMatchByWeight);
			int lastFeatureIndex = -1;

			// Look for repetitions
			for(std::vector<Match>::const_iterator m=matches2.begin(); m<matches2.end(); ++m)
			{
				if(lastFeatureIndex != m->featureInd)
				{
					lastFeatureIndex = m->featureInd;
					Match tempMatch;
					tempMatch.featureInd = m->featureInd;
					tempMatch.otherFeatureInd = m->otherFeatureInd;
					tempMatch.weight = m->weight;
					matches2clean.push_back(tempMatch);
				}
			}
		}
		matches2.clear();

		for(std::vector<Match>::iterator m1 = matches1clean.begin(); m1 < matches1clean.end(); ++m1)
		{
			int flag=0;
			for(std::vector<Match>::iterator m2 = matches2clean.begin(); m2 < matches2clean.end(); ++m2)
			{
				if(m1->featureInd == m2->featureInd && m1->otherFeatureInd == m2->otherFeatureInd)
				{
					pairs.addPair(Xcoord[m1->featureInd],
						Ycoord[m1->featureInd],
						otherFeatureList.Xcoord[m1->otherFeatureInd],
						otherFeatureList.Ycoord[m1->otherFeatureInd],
						(m1->weight+m2->weight)*0.5, match_2way_INTERSECTION);
					flag=1;
					break;
				}
			}
			if(flag==0)
			{
				pairs.addPair(Xcoord[m1->featureInd],
					Ycoord[m1->featureInd],
					otherFeatureList.Xcoord[m1->otherFeatureInd],
					otherFeatureList.Ycoord[m1->otherFeatureInd],
					m1->weight, match_2way_DISJOINT1);
			}
		}

		for(std::vector<Match>::iterator m2 = matches2clean.begin(); m2 < matches2clean.end(); ++m2)
		{
			int flag=0;
			for(std::vector<Match>::iterator m1 = matches1clean.begin(); m1 < matches1clean.end(); ++m1)
			{
				if(m1->featureInd == m2->featureInd && m1->otherFeatureInd == m2->otherFeatureInd){flag=1;break;}
			}
			if(flag==0)
			{
				pairs.addPair(Xcoord[m2->featureInd],
					Ycoord[m2->featureInd],
					otherFeatureList.Xcoord[m2->otherFeatureInd],
					otherFeatureList.Ycoord[m2->otherFeatureInd],
					m2->weight, match_2way_DISJOINT2);
			}
		}

		double disjoin_weigh=0;
		int disjoint_matches = 0;
		int intersection_matches = 0;
		for(int i = 0; i < pairs.nMatched; i++)
		{
			if(pairs.match_dirs[i] == match_2way_INTERSECTION)
				intersection_matches++;
			else
				disjoint_matches++;
		}
		if(disjoint_matches > intersection_matches)
		{
			disjoin_weigh = intersection_matches/(double)disjoint_matches;
			for(int i = 0; i < pairs.nMatched; i++)
			{
				if(pairs.match_dirs[i] != match_2way_INTERSECTION)
					pairs.weights[i] *= disjoin_weigh;
			}
		}

		if (distanceMatrix != NULL)
		{
			for (unsigned int i=0; i<numFeatures; i++)
				if (distanceMatrix[i] != NULL) delete [] distanceMatrix[i];
			delete [] distanceMatrix;
		}
	}
	return pairs.nMatched;

}

/* Macro for enabling compiler-specific builtins, with default fallback */

#if defined(_MSC_VER) && defined(USE_POPCNT)
	#define POPCNT8(v) (__popcnt16((unsigned short) (v)))
	#define POPCNT16(v) (__popcnt16(v))
	#define POPCNT32(v) (__popcnt(v))
	#define POPCNT64(v) (__popcnt64(v))
#elif defined(__GNUC__) || defined(__GNUG__)
	#define POPCNT8(v)  (__builtin_popcount((unsigned int) (v)))
	#define POPCNT16(v) (__builtin_popcount((unsigned int) (v)))
	#define POPCNT32(v) (__builtin_popcount(v))
	#define POPCNT64(v) (__builtin_popcountll(v))
#else

// global lookup table used in POPCOUNT8
static const int costTable[256] =
{
  0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
  4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};

	#define POPCNT8(v) (costTable[(v)])
#endif



#ifdef POPCNT64
inline int CompressedFeatureList::getDistance(const unsigned char * mine, const unsigned char * other, int nbytes)
{
	switch (nbytes)
	{
		case 5:
		{
			const unsigned int * mine32 = (unsigned int *) mine;
			const unsigned int * other32 = (unsigned int *) other;
			return POPCNT32(mine32[0] ^ other32[0]) + POPCNT8(mine[4] ^ other[4]);
		}
		case 10:
		{
			const unsigned long long * mine64 = (unsigned long long *) mine;
			const unsigned long long * other64 = (unsigned long long *) other;
			const unsigned short * mine16 = (unsigned short *) (mine + 8);
			const unsigned short * other16 = (unsigned short *) (other + 8);
			return POPCNT64(mine64[0] ^ other64[0]) + POPCNT16(mine16[0] ^ other16[0]);
		}
		case 16:
		{
			const unsigned long long * mine64 = (unsigned long long *) mine;
			const unsigned long long * other64 = (unsigned long long *) other;
			return POPCNT64(mine64[0] ^ other64[0]) + POPCNT64(mine64[1] ^ other64[1]);
		}
		case 20:
		{
			const unsigned long long * mine64 = (unsigned long long *) mine;
			const unsigned long long * other64 = (unsigned long long *) other;
			const unsigned int * mine32 = (unsigned int *) (mine + 16);
			const unsigned int * other32 = (unsigned int *) (other + 16);
			return POPCNT64(mine64[0] ^ other64[0]) + POPCNT64(mine64[1] ^ other64[1]) + POPCNT32(mine32[0] ^ other32[0]);
		}
		case 32:
		{
			const unsigned long long * mine64 = (unsigned long long *) mine;
			const unsigned long long * other64 = (unsigned long long *) other;
			return POPCNT64(mine64[0] ^ other64[0])
				 + POPCNT64(mine64[1] ^ other64[1])
				 + POPCNT64(mine64[2] ^ other64[2])
				 + POPCNT64(mine64[3] ^ other64[3]);
		}
		default:	// just to allow experimenting with different values of the "numberOfElementGroups" parameter
		{
			int distance = 0;
			for (int i=0; i<nbytes; ++i)
			{
				distance += POPCNT8(mine[i] ^ other[i]);
			}
			return distance;
		}
	}
}
#else
inline int CompressedFeatureList::getDistance(const unsigned char * mine, const unsigned char * other, int nbytes)
{
	int distance = 0;
	for (int i=0; i<nbytes; ++i)
	{
		distance += POPCNT8(mine[i] ^ other[i]);
	}
	return distance;
}
#endif
