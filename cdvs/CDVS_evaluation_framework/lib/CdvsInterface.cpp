/*
 * This software module was originally developed by:
 *
 *   Joint Open Lab VISIBLE (Telecom Italia)
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

#include "CdvsInterface.h"
#include "CdvsServerImpl.h"
#include "CdvsConfigurationImpl.h"
#include "CdvsException.h"

#ifdef MAIN
#include "CdvsClientImpl.h"
#elif defined LOWMEM
#include "CdvsClientLowMem.h"
#elif defined BFLOG
#include "CdvsClientBflog.h"
#endif


using namespace mpeg7cdvs;


CdvsConfiguration * CdvsConfiguration::cdvsConfigurationFactory(const char * configfile)
{
	return new CdvsConfigurationImpl(configfile);
}

int CdvsConfiguration::getMode(int descLen)
{
	if (descLen <= 0 )
		throw CdvsException("Descriptor length out of range");

	if (descLen <= 512)
		return 1;
	if (descLen <= 1024)
		return 2;
	if (descLen <= 2048)
		return 3;
	if (descLen <= 4096)
		return 4;
	if (descLen <= 8192)
		return 5;

	return 6;	// all values above 8K are classified as mode 6.
}

CdvsClient * CdvsClient::cdvsClientFactory(const CdvsConfiguration * config, int mode)
{
#ifdef MAIN
		return new CdvsClientImpl(config, mode);
#elif defined LOWMEM
		return new CdvsClientLowMem(config, mode);
#elif defined BFLOG
		return new CdvsClientBflog(config, mode);
#endif
}


CdvsServer * CdvsServer::cdvsServerFactory(const CdvsConfiguration * config, bool twoWayMatch)
{
	return new CdvsServerImpl(config, twoWayMatch);
}

