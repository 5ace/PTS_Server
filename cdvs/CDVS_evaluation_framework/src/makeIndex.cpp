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
#include <assert.h>
#include <string>
#include <cstring>
#include <iostream>
#include "FileManager.h"
#include "CdvsException.h"
#include "CdvsInterface.h"

using namespace std;
using namespace mpeg7cdvs;

size_t n_images, n_descriptors;

unsigned int modeId;
const char * ext;


/**                                                                                                                                
 * Global and local descriptor index generation function.                                                                                                                                
 * Only descriptors must be used to generate index.                                                                                
 * @param index name of an index file to generate
 * @param cdvsserver an istance of a CdvsServer implementation
 * @param manager the file manager containing the list of input images
 */
void mk_cdvs_index (const char *index,  CdvsServer * cdvsserver, const FileManager & manager)
{
	cout << "start processing " << n_descriptors << " images" << endl;		// print a message
	cdvsserver->createDB(modeId, n_descriptors);

	for (size_t i=0; i<n_descriptors; i++)
	{
		// read descriptor
		string imgname = manager.getAbsolutePathname(i);
		string descname = manager.replaceExt(imgname, ext);

		CdvsDescriptor reference;
		cdvsserver->decode(reference, descname.c_str());
		if (reference.getNumberOfLocalDescriptors() == 0)	{		// this special case happens when no features are extracted from the image by vlfeat
		  cerr << "makeIndex: warning --> skipping empty descriptor " << descname << endl;
		  continue;
		}

		cdvsserver->addDescriptorToDB(reference, manager.getRelativePathname(i).c_str());	// add image to DB
   
		if (((i+1) % 1000) == 0)
    		cout << (i+1) << " images have been processed; last image = " <<  imgname << endl;
	}
  
	cdvsserver->commitDB();

	string indexName = index;
	cdvsserver->storeDB((indexName + ".local").c_str(), (indexName + ".global").c_str());	// store the Data Base
	cout << cdvsserver->sizeofDB() << " images stored in index file " << index << endl;
}

void usage()
{
    fprintf (stdout,
	  "CDVS database index generation module.\n"
	  "usage:\n"
	  "  makeIndex <images> <index> <mode> <datasetPath> <annotationPath> [-h]\n"
	  "where:\n"
	  "  images - database images (text file, 1 file name per line) to be indexed\n"
	  "  index - name of index file (or multiple index files) to be generated\n"
	  "  mode (0..n) - sets the encoding mode to use to build the database\n"
      "  dataset path - the root dir of the CDVS dataset of images\n"
      "  annotation path - the root dir of the CDVS annotation files\n"
      "  -help or -h: help\n");
    exit (1);
}

 /**
 * @file 
 * makeIndex: CDVS index creation module.
 * @verbatim  

  CDVS database index generation module.
	usage:
		makeIndex <images> <index> <mode> <datasetPath> <annotationPath> [-h]
	where:
        images - database images (text file, 1 file name per line) to be indexed
        index - name of index file (or multiple index files) to be generated
        mode (0..n) - sets the encoding mode to use
        dataset path - the root dir of the CDVS dataset of images
        annotation path - the root dir of the CDVS annotation files
   Options:
        -help or -h: help
 
 @endverbatim
 */

int run_make_index (int argc, char *argv[])
{
  // argv 0      1        2        3		4			5
  // makeIndex <images> <index> <mode> <datasetPath> <annotationPath> [-h]

  /* check if sufficient # of arguments were provided: */
  if (argc < 6)
	  usage();

  for (int i=6; i<argc; i++)
  {
	  if (argv[i][0] != '-')
	  {
		  fprintf (stderr, "Invalid option: %s\n", argv[i]);
		  exit (1);
	  }
	  /* check option names: */
	  if (!strcmp (argv[i]+1,"help") || !strcmp (argv[i]+1,"h") || !strcmp (argv[i]+1,"H")) {
		  /* display help: */
		  usage();
	  }
	  else {
		  fprintf (stderr, "Invalid option: %s\n", argv[i]);
		  exit (1);
	  }
  }

  /* read mode */
  modeId = atoi(argv[3]);
  char * datasetPath = argv[4];
  char * annotationPath = argv[5];

  /* read list of images */
  FileManager manager;
  manager.setDatasetPath(datasetPath);
  manager.setAnnotationPath(annotationPath);
  n_images = manager.readAnnotation(argv[1]);

  /* read the index name and set the correct path */
  string indexpathname = string(datasetPath) + "/" + argv[2];

  /* create an instance of CdvsServer */
  CdvsConfiguration * cdvsconfig = CdvsConfiguration::cdvsConfigurationFactory();	// use default values
  CdvsServer * cdvsserver = CdvsServer::cdvsServerFactory(cdvsconfig);

  ext = cdvsconfig->getParameters(modeId).modeExt;	  		// set correct extension

  /* generate list of descriptors: */
  n_descriptors = n_images;

  /* call index generator: */
  mk_cdvs_index(indexpathname.c_str(), cdvsserver, manager);

  delete cdvsserver;
  delete cdvsconfig;		// destroy the CDVS configuration instance

  return 0;
}
//  ----- main -------

int main(int argc, char *argv[])
{
	try {
		run_make_index(argc, argv);		// run "make index" catching any exception
	}
	catch(exception & ex)				// catch any exception, including CdvsException
	{
	    cerr << argv[0] << " exception: " << ex.what() << endl;
	}

	return 0;
}

