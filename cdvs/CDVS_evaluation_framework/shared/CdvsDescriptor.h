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

#pragma once

#include "Parameters.h"
#include "Buffer.h"
#include "ImageBuffer.h"
#include "FeatureList.h"
#include "SCFVIndex.h"

namespace mpeg7cdvs
{

/**
 * @class CdvsDescriptor
 * Helper class to read/write/check CDVS descriptors according to the syntax defined in ISO/IEC 15938-13.
 * @author Massimo Balestri (Telecom Italia)
 * @date April, 2014
 */
class CdvsDescriptor {
private:
	static const int max_size = 32 * 1024;				// maximum CDVS descriptor size (bytes)
	static const int max_16bit_size = 64 * 1024;		// this limit applies to various parameters
	static const int header_nbits = 16 * 4;			// header size (bits)
	static const int max_image_dimension = 64 * 1024;	// max image dimension (either width or height)
	static const int num_features_in_GD = 250;			// number of features used to generate the global descriptor
	static const int cdvs_spec_version = 1;			// MPEG-7 part 13 version
	static const int mode_id_limit = 6;				// maximum mode id
	static const int max_descr_value = 32;				// by definition of the syntax

	int debuglevel;									// the debug level set from parameters

	unsigned int versionID;							// 3 bits
	unsigned int modeID;							// 8 bits
	bool globalHasBitSelection;						// 1 bit
	bool globalHasVariance;							// 1 bit
	bool relevanceBitsPresent;						// 1 bit
	unsigned int originalImageXResolution;			// 16 bits
	unsigned int originalImageYResolution;			// 16 bits
	unsigned int numberOfLocalDescriptors;			// 16 bits
	unsigned int histogramCountSize;				// 16 bits
	unsigned int histogramMapSizeX;					// 16 bits
	unsigned int histogramMapSizeY;					// 16 bits

public:
	Buffer buffer;									///< the buffer containing the input/output bitstream
	FeatureList featurelist;						///< the list of key points
	SCFVSignature scfvSignature;				///< the global descriptor signature

	CdvsDescriptor();
	virtual ~CdvsDescriptor();

	/**
	 * Encode the CDVS descriptor. This implementation does not extract all the
	 * key points from the image, but only those selected for transmission by the feature selection stage.
	 * @param params set of parameters to apply for one specific mode
	 * @param image the input image buffer
	 * @param g_factory the Global Descriptor factory instance that produces the GD signature for the specific mode selected in the parameters
	 * @return the size of the produced descriptor (bytes).
	 */
	size_t encode(const Parameters & params, ImageBuffer & image, const SCFVFactory & g_factory);

	/**
	 * Decode the CDVS descriptor.
	 * @param pset set of parameters to apply for all modes from 0 to 6
	 * @return the size of the consumed descriptor (bytes).
	 */
	size_t decode(const ParameterSet & pset);

	/**
	 * Check the conformance of the descriptor to the syntax defined in ISO/IEC 15938-13.
	 * @return the number of out of range fields
	 */
	int check() const;

	/**
	 * Clear all data.
	 */
	void clear();

	/**
	 * Print the value of the syntax elements defined in ISO/IEC 15938-13.
	 * @param title the title to print as header information
	 */
	void print(const char * title) const;

	unsigned int getVersionID() const;				///< get the version ID
	unsigned int getModeID() const;					///< get the mode ID
	bool getGlobalHasBitSelection() const;			///< get the global descriptor bit selection flag
	bool getGlobalHasVariance() const ;				///< get the global descriptor variance flag
	bool getRelevanceBitsPresent() const;			///< get the relevance bit flag
	unsigned int getOriginalImageXResolution() const;	///< get the original image X resolution
	unsigned int getOriginalImageYResolution() const;	///< get the original image Y resolution
	unsigned int getNumberOfLocalDescriptors() const;	///< get the number of local descriptors
	unsigned int getHistogramCountSize() const;			///< get the coordinate coding histogram count size
	unsigned int getHistogramMapSizeX() const;		///< get the coordinate coding map horizontal size
	unsigned int getHistogramMapSizeY() const;		///< get the coordinate coding map vertical size

	void setVersionID(unsigned int vID);				///< set the version ID
	void setModeID(unsigned int mID);					///< set the mode ID
	void setGlobalHasBitSelection(bool gHasBS);			///< set the global descriptor bit selection flag
	void setGlobalHasVariance(bool gHasV);				///< set the global descriptor variance flag
	void setRelevanceBitsPresent(bool relevance);			///< set the relevance bit flag
	void setOriginalImageXResolution(unsigned int oiXr);	///< set the original image X resolution
	void setOriginalImageYResolution(unsigned int oiYr);	///< set the original image Y resolution
	void setNumberOfLocalDescriptors(unsigned int nLD);	///< set the number of local descriptors
	void setHistogramCountSize(unsigned int hCS);			///< set the coordinate coding histogram count size
	void setHistogramMapSizeX(unsigned int hmsX);		///< set the coordinate coding map horizontal size
	void setHistogramMapSizeY(unsigned int hmsY);		///< set the coordinate coding map vertical size
};

} // namespace mpeg7cdvs
