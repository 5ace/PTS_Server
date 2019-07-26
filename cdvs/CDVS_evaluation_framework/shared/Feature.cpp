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

#include "Feature.h"
#include "CdvsException.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <cassert>

using namespace mpeg7cdvs;


Feature::Feature(void)
{
	x = 0;	
	y = 0;			
	scale = 0;    
	orientation = 0; 
	peak = 0;
	curvRatio = 0;
	curvSigma = 0;
	pdf = 0;
	spatialIndex = 0;
	relevance = 0;
	octave = 0;
	iscale = 0;

	for(int k = 0; k < descrLength; ++k)
		descr[k] = 0.0f;

	memset(qdescr, 0, sizeof(qdescr));

	descr[0] = -1.0f;		// trap to detect uninitialized descriptors when compressing

}

void Feature::toFile(FILE * file) const
{
	size_t fout;
	fout = fwrite(&x, sizeof(x), 1, file);
	fout += fwrite(&y, sizeof(y), 1, file);
	fout += fwrite(&scale, sizeof(scale), 1, file);
	fout += fwrite(&orientation, sizeof(orientation), 1, file);
	fout += fwrite(&peak, sizeof(peak), 1, file);
	fout += fwrite(&curvRatio, sizeof(curvRatio), 1, file);
	fout += fwrite(&curvSigma, sizeof(curvSigma), 1, file);
	fout += fwrite(&descr, sizeof(descr), 1, file);
	fout += fwrite(&pdf, sizeof(pdf), 1, file);
	fout += fwrite(&spatialIndex, sizeof(spatialIndex), 1, file);
	fout += fwrite(&relevance, sizeof(relevance), 1, file);
	fout += fwrite(&octave, sizeof(octave), 1, file);
	fout += fwrite(&iscale, sizeof(iscale), 1, file);
	assert(fout == 13);
}

void Feature::fromFile(FILE * file)
{
	size_t fout;
	fout = fread(&x, sizeof(x), 1, file);
	fout += fread(&y, sizeof(y), 1, file);
	fout += fread(&scale, sizeof(scale), 1, file);
	fout += fread(&orientation, sizeof(orientation), 1, file);
	fout += fread(&peak, sizeof(peak), 1, file);
	fout += fread(&curvRatio, sizeof(curvRatio), 1, file);
	fout += fread(&curvSigma, sizeof(curvSigma), 1, file);
	fout += fread(&descr, sizeof(descr), 1, file);
	fout += fread(&pdf, sizeof(pdf), 1, file);
	fout += fread(&spatialIndex, sizeof(spatialIndex), 1, file);
	fout += fread(&relevance, sizeof(relevance), 1, file);
	fout += fread(&octave, sizeof(octave), 1, file);
	fout += fread(&iscale, sizeof(iscale), 1, file);
	assert(fout == 13);
}


