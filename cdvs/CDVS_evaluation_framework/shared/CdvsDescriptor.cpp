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
 * Copyright (c) ISO/IEC 2014.
 *
 */

#include "CdvsDescriptor.h"
#include "BitInputStream.h"
#include "BitOutputStream.h"
#include "Buffer.h"
#include "CsscCoordinateCoding.h"
#include "CdvsException.h"
#include <algorithm>    // min
#include <iostream>     // cin, cout

using namespace std;
using namespace mpeg7cdvs;

CdvsDescriptor::CdvsDescriptor():featurelist(),scfvSignature(false, false), buffer() {
	debuglevel = 0;
	versionID = 0;
	modeID = 0;
	globalHasBitSelection = false;
	globalHasVariance = false;
	relevanceBitsPresent = false;
	originalImageXResolution = 0;
	originalImageYResolution = 0;
	numberOfLocalDescriptors = 0;
	histogramCountSize = 0;
	histogramMapSizeX = 0;
	histogramMapSizeY = 0;
}

CdvsDescriptor::~CdvsDescriptor() {
}

void CdvsDescriptor::clear() {
	featurelist.clear();
	scfvSignature.clear();
	buffer.clear();
}

size_t CdvsDescriptor::encode(const Parameters & params, ImageBuffer & image, const SCFVFactory & g_factory)
{
	debuglevel = params.debugLevel;			// 0 = off, 1 = on (quiet), 2 = on (verbose), 3 = verbose + dump files
	featurelist.features.clear();			// cleanup featurelist
	featurelist.setResolution(image.width, image.height, image.originalWidth, image.originalHeight);
	image.detect(featurelist, params);		// detect all key points in the image

	// set parameters

	versionID = cdvs_spec_version;
	modeID = params.getModeID();
	globalHasBitSelection = g_factory.hasBitSelection();
	globalHasVariance = g_factory.hasVariance();
	relevanceBitsPresent = (params.numRelevantPoints > 0);
	originalImageXResolution = featurelist.originalWidth;
	originalImageYResolution = featurelist.originalHeight;

	// check mode

	if  (modeID > mode_id_limit)
		throw CdvsException("Unknown mode ID");			// this is not a CDVS descriptor

	// compute Global Descriptor

	image.extract(featurelist, params.selectMaxPoints);	// extract all key points (up to the defined limit)
	featurelist.selectFirst(params.selectMaxPoints);	// drop all other points

    // Generate global signature

    g_factory.generateSCFV(featurelist, scfvSignature, num_features_in_GD);

	// Local descriptors

	featurelist.compress(params.numberOfElementGroups);			// compress all extracted points
	featurelist.setRelevantPoints(params.numRelevantPoints);	// set relevant points (if required)

	// now compute exactly how many points will be encoded
	numberOfLocalDescriptors = featurelist.computeMaxPoints(params, (8 * params.descLength) - header_nbits - scfvSignature.compressedNumBits());
	featurelist.selectFirst(numberOfLocalDescriptors);		// drop unused points

	// declare and initialize the output bitstream buffer

	buffer.resize(max_size);									// allocate memory
	BitOutputStream writer(buffer.data(), buffer.size());		// attach output buffer

	// write the CDVS Descriptor

	writer.write(versionID, 3);
	writer.write(modeID, 8);
	writer.write(globalHasBitSelection & 1, 1);
	writer.write(globalHasVariance & 1, 1);
	writer.write(relevanceBitsPresent & 1, 1);
	writer.align(1);		// align to the closest byte boundary - fill with 1
	writer.write(originalImageXResolution, 16);
	writer.write(originalImageYResolution, 16);
	writer.write(numberOfLocalDescriptors, 16);
	if (numberOfLocalDescriptors > 0)
	{
		scfvSignature.write(writer);					// write global descriptor signature
		CsscCoordinateCoding cc(params);
		cc.generateHistogramMap(featurelist, numberOfLocalDescriptors);
		featurelist.sortSpatialIndex();					// sort according to the coding ordering
		cc.toBinary(writer);							// write coordinate coding
		featurelist.toBinary(writer, relevanceBitsPresent, numberOfLocalDescriptors); // write features
		cc.exportVars(histogramCountSize, histogramMapSizeX, histogramMapSizeY);	// save cc internal state
	}

	buffer.resize(writer.close() >> 3);
	return buffer.size();
}


size_t CdvsDescriptor::decode(const ParameterSet & parset)
{
	BitInputStream reader(buffer.data(), buffer.size());		// attach input buffer;

	versionID = reader.read(3);
	modeID = reader.read(8);

	if ((versionID != cdvs_spec_version) || (modeID > mode_id_limit))
		throw CdvsException("Unknown version or mode ID");			// this is not a CDVS descriptor

	debuglevel = parset[modeID].debugLevel;			// 0 = off, 1 = on (quiet), 2 = on (verbose), 3 = verbose + dump files

	globalHasBitSelection = (reader.read(1) == 1);
	globalHasVariance = (reader.read(1) == 1);
	relevanceBitsPresent = (reader.read(1) == 1);

	reader.align();		// align to the closest byte boundary
	originalImageXResolution = reader.read(16);
	originalImageYResolution = reader.read(16);
	numberOfLocalDescriptors = reader.read(16);

	featurelist.features.clear();	// cleanup featurelist
	featurelist.setResolution(0, 0, originalImageXResolution, originalImageYResolution);

	if (numberOfLocalDescriptors > 0)
	{
		// Read the global descriptor signature

		scfvSignature.hasVar(globalHasVariance);				// set variance
		scfvSignature.hasBitSelection(globalHasBitSelection);	// set bit selection
		scfvSignature.read(reader);

		CsscCoordinateCoding ccdecoder(parset[modeID]);
		ccdecoder.fromBinary(reader);			//decode coordinates
		ccdecoder.generateFeatureList(featurelist);
		featurelist.fromBinary(reader, relevanceBitsPresent);
		ccdecoder.exportVars(histogramCountSize, histogramMapSizeX, histogramMapSizeY);	// save cc internal state
	}

	reader.align();
	size_t consumedBits = reader.consumed();
	reader.close();		// finished reading the cdvs descriptor
	return (consumedBits >> 3);
}

