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

#include "Parameters.h"
#include "CdvsException.h"
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <string>

#if defined(_WIN32) || defined(_WIN64)
	#pragma warning(disable : 4996) // ignore MSVC's concerns about safety of IO library
#endif

using namespace std;
using namespace mpeg7cdvs;

// allowed modes are from 0 to nModes

Parameters::Parameters(void)
{
	// set default values for all parameters
	resizeMaxSize			= 640;
	descLength				= 0;
	blockWidth				= 3;
	ctxTableIdx				= 0;
	debugLevel				= 0;
	selectMaxPoints			= 3000;
	numRelevantPoints		= 0;
	retrievalMaxPoints		= 400;
	sprintf(modeExt, ".cdvs");
	ratioThreshold			= 0.85f;
	minNumInliers			= 5;
	wmThreshold				= 4.0;
	wmThreshold2Way			= 1.7;
	wmMixed					= 3.6;
	wmMixed2Way				= 1.8;
	ransacNumTests			= 10; 
    ransacThreshold 		= 8.0f;
	modeId					= 0;
	chiSquarePercentile     = 99;
    retrievalLoops			= 2500;
	wmRetrieval				= 4;
	wmRetrieval2Way			= 2.2;
    queryExpansionLoops     = 0;
	scfvThreshold			= 0.0f;
	locationBits			= 4.5;
	hasVar					= false;
	hasBitSelection			= false;
	gdThreshold				= 0.0f;
	gdThresholdMixed		= 0.0f;
	numberOfElementGroups	= 10;
#ifdef USE_MBIT
	MBIT_threshold			=3;
#endif
}


Parameters::~Parameters(void)
{
}

unsigned int Parameters::getModeID() const
{
	return modeId;		// export private variable as read-only
}

int Parameters::readParameters (const char *filename, int mode)
{
	if (filename == NULL)
		return readParameters(mode);		// if filename is NULL, revert to default parameter values.

	char buffer[2048];
	char paramName [1000];
	char paramValue [1000];

	int currentMode = -1;
	
	ifstream fin(filename);
	if (! fin.good())
	{
		std::string msg("parameters file not found: ");
		msg.append(filename);
		throw CdvsException(msg);
	}

	int ret = 0;
	int val = 0;

	modeId = mode; 	// set the mode ID

	while (fin.good())
	{
		fin.getline(buffer, 2048);
		if (strlen ( buffer ) < 2)
		{
			// ignore this line - nothing to do
		}
		else if (buffer[0] == '[')			// new section
		{
			int nret = sscanf(buffer, "[%s = %d]", paramName, &val);
			if ((nret == 2) && (strcmp(paramName, "Mode") == 0))
				currentMode = val;

			nret = sscanf(buffer, "%s", paramName);
			if ((nret == 1) && (strcmp(paramName, "[Default]") == 0))
				currentMode = mode;
		}
		else if (buffer[0] != '#')			// allow comments
		{
			if (currentMode == mode)
			{
				sscanf(buffer, "%s = %[^\t\n]", paramName, paramValue);
				ret = setParameter(paramName, paramValue);
				if (ret)
					break;
			}
		}	// endif
	}

	fin.close();
	return ret;
}

