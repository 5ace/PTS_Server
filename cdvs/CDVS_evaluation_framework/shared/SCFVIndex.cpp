/*
 * This software module was originally developed by:
 *
 *   Peking University
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
 * Peking University retain full rights to modify and use the code for their own
 * purposes, assign or donate the code to a third party and to inhibit third parties
 * from using the code for products that do not conform to MPEG-related
 * ITU Recommendations and/or ISO/IEC International Standards.
 *
 * This copyright notice must be included in all copies or derivative works.
 * Copyright (c) ISO/IEC 2012.
 *
 */

#include "SCFVIndex.h"
#include "CdvsException.h"
#include "gaussian_mixture.h"
#include "fisher.h"
#include <iostream>
#include <eigen3/Eigen/Core>
#include <eigen3/Eigen/Dense>
#include <map>
#include <cassert>

/* Macro for enabling compiler-specific builtins, with default fallback */
#if defined(_MSC_VER) && defined(USE_POPCNT)
		#define POPCNT(v) (__popcnt(v))
		#define PREFETCH(v) (_m_prefetchw(v))
#elif defined(__GNUC__) || defined(__GNUG__)
	#define POPCNT(v) (__builtin_popcount(v))
	#define PREFETCH(v) (__builtin_prefetch(v))
#else
	#define POPCNT(v) (lut.f[(v)>>16] + lut.f[(v)&65535])
	#define PREFETCH(v) 
#endif

/* Macro to avoid chcks on USE_WEIGHT_TABLE later on in the two macros below */
#ifdef USE_WEIGHT_TABLE
	#define SUM_MEAN() (fCorrTable[h] * W2_log[nCentroid])
	#define SUM_VAR() (fVarCorrTable[h]* W2_log_var[nCentroid])
#else
	#define SUM_MEAN() (fCorrTable[h])
	#define SUM_VAR() (fVarCorrTable[h])
#endif

/* These macros are handy for force-unrolling the loops over the centroid in
 * ::query() and ::query_bitselection()
 */
#define	sum_mean_var(centroidIndex){ \
	if (querySignature.m_vWordBlock[centroidIndex] && pImage->m_vWordBlock[centroidIndex]){\
		h = POPCNT (querySignature.m_vWordBlock[centroidIndex] ^ pImage->m_vWordBlock[centroidIndex]);\
		PREFETCH (&fCorrTable[h]);\
		fTotalCorrelation += SUM_MEAN();\
		h = POPCNT (querySignature.m_vWordVarBlock[centroidIndex] ^ pImage->m_vWordVarBlock[centroidIndex]);\
		fTotalCorrelation += SUM_VAR();\
	}\
}


#define	sum_mean_only(centroidIndex){ \
	if (querySignature.m_vWordBlock[centroidIndex] && pImage->m_vWordBlock[centroidIndex]){\
		h = POPCNT (querySignature.m_vWordBlock[centroidIndex] ^ pImage->m_vWordBlock[centroidIndex]);\
		PREFETCH (&fCorrTable[h]);\
		fTotalCorrelation += SUM_MEAN();\
	}\
}

#define	sum_mean_var_bitselection(centroidIndex){ \
	if (bitsOfQuery[centroidIndex] && pImage->m_vWordBlock[centroidIndex]){\
		h = POPCNT ((bitsOfQuery[centroidIndex] ^ pImage->m_vWordBlock[centroidIndex]) & SCFVSignature::table_bit_selection[centroidIndex]);\
		PREFETCH (&fCorrTableBitSelection[h]);\
		fTotalCorrelation += fCorrTableBitSelection[h];\
		h = POPCNT (querySignature.m_vWordVarBlock[centroidIndex] ^ pImage->m_vWordVarBlock[centroidIndex]);\
		fTotalCorrelation += fVarCorrTableBitSelection[h];\
	}\
}

#define	sum_mean_only_bitselection(centroidIndex){ \
	if (bitsOfQuery[centroidIndex] && pImage->m_vWordBlock[centroidIndex]){\
		h = POPCNT ((bitsOfQuery[centroidIndex] ^ pImage->m_vWordBlock[centroidIndex])  & SCFVSignature::table_bit_selection[centroidIndex]);\
		PREFETCH (&fCorrTableBitSelection[h]);\
		fTotalCorrelation += fCorrTableBitSelection[h];\
	}\
}




#ifndef PUSH_BIT
#define PUSH_BIT(packed, bit) \
	packed = packed << 1; \
	packed += bit;
#endif

#ifndef PULL_BIT
#define PULL_BIT(packed, bit) \
	bit = packed & 1;\
	packed = packed >> 1;
#endif

using namespace std;
using namespace mpeg7cdvs;
using namespace Eigen;


// default constructor (private)
SCFVSignature::SCFVSignature():bHasVar(false), bHasBitSelection(false)
{
	clear();			// clear all data
}

// public constructor defining hasVar() value
SCFVSignature::SCFVSignature(bool myHasVar, bool myHasBitSelection)
{
	clear();			// clear all data
	hasVar(myHasVar);	// set the correct hasVar() value
	hasBitSelection(myHasBitSelection);	// set the correct hasBitSelection value
}


unsigned int SCFVSignature::getVisited() const 
{
	return m_numVisited;
}

void SCFVSignature::setVisited()
{
	int	count = 0;
	for (size_t k=0; k < numberCentroids; ++k)
	{
		if (m_vWordBlock[k])
			++count;
	} 
	m_numVisited = count;
}

float SCFVSignature::getNorm() const
{
	return fNorm;
}

void SCFVSignature::setNorm()
{
	setVisited();											// compute visited
	
	fNorm = pow( (float) (getVisited()) , gama);
}

int SCFVSignature::compressedNumBits() const
{
	int centroidSize = hasBitSelection()? num_bit_selection: PCASiftLength;
	int numbits = numberCentroids;	// (bits)
	numbits += getVisited() * centroidSize * (hasVar()? 2:1) ;
	return numbits ;
}

size_t SCFVSignature::size() const
{
	if (hasVar()) 
		  return 2 * numberCentroids * PCASiftLength;
	  else 
		  return numberCentroids * PCASiftLength;
}

void SCFVSignature::clear()
{
	m_numVisited = 0;
	fNorm = 0.0f;	
	memset(m_vWordBlock, 0, sizeof(m_vWordBlock));				// reset all values to zero
	memset(m_vWordVarBlock, 0, sizeof(m_vWordVarBlock));		// reset all values to zero
}

bool SCFVSignature::hasVar()  const
{
	return bHasVar;
}

void SCFVSignature::hasVar(bool value)
{
	bHasVar = value;
}

bool SCFVSignature::hasBitSelection()  const
{
	return bHasBitSelection;
}

void SCFVSignature::hasBitSelection(bool value)
{
	bHasBitSelection = value;
}

void SCFVSignature::write_bitselection(BitOutputStream & out) const
{
	for (int k=0; k < numberCentroids; ++k)
		out.write(m_vWordBlock[k] > 0 ? 1 : 0);		// write visited bit
	
	unsigned int one = 1;
	for (int k=0; k < numberCentroids; ++k)
	{
		if (m_vWordBlock[k])
		{
			
			unsigned int bit = 0;

			for(int i = PCASiftLength - 1 ; i >= 0 ; i --)
				if( ((one<<i) & table_bit_selection[k]) != 0)
			{
				if( ((one<<i) & m_vWordBlock[k]) == 0)
					bit = (bit << 1);
				else
					bit = ((bit << 1) + 1);
			}

			out.write( bit , num_bit_selection );

		}
	}

}

void SCFVSignature::read_bitselection(BitInputStream & in)
{
	bool visited[numberCentroids];			// temporary array
	memset(visited, 0, sizeof(visited));	// clear all values

	clear();	// clear all values
	
	for (size_t k=0; k < numberCentroids; ++k)
		visited[k] = (in.read() == 1);		// read visited bits

	for (int k=0; k < numberCentroids; ++k)
	{
		if (visited[k])
			m_vWordBlock[k] = in.read(num_bit_selection);
	}
	
	setNorm();		// compute the norm of this signature
}

void SCFVSignature::write(BitOutputStream & out) const
{
	if (hasBitSelection())
	{
		write_bitselection(out);
		return;
	}

	for (int k=0; k < numberCentroids; ++k)
		out.write(m_vWordBlock[k] > 0 ? 1 : 0);		// write visited bit

	for (int k=0; k < numberCentroids; ++k)
	{
		if (m_vWordBlock[k])
			out.write(m_vWordBlock[k], PCASiftLength);
	}

	if (hasVar())
	{
		for (int k=0; k < numberCentroids; ++k)
		{
			if (m_vWordBlock[k])
				out.write(m_vWordVarBlock[k], PCASiftLength);
		}
	}
}

void SCFVSignature::read(BitInputStream & in)
{
	if (hasBitSelection())
	{
		read_bitselection(in);
		return;
	}

	bool visited[numberCentroids];			// temporary array
	memset(visited, 0, sizeof(visited));	// clear all values

	clear();	// clear all values
	
	for (size_t k=0; k < numberCentroids; ++k)
		visited[k] = (in.read() == 1);		// read visited bits

	for (int k=0; k < numberCentroids; ++k)
	{
		if (visited[k])
			m_vWordBlock[k] = in.read(PCASiftLength);
	}

	if (hasVar())
	{
		for (int k=0; k < numberCentroids; ++k)
		{
			if (visited[k])
				m_vWordVarBlock[k] = in.read(PCASiftLength);
		}
	}
	
	setNorm();		// compute the norm of this signature
}