int CdvsDescriptor::check() const {
	int counter = 0;
	if (versionID != cdvs_spec_version) ++counter;
	if (modeID > 6) ++counter;
	if (originalImageXResolution > max_image_dimension) ++counter;
	if (originalImageYResolution > max_image_dimension) ++counter;
	if (numberOfLocalDescriptors > max_16bit_size) ++counter;
	if (histogramCountSize > max_16bit_size) ++counter;
	if (histogramMapSizeX > max_16bit_size) ++counter;
	if (histogramMapSizeY > max_16bit_size) ++counter;
	if (featurelist.qdescr_size > 4*max_descr_value) ++counter;

	return counter;
}

void CdvsDescriptor::print(const char * title) const
{
	cout << "------ " << title << " ------" << endl;
	cout << "VersionID                = " << versionID << endl;
	cout << "ModeID                   = " << modeID << endl;
	cout << "GlobalHasBitSelection    = " << globalHasBitSelection << endl;
	cout << "GlobalHasVariance        = " << globalHasVariance << endl;
	cout << "RelevanceBitsPresent     = " << relevanceBitsPresent << endl;
	cout << "OriginalImageXResolution = " << originalImageXResolution << endl;
	cout << "OriginalImageYResolution = " << originalImageYResolution << endl;
	cout << "NumberOfGlobalFunctions  = " << numberCentroids << endl;

	if (debuglevel > 0)
		scfvSignature.print();

	cout << "NumberOfLocalDescriptors = " << numberOfLocalDescriptors << endl;
	cout << "HistogramCountSize       = " << histogramCountSize << endl;
	cout << "HistogramMapSizeX        = " << histogramMapSizeX << endl;
	cout << "HistogramMapSizeY        = " << histogramMapSizeY << endl;
	cout << "NumberOfElementGroups    = " << (featurelist.qdescr_size/4) << endl;

	if (debuglevel > 0)
		featurelist.print();
}


unsigned int CdvsDescriptor::getVersionID() const
{
	return versionID;
}

unsigned int CdvsDescriptor::getModeID() const
{
	return modeID;
}

bool CdvsDescriptor::getGlobalHasBitSelection() const
{
	return globalHasBitSelection;
}

bool CdvsDescriptor::getGlobalHasVariance() const
{
	return globalHasVariance;
}

bool CdvsDescriptor::getRelevanceBitsPresent() const
{
	return relevanceBitsPresent;
}

unsigned int CdvsDescriptor::getOriginalImageXResolution() const
{
	return originalImageXResolution;
}

unsigned int CdvsDescriptor::getOriginalImageYResolution() const
{
	return originalImageYResolution;
}

unsigned int CdvsDescriptor::getNumberOfLocalDescriptors() const
{
	return numberOfLocalDescriptors;
}

unsigned int CdvsDescriptor::getHistogramCountSize() const
{
	return histogramCountSize;
}

unsigned int CdvsDescriptor::getHistogramMapSizeX() const
{
	return histogramMapSizeX;
}

unsigned int CdvsDescriptor::getHistogramMapSizeY() const
{
	return histogramMapSizeY;
}


void CdvsDescriptor::setVersionID(unsigned int vID)
{
	versionID = vID;
}

void CdvsDescriptor::setModeID(unsigned int mID)
{
	modeID = mID;
}

void CdvsDescriptor::setGlobalHasBitSelection(bool gHasBS)
{
	globalHasBitSelection = gHasBS;
}

void CdvsDescriptor::setGlobalHasVariance(bool gHasV)
{
	globalHasVariance = gHasV;
}

void CdvsDescriptor::setRelevanceBitsPresent(bool relevance)
{
	relevanceBitsPresent = relevance;
}

void CdvsDescriptor::setOriginalImageXResolution(unsigned int oiXr)
{
	originalImageXResolution = oiXr;
}

void CdvsDescriptor::setOriginalImageYResolution(unsigned int oiYr)
{
	originalImageYResolution = oiYr;
}

void CdvsDescriptor::setNumberOfLocalDescriptors(unsigned int nLD)
{
	numberOfLocalDescriptors = nLD;
}

void CdvsDescriptor::setHistogramCountSize(unsigned int hCS)
{
	histogramCountSize = hCS;
}

void CdvsDescriptor::setHistogramMapSizeX(unsigned int hmsX)
{
	histogramMapSizeX = hmsX;
}

void CdvsDescriptor::setHistogramMapSizeY(unsigned int hmsY)
{
	histogramMapSizeY = hmsY;
}
