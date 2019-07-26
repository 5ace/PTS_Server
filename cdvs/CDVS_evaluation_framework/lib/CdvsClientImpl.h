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

/*
 * CdvsClient.h
 *
 *  Created on: Jun 10, 2014
 *      Author: massimo
 */
#pragma once
#include "CdvsInterface.h"
#include "Parameters.h"
#include "SCFVIndex.h"
#include "CdvsDescriptor.h"

namespace mpeg7cdvs
{

/**
 * @class CdvsClientImpl
 * Main implementation of the high level interface to the client-side functionality of the CDVS Library.
 * @author  Massimo Balestri
 * @date 2014
 */
class CdvsClientImpl  : public CdvsClient {
protected:
	SCFVFactory g_factory;	///< global descriptor factory for the query
	Parameters params;		///< parameters used by this client
	int modeId;				///< the mode ID used by this client

public:
	CdvsClientImpl(const CdvsConfiguration * config, int mode);
	virtual ~CdvsClientImpl();

	virtual unsigned int encode(CdvsDescriptor & output, int width, int height, const unsigned char * buffer) const;
};


}  // end namespace
