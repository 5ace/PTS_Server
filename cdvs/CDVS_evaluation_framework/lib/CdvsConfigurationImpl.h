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
 * CdvsInterface.h
 *
 *  Created on: Oct 6, 2014
 *      Author: massimo
 */
#pragma once
#include "CdvsInterface.h"

namespace mpeg7cdvs
{

	/**
	 * @class CdvsConfigurationImpl
	 * Interface to all configuration parameters for clients and servers.
	 * @author  Massimo Balestri
	 * @date 2014
	 */
	class CdvsConfigurationImpl : public CdvsConfiguration
	{
	private:
		Parameters params[Parameters::nModes];

	public:
		CdvsConfigurationImpl(const char * configfile);
		virtual ~CdvsConfigurationImpl() {};

		/**
		 * Get one of the Parameters instances (note that this class keeps an instance of all parameters for all modes).
		 * @param mode the mode for which parameters are requested.
		 * @return a read-only instance of the parameters.
		 */
		virtual const Parameters & getParameters(int mode) const;

		/**
		 * Set some Parameters value for a specific mode.
		 * @param mode the mode for which parameters are requested.
		 * @return a modifiable instance of the parameters.
		 */
		virtual Parameters & setParameters(int mode);
	};

}	// end of namespace