int SCFVSignature::compare(const SCFVSignature & other) const
{
      int nNumWordBlockErrors = 0;
      for (int nBlock = 0; nBlock < numberCentroids; nBlock++)
        {
          if( m_vWordBlock[nBlock] != other.m_vWordBlock[nBlock]) 
			  nNumWordBlockErrors++;
        } // nBlock                                                                                                                 

	   if (hasVar())
	   {
	    for (int nBlock = 0; nBlock < numberCentroids; nBlock++)
        {
          if( m_vWordVarBlock[nBlock] != other.m_vWordVarBlock[nBlock]) 
			  nNumWordBlockErrors++;
        } // nBlock                                                                                                                 
	   }

	  return nNumWordBlockErrors;
}

void SCFVSignature::toFile(FILE * file) const
{
	size_t fout = fwrite(&bHasVar, sizeof(bHasVar), 1, file);
	fout += fwrite(&bHasBitSelection, sizeof(bHasBitSelection), 1, file);
	fout += fwrite(&fNorm, sizeof(fNorm), 1, file);
	fout += fwrite(&m_numVisited, sizeof(m_numVisited), 1, file);
	fout += fwrite(&m_vWordBlock, sizeof(m_vWordBlock), 1, file);
	fout += fwrite(&m_vWordVarBlock, sizeof(m_vWordVarBlock), 1, file);
	assert(fout == 6);
}

void SCFVSignature::fromFile(FILE * file)
{
	size_t fout = fread(&bHasVar, sizeof(bHasVar), 1, file);
	fout += fread(&bHasBitSelection, sizeof(bHasBitSelection), 1, file);
	fout += fread(&fNorm, sizeof(fNorm), 1, file);
	fout += fread(&m_numVisited, sizeof(m_numVisited), 1, file);
	fout += fread(&m_vWordBlock, sizeof(m_vWordBlock), 1, file);
	fout += fread(&m_vWordVarBlock, sizeof(m_vWordVarBlock), 1, file);
	assert(fout == 6);
}

void SCFVSignature::print() const
{
	cout << "GD:Norm                  = " << getNorm() << endl;
	cout << "GD:Visited               = " << getVisited() << endl;
	cout << "GD:Size (bits)           = " << 8 * size() << endl;
	cout << "GD:CompressedSize (bits) = " << compressedNumBits() << endl;

}

LookUpTable::LookUpTable()
{
	// initialize the bit count lookup table

	f[0] = 0;
	for(int i = 1 ; i < (1<<16) ; i ++)
		f[i] = f[i>>1] + (i&1);
}

const LookUpTable SCFVIndex::lut;			// initialize look up table
const float SCFVIndex::beta = 1.2f;

SCFVIndex::SCFVIndex()
{
}


void SCFVIndex::append(const SCFVSignature& signature)
{
	m_signatures.push_back(signature);
}
void SCFVIndex::replace(size_t index, const SCFVSignature & signature)
{
	m_signatures[index] = signature;
}


void SCFVIndex::dfs(vector<unsigned int> &vec , unsigned int var , int level , int dis)
{
	int sz = PCASiftLength / M;

	if(level == sz)
	{
		vec.push_back(var);
		return ;
	}

	if(dis == h_t)
	{
		dfs( vec , var , level + 1 , dis );
		return ;
	}

	dfs( vec , var , level + 1 , dis );

	dfs( vec , var^(1<<level) , level + 1 , dis + 1 );
}

vector<unsigned int> SCFVIndex::getKNN( unsigned int var )
{
	vector<unsigned int> vec;

	int sz = PCASiftLength / M;

	dfs( vec , var , 0 , 0);

	return vec;
}

/*
void SCFVIndex::weightOP( int **W[numberCentroids] , unsigned int b , int nCentroid )
{
	int sz = PCASiftLength / M;

	for(int i = 0 ; i < M ; i ++)
	{
		unsigned int var = (b & ((1<<sz) - 1));

		vector<unsigned int> vec = getKNN( var );

		for(int j = 0 ; j < vec.size() ; j ++)
			W[nCentroid][i][vec[j]] ++;

		b = (b >> sz);
	}
}
*/

void SCFVIndex::write(string sIndexName) const
{
	//cout << "Saving index to:  "<< sIndexName << endl;
	FILE* pIndexFile = fopen(sIndexName.c_str(), "wb");
	if (pIndexFile == NULL) {
		throw CdvsException(string("SCFVIndex::write - Error opening ").append(sIndexName));
	}

	unsigned int nNumImages = numberImages();
	int nWrite = fwrite(&nNumImages, sizeof(unsigned int), 1, pIndexFile);
  
	for (int k = 0; k < nNumImages; ++k) 
	{
		m_signatures[k].toFile(pIndexFile);
	}
  
	fclose(pIndexFile);
	//cout << "Done." << endl;


}

#ifdef USE_WEIGHT_TABLE
void SCFVIndex::loadHammingWeight()
{
	memset( weight , 0 , sizeof(weight) );

	memset( weight_var , 0 , sizeof(weight_var) );

	int len = m_signatures.size();

	for(int n_scfv = 0  ; n_scfv < len ; n_scfv ++)
	{
		for(int nCentroid = 0 ; nCentroid < numberCentroids ; nCentroid ++)
		{
			unsigned int a = m_signatures[n_scfv].m_vWordBlock[nCentroid];
			unsigned int b = m_signatures[n_scfv].m_vWordVarBlock[nCentroid];
			
			int sz = PCASiftLength / M;

			unsigned int aa , bb;

			if(a)
			{
				for(int nblock = 0 ; nblock < M ; nblock ++)
				{
					aa = (a & ((1<<(sz))-1));
					bb = (b & ((1<<(sz))-1));

					weight[nCentroid][nblock][aa] ++;
					weight_var[nCentroid][nblock][bb] ++;

					a = (a >> sz);
					b = (b >> sz);
				}
			}
		}
	}
}
#else
void SCFVIndex::loadHammingWeight()
{
}
#endif

void SCFVIndex::read(string sIndexName)
{
	//cout << "Reading index from: " << sIndexName << endl;
	FILE* pIndexFile = fopen(sIndexName.c_str(), "rb");
	if (pIndexFile == NULL) {
		throw CdvsException(string("SCFVIndex::read - Error opening ").append(sIndexName));
	}

	size_t nNumImages = 0;
	size_t nNumImagesPrev = numberImages();
	int nRead = fread(&nNumImages, sizeof(unsigned int), 1, pIndexFile);
	//cout << nNumImages <<" images in index" << endl;
	
	m_signatures.resize(nNumImages + nNumImagesPrev, SCFVSignature(false, false));	// create empty instances of signatures
	
	for (int k = nNumImagesPrev; k < nNumImagesPrev + nNumImages; ++k) 
	{
		m_signatures[k].fromFile(pIndexFile);		// read from file
	}
  
 	fclose(pIndexFile);
	//cout << "Done." << endl; 

	
}

unsigned int SCFVIndex::compressToOriginal(unsigned int a , int nCentroid)
{
	unsigned int bit = 0 , one = 1;

	for(int i = 0 ; i < PCASiftLength ; i ++)
	{
		if( ( (one << i) & SCFVSignature::table_bit_selection[nCentroid] ) != 0 )
		{
			bit += (one << i) * (a & 1);
			a = (a >> 1);
		}
	}

	return bit;
}


void SCFVIndex::query_bitselection(const SCFVSignature& querySignature, vector< pair<double,unsigned int> >& vDatabaseScoresIndices, size_t numRankedOuput) const
{
	float fQueryNorm = querySignature.getNorm();	// get query norm

	// Compare against database signatures: 1st round
	size_t nNumDatabaseImages = numberImages();
	vDatabaseScoresIndices.resize(nNumDatabaseImages);

	unsigned int h;
	int nImage = 0;

	unsigned int *bitsOfQuery = new unsigned int[numberCentroids];

	for(int i = 0 ; i < numberCentroids ; i ++)
		bitsOfQuery[i] = compressToOriginal( querySignature.m_vWordBlock[i] , i );

	for(std::vector<SCFVSignature>::const_iterator pImage=m_signatures.begin(); pImage < m_signatures.end(); ++pImage) {
		vDatabaseScoresIndices[nImage].second = nImage;
		if (pImage->getVisited() <= 5) {
			vDatabaseScoresIndices[nImage++].first = 0;
			continue;
		}

		// Compute correlation for first few centroids
		float fTotalCorrelation = 0;

		/* Signatures have variance and mean information */
		if (querySignature.hasVar() && pImage->hasVar()) {
			/* Unrolling this loop gives some extra speed */
			for (int nCentroid = 0; nCentroid < numberCentroids; nCentroid++) {
				sum_mean_var_bitselection(nCentroid);
				nCentroid++;
				sum_mean_var_bitselection (nCentroid);
				nCentroid++;
				sum_mean_var_bitselection (nCentroid);
				nCentroid++;
				sum_mean_var_bitselection (nCentroid);
			} 
			vDatabaseScoresIndices[nImage++].first =  fTotalCorrelation/pImage->getNorm();
		}
		/* Signatures only have mean information */
		else {
			for (int nCentroid = 0; nCentroid < numberCentroids; nCentroid++) {
				sum_mean_only_bitselection (nCentroid);
				nCentroid++;
				sum_mean_only_bitselection (nCentroid);
				nCentroid++;
				sum_mean_only_bitselection (nCentroid);
				nCentroid++;
				sum_mean_only_bitselection (nCentroid);
			}
		vDatabaseScoresIndices[nImage++].first =  fTotalCorrelation/pImage->getNorm();
		} //!hasVar


	} // pImage

	delete[] bitsOfQuery;
	// Sort scores  was: sort(vDatabaseScoresIndices.begin(), vDatabaseScoresIndices.end(), cmpDoubleUintAscend);
	// Sort scores and produce the final ranking of numRankedOuput images (without sorting all images)
	size_t numOut = min(numRankedOuput, nNumDatabaseImages);
	partial_sort(vDatabaseScoresIndices.begin(), vDatabaseScoresIndices.begin() + numOut,vDatabaseScoresIndices.end(), cmpDoubleUintDescend);
	vDatabaseScoresIndices.resize(numOut);
}

