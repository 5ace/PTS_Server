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
 * CdvsConfigurationImpl.cpp
 *
 *  Created on: Jun 10, 2014
 *      Author: massimo
 *
 */

#include "CdvsConfigurationImpl.h"
#include "CdvsException.h"
#include "Parameters.h"
#include <cstring>

using namespace mpeg7cdvs;

CdvsConfigurationImpl::CdvsConfigurationImpl(const char * configfile)
{
	Parameters::readAll(params);		// set default values
	if (configfile != NULL)
		Parameters::readAll(configfile, params);		// override default values
}

const Parameters & CdvsConfigurationImpl::getParameters(int mode) const
{
	if ((mode < 0) || (mode > 6))
			throw CdvsException("Mode ID out of range! must be in the range [0..6]");

	return params[mode];
}

Parameters &  CdvsConfigurationImpl::setParameters(int mode)
{
	if ((mode < 0) || (mode > 6))
			throw CdvsException("Mode ID out of range! must be in the range [0..6]");

	return params[mode];
}