//
// optional hard-coded parameters
// to be used in cases where reading a text file is not practical
//
int Parameters::readParameters (int mode)
{
	// defaults are already set
	modeId = mode; 	// set the mode ID

	switch (mode)
	{
	case 0:
		// This is the profile used to build the database (similar to mode 4, but always encodes 300 keypoints)
		setParameter("modeExt",".DB.cdvs");
		descLength = 16384;
		chiSquarePercentile = 80;
		retrievalLoops = 500;
		selectMaxPoints = 300;
		retrievalMaxPoints = 300;
		ratioThreshold = 0.9;
		scfvThreshold = 0.003353;
		gdThreshold = 7.235;
		wmThreshold = 1.86;
		wmThreshold2Way = 1.575;
		wmRetrieval = 2.3;
		wmRetrieval2Way = 2.0;
		hasVar = 1;
		locationBits = 4.5;
		ctxTableIdx	= 3;
		numberOfElementGroups = 10;
		break;

	case 1:
		setParameter("modeExt",".512.cdvs");
		descLength = 512;
		retrievalLoops = 500;
		chiSquarePercentile = 90;
		ratioThreshold = 0.9;
		scfvThreshold = 80;
		hasVar = 0;
		gdThreshold = 9.285;
		wmThreshold = 1.645;
		wmThreshold2Way = 1.64;
		wmRetrieval = 3.6;
		wmRetrieval2Way = 2.7;
		selectMaxPoints = 250;
		locationBits = 7.5;
		hasBitSelection = true;
		ctxTableIdx	= 0;
		numberOfElementGroups = 5;
		break;

	case 2:
		setParameter("modeExt",".1024.cdvs");
		descLength = 1024;
		chiSquarePercentile = 80;
		retrievalLoops = 500;
		retrievalMaxPoints = 300;
		ratioThreshold = 0.9;
		scfvThreshold = 80;
		hasVar = 0;
		gdThreshold = 5.725;
		gdThresholdMixed = 5.985;
		wmThreshold = 2.7;
		wmThreshold2Way = 2.625;
		wmMixed = 2.785;
		wmMixed2Way = 2.945;
		wmRetrieval = 3.6;
		wmRetrieval2Way = 3.3;
		selectMaxPoints = 250;
		locationBits = 7.0;
		ctxTableIdx	= 1;
		numberOfElementGroups = 5;
		break;

	case 3:
		setParameter("modeExt",".2048.cdvs");
		descLength = 2048;
		chiSquarePercentile = 80;
		retrievalLoops = 500;
		retrievalMaxPoints = 300;
		ratioThreshold = 0.9;
		scfvThreshold = 85;
		hasVar = 0;
		gdThreshold = 5.98;
		gdThresholdMixed = 6.025;
		wmThreshold = 2.12;
		wmThreshold2Way = 1.87;
		wmMixed = 2.19;
		wmMixed2Way = 1.98;
		wmRetrieval = 2.3;
		wmRetrieval2Way = 2.2;
		selectMaxPoints = 250;
		locationBits = 5.0;
		ctxTableIdx	= 2;
		numberOfElementGroups = 10;
		break;

	case 4:
		setParameter("modeExt",".4096.cdvs");
		descLength = 4096;
		chiSquarePercentile = 80;
		retrievalLoops = 500;
		retrievalMaxPoints = 300;
		ratioThreshold = 0.9;
		scfvThreshold = 0.009195;
		hasVar = 1;
		gdThreshold = 7.235;
		wmThreshold = 1.86;
		wmThreshold2Way = 1.575;
		wmRetrieval = 2.3;
		wmRetrieval2Way = 2.0;
		selectMaxPoints = 300;
		locationBits = 4.8;
		ctxTableIdx	= 3;
		numberOfElementGroups = 16;
		break;

	case 5:
		setParameter("modeExt",".8192.cdvs");
		descLength = 8192;
		chiSquarePercentile = 95;
		retrievalLoops = 500;
		retrievalMaxPoints = 300;
		numRelevantPoints = 300;
		ratioThreshold = 0.9;
		scfvThreshold = 0.009195;
		hasVar = 1;
		gdThreshold = 7.215;
		wmThreshold = 2.165;
		wmThreshold2Way = 1.725;
		wmRetrieval = 2.3;
		wmRetrieval2Way = 2.0;
		selectMaxPoints = 500;
		locationBits = 4.7;
		ctxTableIdx	= 4;
		numberOfElementGroups = 20;
		break;

	case 6:
		setParameter("modeExt",".16384.cdvs");
		descLength = 16384;
		chiSquarePercentile = 95;
		retrievalLoops = 500;
		retrievalMaxPoints = 300;
		numRelevantPoints = 300;
		ratioThreshold = 0.9;
		scfvThreshold = 0.009195;
		hasVar = 1;
		gdThreshold = 7.31;
		wmThreshold = 2.15;
		wmThreshold2Way = 1.665;
		wmRetrieval = 2.3;
		wmRetrieval2Way = 2.0;
		selectMaxPoints = 650;
		locationBits = 4.6;
		ctxTableIdx	= 5;
		numberOfElementGroups = 32;
		break;

	default: break;

	}

	return 0;
}

