/*
 * This software module was originally developed by:
 *
 *   Stanford University, Huawei Technologies Co., Ltd.
 *
 * in the course of development of ISO/IEC <number> (Compact Descriptors for Visual 
 * Search) standard for reference purposes and its performance may not have been 
 * optimized. This software module includes implementation of one or more tools as 
 * specified by the ISO/IEC <number> standard.
 *
 * ISO/IEC gives you a royalty-free, worldwide, non-exclusive, copyright license to copy, 
 * distribute, and make derivative works of this software module or modifications thereof 
 * for use in implementations of the ISO/IEC <number> standard in products that satisfy 
 * conformance criteria (if any).
 *
 * Those intending to use this software module in products are advised that its use may 
 * infringe existing patents. ISO/IEC have no liability for use of this software module 
 * or modifications thereof.
 *
 * Copyright is not released for products that do not conform to audiovisual and image-
 * coding related ITU Recommendations and/or ISO/IEC International Standards.
 *
 * Stanford University and Huawei Technologies Co., Ltd. retain full rights to modify and use the code for their own 
 * purposes, assign or donate the code to a third party and to inhibit third parties 
 * from using the code for products that do not conform to MPEG-related 
 * ITU Recommendations and/or ISO/IEC International Standards.
 *
 * This copyright notice must be included in all copies or derivative works.
 * Copyright (c) ISO/IEC 2012.
 *
 */

#include "CsscCoordinateCoding.h"
#include "ArithmeticCoding.h"
#include "CdvsException.h"
#include "assert.h"
#include <stdlib.h>
#include <cmath>
#include <algorithm>

#define ADAPT 0

using namespace mpeg7cdvs;

//Tables for the creation of the contexts used for the histogram map encoding
//Each row correspondes to one block size (up to a maximum of 12). 
const int CsscCoordinateCoding::CONTEXT_TABLE[12][6] = {{65460,   -26,    65525,   -23,   -27,    -6},		//block size = 1
		{65249,   -105,     65488,   -69,   -89,     1},								//block size = 2
		{64901,   -239,     65439,   -99,   -170,   -18},								//block size = 3
		{64408,   -417,     65393,   -107,  -276,   -28},								//block size = 4
		{63795,   -653,     65361,   -100,  -374,   -32},								//block size = 5
		{63064,   -942,     65339,   -87,   -430,   -48},								//block size = 6
		{62133,   -1240,    65316,   -73,   -474,   -45},								//block size = 7
		{61165,   -1598,    65313,   -62,   -526,   -49},								//block size = 8
		{60086,   -1963,    65305,   -47,   -548,   -57},								//block size = 9
		{58872,   -2313,    65310,   -47,   -603,   -58},								//block size = 10
		{57560,   -2674,    65309,   -35,   -631,   -53},								//block size = 11
		{56284,   -3023,    65316,   -32,   -622,   -69}};								//block size = 12

//Tables for the creation of the contexts used for the histogram count encoding
//Each row correspondes to one block size (up to a maximum of 12).
const int CsscCoordinateCoding::HIST_COUNT_TABLE[12][3] =
	{{55215,    65029,    65447},								//block size = 1
	{54857,    64959,    65411},								//block size = 2
	{54600,    64897,    65382},								//block size = 3
	{54361,    64839,    65366},								//block size = 4	
	{54115,    64777,    65348},								//block size = 5
	{53832,    64693,    65326},								//block size = 6
	{53468,    64571,    65301},								//block size = 7
	{52968,    64397,    65260},								//block size = 8
	{52332,    64160,    65209},								//block size = 9
	{51592,    63843,    65141},								//block size = 10
	{50717,    63448,    65048},								//block size = 11
	{49691,    62982,    64919}};								//block size = 12


CsscCoordinateCoding::CsscCoordinateCoding(const Parameters & params)
{
	m_blockWidth = params.blockWidth; 
	m_queryLength = params.descLength;
	m_debugLevel = params.debugLevel;
	m_hCount = NULL;
	m_hCountSize = 0;
	m_hMap = NULL;
	m_hMapSizeX = 0;
	m_hMapSizeY = 0;
	m_bIsTransposed = false;
	m_bTrainingMode = false;

	//Creation of the histogram count context
	this->m_csc.vCount[0] =HIST_COUNT_TABLE[params.blockWidth-1][0];
	this->m_csc.vCount[1] =HIST_COUNT_TABLE[params.blockWidth-1][1];
	this->m_csc.vCount[2] =HIST_COUNT_TABLE[params.blockWidth-1][2];
	for (int i=3; i < 64; i++)
	{
		this->m_csc.vCount[i] = 65535- (64 - i);
	}

	//Computation of the histogram map context
	int valueA = CONTEXT_TABLE[params.blockWidth-1][0];
	int ind = params.ctxTableIdx;
	int value  = CONTEXT_TABLE[params.blockWidth-1][1];
	valueA = valueA + CONTEXT_TABLE[params.blockWidth-1][1]*ind;
	int valueB = CONTEXT_TABLE[params.blockWidth-1][2];
	valueB = valueB + CONTEXT_TABLE[params.blockWidth-1][3]*ind;
	int valueC = CONTEXT_TABLE[params.blockWidth-1][4];
	valueC = valueC + CONTEXT_TABLE[params.blockWidth-1][5]*ind;

	m_csc.vInitialMap[0] = valueA;
	m_csc.vInitialMap[1] = 65535;

	for (int i=0; i <= MAXIMUM_SUM_CONTEXT; i++)
	{
		this->m_csc.vMap[i][0] = valueB+valueC*(i);
		this->m_csc.vMap[i][1] = 65535;
	}
}


CsscCoordinateCoding::~CsscCoordinateCoding()
{
	free(m_hMap); 		
	free(m_hCount); 	
}