#ifdef USE_WEIGHT_TABLE
void SCFVIndex::generateWeight(const SCFVSignature &querySignature , float *W2_log , float *W2_log_var , float weight_base) const
{
	unsigned int a, b, vara, varb, v, h;

	for (int nCentroid = 0; nCentroid < numberCentroids; nCentroid++) {
		a = querySignature.m_vWordBlock[nCentroid];

		if(a)
		{
			int sum = 0;
			int sz = PCASiftLength / M;

			for(int i = 0 ; i < M ; i ++)
			{
				int var = (a & ((1<<sz) - 1));

				vector<unsigned int> vec = getKNN( var );
				
				int len = vec.size();

				for(int j = 0 ; j < len ; j ++)
					sum += weight[nCentroid][i][vec[j]];

				a = (a >> sz);
			}

			W2_log[nCentroid] = log(weight_base) - log(1.0 * (sum + 0.000001));

			bool hasVar = querySignature.hasVar();
			if (hasVar) {
				vara = querySignature.m_vWordVarBlock[nCentroid];

				int sum = 0;
				int sz = PCASiftLength / M;

				for(int i = 0 ; i < M ; i ++)
				{
					int var = (vara & ((1<<sz) - 1));

					vector<unsigned int> vec = getKNN( var );
				
					int len = vec.size();

					for(int j = 0 ; j < len ; j ++)
						sum += weight_var[nCentroid][i][vec[j]];

					vara = (vara >> sz);
				}

				W2_log_var[nCentroid] = log(weight_base) - log(1.0 * (sum + 0.000001));
			}
		}
	}
}
#else
void SCFVIndex::generateWeight(const SCFVSignature &querySignature , float *W2_log , float *W2_log_var , float weight_base) const
{
}
#endif



void SCFVIndex::query(const SCFVSignature& querySignature, vector< pair<double,unsigned int> >& vDatabaseScoresIndices, size_t numRankedOuput) const
{

#ifdef USE_WEIGHT_TABLE
	float weight_base = m_signatures.size() * M;
	float *W2_log = new float[numberCentroids];
	float *W2_log_var = new float[numberCentroids];

	generateWeight(querySignature , W2_log , W2_log_var , weight_base);
#endif

	float fQueryNorm = querySignature.getNorm();	// get query norm

	// Compare against database signatures: 1st round
	size_t nNumDatabaseImages = numberImages();
	vDatabaseScoresIndices.resize(nNumDatabaseImages);

	unsigned int h;
	int nImage = 0;

	for(std::vector<SCFVSignature>::const_iterator pImage=m_signatures.begin(); pImage < m_signatures.end(); ++pImage) {
		vDatabaseScoresIndices[nImage].second = nImage;
		if (pImage->getVisited() <= 5) {
			vDatabaseScoresIndices[nImage++].first = 0;
			continue;
		}

		// Compute correlation for first few centroids
		float fTotalCorrelation = 0;

//		int num_overlap = 0;
		/* Signatures have variance and mean information */
		if (querySignature.hasVar() && pImage->hasVar()) {
			// TODO: Unrolling this loop gives some extra speed
			for (int nCentroid = 0; nCentroid < numberCentroids; nCentroid++) {
				sum_mean_var (nCentroid);
				nCentroid++;
				sum_mean_var (nCentroid);
				nCentroid++;
				sum_mean_var (nCentroid);
				nCentroid++;
				sum_mean_var (nCentroid);
			} 
			vDatabaseScoresIndices[nImage++].first =  fTotalCorrelation/pImage->getNorm();
		}
		/* Signatures only have mean information */
		else {
			for (int nCentroid = 0; nCentroid < numberCentroids; nCentroid++) {
				sum_mean_only (nCentroid);
				nCentroid++;
				sum_mean_only (nCentroid);
				nCentroid++;
				sum_mean_only (nCentroid);
				nCentroid++;
				sum_mean_only (nCentroid);
			}
		vDatabaseScoresIndices[nImage++].first =  fTotalCorrelation/pImage->getNorm();
		} //!hasVar


	} // pImage

	// Sort scores  was: sort(vDatabaseScoresIndices.begin(), vDatabaseScoresIndices.end(), cmpDoubleUintAscend);
	// Sort scores and produce the final ranking of numRankedOuput images (without sorting all images)
	size_t numOut = min(numRankedOuput, nNumDatabaseImages);
	partial_sort(vDatabaseScoresIndices.begin(), vDatabaseScoresIndices.begin() + numOut,vDatabaseScoresIndices.end(), cmpDoubleUintDescend);
	vDatabaseScoresIndices.resize(numOut);

#ifdef USE_WEIGHT_TABLE
	delete[] W2_log;
	delete[] W2_log_var;
#endif

}

const float SCFVIndex::fCorrTable[] = {
		PCASiftLength * pow(CorrelationWeights[0],beta),
		(PCASiftLength -2) * pow(CorrelationWeights[1],beta),
		(PCASiftLength -4) * pow(CorrelationWeights[2],beta),
		(PCASiftLength -6) * pow(CorrelationWeights[3],beta),
		(PCASiftLength -8) * pow(CorrelationWeights[4],beta),
		(PCASiftLength -10) * pow(CorrelationWeights[5],beta),
		(PCASiftLength -12) * pow(CorrelationWeights[6],beta),
		(PCASiftLength -14) * pow(CorrelationWeights[7],beta),
		(PCASiftLength -16) * pow(CorrelationWeights[8],beta),
		(PCASiftLength -18) * pow(CorrelationWeights[9],beta),
		(PCASiftLength -20) * pow(CorrelationWeights[10],beta),
		(PCASiftLength -22) * pow(CorrelationWeights[11],beta),
		(PCASiftLength -24) * pow(CorrelationWeights[12],beta),
		(PCASiftLength -26) * pow(CorrelationWeights[13],beta),
		(PCASiftLength -28) * pow(CorrelationWeights[14],beta),
		(PCASiftLength -30) * pow(CorrelationWeights[15],beta),
		(PCASiftLength -32) * pow(CorrelationWeights[16],beta),
		(PCASiftLength -34) * pow(CorrelationWeights[17],beta),
		(PCASiftLength -36) * pow(CorrelationWeights[18],beta),
		(PCASiftLength -38) * pow(CorrelationWeights[19],beta),
		(PCASiftLength -40) * pow(CorrelationWeights[20],beta),
		(PCASiftLength -42) * pow(CorrelationWeights[21],beta),
		(PCASiftLength -44) * pow(CorrelationWeights[22],beta),
		(PCASiftLength -46) * pow(CorrelationWeights[23],beta),
		(PCASiftLength -48) * pow(CorrelationWeights[24],beta),
		(PCASiftLength -50) * pow(CorrelationWeights[25],beta),
		(PCASiftLength -52) * pow(CorrelationWeights[26],beta),
		(PCASiftLength -54) * pow(CorrelationWeights[27],beta),
		(PCASiftLength -56) * pow(CorrelationWeights[28],beta),
		(PCASiftLength -58) * pow(CorrelationWeights[29],beta),
		(PCASiftLength -60) * pow(CorrelationWeights[30],beta),
		(PCASiftLength -62) * pow(CorrelationWeights[31],beta),
		(PCASiftLength -64) * pow(CorrelationWeights[32],beta)
};
const float SCFVIndex::fVarCorrTable[] = {
		PCASiftLength * pow(VarCorrelationWeights[0],beta),
		(PCASiftLength -2) * pow(VarCorrelationWeights[1],beta),
		(PCASiftLength -4) * pow(VarCorrelationWeights[2],beta),
		(PCASiftLength -6) * pow(VarCorrelationWeights[3],beta),
		(PCASiftLength -8) * pow(VarCorrelationWeights[4],beta),
		(PCASiftLength -10) * pow(VarCorrelationWeights[5],beta),
		(PCASiftLength -12) * pow(VarCorrelationWeights[6],beta),
		(PCASiftLength -14) * pow(VarCorrelationWeights[7],beta),
		(PCASiftLength -16) * pow(VarCorrelationWeights[8],beta),
		(PCASiftLength -18) * pow(VarCorrelationWeights[9],beta),
		(PCASiftLength -20) * pow(VarCorrelationWeights[10],beta),
		(PCASiftLength -22) * pow(VarCorrelationWeights[11],beta),
		(PCASiftLength -24) * pow(VarCorrelationWeights[12],beta),
		(PCASiftLength -26) * pow(VarCorrelationWeights[13],beta),
		(PCASiftLength -28) * pow(VarCorrelationWeights[14],beta),
		(PCASiftLength -30) * pow(VarCorrelationWeights[15],beta),
		(PCASiftLength -32) * pow(VarCorrelationWeights[16],beta),
		(PCASiftLength -34) * pow(VarCorrelationWeights[17],beta),
		(PCASiftLength -36) * pow(VarCorrelationWeights[18],beta),
		(PCASiftLength -38) * pow(VarCorrelationWeights[19],beta),
		(PCASiftLength -40) * pow(VarCorrelationWeights[20],beta),
		(PCASiftLength -42) * pow(VarCorrelationWeights[21],beta),
		(PCASiftLength -44) * pow(VarCorrelationWeights[22],beta),
		(PCASiftLength -46) * pow(VarCorrelationWeights[23],beta),
		(PCASiftLength -48) * pow(VarCorrelationWeights[24],beta),
		(PCASiftLength -50) * pow(VarCorrelationWeights[25],beta),
		(PCASiftLength -52) * pow(VarCorrelationWeights[26],beta),
		(PCASiftLength -54) * pow(VarCorrelationWeights[27],beta),
		(PCASiftLength -56) * pow(VarCorrelationWeights[28],beta),
		(PCASiftLength -58) * pow(VarCorrelationWeights[29],beta),
		(PCASiftLength -60) * pow(VarCorrelationWeights[30],beta),
		(PCASiftLength -62) * pow(VarCorrelationWeights[31],beta),
		(PCASiftLength -64) * pow(VarCorrelationWeights[32],beta)
};