int Parameters::setParameter(const char *paramName, const char *paramValue)
{
	
	if(strcmp(paramName, "resizeMaxSize")==0)
	{
		int numeric_value = atoi(paramValue);
		this->resizeMaxSize = numeric_value;
		if(numeric_value <0 || numeric_value> 3000)
			throw CdvsException(string("resizeMaxSize out of range: ").append(paramValue));
	}
	else if (strcmp(paramName, "descLength")==0)
	{
		descLength = atoi(paramValue);
	}
	else if (strcmp(paramName, "blockWidth")==0)
	{
		blockWidth = atoi(paramValue);
	}
	else if (strcmp(paramName, "ctxTableIdx")==0)
	{
		ctxTableIdx = atoi(paramValue);
	}
	else if (strcmp(paramName, "debugLevel")==0)
	{
		debugLevel = atoi(paramValue);
	}
	else if (strcmp(paramName, "selectMaxPoints")==0)
	{
		selectMaxPoints = atoi(paramValue);
	}
	else if (strcmp(paramName, "numRelevantPoints")==0)
	{
		numRelevantPoints = atoi(paramValue);
	}
	else if (strcmp(paramName, "modeExt")==0)
	{
		sprintf(modeExt, "%s", paramValue);
	}
	else if (strcmp(paramName, "ratioThreshold")==0)
	{
		ratioThreshold = (float) atof(paramValue);
	}
	else if (strcmp(paramName, "minNumInliers")==0)
	{
		minNumInliers = atoi(paramValue);
	}
	else if (strcmp(paramName, "ransacNumTests")==0)
	{
		ransacNumTests = atoi(paramValue);
	}
	else if (strcmp(paramName, "ransacThreshold")==0)
	{
		ransacThreshold = (float) atof(paramValue);
	}
	else if (strcmp(paramName, "chiSquarePercentile")==0)
	{
		chiSquarePercentile = atoi(paramValue);
	}
	else if (strcmp(paramName, "retrievalLoops")==0)
	{
		retrievalLoops = atoi(paramValue);
	}
	else if (strcmp(paramName, "wmThreshold2Way")==0)
	{
		wmThreshold2Way = atof(paramValue);
	}
	else if (strcmp(paramName, "wmMixed2Way")==0)
	{
		wmMixed2Way = atof(paramValue);
	}
	else if (strcmp(paramName, "wmRetrieval2Way")==0)
	{
		wmRetrieval2Way = atof(paramValue);
	}
	else if (strcmp(paramName, "wmThreshold")==0)
	{
		wmThreshold = atof(paramValue);
	}
	else if (strcmp(paramName, "wmMixed")==0)
	{
		wmMixed = atof(paramValue);
	}
	else if (strcmp(paramName, "wmRetrieval")==0)
	{
		wmRetrieval = atof(paramValue);
	}
	else if (strcmp(paramName, "retrievalMaxPoints")==0)
	{
		retrievalMaxPoints = atoi(paramValue);
	}
	else if (strcmp(paramName, "queryExpansionLoops")==0)
	{
		queryExpansionLoops = atoi(paramValue);
	}
	else if (strcmp(paramName, "scfvThreshold")==0)
	{
		scfvThreshold = atof(paramValue);
	}
	else if (strcmp(paramName, "locationBits")==0)
	{
		locationBits = atof(paramValue);
	}
	else if (strcmp(paramName, "hasVar")==0)
	{
		hasVar = (atoi(paramValue) == 1);
	}
	else if (strcmp(paramName, "hasBitSelection")==0)
	{
		hasBitSelection = (atoi(paramValue) == 1);
	}
	else if (strcmp(paramName, "gdThreshold")==0)
	{
		gdThreshold = atof(paramValue);
	}
	else if (strcmp(paramName, "gdThresholdMixed")==0)
	{
		gdThresholdMixed = atof(paramValue);
	}
	else if (strcmp(paramName, "numberOfElementGroups")==0)
	{
		numberOfElementGroups = atoi(paramValue);
	}
#ifdef USE_MBIT
	else if (strcmp(paramName, "MBIT_Threshold")==0)
	{
		MBIT_threshold = atof(paramValue);
	}
#endif
	else
		throw CdvsException(string("unknown parameter: ").append(paramName));

	return 0;
}

void Parameters::readAll(const char *filename, Parameters params[])
{
	for (int k=0; k<nModes; k++)
		params[k].readParameters(filename, k);
}

void Parameters::readAll(Parameters params[])
{
	for (int k=0; k<nModes; k++)
		params[k].readParameters(k);
}