inline void GenerateIntegralImage(int* pIntegral, int* pImage, int w, int h)
{
	// note that pImage and pIntegral are stored by columns!

	for (int x=0; x<w; ++x)
		for (int y=0; y<h; ++y)
			pIntegral[x*h + y] = pImage[x*h + y] != 0 ? 1 : 0;		// initialize

	for (int x=0; x<w; ++x)
		for (int y=1; y<h; ++y)
			pIntegral[x*h + y] += pIntegral[x*h + y - 1];			// sum columns

	for (int y=0; y<h; ++y)
		for (int x=1; x<w; ++x)
			pIntegral[x*h + y] += pIntegral[(x-1)*h + y];			// sum rows

}

inline int GenerateSumIntegral(
	int iX, int iY,
	int iMinX, int iMaxX, int iMinY, int iMaxY,
	int* pIntegralImage, int iHeight,
	int iContextRange, int iMaximumSum)
{
	// Generate sum context
	int iNbMinX = std::max( iX-iContextRange, iMinX + 1 );
	int iNbMaxX = std::min( iX+iContextRange, iMaxX - 1 );
	int iNbMinY = std::max( iY-iContextRange, iMinY + 1 );
	int iNbMaxY = std::min( iY+iContextRange, iMaxY - 1 );
	int iNbArea = (iNbMaxY - iNbMinY + 1) * (iNbMaxX - iNbMinX + 1 );

	int iSum = pIntegralImage[iNbMaxY + iNbMaxX*iHeight] + pIntegralImage[iNbMinY-1 + (iNbMinX-1)*iHeight]
	         - pIntegralImage[iNbMinY-1 + iNbMaxX*iHeight] - pIntegralImage[iNbMaxY + (iNbMinX-1)*iHeight];

	if( iNbArea != iMaximumSum)
	{
		float fSum = ((float)iSum * iMaximumSum) / ((float)iNbArea);
		iSum = (int) std::min( iMaximumSum, (int)floor(fSum));
	}

	return iSum;
}

inline int GenerateSumContext( 
	int iX, int iY,
	int iMinX, int iMaxX, int iMinY, int iMaxY, 
	int* pImageArray, int iHeight, 
	int iContextRange, int iMaximumSum)
{
	// Generate sum context
	int iNbMinX = std::max( iX-iContextRange, iMinX + 1 );
	int iNbMaxX = std::min( iX+iContextRange, iMaxX - 1 );
	int iNbMinY = std::max( iY-iContextRange, iMinY + 1 );
	int iNbMaxY = std::min( iY+iContextRange, iMaxY - 1 );
	int iNbArea = (iNbMaxY - iNbMinY + 1) * (iNbMaxX - iNbMinX + 1 );

	int iSum = 0;
	for( int iNbX=iNbMinX; iNbX<=iNbMaxX; iNbX++)
		for( int iNbY=iNbMinY; iNbY<=iNbMaxY; iNbY++)
			iSum += (pImageArray[ iNbX*iHeight + iNbY]!=0);

	if( iNbArea != iMaximumSum)
	{
		float fSum = ((float)iSum * iMaximumSum) / ((float)iNbArea);
		iSum = (int) std::min( iMaximumSum, (int)floor(fSum));
	}

	return iSum;
}

void CsscCoordinateCoding::exportVars(unsigned int & histogramCountSize, unsigned int & histogramMapSizeX, unsigned int & histogramMapSizeY) const
{
	if( m_bIsTransposed)
	{
		histogramMapSizeX = m_hMapSizeY;
		histogramMapSizeY = m_hMapSizeX;
	} else {
		histogramMapSizeX = m_hMapSizeX;
		histogramMapSizeY = m_hMapSizeY;
	}

	// Write the histogram count size
	histogramCountSize =  m_hCountSize;

}