float SCFVIndex::matchImages(const SCFVSignature& signature1, const SCFVSignature& signature2, unsigned int* pNumWords1, unsigned int* pNumWords2, unsigned int* overlap) const 
{
	// SCFV signature for image 1
	unsigned int nNumWordsVisited1 = signature1.getVisited();
	float fNorm1 = signature1.getNorm();
	*pNumWords1 = nNumWordsVisited1;

	// SCFV signature for image 2
	unsigned int nNumWordsVisited2 = signature2.getVisited();
	float fNorm2 = signature2.getNorm();
	*pNumWords2 = nNumWordsVisited2;

	bool useVar = signature1.hasVar() && signature2.hasVar();	// both signatures must have the variance information

	// Compute correlation
	float fTotalCorrelation = 0;
	unsigned int a, b, vara, varb, v, h;

	if ( (nNumWordsVisited1 > 0) && (nNumWordsVisited2 > 0) ) {
		for (int nCentroid = 0; nCentroid < numberCentroids; nCentroid++) {
			a = signature1.m_vWordBlock[nCentroid];
			b = signature2.m_vWordBlock[nCentroid];
			if (a && b) {
				// Compute Hamming distance
				v = ((a ^ b));
				h = POPCNT(v);
				// Add to correlation
				fTotalCorrelation += fCorrTable[h];	// use pre-computed table (faster)

				if (useVar) {
					vara = signature1.m_vWordVarBlock[nCentroid];
					varb = signature2.m_vWordVarBlock[nCentroid];
					v = (vara ^ varb);
					h = POPCNT(v);
					// Add to correlation
					fTotalCorrelation += fVarCorrTable[h];	// use pre-computed table (faster)
				}
			}
		} // nCentroid

		if (useVar)
			return fTotalCorrelation / (2 * fNorm1 * fNorm2);
		else
			return fTotalCorrelation / (fNorm1 * fNorm2);
	}

	return 0;
}

const float SCFVIndex::fCorrTableBitSelection[] = {
		num_bit_selection * CorrelationWeights_bit_selection[0],
		(num_bit_selection -2) * CorrelationWeights_bit_selection[1],
		(num_bit_selection -4) * CorrelationWeights_bit_selection[2],
		(num_bit_selection -6) * CorrelationWeights_bit_selection[3],
		(num_bit_selection -8) * CorrelationWeights_bit_selection[4],
		(num_bit_selection -10) * CorrelationWeights_bit_selection[5],
		(num_bit_selection -12) * CorrelationWeights_bit_selection[6],
		(num_bit_selection -14) * CorrelationWeights_bit_selection[7],
		(num_bit_selection -16) * CorrelationWeights_bit_selection[8],
		(num_bit_selection -18) * CorrelationWeights_bit_selection[9],
		(num_bit_selection -20) * CorrelationWeights_bit_selection[10],
		(num_bit_selection -22) * CorrelationWeights_bit_selection[11],
		(num_bit_selection -24) * CorrelationWeights_bit_selection[12],
		(num_bit_selection -26) * CorrelationWeights_bit_selection[13],
		(num_bit_selection -28) * CorrelationWeights_bit_selection[14],
		(num_bit_selection -30) * CorrelationWeights_bit_selection[15],
		(num_bit_selection -32) * CorrelationWeights_bit_selection[16],
		(num_bit_selection -34) * CorrelationWeights_bit_selection[17],
		(num_bit_selection -36) * CorrelationWeights_bit_selection[18],
		(num_bit_selection -38) * CorrelationWeights_bit_selection[19],
		(num_bit_selection -40) * CorrelationWeights_bit_selection[20],
		(num_bit_selection -42) * CorrelationWeights_bit_selection[21],
		(num_bit_selection -44) * CorrelationWeights_bit_selection[22],
		(num_bit_selection -46) * CorrelationWeights_bit_selection[23],
		(num_bit_selection -48) * CorrelationWeights_bit_selection[24]
};

const float SCFVIndex::fVarCorrTableBitSelection[] = {
		num_bit_selection * VarCorrelationWeights_bit_selection[0],
		(num_bit_selection -2) * VarCorrelationWeights_bit_selection[1],
		(num_bit_selection -4) * VarCorrelationWeights_bit_selection[2],
		(num_bit_selection -6) * VarCorrelationWeights_bit_selection[3],
		(num_bit_selection -8) * VarCorrelationWeights_bit_selection[4],
		(num_bit_selection -10) * VarCorrelationWeights_bit_selection[5],
		(num_bit_selection -12) * VarCorrelationWeights_bit_selection[6],
		(num_bit_selection -14) * VarCorrelationWeights_bit_selection[7],
		(num_bit_selection -16) * VarCorrelationWeights_bit_selection[8],
		(num_bit_selection -18) * VarCorrelationWeights_bit_selection[9],
		(num_bit_selection -20) * VarCorrelationWeights_bit_selection[10],
		(num_bit_selection -22) * VarCorrelationWeights_bit_selection[11],
		(num_bit_selection -24) * VarCorrelationWeights_bit_selection[12],
		(num_bit_selection -26) * VarCorrelationWeights_bit_selection[13],
		(num_bit_selection -28) * VarCorrelationWeights_bit_selection[14],
		(num_bit_selection -30) * VarCorrelationWeights_bit_selection[15],
		(num_bit_selection -32) * VarCorrelationWeights_bit_selection[16],
		(num_bit_selection -34) * VarCorrelationWeights_bit_selection[17],
		(num_bit_selection -36) * VarCorrelationWeights_bit_selection[18],
		(num_bit_selection -38) * VarCorrelationWeights_bit_selection[19],
		(num_bit_selection -40) * VarCorrelationWeights_bit_selection[20],
		(num_bit_selection -42) * VarCorrelationWeights_bit_selection[21],
		(num_bit_selection -44) * VarCorrelationWeights_bit_selection[22],
		(num_bit_selection -46) * VarCorrelationWeights_bit_selection[23],
		(num_bit_selection -48) * VarCorrelationWeights_bit_selection[24]
};

float SCFVIndex::matchImages_bitselection(const SCFVSignature& signature1, const SCFVSignature& signature2, unsigned int* pNumWords1, unsigned int* pNumWords2, unsigned int* overlap) const 
{
	// SCFV signature for image 1
	unsigned int nNumWordsVisited1 = signature1.getVisited();
	float fNorm1 = signature1.getNorm();
	*pNumWords1 = nNumWordsVisited1;

	// SCFV signature for image 2
	unsigned int nNumWordsVisited2 = signature2.getVisited();
	float fNorm2 = signature2.getNorm();
	*pNumWords2 = nNumWordsVisited2;

	bool useVar = signature1.hasVar() && signature2.hasVar();	// both signatures must have the variance information

	// Compute correlation
	float fCorrelation = 0;
	float fTotalCorrelation = 0;
	unsigned int a, b, vara, varb, v, h;

	if ( (nNumWordsVisited1 > 0) && (nNumWordsVisited2 > 0) ) {
		for (int nCentroid = 0; nCentroid < numberCentroids; nCentroid++) {
			a = signature1.m_vWordBlock[nCentroid];
			b = signature2.m_vWordBlock[nCentroid];
			if (a && b) {
				// Compute Hamming distance
				a = compressToOriginal(a , nCentroid);
				b = compressToOriginal(b , nCentroid);

				v = ( (a ^ b) & SCFVSignature::table_bit_selection[nCentroid] ) ;

				h = POPCNT(v);
				// Add to correlation
				fTotalCorrelation += fCorrTableBitSelection[h];	// use pre-computed table (faster)

				if (useVar) {
					vara = signature1.m_vWordVarBlock[nCentroid];
					varb = signature2.m_vWordVarBlock[nCentroid];
					v = (vara ^ varb);
					h = POPCNT(v);
					// Add to correlation
					fTotalCorrelation += fVarCorrTableBitSelection[h];	// use pre-computed table (faster)
				}
			}
		} // nCentroid

		if (useVar)
			return fTotalCorrelation / (2 * fNorm1 * fNorm2);
		else
			return fTotalCorrelation / (fNorm1 * fNorm2);
	}

	// return
	return 0;
}

// ====================== class SCFVFactory ======================//

SCFVFactory::SCFVFactory():
		m_bHasVar(false),
		m_bHasBitSelection(false),
		m_fSCFVThreshold(0.0f)
{
}

void SCFVFactory::init(const Parameters & params)
{
	  m_bHasBitSelection = params.hasBitSelection;	// Set true if using bit selection
	  m_bHasVar = params.hasVar;	  			// Set true if using variance
	  m_fSCFVThreshold = params.scfvThreshold;	// Set threshold value to control the sparsity of scfv vector
}


