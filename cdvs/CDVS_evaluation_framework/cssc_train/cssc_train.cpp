/*
 * This software module was originally developed by:
 *
 *   Stanford University, Huawei Technologies Co., Ltd.
 *
 * in the course of development of ISO/IEC <number> (Compact Descriptors for Visual 
 * Search) standard for reference purposes and its performance may not have been 
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
 * Stanford University and Huawei Technologies Co., Ltd. retain full rights to modify and use the code for their own 
 * purposes, assign or donate the code to a third party and to inhibit third parties 
 * from using the code for products that do not conform to MPEG-related 
 * ITU Recommendations and/or ISO/IEC International Standards.
 *
 * This copyright notice must be included in all copies or derivative works.
 * Copyright (c) ISO/IEC 2012.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <iostream>

/* includes for Telecom Italia implementation */

#include "CdvsInterface.h"
#include "FileManager.h"
#include "CsscCoordinateCoding.h"

using namespace std;
using namespace mpeg7cdvs;

#define _HUAWEI

/* use std::max instead of #define max ... */
#if defined(max)
#undef max
#endif


/*
 * Descriptor buffers: 
 */
#define MAX_DSC_LENGTH (32 * 1024)
unsigned char q_dsc[MAX_DSC_LENGTH];
int q_dsc_len;

/**
 * @file
 * lc_train.exe: CDVS location coding training module.
 * Computes the probability (score) that two images share the same visual object.
 * This is done for all image pairs given in input.
 * @verbatim

Usage:
	lc_train <image-list> <ext> <paramfile> <rate>[-l] [-t tracefile] [-h]\n"
Parameters:
	image-list - list of matching pairs of images
	ext - extensions (e.g., .cdvs) assigned to descriptor files
	results - file to contain list of matching results
	paramfile - file containing coding parameters for all profiles
	rate - Rate profile
Options:
	-h -help: show help

@endverbatim
 */
int run_training (int argc, char *argv[])
{
	/* files: */
	FILE *q_file = NULL; 
	int iRate;
	char *sOutputBinaryFile;

	/* check if sufficent # of arguments were provided: */
	if (argc < 5) {
usage:
		fprintf (stdout, 
				"CDVS descriptor matching module.\n"
				"Usage:\n"
				"  cssc_train <image-list> <mode> <paramfile> <output binary file>\n"
				"Parameters:\n"
				"  image-list - list of images\n"
				"  mode - feature mode\n"
				"  paramfile - file containing coding parameters for all profiles\n"
				"  output binary file - binary context output file\n"
				"Options:\n"
				"  -h -help: show help\n");
		exit (1);
	}

	/* parse options: */
	for ( int i=5; i<argc; i++) {
		if (argv[i][0] == 0x2D /* "-" */) {
			/* check option names: */
			if (!strcmp (argv[i]+1,"help") || !strcmp (argv[i]+1,"h") || !strcmp (argv[i]+1,"H")) {
				/* display help: */
				goto usage;
			}
			/*
			 * parse other options ... 
			 */
			{
				fprintf (stderr, "Invalid option: %s\n", argv[i]);
				exit (1);
			}
		}
	}

	/* read file list: */
	FileManager manager;
	size_t n_images = manager.readAnnotation(argv[1]);

	/* get encoding mode: */
	int iMode = atoi(argv[2]);

	/* Read parameters file and the quantized sift data */
	CdvsConfiguration * cdvsconfig = CdvsConfiguration::cdvsConfigurationFactory(argv[3]);	// use default values
	CdvsServer * cdvsserver = CdvsServer::cdvsServerFactory(cdvsconfig);

	const char * ext = cdvsconfig->getParameters(iMode).modeExt;

	/* Setup output strings */
	sOutputBinaryFile = argv[4];

  	/* global descriptor options */
	bool use_global = true;
	bool use_local = true;

	CsscCoordinateCoding trainer(cdvsconfig->getParameters(iMode));
	trainer.StartTrainingMode();
	for( int i=0; i<n_images; i++) 
	{
		/* Setup training file name */
		unsigned char*	pReadPtr = q_dsc;
		string 	query_name = manager.getQueryName(i);
		string sTrainingFile = manager.replaceExt(query_name, ext);

		CdvsDescriptor query;

		cdvsserver->decode(query, sTrainingFile.c_str());

		/* train the model using image */
		trainer.AddImageSample(query.featurelist);
	}

	trainer.EndTrainingMode();

	/* print the context models */
	printf("Output context to file %s\n", sOutputBinaryFile);
	trainer.writeSeparateContext( sOutputBinaryFile);

	delete cdvsserver;
	delete cdvsconfig;		// destroy the CDVS configuration instance

	return 0;
}



int main(int argc, char *argv[]) {
	try {
		run_training(argc, argv);		// run "training" catching any exception
	}
	catch (const char *msg) // catch exceptions
	{
		cerr << "Exception message: " << msg << endl;
	}
	catch (...) // catch everything
	{
		cerr << "Exception detected." << endl;
	}
	return 0;
}



/******************* I/O functions: *****************************/




/* cssc_train.cpp -- end of file */