void CsscCoordinateCoding::toBinary(BitOutputStream & writer)
{
	if (m_debugLevel > 0)
		printf("\n---------- CsscCoordinateCoding writing output stream at %d --------\n", (int)writer.produced());

	// Write the histogram count size
	writer.write(m_hCountSize, 16);
	
	// Write the two vectors to define rank-1 support
	if( m_bIsTransposed)
	{
		writer.write(m_hMapSizeY, 16);			// write the size of mapY (represented in 16 bits)
		writer.write(m_hMapSizeX, 16);			// write the size of mapX (represented in 16 bits)
	} else {
		writer.write(m_hMapSizeX, 16);			// write the size of mapX (represented in 16 bits)
		writer.write(m_hMapSizeY, 16);			// write the size of mapY (represented in 16 bits)
	}

	if (m_debugLevel > 0)
	{
		printf("histogram size: (%d, %d)\n", m_hMapSizeX, m_hMapSizeY);
		printf("histogram count size: %d\n", m_hCountSize);
	}

	// Encode the histogram count 
	AC_encoder aceHistCount;
	AC_model acmHistCount;
	acmHistCount.init(SUM_HIST_COUNT_SIZE,m_csc.vCount);
	aceHistCount.init(writer);
	for (int i=0; i< m_hCountSize; i++)
		aceHistCount.encode_symbol(acmHistCount, m_hCount[i]-1);	// write the symbols
	aceHistCount.done();

	if (m_debugLevel > 0)
		printf("Bits for histogram count: %d\n", (int)writer.produced());

	// Setup entropy coding models
	AC_model acmMap[2];
	for( int i=0; i<2; i++)
	{
		acmMap[i].init(2, m_csc.vInitialMap);
	}
	AC_model acmSum[MAXIMUM_SUM_CONTEXT+1];
	for( int i=0; i<MAXIMUM_SUM_CONTEXT+1; i++)
	{
		acmSum[i].init(2, m_csc.vMap[i]);
	}

	// Setup AC encoder
	AC_encoder 	aceMap;
	aceMap.init(writer);

	// Start the circular scan
	int 	iWidth = m_hMapSizeX;
	int		iHeight = m_hMapSizeY;
	int* 	pImageArray = m_hMap;

	bool 	isOdd = iHeight%2;
	int 	iTotalSteps = (iHeight - isOdd) / 2;
	int 	iSymbol;
	int 	iEncodedCount = 0;

	// Circular scan box
	int		iMinX, iMaxX, iMinY, iMaxY;
	iMinX = iTotalSteps-1; 
	iMaxX = iWidth-1 - (iTotalSteps - 1);
	iMinY = iTotalSteps-1;
	iMaxY = iHeight-1 - (iTotalSteps - 1);

	// use integral image
	int *   pIntegralImage = new int[m_hMapSizeY * m_hMapSizeX];

	if( iEncodedCount == m_hCountSize) 
		goto label_histogram_map_encode_complete;

	if( isOdd )
	{
		// encode the center part using plain probability
		int iY = iMinY+1;
		for( int iX=iMinX+1; iX<iMaxX; iX++)
		{
			aceMap.encode_symbol( acmMap[0], (iSymbol=pImageArray[iX*iHeight+iY]));
			if( iSymbol != 0 )	if( ++iEncodedCount == m_hCountSize ) goto label_histogram_map_encode_complete;
		}
	}

	int iSteps, iContext, iY, iX;

	// encode the center part using plain probability
	for( iSteps=0; iSteps<iTotalSteps; iSteps++)
	{
		// Left side of the rectangle
		for( iX=iMinX, iY=iMinY; iY<=iMaxY; iY++)
		{
			aceMap.encode_symbol( acmMap[0], (iSymbol = pImageArray[iX*iHeight+iY]));
			if( iSymbol != 0 ) if( ++iEncodedCount == m_hCountSize ) goto label_histogram_map_encode_complete;
		}
		// Bottom side of the rectangle
		for( iX=iMinX+1, iY=iMaxY; iX<iMaxX; iX++)
		{
			aceMap.encode_symbol( acmMap[0], (iSymbol = pImageArray[iX*iHeight+iY]));
			if( iSymbol != 0 ) if( ++iEncodedCount == m_hCountSize ) goto label_histogram_map_encode_complete;
		}
		// Right side of the rectangle
		for( iX=iMaxX, iY=iMaxY; iY>=iMinY; iY--)
		{
			aceMap.encode_symbol( acmMap[0], (iSymbol = pImageArray[iX*iHeight+iY]));
			if( iSymbol != 0 ) if( ++iEncodedCount == m_hCountSize ) goto label_histogram_map_encode_complete;
		}
		// Upper side of the rectangle
		for( iX=iMaxX-1, iY=iMinY; iX>iMinX; iX--)
		{
			aceMap.encode_symbol( acmMap[0], (iSymbol = pImageArray[iX*iHeight+iY]));
			if( iSymbol != 0 ) if( ++iEncodedCount == m_hCountSize ) goto label_histogram_map_encode_complete;
		}
		iMinX--; iMinY--; iMaxX++; iMaxY++;
		if( iMaxY - iMinY + 1 >= CONTEXT_RANGE ) break;
	}

	// Code the remaining part of the histogram map using sum-context
	GenerateIntegralImage(pIntegralImage, pImageArray, m_hMapSizeX, m_hMapSizeY);
	
	for( ; iSteps<iTotalSteps; iSteps++)
	{
		// Left side of the rectangle
		for( iX=iMinX, iY=iMinY; iY<=iMaxY; iY++)
		{
			iContext = GenerateSumIntegral( iX, iY, iMinX, iMaxX, iMinY, iMaxY, pIntegralImage, iHeight, CONTEXT_RANGE, MAXIMUM_SUM_CONTEXT);
			aceMap.encode_symbol( acmSum[iContext], (iSymbol = pImageArray[iX*iHeight+iY]));
			if( iSymbol != 0 ) if( ++iEncodedCount == m_hCountSize ) goto label_histogram_map_encode_complete;
		}
		// Bottom side of the rectangle
		for( iX=iMinX+1, iY=iMaxY; iX<iMaxX; iX++)
		{
			iContext = GenerateSumIntegral( iX, iY, iMinX, iMaxX, iMinY, iMaxY, pIntegralImage, iHeight, CONTEXT_RANGE, MAXIMUM_SUM_CONTEXT);
			aceMap.encode_symbol( acmSum[iContext], (iSymbol = pImageArray[iX*iHeight+iY]));
			if( iSymbol != 0 ) if( ++iEncodedCount == m_hCountSize ) goto label_histogram_map_encode_complete;
		}
		// Right side of the rectangle
		for( iX=iMaxX, iY=iMaxY; iY>=iMinY; iY--)
		{
			iContext = GenerateSumIntegral( iX, iY, iMinX, iMaxX, iMinY, iMaxY, pIntegralImage, iHeight, CONTEXT_RANGE, MAXIMUM_SUM_CONTEXT);
			aceMap.encode_symbol( acmSum[iContext], (iSymbol = pImageArray[iX*iHeight+iY]));
			if( iSymbol != 0 ) if( ++iEncodedCount == m_hCountSize ) goto label_histogram_map_encode_complete;
		}
		// Upper side of the rectangle
		for( iX=iMaxX-1, iY=iMinY; iX>iMinX; iX--)
		{
			iContext = GenerateSumIntegral( iX, iY, iMinX, iMaxX, iMinY, iMaxY, pIntegralImage, iHeight, CONTEXT_RANGE, MAXIMUM_SUM_CONTEXT);
			aceMap.encode_symbol( acmSum[iContext], (iSymbol = pImageArray[iX*iHeight+iY]));
			if( iSymbol != 0 ) if( ++iEncodedCount == m_hCountSize ) goto label_histogram_map_encode_complete;
		}
		iMinX--; iMinY--; iMaxX++; iMaxY++;
	}


label_histogram_map_encode_complete:
	aceMap.done();
	
	if (m_debugLevel > 0)
		printf("Bits for all CsscCoordinateCoding data: %d\n", (int)writer.produced());

	delete [] pIntegralImage;
}