unsigned int SCFVFactory::generateSFV(const FeatureList& featureList, float* pFisherVector, bool vWordVisited[], int nNumFeatures) const
{
  /* 128-d SIFT descriptor projection */
  
  int m = UNCOMPRESSED_SIFT_DESC_LENGTH;
  Matrix<float,UNCOMPRESSED_SIFT_DESC_LENGTH,Dynamic> DataPoints(m, nNumFeatures);  // matrix

  for (int nFeature = 0; nFeature < nNumFeatures; ++nFeature) {
	  float* descr = new float[UNCOMPRESSED_SIFT_DESC_LENGTH];
	  float l1norm = .0;
	  for(int nDim = 0; nDim < UNCOMPRESSED_SIFT_DESC_LENGTH; ++nDim) {
		  descr[nDim] = featureList.features[nFeature].descr[nDim];
		  l1norm += fabs(descr[nDim]);
	  }
	  if (l1norm == 0) l1norm = 1;
	  for(int nDim = 0; nDim < UNCOMPRESSED_SIFT_DESC_LENGTH; ++nDim)
		  DataPoints(nDim, nFeature) = descr[nDim]/l1norm;
	  delete[] descr;
	}

  // preprocess
  DataPoints =  DataPoints.array().pow(PCA_SIFT_POWER_LAW).matrix();

  // use mu as a VectorXf object
  Map<VectorXf> meanVector((float *) PCA_SIFT_MU, UNCOMPRESSED_SIFT_DESC_LENGTH);

  // for each point center the point with the mean among all the coordinates
  for (int i = 0; i < DataPoints.cols(); i++){
	   DataPoints.col(i) -= meanVector;
  }

  // use eigvec as a MatrixXf object
  Map<MatrixXf> eigvectors((float *) PCA_SIFT_EIGEN, UNCOMPRESSED_SIFT_DESC_LENGTH, PCASiftLength);
  MatrixXf projected = eigvectors.transpose() * DataPoints;

  fisher_param myparams;
  myparams.grad_variances = m_bHasVar;
  myparams.alpha = SCFV_POWER_LAW;

  // myparams.print();

  fisher myfisher(myparams);
  static const gaussian_mixture mygmm(numberCentroids, PCASiftLength, GMM_W, GMM_MU, GMM_SIGMA);
  myfisher.set_model(mygmm);

  float * myprojected = projected.data();
  size_t ncols = projected.cols();
  vector<float *> proj_vec(ncols, NULL);
  int nrows = projected.rows();
  for (int i=0; i<projected.cols(); ++i)
  {
	proj_vec[i] = myprojected + i*nrows;
  }

  myfisher.compute( proj_vec, pFisherVector );

	/* fv sparsity for scalable compact descriptor */
  unsigned int nNumWordsVisited = 0;
  float* standardDeviations = new float[numberCentroids];

  for (int nCentroid = 0; nCentroid < numberCentroids; nCentroid++) 
  {
	    float* pGmm = pFisherVector + nCentroid*PCASiftLength;
  		float mean=0.0, var=0.0;
  		for (int nInDim = 0; nInDim < PCASiftLength; nInDim++)
  			mean += pGmm[nInDim];
  		mean /= PCASiftLength;
  		for (int nInDim = 0; nInDim < PCASiftLength; nInDim++)
  			var += (pGmm[nInDim]-mean)*(pGmm[nInDim]-mean);
  		standardDeviations[nCentroid] = sqrt(var / PCASiftLength);
  }

  if ((int) m_fSCFVThreshold > 1) 
  {
  		// use variance to reject gmm words
		map<float, int> stds;
  		for (int nCentroid = 0; nCentroid < numberCentroids; nCentroid++) 
  			stds.insert(pair<float, int>(abs(standardDeviations[nCentroid]), nCentroid));

  		// mark visits
  		map<float, int>::reverse_iterator rit;
  		for (rit=stds.rbegin(); rit!=stds.rend(); rit++) 
		{
  			if (nNumWordsVisited >= (int)m_fSCFVThreshold)
  				break;
  			vWordVisited[rit->second] = true;
  			nNumWordsVisited++;
  		}
  }
  else 
  {
		for (int nCentroid = 0; nCentroid < numberCentroids; nCentroid++) 
		{
			vWordVisited[nCentroid] = (standardDeviations[nCentroid] >= m_fSCFVThreshold);
			if (vWordVisited[nCentroid]) nNumWordsVisited++;
		}
  }

  delete[] standardDeviations;

  return nNumWordsVisited;
}


void SCFVFactory::generateSCFV(const FeatureList& featureList, SCFVSignature & signature, int nNumFeatures) const
{
  int numExtracted = std::min(nNumFeatures, (int) featureList.features.size());	// check size
  signature.clear();				// clear all values
  signature.hasVar(m_bHasVar);		// set the hasVar flag (must be done before calling signature.size())
  signature.hasBitSelection(m_bHasBitSelection);	// set the bit selection flag
  if (numExtracted == 0)
	  return;

  float* pFisherVector = new float[signature.size()];
  memset( pFisherVector, 0, signature.size()*sizeof(float));
  bool visited[numberCentroids];		// temporary array
  memset(visited, 0, sizeof(visited));				// clear all values in visited

  // Generate uncompressed SFV signature
  unsigned int nNumWordsVisited = generateSFV(featureList, pFisherVector, visited, numExtracted);

  // Sign binarize
  for (int nCentroid = 0; nCentroid < numberCentroids; nCentroid++) {
	  if (visited[nCentroid]) {
		  unsigned int nPackedBlock = 0;
		  float* pGmm = pFisherVector + nCentroid*PCASiftLength; // assumes SIFT PCA dimension <= block width = #bits(uint)
		  for (int nDim = 0; nDim < PCASiftLength; nDim++) {
			  unsigned int nBit = (pGmm[nDim] > 0) ? 1 : 0;
			  PUSH_BIT(nPackedBlock, nBit);
		  } // nDim
		  signature.m_vWordBlock[nCentroid] = nPackedBlock;
	  }
  } // nCentroid

  if (m_bHasVar) {
	  for (int nCentroid = 0; nCentroid < numberCentroids; nCentroid++) {
		  if (visited[nCentroid]) {
			  unsigned int nPackedBlock = 0;
			  float* pGmm = pFisherVector + (nCentroid+numberCentroids)*PCASiftLength; // assumes SIFT PCA dimension <= block width = #bits(uint)
			  for (int nDim = 0; nDim < PCASiftLength; nDim++) {
				  unsigned int nBit = (pGmm[nDim] > 0) ? 1 : 0;
				  PUSH_BIT(nPackedBlock, nBit);
			  } // nDim
			  signature.m_vWordVarBlock[nCentroid] = nPackedBlock;
		  }
	  } // nCentroid
  }
  
  signature.setNorm();				// compute the norm of this signature

  delete [] pFisherVector;
}

#ifdef USE_MBIT

// MBIT struct
void SCFVIndex::initMBIT()
{
	
	int nNumDatabaseImages = m_signatures.size();
	for(int nImage = 0 ; nImage < nNumDatabaseImages ; nImage ++)
	{
		for (int nCentroid = 0; nCentroid < numberCentroids; nCentroid++) 
		{
			unsigned int rBlock = m_signatures[nImage].m_vWordBlock[nCentroid];
			if (rBlock != 0) 
			{
				unsigned int leftPart = (rBlock >> 16);
				unsigned int rightPart = (rBlock & 65535);
			}
		}
	}
	
	for (int nCentroid = 0; nCentroid < numberCentroids; nCentroid++)
	{
		for(int i = 0 ; i < (1<<16) ; i ++)
			vector<int>().swap(vec[i]);

		for(int nImage = 0 ; nImage < nNumDatabaseImages ; nImage ++)
		{
			unsigned int rBlock = m_signatures[nImage].m_vWordBlock[nCentroid];

			if (rBlock != 0) 
			{
				unsigned int leftPart = (rBlock >> 16);
				vec[leftPart].push_back(nImage);
			}
		}

		for(int i = 0 ; i < (1<<16) ; i ++)
		{
			SizeOfMBIT[nCentroid * 2][i] = vec[i].size();
			MBIT[nCentroid * 2][i] = new int[SizeOfMBIT[nCentroid * 2][i]];
			for(int j = 0 ; j < SizeOfMBIT[nCentroid * 2][i] ; j ++)
				MBIT[nCentroid * 2][i][j] = vec[i][j];
		}

		for(int i = 0 ; i < (1<<16) ; i ++)
			vector<int>().swap(vec[i]);

		for(int nImage = 0 ; nImage < nNumDatabaseImages ; nImage ++)
		{
			unsigned int rBlock = m_signatures[nImage].m_vWordBlock[nCentroid];

			if (rBlock != 0) 
			{
				unsigned int rightPart = (rBlock & 65535);
				vec[rightPart].push_back(nImage);
			}
		}

		for(int i = 0 ; i < (1<<16) ; i ++)
		{
			SizeOfMBIT[nCentroid * 2 + 1][i] = vec[i].size();
			MBIT[nCentroid * 2 + 1][i] = new int[SizeOfMBIT[nCentroid * 2 + 1][i]];
			for(int j = 0 ; j < SizeOfMBIT[nCentroid * 2 + 1][i] ; j ++)
				MBIT[nCentroid * 2 + 1][i][j] = vec[i][j];
		}
	}

	for(int i = 0 ; i < (1<<16) ; i ++)
		vector<int>().swap(vec[i]);
	

	bitsDiff.push_back(0);
	numDiff.push_back(1<<7);
	for(int i = 0 ; i < 16 ; i ++)
		bitsDiff.push_back( (unsigned int)(1<<i) ) , numDiff.push_back(1<<6);
	for(int i = 0 ; i < 16 ; i ++)
		for(int j = i + 1 ; j < 16 ; j ++)
			bitsDiff.push_back( (unsigned int)((1<<i) + (1<<j)) ) , numDiff.push_back(1<<4);
	for(int i = 0 ; i < 16 ; i ++)
		for(int j = i + 1 ; j < 16 ; j ++)
			for(int k = j + 1 ; k < 16 ; k ++)
				bitsDiff.push_back( (unsigned int)( (1<<i) + (1<<j) + (1<<k) ) ) , numDiff.push_back(1);
}

