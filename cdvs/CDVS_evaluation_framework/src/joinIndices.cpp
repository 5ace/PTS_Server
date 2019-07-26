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

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <cstring>
#include "CdvsInterface.h"
#include "CdvsException.h"
#include "FileManager.h"


#ifdef _OPENMP
  #include <omp.h>		// use OpenMP multithreading if supported
#endif

using namespace std;
using namespace mpeg7cdvs;

/* lists: */
size_t n_indices;

void join_indices(const char* outfname, const FileManager & manager)
{
    string outName = outfname;

	/* create an instance of CdvsServer */

	CdvsConfiguration * cdvsconfig = CdvsConfiguration::cdvsConfigurationFactory();	// use default values
	CdvsServer * cdvsserver = CdvsServer::cdvsServerFactory(cdvsconfig);

	for(size_t i=0; i<n_indices; ++i)
	{
		string indexName = manager.getAbsolutePathname(i);
		cdvsserver->loadDB((indexName + ".local").c_str(), (indexName + ".global").c_str());
	}

	cdvsserver->storeDB((outName + ".local").c_str(), (outName + ".global").c_str());

	delete cdvsconfig;
	delete cdvsserver;
}

void usage()
{
		fprintf (stdout,
		"CDVS image database building module.\n"
		"Usage:\n"
		"  joinIndices <input list> <output filename> <datasetPath> <annotationPath> [-help]\n"
		"where:\n"
		"  input list - file containing the list of indices to be joined (text file, one file name per line)\n"
		"  output filename - name of the binary output file containing joined indexes\n"
		"  dataset path - the root dir of the CDVS dataset of images\n"
		"  annotation path - the root dir of the CDVS annotation files\n"
		"options:\n"
		"  -help or -h: help\n"
		"\n");
}

/**
 * @file 
 * joinIndices.exe: CDVS image database building module.
 * Joins a number of different indices into a single big index.
 * @verbatim

	usage:
		joinIndices <input list> <output filename> <datasetPath> <annotationPath> [-help]
	where:
		input list - file containing the list of indices to be joined (text file, one file name per line)
		output filename - name of the binary output file containing joined indexes
        dataset path - the root dir of the CDVS dataset of images
        annotation path - the root dir of the CDVS annotation files
	options:
		-help or -h: help
  
  @endverbatim
 */ 
 
 /**
  * Join indices
  * @param argc arg counter 
  * @param argv arg values
  */
int run_joinIndices(int argc, char *argv[]) {


	if (argc < 5) {
		usage();
		return 1;
	}

	/* parse options: */

	for (int i=5; i<argc; i++)
	{
		if (argv[i][0] != '-')
		{
			fprintf (stderr, "Invalid option: %s\n", argv[i]);
			return 1;
		}
		/* check option names: */
		if (!strcmp (argv[i]+1,"help") || !strcmp (argv[i]+1,"h") || !strcmp (argv[i]+1,"H")) {
			usage();		/* display help: */
			return 0;
		}
	}

	const char * datasetPath = argv[3];
	const char * annotationPath = argv[4];

	/* read file list: */
	FileManager manager;
	manager.setAnnotationPath(annotationPath);
	n_indices = manager.readAnnotation(argv[1]);		// read file list
	manager.setDatasetPath(datasetPath);

	char outfname[512];
	sprintf(outfname, "%s/%s", datasetPath, argv[2]);

	join_indices(outfname, manager);


	return 0;
}

//  ----- main -------

int main(int argc, char *argv[])
{
	try {
		run_joinIndices(argc, argv);		// run "joinIndices" catching any exception
	}
	catch(exception & ex)				// catch any exception, including CdvsException
	{
	    cerr << argv[0] << " exception: " << ex.what() << endl;
	}

	return 0;
}