void CsscCoordinateCoding::fromBinary(BitInputStream & reader)
{
	if (m_debugLevel > 0)
		printf("\n---------- CsscCoordinateCoding reading input stream at %d --------\n", (int)reader.consumed());
	
	// Read the number of elements in the histogram count
	m_hCountSize= reader.read(16);

	// Read the two vectors to define rank-1 support
	m_hMapSizeX = reader.read(16);			// read the size of mapX (represented in 8 bits)
	m_hMapSizeY = reader.read(16);			// read the size of mapY (represented in 8 bits)

	m_bIsTransposed = false;
	if(m_hMapSizeY>m_hMapSizeX)
	{
		 m_bIsTransposed = true;
		 int t = m_hMapSizeY;
		 m_hMapSizeY = m_hMapSizeX;
		 m_hMapSizeX = t;
	}

	if (m_debugLevel > 0)
	{
		printf("histogram size: (%d, %d)\n", m_hMapSizeX, m_hMapSizeY);
		printf("histogram count size: %d\n", m_hCountSize);
		printf("is transposed %d\n", m_bIsTransposed);
	}

	// Decoding of the histogram count
	AC_decoder acdHistCount;
	AC_model acmHistCount;
	acmHistCount.init(SUM_HIST_COUNT_SIZE, m_csc.vCount);

	acdHistCount.init(reader);
	m_hCount = (int *) calloc(m_hCountSize, sizeof(int));
	for (int i=0; i< m_hCountSize; i++)
		m_hCount[i] = acdHistCount.decode_symbol(acmHistCount)+1;
	acdHistCount.done();

	if (m_debugLevel > 0)
		printf("Bits for histogram count: %d\n", (int)reader.consumed());

	// Setup memory space for the histogram map
	m_hMap = (int *) calloc(m_hMapSizeY * m_hMapSizeX, sizeof(int)); 
	for( int i=0; i<m_hMapSizeY * m_hMapSizeX; i++)
		m_hMap[i] = 0;

	// Setup entropy coding models
	AC_model acmMap[2];
	for( int i=0; i<2; i++)
	{
		acmMap[i].init( 2, m_csc.vInitialMap);
	}
	AC_model acmSum[MAXIMUM_SUM_CONTEXT+1];
	for( int i=0; i<MAXIMUM_SUM_CONTEXT+1; i++)
	{
		acmSum[i].init(2, m_csc.vMap[i]);
	}

	// Setup AC encoder
	AC_decoder 	acdMap;
	acdMap.init(reader);

	// Start the circular scan
	int 	iWidth = m_hMapSizeX;
	int		iHeight = m_hMapSizeY;
	int* 	pImageArray = m_hMap;

	bool 	isOdd = iHeight%2;
	int 	iTotalSteps = (iHeight - isOdd) / 2;
	int		iSymbol;
	int 	iDecodedCount = 0;

	// Circular scan box
	int		iMinX, iMaxX, iMinY, iMaxY;
	iMinX = iTotalSteps-1; 
	iMaxX = iWidth-1 - (iTotalSteps - 1);
	iMinY = iTotalSteps-1;
	iMaxY = iHeight-1 - (iTotalSteps - 1);

	if( iDecodedCount == m_hCountSize) 
		goto label_histogram_map_decode_complete;

	if( isOdd )
	{
		int iY = iMinY+1;
		for( int iX=iMinX+1; iX<iMaxX; iX++)
		{
			pImageArray[iX*iHeight+iY] = iSymbol = acdMap.decode_symbol( acmMap[0]);
			if( iSymbol != 0 )	if( ++iDecodedCount == m_hCountSize ) goto label_histogram_map_decode_complete;
		}
	}

	int iY, iX, iSteps, iContext;

	for( iSteps=0; iSteps<iTotalSteps; iSteps++)
	{
		// Left side of the rectangle
		for( iX=iMinX, iY=iMinY; iY<=iMaxY; iY++)
		{
			pImageArray[iX*iHeight+iY] = iSymbol = acdMap.decode_symbol( acmMap[0]);
			if( iSymbol != 0 )	if( ++iDecodedCount == m_hCountSize ) goto label_histogram_map_decode_complete;
		}
		// Bottom side of the rectangle
		for( iX=iMinX+1, iY=iMaxY; iX<iMaxX; iX++)
		{
			pImageArray[iX*iHeight+iY] = iSymbol = acdMap.decode_symbol( acmMap[0]);
			if( iSymbol != 0 )	if( ++iDecodedCount == m_hCountSize ) goto label_histogram_map_decode_complete;
		}
		// Right side of the rectangle
		for( iX=iMaxX, iY=iMaxY; iY>=iMinY; iY--)
		{
			pImageArray[iX*iHeight+iY] = iSymbol = acdMap.decode_symbol( acmMap[0]);
			if( iSymbol != 0 )	if( ++iDecodedCount == m_hCountSize ) goto label_histogram_map_decode_complete;
		}
		// Upper side of the rectangle
		for( iX=iMaxX-1, iY=iMinY; iX>iMinX; iX--)
		{
			pImageArray[iX*iHeight+iY] = iSymbol = acdMap.decode_symbol( acmMap[0]);
			if( iSymbol != 0 )	if( ++iDecodedCount == m_hCountSize ) goto label_histogram_map_decode_complete;
		}
		iMinX--; iMinY--; iMaxX++; iMaxY++;
		if( iMaxY - iMinY + 1 >= CONTEXT_RANGE ) break;
	}

	for( ; iSteps<iTotalSteps; iSteps++)
	{
		// Left side of the rectangle
		for( iX=iMinX, iY=iMinY; iY<=iMaxY; iY++)
		{
			iContext = GenerateSumContext( iX, iY, iMinX, iMaxX, iMinY, iMaxY, pImageArray, iHeight, CONTEXT_RANGE, MAXIMUM_SUM_CONTEXT);
			pImageArray[iX*iHeight+iY] = iSymbol = acdMap.decode_symbol( acmSum[iContext]);
			if( iSymbol != 0 )	if( ++iDecodedCount == m_hCountSize ) goto label_histogram_map_decode_complete;
		}
		// Bottom side of the rectangle
		for( iX=iMinX+1, iY=iMaxY; iX<iMaxX; iX++)
		{
			iContext = GenerateSumContext( iX, iY, iMinX, iMaxX, iMinY, iMaxY, pImageArray, iHeight, CONTEXT_RANGE, MAXIMUM_SUM_CONTEXT);
			pImageArray[iX*iHeight+iY] = iSymbol = acdMap.decode_symbol( acmSum[iContext]);
			if( iSymbol != 0 )	if( ++iDecodedCount == m_hCountSize ) goto label_histogram_map_decode_complete;
		}
		// Right side of the rectangle
		for( iX=iMaxX, iY=iMaxY; iY>=iMinY; iY--)
		{
			iContext = GenerateSumContext( iX, iY, iMinX, iMaxX, iMinY, iMaxY, pImageArray, iHeight, CONTEXT_RANGE, MAXIMUM_SUM_CONTEXT);
			pImageArray[iX*iHeight+iY] = iSymbol = acdMap.decode_symbol( acmSum[iContext]);
			if( iSymbol != 0 )	if( ++iDecodedCount == m_hCountSize ) goto label_histogram_map_decode_complete;
		}
		// Upper side of the rectangle
		for( iX=iMaxX-1, iY=iMinY; iX>iMinX; iX--)
		{
			iContext = GenerateSumContext( iX, iY, iMinX, iMaxX, iMinY, iMaxY, pImageArray, iHeight, CONTEXT_RANGE, MAXIMUM_SUM_CONTEXT);
			pImageArray[iX*iHeight+iY] = iSymbol = acdMap.decode_symbol( acmSum[iContext]);
			if( iSymbol != 0 )	if( ++iDecodedCount == m_hCountSize ) goto label_histogram_map_decode_complete;
		}
		iMinX--; iMinY--; iMaxX++; iMaxY++;
	}


label_histogram_map_decode_complete:
	acdMap.done();
}