void SCFVIndex::queryMBIT(const SCFVSignature& querySignature, vector< pair<double,unsigned int> >& vDatabaseScoresIndices, size_t numRankedOuput, size_t MBIT_Threshold) const
{
	float fQueryNorm = querySignature.getNorm();	// get query norm

	// Compare against database signatures: 1st round
	size_t nNumDatabaseImages = numberImages();
	vDatabaseScoresIndices.resize(nNumDatabaseImages);
	unsigned int a, b, vara, varb, v, h;
	int nImage = 0;

	//MBIT voting stage
	unsigned char * flag;
	unsigned char * weight;
	flag = new unsigned char[nNumDatabaseImages];
	weight = new unsigned char[nNumDatabaseImages];
	memset( flag , 0 , sizeof(unsigned char) * nNumDatabaseImages);
	memset( weight , 0 , sizeof(unsigned char) * nNumDatabaseImages);
	
	for (int nCentroid = 0; nCentroid < numberCentroids; nCentroid++) 
	{
		unsigned int qBlock = querySignature.m_vWordBlock[nCentroid];
		
		if(qBlock == 0)
			continue;

		unsigned int leftPart = (qBlock >> 16);
		unsigned int rightPart = (qBlock & 65535);
		
		int listSize;
		int blockSize = bitsDiff.size();

		for(int i = 0 ; i < blockSize ; i ++)
		{
			unsigned int id = ( leftPart ^ bitsDiff[i] );

			//listSize = vecMBIT[ 2 * nCentroid ][ id ].size();
			listSize = SizeOfMBIT[ 2 * nCentroid ][ id ];
			//listSize = hashTable[ 2 * nCentroid ][ id ].size();
			for(int j = 0 ; j < listSize ; j ++)
			{
				//int imageId =  vecMBIT[ 2 * nCentroid ][ id ][ j ] ;
				int imageId =  MBIT[ 2 * nCentroid ][ id ][ j ] ;
				//int imageId =  hashTable[ 2 * nCentroid ][ id ][ j ] ;
				weight[ imageId ] = (unsigned char)min( 255 ,  ( (int)weight[ imageId ] + numDiff[i] ) );
			}
		}
		
		for(int i = 0 ; i < blockSize ; i ++)
		{
			unsigned int id = ( rightPart ^ bitsDiff[i] );

			//listSize = vecMBIT[ 2 * nCentroid + 1 ][ id ].size();
			listSize = SizeOfMBIT[ 2 * nCentroid + 1 ][ id ];
			//listSize = hashTable[ 2 * nCentroid + 1 ][ id ].size();
			for(int j = 0 ; j < listSize ; j ++)
			{
				//int imageId = hashTable[ 2 * nCentroid + 1 ][ id ][ j ];
				int imageId = MBIT[ 2 * nCentroid + 1 ][ id ][ j ];
				//int imageId = vecMBIT[ 2 * nCentroid + 1 ][ id ][ j ];
				weight[ imageId ] = (unsigned char)min( 255 ,  ( (int)weight[ imageId ] + numDiff[i] ) );
			}
		}
	}

	for(std::vector<SCFVSignature>::const_iterator pImage=m_signatures.begin(); pImage < m_signatures.end(); ++pImage) {
		vDatabaseScoresIndices[nImage].second = nImage;
		if (pImage->getVisited() <= 5 || weight[ nImage ] <= MBIT_Threshold) {
			vDatabaseScoresIndices[nImage++].first = 2.5;
			continue;
		}

		// Compute correlation for first few centroids
		float fCorrelation = 0;
		float fTotalCorrelation = 0;
		float fTotalVarCorrelation = 0;
		bool hasVar = querySignature.hasVar() && pImage->hasVar();	// both signatures must have the variance information

		for (int nCentroid = 0; nCentroid < numberCentroids; nCentroid++) {
			a = querySignature.m_vWordBlock[nCentroid];
			b = pImage->m_vWordBlock[nCentroid];
			if (hasVar) {
				vara = querySignature.m_vWordVarBlock[nCentroid];
				varb = pImage->m_vWordVarBlock[nCentroid];
			}
			if(a && b){
				// Compute Hamming distance
				v = (a ^ b);
				h = lut.f[v>>16] + lut.f[v&65535];
				// Add to correlation
				fTotalCorrelation += fCorrTable[h];		// use pre-computed table (faster)

				if (hasVar) {
					v = (vara ^ varb);
					h = lut.f[v>>16] + lut.f[v&65535];
					// Add to correlation
					fTotalCorrelation += fVarCorrTable[h];	// use pre-computed table (faster)
				}
			}
		} // nCentroid
		fTotalCorrelation /= pImage->getNorm() * fQueryNorm;
		fTotalVarCorrelation /= pImage->getNorm() * fQueryNorm;

		if (hasVar)
			fCorrelation = (fTotalCorrelation + fTotalVarCorrelation) * 0.5f;
		else
			fCorrelation = fTotalCorrelation;

		vDatabaseScoresIndices[nImage++].first = 2 - 2*fCorrelation;
	} // nImage

	// Sort scores  was: sort(vDatabaseScoresIndices.begin(), vDatabaseScoresIndices.end(), cmpDoubleUintAscend);
	// Sort scores and produce the final ranking of numRankedOuput images (without sorting all images)
	size_t numOut = min(numRankedOuput, nNumDatabaseImages);
	partial_sort(vDatabaseScoresIndices.begin(), vDatabaseScoresIndices.begin() + numOut,vDatabaseScoresIndices.end(), cmpDoubleUintAscend);
	vDatabaseScoresIndices.resize(numOut);
	delete []flag;
	delete []weight;
}




unsigned int SCFVIndex::inv_bits(unsigned int a)
{
	unsigned int ans = 0;

	for(int i = 0 ; i < num_bit_selection ; i ++)
	{
		if(a & (1<<i))
			ans = (ans << 1) + 1;
		else
			ans = (ans << 1);
	}

	return ans;
}

unsigned int SCFVIndex::originalTocompress(unsigned int a , int nCentroid)
{
	//int cnt = 0;
	unsigned int mask = SCFVSignature::table_bit_selection[nCentroid];
	unsigned cbits = 0;

	while(mask)
	{
		if(mask & 1)
		{
			if( a & 1 )
			{
				cbits = ((cbits << 1) | 1);
			}
			else
			{
				cbits = (cbits << 1);
			}
		}
		
		mask = (mask >> 1);
		a = (a >> 1);
	}

	return cbits;
}

bool SCFVIndex::cmp(Node A , Node B)
{
	return A.val > B.val;
}