// compute the number of differences
int CsscCoordinateCoding::compare(const CsscCoordinateCoding & other)
{
	int diff = 0;	
	diff += (m_hMapSizeX - other.m_hMapSizeX);
	diff += (m_hMapSizeY - other.m_hMapSizeY);
	diff += (m_hCountSize - other.m_hCountSize);
	diff += compareArray(m_hCount, other.m_hCount, m_hCountSize);
	diff += compareArray(m_hMap, (int*)other.m_hMap, m_hMapSizeX, m_hMapSizeY, other.m_bIsTransposed); 
	
	return diff;
}

// compare two arrays, return num of differences
int CsscCoordinateCoding::compareArray(int * a, int * b, int iWidth, int iHeight, bool bTranspose)
{
	int diff = 0;
	for (int iX=0; iX<iWidth; iX++)
	{
		for( int iY=0; iY<iHeight; iY++)
		{
			if( bTranspose)
			{
				if( a[iY + iX*iHeight] != b[iX + iY*iWidth] )
					diff++;
			}
			else
			{
				if( a[iY + iX*iHeight] != b[iY + iX*iHeight] )
					diff++;
			}
		}
	}
	return diff;
}

// compare two arrays, return num of differences
int CsscCoordinateCoding::compareArray(int * a, int * b, int size)
{
	int diff = 0;
	for (int i=0; i<size; i++)
	{
		if (a[i] != b[i])
		{
			diff++;
		}
	}
	return diff;
}

// compare two arrays, return num of differences
int CsscCoordinateCoding::compareArray(bool * a, bool * b, int size)
{
	int diff = 0;
	for (int i=0; i<size; i++)
	{
		if (a[i] != b[i])
		{
			diff++;
		}
	}
	return diff;
}


void CsscCoordinateCoding::generateHistogramMap(FeatureList & featurelist, int numPoints)
{
	if (numPoints > featurelist.nFeatures())
		numPoints = featurelist.nFeatures();

	int xSize = featurelist.imageWidth;
	int ySize = featurelist.imageHeight;

	// Quantize the location x of the features; rEntries = ceil(x(:,1)/blksz);
	std::vector<int> 	xEntriesContainer(numPoints);
	int* 				xEntries = &xEntriesContainer[0];
	for (int i=0; i<numPoints; i++)
		xEntries[i] = (int) floor(featurelist.features[i].x / m_blockWidth);

	// Quantize the location y of the features; cEntries = ceil(x(:,2)/blksz);
	std::vector<int> 	yEntriesContainer(numPoints);
	int* 				yEntries = &yEntriesContainer[0];
	for (int i=0; i<numPoints; i++)
		yEntries[i] = (int) floor(featurelist.features[i].y / m_blockWidth);

	// Calculate the histogram block size; hMap = sparse(rEntries, cEntries, ones(size(rEntries)),    ceil(nr/blksz), ceil(nc/blksz) );
	m_hMapSizeX = (xSize + m_blockWidth - 1) / m_blockWidth;
	m_hMapSizeY = (ySize + m_blockWidth - 1) / m_blockWidth;

	// Allocate memory for the histogram map and find the borders and histogram count size
	m_hMap = (int *) calloc(m_hMapSizeX * m_hMapSizeY, sizeof(int));
	m_hCountSize = 0;

	//New ordering based on the matrix position
	for (int i=0; i<numPoints; i++)
	{
		int index = xEntries[i] * m_hMapSizeY + yEntries[i];
		assert((index >= 0) && (index <(m_hMapSizeX * m_hMapSizeY)));
		featurelist.features[i].spatialIndex = index ;

		// Update histogram and calculate non empty histogram count
		if( m_hMap[index] == 0 ) m_hCountSize ++;
		m_hMap[index] += 1;
	}

	// Copy feature counts and maps in the new data structure
	m_hCount = (int *) calloc(m_hCountSize, sizeof(int));	// the size is <= numPoints
	int n = 0;
	int iCount = 0;
	for ( int x=0; x<m_hMapSizeX; x++)
	{
		int* pMapColumn  = m_hMap + x*m_hMapSizeY;
		for ( int y=0; y<m_hMapSizeY; y++)
		{
			if( pMapColumn[y] > 0 )
			{
				m_hCount[n++] = pMapColumn[y];
				pMapColumn[y] = 1;
				iCount ++;
			}
		}
	}
	
	// If the map size height is > map size width, transpose the map
	m_bIsTransposed = false;
	if(m_hMapSizeY>m_hMapSizeX)
	{
		TransposeMatrix( m_hMap, m_hMapSizeY, m_hMapSizeX);
		m_bIsTransposed = true;
	}

	return;
}

void CsscCoordinateCoding::TransposeMatrix( int* &pMatrix, int &iRow, int &iCol)
{
	// Transpose matrix so that the 
	int *tempData = (int *) calloc( iRow * iCol, sizeof(int));
	for(int i=0; i< iCol; i++ )
		for(int j=0; j< iRow; j++)
			tempData[j*iCol+i] = pMatrix[i*iRow+j];
	free( pMatrix);
	pMatrix = tempData;

	int t = iRow;
	iRow = iCol;
	iCol = t;
} 


void CsscCoordinateCoding::generateFeatureList(FeatureList & descriptors)
{
	//Transpose if necessary
	if(m_bIsTransposed)
	{
		TransposeMatrix( m_hMap, m_hMapSizeY, m_hMapSizeX);
	}

	// reconstruct scaled height and width
	descriptors.imageWidth  = m_hMapSizeX * m_blockWidth;
	descriptors.imageHeight = m_hMapSizeY * m_blockWidth;

	// reconstruct points
	Feature descriptor;	
	int hcounter = 0;
	float bwcenter = m_blockWidth * 0.5;
	for (int i=0; i < m_hMapSizeX; i++)
	{
		for (int j=0; j < m_hMapSizeY; j++)
		{
			if(*(m_hMap+i*m_hMapSizeY+j)==1)
			{
				descriptor.x = i*m_blockWidth + bwcenter;
				descriptor.y = j*m_blockWidth + bwcenter;
				
				for (int k=0; k<m_hCount[hcounter]; k++)
				{
					descriptors.features.push_back(descriptor);
				}
				hcounter++;
			}
		}
	}

	return;
}


void CsscCoordinateCoding::StartTrainingMode()
{
	m_bTrainingMode = true; 

	// Clean up counts 
	for( int i=0; i<SUM_HIST_COUNT_SIZE; i++)
		m_csc.vCount[i] = 1;

	// Clean the histogram map counts
	m_csc.vInitialMap[0] = 1;
	m_csc.vInitialMap[1] = 1;
	for( int j=0; j<MAXIMUM_SUM_CONTEXT+1; j++)
	{
		m_csc.vMap[j][0] = 1;
		m_csc.vMap[j][1] = 1;
	}
}