void SCFVIndex::query_sort_bit_selection(const SCFVSignature& querySignature, vector< pair<double,unsigned int> >& vDatabaseScoresIndices, size_t numRankedOuput, size_t MBIT_Threshold, int &num_candidate) const
{
	

	float weight_base = m_signatures.size() * M;

	float fQueryNorm = querySignature.getNorm();	// get query norm

	// Compare against database signatures: 1st round
	size_t nNumDatabaseImages = numberImages();
	vDatabaseScoresIndices.resize(nNumDatabaseImages);
	unsigned int a, b, vara, varb, v, h;
	int nImage = 0;

	//MBIT voting stage
	unsigned char * flag;
	unsigned char * weight;
	flag = new unsigned char[nNumDatabaseImages];
	weight = new unsigned char[nNumDatabaseImages];
	memset( flag , 0 , sizeof(unsigned char) * nNumDatabaseImages);
	memset( weight , 0 , sizeof(unsigned char) * nNumDatabaseImages);
	
	num_candidate = 0;

	struct Node *nodes = new Node[nNumDatabaseImages];

	for (int nCentroid = 0; nCentroid < numberCentroids; nCentroid++) 
	{
		unsigned int qBlock = querySignature.m_vWordBlock[nCentroid];
		
		if(qBlock == 0)
			continue;

		unsigned int c_qBlock = inv_bits(qBlock);

		unsigned int leftPart = (c_qBlock >> (num_bit_selection - mbit_m));
		unsigned int rightPart = (c_qBlock & ((1<<mbit_m)-1));
		
		int listSize;
		int blockSize = bitsDiff.size();

		for(int i = 0 ; i < blockSize ; i ++)
		{
			unsigned int id = ( leftPart ^ bitsDiff[i] );

			//listSize = vecMBIT[ 2 * nCentroid ][ id ].size();
			listSize = SizeOfMBIT[ 2 * nCentroid ][ id ];
			//listSize = hashTable[ 2 * nCentroid ][ id ].size();
			for(int j = 0 ; j < listSize ; j ++)
			{
				//int imageId =  vecMBIT[ 2 * nCentroid ][ id ][ j ] ;
				int imageId =  MBIT[ 2 * nCentroid ][ id ][ j ] ;
				//int imageId =  hashTable[ 2 * nCentroid ][ id ][ j ] ;
				weight[ imageId ] = (unsigned char)min( 255 ,  ( (int)weight[ imageId ] + numDiff[i] ) );
			}
		}
		
		for(int i = 0 ; i < blockSize ; i ++)
		{
			unsigned int id = ( rightPart ^ bitsDiff[i] );

			//listSize = vecMBIT[ 2 * nCentroid + 1 ][ id ].size();
			listSize = SizeOfMBIT[ 2 * nCentroid + 1 ][ id ];
			//listSize = hashTable[ 2 * nCentroid + 1 ][ id ].size();
			for(int j = 0 ; j < listSize ; j ++)
			{
				//int imageId = hashTable[ 2 * nCentroid + 1 ][ id ][ j ];
				int imageId = MBIT[ 2 * nCentroid + 1 ][ id ][ j ];
				//int imageId = vecMBIT[ 2 * nCentroid + 1 ][ id ][ j ];
				weight[ imageId ] = (unsigned char)min( 255 ,  ( (int)weight[ imageId ] + numDiff[i] ) );
			}
		}
	}

	for(int i = 0 ; i < nNumDatabaseImages ; i ++)
	{
		nodes[i].ID = i;
		nodes[i].val = weight[i];
	}

	sort(nodes , nodes + nNumDatabaseImages, cmp);

	int re_calc = (int)(candidate_size * nNumDatabaseImages);

	for(int i = 0 ; i < re_calc ; i ++)
		flag[ nodes[i].ID ] = 1;


	float *fCorrTable = new float[num_bit_selection + 1];
	float *fVarCorrTable = new float[num_bit_selection + 1];

	for (int h=0; h <= num_bit_selection; ++h)	
	{
		fCorrTable[h] = ((num_bit_selection - 2*h) * CorrelationWeights_bit_selection[h]);
		fVarCorrTable[h] = ((num_bit_selection - 2*h) * VarCorrelationWeights_bit_selection[h]);
	}


	unsigned int *bitsOfQuery = new unsigned int[numberCentroids];

	for(int i = 0 ; i < numberCentroids ; i ++)
		bitsOfQuery[i] = compressToOriginal( querySignature.m_vWordBlock[i] , i );

	for(std::vector<SCFVSignature>::const_iterator pImage=m_signatures.begin(); pImage < m_signatures.end(); ++pImage) {
		vDatabaseScoresIndices[nImage].second = nImage;
		if (pImage->getVisited() <= 5 || flag[nImage] == 0) {
			vDatabaseScoresIndices[nImage++].first = 2.5;
			continue;
		}

		num_candidate ++;

		// Compute correlation for first few centroids
		float fCorrelation = 0;
		float fTotalCorrelation = 0;
		float fTotalVarCorrelation = 0;
		bool hasVar = querySignature.hasVar() && pImage->hasVar();	// both signatures must have the variance information

		for (int nCentroid = 0; nCentroid < numberCentroids; nCentroid++) {
			
			a = bitsOfQuery[nCentroid];
			b = pImage->m_vWordBlock[nCentroid];
			if (hasVar) {
				vara = querySignature.m_vWordVarBlock[nCentroid];
				varb = pImage->m_vWordVarBlock[nCentroid];
			}
			if(a && b){
				// Compute Hamming distance

				v = ((a ^ b) & SCFVSignature::table_bit_selection[nCentroid]);
				h = lut.f[v>>16] + lut.f[v&65535];
				// Add to correlation
				fTotalCorrelation += fCorrTable[h];		// use pre-computed table (faster)

				if (hasVar) {
					v = (vara ^ varb);
					h = lut.f[v>>16] + lut.f[v&65535];
					// Add to correlation
					fTotalCorrelation += fVarCorrTable[h];	// use pre-computed table (faster)
				}
			}
		} // nCentroid
		fTotalCorrelation /= pImage->getNorm() * fQueryNorm;
		fTotalVarCorrelation /= pImage->getNorm() * fQueryNorm;

		if (hasVar)
			fCorrelation = (fTotalCorrelation + fTotalVarCorrelation) * 0.5f;
		else
			fCorrelation = fTotalCorrelation;

		vDatabaseScoresIndices[nImage++].first = 2 - 2*fCorrelation;
	} // nImage

	delete[] bitsOfQuery;
	// Sort scores  was: sort(vDatabaseScoresIndices.begin(), vDatabaseScoresIndices.end(), cmpDoubleUintAscend);
	// Sort scores and produce the final ranking of numRankedOuput images (without sorting all images)
	size_t numOut = min(numRankedOuput, nNumDatabaseImages);
	partial_sort(vDatabaseScoresIndices.begin(), vDatabaseScoresIndices.begin() + numOut,vDatabaseScoresIndices.end(), cmpDoubleUintAscend);
	vDatabaseScoresIndices.resize(numOut);

	delete[] fCorrTable;
	delete[] fVarCorrTable;
	delete[] nodes;
	delete []flag;
	delete []weight;
}


void SCFVIndex::queryMBIT_sort(const SCFVSignature& querySignature, vector< pair<double,unsigned int> >& vDatabaseScoresIndices, size_t numRankedOuput, size_t MBIT_Threshold, int &num_candidate) const
{
	
	float *fCorrTable = new float[PCASiftLength + 1];
	float *fVarCorrTable = new float[PCASiftLength + 1];

	for (int h=0; h <= PCASiftLength; ++h)	
	{
		fCorrTable[h] = ((PCASiftLength - 2*h) * pow(CorrelationWeights[h],beta) );
		fVarCorrTable[h] = ((PCASiftLength - 2*h) * pow(VarCorrelationWeights[h],beta));
	}
	float weight_base = m_signatures.size() * M;

	float *W2_log = new float[numberCentroids];
	float *W2_log_var = new float[numberCentroids];

	generateWeight(querySignature , W2_log , W2_log_var , weight_base);

	float fQueryNorm = querySignature.getNorm();	// get query norm

	// Compare against database signatures: 1st round
	size_t nNumDatabaseImages = numberImages();
	vDatabaseScoresIndices.resize(nNumDatabaseImages);
	unsigned int a, b, vara, varb, v, h;
	int nImage = 0;

	//MBIT voting stage
	unsigned char * flag;
	unsigned char * weight;
	flag = new unsigned char[nNumDatabaseImages];
	weight = new unsigned char[nNumDatabaseImages];
	memset( flag , 0 , sizeof(unsigned char) * nNumDatabaseImages);
	memset( weight , 0 , sizeof(unsigned char) * nNumDatabaseImages);
	
	num_candidate = 0;

	struct Node *nodes = new Node[nNumDatabaseImages];

	for (int nCentroid = 0; nCentroid < numberCentroids; nCentroid++) 
	{
		unsigned int qBlock = querySignature.m_vWordBlock[nCentroid];
		
		if(qBlock == 0)
			continue;

		unsigned int leftPart = (qBlock >> (PCASiftLength - mbit_m));
		unsigned int rightPart = (qBlock & ((1<<mbit_m)-1));
		
		int listSize;
		int blockSize = bitsDiff.size();

		for(int i = 0 ; i < blockSize ; i ++)
		{
			unsigned int id = ( leftPart ^ bitsDiff[i] );

			//listSize = vecMBIT[ 2 * nCentroid ][ id ].size();
			listSize = SizeOfMBIT[ 2 * nCentroid ][ id ];
			//listSize = hashTable[ 2 * nCentroid ][ id ].size();
			for(int j = 0 ; j < listSize ; j ++)
			{
				//int imageId =  vecMBIT[ 2 * nCentroid ][ id ][ j ] ;
				int imageId =  MBIT[ 2 * nCentroid ][ id ][ j ] ;
				//int imageId =  hashTable[ 2 * nCentroid ][ id ][ j ] ;
				weight[ imageId ] = (unsigned char)min( 255 ,  ( (int)weight[ imageId ] + numDiff[i] ) );
			}
		}
		
		for(int i = 0 ; i < blockSize ; i ++)
		{
			unsigned int id = ( rightPart ^ bitsDiff[i] );

			//listSize = vecMBIT[ 2 * nCentroid + 1 ][ id ].size();
			listSize = SizeOfMBIT[ 2 * nCentroid + 1 ][ id ];
			//listSize = hashTable[ 2 * nCentroid + 1 ][ id ].size();
			for(int j = 0 ; j < listSize ; j ++)
			{
				//int imageId = hashTable[ 2 * nCentroid + 1 ][ id ][ j ];
				int imageId = MBIT[ 2 * nCentroid + 1 ][ id ][ j ];
				//int imageId = vecMBIT[ 2 * nCentroid + 1 ][ id ][ j ];
				weight[ imageId ] = (unsigned char)min( 255 ,  ( (int)weight[ imageId ] + numDiff[i] ) );
			}
		}
	}

	for(int i = 0 ; i < nNumDatabaseImages ; i ++)
	{
		nodes[i].ID = i;
		nodes[i].val = weight[i];
	}

	sort(nodes , nodes + nNumDatabaseImages, cmp);

	int re_calc = (int)(candidate_size * nNumDatabaseImages);

	for(int i = 0 ; i < re_calc ; i ++)
		flag[ nodes[i].ID ] = 1;


	for(std::vector<SCFVSignature>::const_iterator pImage=m_signatures.begin(); pImage < m_signatures.end(); ++pImage) {
		vDatabaseScoresIndices[nImage].second = nImage;
		if (pImage->getVisited() <= 5 || flag[nImage] == 0) {
			vDatabaseScoresIndices[nImage++].first = 2.5;
			continue;
		}

		num_candidate ++;

		// Compute correlation for first few centroids
		float fCorrelation = 0;
		float fTotalCorrelation = 0;
		float fTotalVarCorrelation = 0;
		bool hasVar = querySignature.hasVar() && pImage->hasVar();	// both signatures must have the variance information

		int num_overlap = 0;

		for (int nCentroid = 0; nCentroid < numberCentroids; nCentroid++) {
			a = querySignature.m_vWordBlock[nCentroid];
			b = pImage->m_vWordBlock[nCentroid];
			if (hasVar) {
				vara = querySignature.m_vWordVarBlock[nCentroid];
				varb = pImage->m_vWordVarBlock[nCentroid];
			}
			if(a && b){
				// Compute Hamming distance
				
				v = ((a ^ b));
				h = lut.f[v>>16] + lut.f[v&65535];

				fTotalCorrelation += fCorrTable[h] * W2_log[nCentroid];
				if (hasVar) {
					v = (vara ^ varb);
					h = lut.f[v>>16] + lut.f[v&65535];
					// Add to correlation
					fTotalCorrelation += fVarCorrTable[h] * W2_log_var[nCentroid];
				}

				num_overlap++;
			}
		} // nCentroid

		fTotalCorrelation /= pImage->getNorm() * fQueryNorm;
		fTotalVarCorrelation /= pImage->getNorm() * fQueryNorm;

		if (hasVar)
			fCorrelation = (fTotalCorrelation + fTotalVarCorrelation) * 0.5f;
		else
			fCorrelation = fTotalCorrelation;

		vDatabaseScoresIndices[nImage++].first = 2 - 2*fCorrelation;
	} // nImage

	// Sort scores  was: sort(vDatabaseScoresIndices.begin(), vDatabaseScoresIndices.end(), cmpDoubleUintAscend);
	// Sort scores and produce the final ranking of numRankedOuput images (without sorting all images)
	size_t numOut = min(numRankedOuput, nNumDatabaseImages);
	partial_sort(vDatabaseScoresIndices.begin(), vDatabaseScoresIndices.begin() + numOut,vDatabaseScoresIndices.end(), cmpDoubleUintAscend);
	vDatabaseScoresIndices.resize(numOut);

	delete[] W2_log;
	delete[] W2_log_var;
	delete[] fCorrTable;
	delete[] fVarCorrTable;
	delete[] nodes;
	delete []flag;
	delete []weight;
}