int CsscCoordinateCoding::AddImageSample( FeatureList& featurelist)
{
	int numPoints = (int) featurelist.features.size();
	int xSize = featurelist.imageWidth;
	int ySize = featurelist.imageHeight;

	// Quantize the location x of the features; rEntries = ceil(x(:,1)/blksz);
	std::vector<int> 	xEntriesContainer(numPoints);
	int* 				xEntries = &xEntriesContainer[0];
	for (int i=0; i<numPoints; i++)
		xEntries[i] = (int) floor(featurelist.features[i].x / m_blockWidth);

	// Quantize the location y of the features; cEntries = ceil(x(:,2)/blksz);
	std::vector<int> 	yEntriesContainer(numPoints);
	int* 				yEntries = &yEntriesContainer[0];
	for (int i=0; i<numPoints; i++)
		yEntries[i] = (int) floor(featurelist.features[i].y / m_blockWidth);

	// Calculate the histogram block size; hMap = sparse(rEntries, cEntries, ones(size(rEntries)),    ceil(nr/blksz), ceil(nc/blksz) );
	m_hMapSizeX = (xSize + m_blockWidth - 1) / m_blockWidth;
	m_hMapSizeY = (ySize + m_blockWidth - 1) / m_blockWidth;

	// Allocate memory for the histogram map and find the borders and histogram count size
	if( m_hMap != NULL ) free(m_hMap);
	m_hMap = (int *) calloc(m_hMapSizeX * m_hMapSizeY, sizeof(int));
	m_hCountSize = 0;

	//New ordering based on the matrix position
	for (int i=0; i<numPoints; i++)
	{
		int index = xEntries[i] * m_hMapSizeY + yEntries[i];
		assert((index >= 0) && (index <(m_hMapSizeX * m_hMapSizeY)));
		featurelist.features[i].spatialIndex = index ;

		// Update histogram and calculate non empty histogram count
		if( m_hMap[index] == 0 ) m_hCountSize ++;
		m_hMap[index] += 1;
	}

	// Copy feature counts and maps in the new data structure
	if ( m_hCount != NULL ) free(m_hCount);
	m_hCount = (int *) calloc(m_hCountSize, sizeof(int));	// the size is <= numPoints
	int n = 0;
	for ( int x=0; x<m_hMapSizeX; x++)
	{
		int* pMapColumn  = m_hMap + x*m_hMapSizeY;
		for ( int y=0; y<m_hMapSizeY; y++)
		{
			if( pMapColumn[y] > 0 )
			{
				m_hCount[n++] = pMapColumn[y];
				pMapColumn[y] = 1;
			}
		}
	}
	
	// If the map size height is > map size width, transpose the map
	if(m_hMapSizeY>m_hMapSizeX)
		TransposeMatrix( m_hMap, m_hMapSizeY, m_hMapSizeX);

	// Gather histogram statistics	
	for (int i=0; i< m_hCountSize; i++)
		m_csc.vCount[m_hCount[i]-1]++;	// write the symbols

	// Start the circular scan
	int 	iWidth = m_hMapSizeX;
	int		iHeight = m_hMapSizeY;
	int* 	pImageArray = m_hMap;

	bool 	isOdd = iHeight%2;
	int 	iTotalSteps = (iHeight - isOdd) / 2;
	int 	iSymbol;
	int 	iEncodedCount = 0;

	// Circular scan box
	int		iMinX, iMaxX, iMinY, iMaxY;
	iMinX = iTotalSteps-1; 
	iMaxX = iWidth-1 - (iTotalSteps - 1);
	iMinY = iTotalSteps-1;
	iMaxY = iHeight-1 - (iTotalSteps - 1);

	if( isOdd )
	{
		// encode the center part using plain probability
		int iY = iMinY+1;
		for( int iX=iMinX+1; iX<iMaxX; iX++)
		{
			m_csc.vInitialMap[(iSymbol=pImageArray[iX*iHeight+iY])] ++;
			if( iSymbol != 0 )	if( ++iEncodedCount == m_hCountSize ) goto label_training_histogram_map_encode_complete;
		}
	}

	int iSteps, iContext, iY, iX;

	// encode the center part using plain probability
	for( iSteps=0; iSteps<iTotalSteps; iSteps++)
	{
		// Left side of the rectangle
		for( iX=iMinX, iY=iMinY; iY<=iMaxY; iY++)
		{
			m_csc.vInitialMap[(iSymbol = pImageArray[iX*iHeight+iY])]++;
			if( iSymbol != 0 ) if( ++iEncodedCount == m_hCountSize ) goto label_training_histogram_map_encode_complete;
		}
		// Bottom side of the rectangle
		for( iX=iMinX+1, iY=iMaxY; iX<iMaxX; iX++)
		{
			m_csc.vInitialMap[(iSymbol = pImageArray[iX*iHeight+iY])]++;
			if( iSymbol != 0 ) if( ++iEncodedCount == m_hCountSize ) goto label_training_histogram_map_encode_complete;
		}
		// Right side of the rectangle
		for( iX=iMaxX, iY=iMaxY; iY>=iMinY; iY--)
		{
			m_csc.vInitialMap[(iSymbol = pImageArray[iX*iHeight+iY])]++;
			if( iSymbol != 0 ) if( ++iEncodedCount == m_hCountSize ) goto label_training_histogram_map_encode_complete;
		}
		// Upper side of the rectangle
		for( iX=iMaxX-1, iY=iMinY; iX>iMinX; iX--)
		{
			m_csc.vInitialMap[(iSymbol = pImageArray[iX*iHeight+iY])]++;
			if( iSymbol != 0 ) if( ++iEncodedCount == m_hCountSize ) goto label_training_histogram_map_encode_complete;
		}
		iMinX--; iMinY--; iMaxX++; iMaxY++;
		if( iMaxY - iMinY + 1 >= CONTEXT_RANGE ) break;
	}

	// Code the remaining part of the histogram map using sum-context
	for( ; iSteps<iTotalSteps; iSteps++)
	{
		// Left side of the rectangle
		for( iX=iMinX, iY=iMinY; iY<=iMaxY; iY++)
		{
			iContext = GenerateSumContext( iX, iY, iMinX, iMaxX, iMinY, iMaxY, pImageArray, iHeight, CONTEXT_RANGE, MAXIMUM_SUM_CONTEXT);
			m_csc.vMap[iContext][(iSymbol = pImageArray[iX*iHeight+iY])]++;
			if( iSymbol != 0 ) if( ++iEncodedCount == m_hCountSize ) goto label_training_histogram_map_encode_complete;
		}
		// Bottom side of the rectangle
		for( iX=iMinX+1, iY=iMaxY; iX<iMaxX; iX++)
		{
			iContext = GenerateSumContext( iX, iY, iMinX, iMaxX, iMinY, iMaxY, pImageArray, iHeight, CONTEXT_RANGE, MAXIMUM_SUM_CONTEXT);
			m_csc.vMap[iContext][(iSymbol = pImageArray[iX*iHeight+iY])]++;
			if( iSymbol != 0 ) if( ++iEncodedCount == m_hCountSize ) goto label_training_histogram_map_encode_complete;
		}
		// Right side of the rectangle
		for( iX=iMaxX, iY=iMaxY; iY>=iMinY; iY--)
		{
			iContext = GenerateSumContext( iX, iY, iMinX, iMaxX, iMinY, iMaxY, pImageArray, iHeight, CONTEXT_RANGE, MAXIMUM_SUM_CONTEXT);
			m_csc.vMap[iContext][(iSymbol = pImageArray[iX*iHeight+iY])]++;
			if( iSymbol != 0 ) if( ++iEncodedCount == m_hCountSize ) goto label_training_histogram_map_encode_complete;
		}
		// Upper side of the rectangle
		for( iX=iMaxX-1, iY=iMinY; iX>iMinX; iX--)
		{
			iContext = GenerateSumContext( iX, iY, iMinX, iMaxX, iMinY, iMaxY, pImageArray, iHeight, CONTEXT_RANGE, MAXIMUM_SUM_CONTEXT);
			m_csc.vMap[iContext][(iSymbol = pImageArray[iX*iHeight+iY])]++;
			if( iSymbol != 0 ) if( ++iEncodedCount == m_hCountSize ) goto label_training_histogram_map_encode_complete;
		}
		iMinX--; iMinY--; iMaxX++; iMaxY++;
	}
label_training_histogram_map_encode_complete:

	return 0;
}


void CsscCoordinateCoding::EndTrainingMode()
{
	m_bTrainingMode = false; 

	// process the histogram count distribution	
	unsigned int iSum = 0;
	for( int i=0; i< SUM_HIST_COUNT_SIZE; i++)
	{
		iSum += m_csc.vCount[i];
		assert( iSum < 0x7fffffff);;
		m_csc.vCount[i] = iSum;
	}

	// process the histogram map distribution
	const int	iTotalSum = 65536;
	// 1) initial map
	{
		double 	fProb0 = ((double)m_csc.vInitialMap[0]) / ((double)(m_csc.vInitialMap[0]+m_csc.vInitialMap[1]));
		int 	iProb0 = (int)floor(((double)iTotalSum)*fProb0);
		m_csc.vInitialMap[0] = iProb0;
		m_csc.vInitialMap[1] = iTotalSum;
	}
	// 2) second context map
	{
		for( int j=0; j<MAXIMUM_SUM_CONTEXT+1; j++)
		{
			double 	fProb0 = ((double)m_csc.vMap[j][0]) / ((double)(m_csc.vMap[j][0]+m_csc.vMap[j][1]));
			int 	iProb0 = (int)floor(((double)iTotalSum)*fProb0);
			m_csc.vMap[j][0] = iProb0;
			m_csc.vMap[j][1] = iTotalSum;
		}
	}
}

int CsscCoordinateCoding::writeSeparateContext(char *filename)
{
	char vBuffer[256];

	// Output histogram count binary
	sprintf( vBuffer, "%s.count", filename);
	FILE *fp = fopen( vBuffer, "wb");
	if (fp==NULL)
		return -1;

	// print the histogram counts
	for( int i=0; i< SUM_HIST_COUNT_SIZE; i++)
	{
		int iTemp = m_csc.vCount[i];
		fwrite(&iTemp, sizeof(int), 1, fp);	// write the symbols
	}
	fclose(fp);

	// Output histogram map binary
	sprintf( vBuffer, "%s.map", filename);
	fp = fopen( vBuffer, "wb");
	// 1) initial map
	{
		int iTemp = m_csc.vInitialMap[0];
		fwrite(&iTemp, 1, sizeof(int), fp);	
		iTemp = m_csc.vInitialMap[1];
		fwrite(&iTemp, 1, sizeof(int), fp);	
	}

	// 2) print second context map
	for( int j=0; j<MAXIMUM_SUM_CONTEXT+1; j++)
	{
		int iTemp = m_csc.vMap[j][0];
		fwrite(&iTemp, 1, sizeof(int), fp);	
		iTemp = m_csc.vMap[j][1];
		fwrite(&iTemp, 1, sizeof(int), fp);	
	}

	fclose(fp);
	return 0;
}

int CsscCoordinateCoding::readSeparateContext(char * CssCFile, CircularSumContext& cCsc)
{
	char vBuffer[512];

	// Read histogram count binary
	sprintf( vBuffer, "%s.count", CssCFile);
//	printf("Read cssc context counts %s\n", vBuffer);
	FILE *fp = fopen( vBuffer, "rb");
	if (fp==NULL)
	{
		throw CdvsException(std::string("CsscCoordinateCoding::readSeparateContext - file not found: ").append(CssCFile));
	}

	int histCountBuffer [SUM_HIST_COUNT_SIZE];
	fread(histCountBuffer, sizeof(int), SUM_HIST_COUNT_SIZE, fp);
	for( int j=0; j< SUM_HIST_COUNT_SIZE; j++)
	{
		cCsc.vCount[j]= histCountBuffer[j];
	}
	fclose( fp);

	sprintf( vBuffer, "%s.map", CssCFile);
//	printf("Read cssc context map %s\n", vBuffer);
	fp = fopen( vBuffer, "rb");
	// read the histogram map distribution
	// 1) initial map
	int inMap [2];
	fread(inMap, sizeof(int), 2, fp);
	cCsc.vInitialMap[0] = inMap[0];
	cCsc.vInitialMap[1] = inMap[1];

	// 2) read second context map
	int contextMap [(MAXIMUM_SUM_CONTEXT+1)*2];
	fread(contextMap, sizeof(int), (MAXIMUM_SUM_CONTEXT+1)*2, fp);
	for( int j=0; j< MAXIMUM_SUM_CONTEXT+1; j++)
	{
		cCsc.vMap[j][0]= contextMap[j*2];
		cCsc.vMap[j][1]= contextMap[j*2+1];
	}
	fclose (fp);
	return 0;
}

int CsscCoordinateCoding::readSeparateContext(char * CssCFile)
{
	return readSeparateContext( CssCFile, m_csc);
}