void SCFVIndex::initMBIT(int modeID)
{
	if(modeID == 1)
	{
		mbit_m = 8 , mbit_r = 1;
		candidate_size = 1.0  / mbit_speedup;
	}
	else if(modeID == 2)
	{
		mbit_m = 16 , mbit_r = 3;
		candidate_size = 1.0  / mbit_speedup;
	}
	else
	{
		mbit_m = 16 , mbit_r = 3;
		candidate_size = 1.0  / mbit_speedup;
	}

	vec = new vector<int>[1<<mbit_m];

	MBIT = new int**[1<<10];
	for(int i = 0 ; i < (1<<10) ; i ++)
		MBIT[i] = new int*[1<<mbit_m];

	SizeOfMBIT = new int*[1<<10];
	for(int i = 0 ; i < (1<<10) ; i ++)
		SizeOfMBIT[i] = new int[1<<mbit_m];

	int nNumDatabaseImages = m_signatures.size();
	for(int nImage = 0 ; nImage < nNumDatabaseImages ; nImage ++)
	{
		for (int nCentroid = 0; nCentroid < numberCentroids; nCentroid++) 
		{
			unsigned int rBlock = m_signatures[nImage].m_vWordBlock[nCentroid];
			if (rBlock != 0) 
			{
				unsigned int leftPart = (rBlock >> (PCASiftLength - mbit_m));
				unsigned int rightPart = (rBlock & ((1<<mbit_m)-1));
			}
		}
	}
	
	for (int nCentroid = 0; nCentroid < numberCentroids; nCentroid++)
	{
		for(int i = 0 ; i < (1<<mbit_m) ; i ++)
			vector<int>().swap(vec[i]);

		for(int nImage = 0 ; nImage < nNumDatabaseImages ; nImage ++)
		{
			unsigned int rBlock = m_signatures[nImage].m_vWordBlock[nCentroid];

			if (rBlock != 0) 
			{
				unsigned int leftPart = (rBlock >> (PCASiftLength - mbit_m));
				vec[leftPart].push_back(nImage);
			}
		}

		for(int i = 0 ; i < (1<<mbit_m) ; i ++)
		{
			SizeOfMBIT[nCentroid * 2][i] = vec[i].size();
			MBIT[nCentroid * 2][i] = new int[SizeOfMBIT[nCentroid * 2][i]];
			for(int j = 0 ; j < SizeOfMBIT[nCentroid * 2][i] ; j ++)
				MBIT[nCentroid * 2][i][j] = vec[i][j];
		}

		for(int i = 0 ; i < (1<<mbit_m) ; i ++)
			vector<int>().swap(vec[i]);

		for(int nImage = 0 ; nImage < nNumDatabaseImages ; nImage ++)
		{
			unsigned int rBlock = m_signatures[nImage].m_vWordBlock[nCentroid];

			if (rBlock != 0) 
			{
				unsigned int rightPart = (rBlock & ((1<<mbit_m)-1));
				vec[rightPart].push_back(nImage);
			}
		}

		for(int i = 0 ; i < (1<<mbit_m) ; i ++)
		{
			SizeOfMBIT[nCentroid * 2 + 1][i] = vec[i].size();
			MBIT[nCentroid * 2 + 1][i] = new int[SizeOfMBIT[nCentroid * 2 + 1][i]];
			for(int j = 0 ; j < SizeOfMBIT[nCentroid * 2 + 1][i] ; j ++)
				MBIT[nCentroid * 2 + 1][i][j] = vec[i][j];
		}
	}

	for(int i = 0 ; i < (1<<mbit_m) ; i ++)
		vector<int>().swap(vec[i]);
	

	//bitsDiff.push_back(0);
	//numDiff.push_back(1);
	
	for(int i = 0 ; i < ((1<<mbit_m)-1) ; i ++)
	{
		if(lut.f[i] <= mbit_r)
		{
			 bitsDiff.push_back(i);
			 numDiff.push_back(1);
		}
	}
}



void SCFVIndex::initMBIT_bit_selection(int modeID)
{
	if(modeID == 1)
	{
		mbit_m = 8 , mbit_r = 1;
		candidate_size = 1.0  / mbit_speedup;
	}
	else if(modeID == 2)
	{
		mbit_m = 16 , mbit_r = 3;
		candidate_size = 1.0  / mbit_speedup;
	}
	else
	{
		mbit_m = 16 , mbit_r = 3;
		candidate_size = 1.0  / mbit_speedup;
	}

	vec = new vector<int>[1<<mbit_m];

	MBIT = new int**[1<<10];
	for(int i = 0 ; i < (1<<10) ; i ++)
		MBIT[i] = new int*[1<<mbit_m];

	SizeOfMBIT = new int*[1<<10];
	for(int i = 0 ; i < (1<<10) ; i ++)
		SizeOfMBIT[i] = new int[1<<mbit_m];

	int nNumDatabaseImages = m_signatures.size();
	for(int nImage = 0 ; nImage < nNumDatabaseImages ; nImage ++)
	{
		for (int nCentroid = 0; nCentroid < numberCentroids; nCentroid++) 
		{
			unsigned int rBlock = m_signatures[nImage].m_vWordBlock[nCentroid];
			if (rBlock != 0) 
			{
				unsigned int c_rBlock = originalTocompress(rBlock , nCentroid);

				unsigned int leftPart = (c_rBlock >> (num_bit_selection - mbit_m));
				unsigned int rightPart = (c_rBlock & ((1<<mbit_m)-1));
			}
		}
	}
	
	for (int nCentroid = 0; nCentroid < numberCentroids; nCentroid++)
	{
		for(int i = 0 ; i < (1<<mbit_m) ; i ++)
			vector<int>().swap(vec[i]);

		//unsigned int c_rBlock = originalTocompress(m_signatures[nImage].m_vWordBlock[nCentroid] , nCentroid);

		for(int nImage = 0 ; nImage < nNumDatabaseImages ; nImage ++)
		{
			unsigned int rBlock = m_signatures[nImage].m_vWordBlock[nCentroid];

			if (rBlock != 0) 
			{
				unsigned int c_rBlock = originalTocompress(rBlock , nCentroid);

				unsigned int leftPart = (c_rBlock >> (num_bit_selection - mbit_m));
				vec[leftPart].push_back(nImage);
			}
		}

		for(int i = 0 ; i < (1<<mbit_m) ; i ++)
		{
			SizeOfMBIT[nCentroid * 2][i] = vec[i].size();
			MBIT[nCentroid * 2][i] = new int[SizeOfMBIT[nCentroid * 2][i]];
			for(int j = 0 ; j < SizeOfMBIT[nCentroid * 2][i] ; j ++)
				MBIT[nCentroid * 2][i][j] = vec[i][j];
		}

		for(int i = 0 ; i < (1<<mbit_m) ; i ++)
			vector<int>().swap(vec[i]);

		for(int nImage = 0 ; nImage < nNumDatabaseImages ; nImage ++)
		{
			unsigned int rBlock = m_signatures[nImage].m_vWordBlock[nCentroid];

			if (rBlock != 0) 
			{
				unsigned int c_rBlock = originalTocompress(rBlock , nCentroid);

				unsigned int rightPart = (c_rBlock & ((1<<mbit_m)-1));
				vec[rightPart].push_back(nImage);
			}
		}

		for(int i = 0 ; i < (1<<mbit_m) ; i ++)
		{
			SizeOfMBIT[nCentroid * 2 + 1][i] = vec[i].size();
			MBIT[nCentroid * 2 + 1][i] = new int[SizeOfMBIT[nCentroid * 2 + 1][i]];
			for(int j = 0 ; j < SizeOfMBIT[nCentroid * 2 + 1][i] ; j ++)
				MBIT[nCentroid * 2 + 1][i][j] = vec[i][j];
		}
	}

	for(int i = 0 ; i < (1<<mbit_m) ; i ++)
		vector<int>().swap(vec[i]);
	

	//bitsDiff.push_back(0);
	//numDiff.push_back(1);
	
	for(int i = 0 ; i < ((1<<mbit_m)-1) ; i ++)
	{
		if(lut.f[i] <= mbit_r)
		{
			 bitsDiff.push_back(i);
			 numDiff.push_back(1);
		}
	}
}
#endif

