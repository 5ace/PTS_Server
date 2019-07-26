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

#include <algorithm>
#include <iostream>
#include <string>
#include <cstring>
/* overlap ratio test for localization precision: */
#include "BoundingBox.h"

/* includes for Telecom Italia implementation */

#include "CdvsInterface.h"
#include "FileManager.h"
#include "HiResTimer.h"
#include "TraceManager.h"
#include "CdvsException.h"

using namespace std;
using namespace mpeg7cdvs;

/* use std::max instead of #define max ... */
#if defined(max)
	#undef max
#endif



/* 
 * Lists of matching and non-matching pairs and associated localization information: 
 */
#define MAX_FILENAME_LENGTH     128
typedef char FILENAME [MAX_FILENAME_LENGTH];

/**
 * Structure defining a pair of images, one being a query and the other a reference picture.
 */
typedef struct {
  FILENAME q;						///< query file descriptor 
  FILENAME r;						///< reference file descriptor
  BoundingBox q_bbox;				///< query bounding box
  BoundingBox r_bbox;				///< reference bounding box
  int q_height;						///< query image height
  int q_width;						///< query image width
} PAIR;


// data structures

PAIR matching_pair, non_matching_pair;

void usage()
{
    fprintf (stdout,
	  "CDVS descriptor matching module.\n"
	  "Usage:\n"
      "  match <matching-pairs> <non-matching-pairs> <mode> <dataset path> <annotation path>  [-j] [-t|x tracefile] [-r refmode] [-p paramfile] [-q queryparamfile] [-o] [-m N][-h]\n"
	  "Parameters:\n"
	  "  matching-pairs - list of matching pairs of images\n"
	  "  non-matching-pairs - list of non-matching pairs of images\n"
	  "  mode (0..n) - the decoding mode to use (described in the parameters file)\n"
      "  dataset path - the root dir of the CDVS dataset of images\n"
      "  annotation path - the root dir of the CDVS annotation files\n"
	  "Options:\n"
	  "  -j -jaccard : test accuracy of localization of matches using the full Jaccard index\n"
#ifdef USE_OLD_LOCALIZATION
	  "  -l -localize: test accuracy of localization of matches using a constrained Jaccard index\n"
#endif
	  "  -t -trace filename: trace all results in a text file\n"
	  "  -x -xmltrace filename: trace all results in an XML file\n"
	  "  -r -refmode : reference mode (if different from query mode)\n"
      "  -p -params parameters: text file containing initialization parameters for all modes\n"
      "  -q -queryparams parameters: text file containing initialization parameters for queries (not for references - used only in interoperability tests)\n"
      "  -o -oneway: use one-way local matching (instead of two-way local matching which is the default)\n"
      "  -m -matchtype N: use match type N, where N can be one of the following values:\n"
      "     0: ignore global if local match\n"
	  "     1: compute both local and global matching scores\n"
	  "     2: compute only local matching score\n"
	  "     3: compute only global matching score\n"
	  "  -h -help: show help\n");
    exit (1);
}


/**
 * @file
 * match.exe: CDVS matching module.
 * Computes the probability (score) that two images share the same visual object.
 * This is done for all image pairs given in input.
 * @verbatim
 
   Usage:
        match <matching-pairs> <non-matching-pairs> <mode> <dataset path> <annotation path> [-j] [-t|x tracefile] [-r refmode] [-p paramfile] [-q queryparamfile] [-o] [-h]
   Parameters:
        matching-pairs - list of matching pairs of images
        non-matching-pairs - list of non-matching pairs of images
        mode (0..n) - the decoding mode to use (described in the parameters file)
        dataset path - the root dir of the CDVS dataset of images
        annotation path - the root dir of the CDVS annotation files
   Options:
		-j -jaccard : test accuracy of localization of matches using the full Jaccard index
		-l -localize - test accuracy of localization of matches using a constrained Jaccard index. When this option is specified, the program reads localization annotation files (*.bbox files with same names/locations as descriptors),
					  then calls match_and_localize() function, which projects bounding quadrilateral from the second image to the first one, and compares it with the ground truth data. This test is performed only for matching pairs.
		-t -trace filename: trace all results in a trace file
		-x -xmltrace filename: trace all results in an XML file
		-r -refmode mode: reference mode (if different from query mode)
		-p -params parameters: text file containing initialization parameters for all modes
		-q -queryparams parameters: text file containing initialization parameters for queries only (not for references - used only in interoperability tests)
        -o -oneway: use one-way local matching (instead of two-way local matching which is the default)
        -m -matchtype n: use match type n, where n can be one of the following values:
           0: ignore global if local match
           1: compute both local and global matching scores
           2: compute only local matching score
           3: compute only global matching score
		-h -help: show help
   
   @endverbatim
 */
int run_match(int argc, char *argv[])
{
	TraceManager traceMgr;
	int matchType = MATCH_TYPE_DEFAULT;

	/* timing variables: */
	HiResTimer timer;
	double duration, max_duration, total_duration, average_duration;

	/* match score: */
	int TP, FP, TN, FN;

	/* localization accuracy test variables: */
	int localization_test = 0;
	BoundingBox pr_bbox;
	double overlap_ratio, acc_overlap_ratio, average_overlap_ratio;
	double acc_tp_overlap_ratio, average_tp_overlap_ratio; // computed only for true positives

	double acc_local_overlap_ratio = 0.0;		// computed only for true positive with local descriptors
	int acc_local_overlap_counter = 0;		// computed only for true positive with local descriptors


	/* check if sufficient # of arguments were provided: */
	if (argc < 6)
	  usage();

	const char * matching_pairs = argv[1];
	const char * non_matching_pairs = argv[2];
	int query_mode = atoi(argv[3]);	// set query mode
	const char * datasetPath = argv[4];
	const char * annotationPath = argv[5];
	const char * paramfile = NULL;
	const char * qparamfile = NULL;
	bool useTwoWayMatching = true;

	int ref_mode = query_mode;		// set default reference mode

	argv += 5;	// skip the first 5 params
	argc -= 5;	// skip the first 5 params

	// parse other params
	while ((argc > 1) && (argv[1][0] == '-'))
	{
		int n = 1;
		switch (argv[1][1])
		{
			case 'h': usage(); break;
			case 'l': localization_test = 1; break;
			case 'j': localization_test = 2; break;
			case 'r': ref_mode = atoi(argv[2]); n = 2; break;
			case 't': traceMgr.openTxt(argv[2]); matchType = MATCH_TYPE_BOTH; n = 2; break;
			case 'x': traceMgr.openXml(argv[2]); matchType = MATCH_TYPE_BOTH; n = 2; break;
			case 'p': paramfile = argv[2]; n = 2; break;
			case 'q': qparamfile = argv[2]; n = 2; break;
			case 'm': matchType = atoi(argv[2]); n = 2; break;
			case 'o': useTwoWayMatching = false; break;
			default : cerr << "wrong argument: " << argv[1] << endl; usage(); break;
		}
		argv += n;
		argc -= n;
	}

	// check args

	if ((matchType < 0) || (matchType > 3))
		usage();

	/* read file list: */

	FileManager fm_matching;
	fm_matching.setAnnotationPath(annotationPath);
	size_t n_matching_pairs = fm_matching.readAnnotation(matching_pairs);
	//FileManager fm_non_matching;
	//fm_non_matching.setAnnotationPath(annotationPath);
	//size_t n_non_matching_pairs = fm_non_matching.readAnnotation(non_matching_pairs);

	/* read lists of matching and non-matching pairs: */

	fm_matching.setDatasetPath(datasetPath);
	//fm_non_matching.setDatasetPath(datasetPath);

	/* create an instance of CdvsServer */

	CdvsConfiguration * cdvsconfig = CdvsConfiguration::cdvsConfigurationFactory(paramfile);	// if paramfile is NULL, use default values
	CdvsServer * cdvsserver = CdvsServer::cdvsServerFactory(cdvsconfig, useTwoWayMatching);
	char queryExt[40];

	/* get extension: */
	const char * ext = cdvsconfig->getParameters(query_mode).modeExt;
	const char * ref_ext = cdvsconfig->getParameters(ref_mode).modeExt;

	/* check if queries have a different extension (for interoperability tests only) */
	if (qparamfile != NULL)
	{
		CdvsConfiguration * queryconfig = CdvsConfiguration::cdvsConfigurationFactory(qparamfile);
		memcpy(queryExt, queryconfig->getParameters(query_mode).modeExt, sizeof(queryExt));
		ext = queryExt;			// use a different extension (for interoperability tests)
		delete queryconfig;		// all other parameters must be the same
	}

	/* zero counters: */
	TP = FP = TN = FN = 0;
	acc_overlap_ratio = 0.; acc_tp_overlap_ratio = 0.;
	max_duration = total_duration = 0.;

	traceMgr.start("trace");

	  /*
	   * Process matching pairs:
	   */
	  cout << "Processing matching pairs: (" << n_matching_pairs << " pairs, list: " << matching_pairs << ")" << endl;
	  traceMgr.start("matching_pairs");

	  for (size_t i=0; i<n_matching_pairs; i++)
	  {
		/* read descriptors: */
		string query_name = fm_matching.getQueryName(i);
		string ref_name = fm_matching.getReferenceName(i);
		string q_fname = fm_matching.replaceExt(query_name, ext);
		string r_fname = fm_matching.replaceExt(ref_name, ref_ext);

		PointPairs matchingPairs;

		/* progress report: */
		//cout << "[" << i+1 << "/" << n_matching_pairs << "]: " << q_fname << " vs "<< r_fname;
		traceMgr.start("pair");
		traceMgr.matchPair(q_fname.c_str(), r_fname.c_str());

		/* start timer: */
		timer.start();

		CdvsDescriptor query, reference;
		cdvsserver->decode(query, q_fname.c_str());			// decode query descriptor
		cdvsserver->decode(reference, r_fname.c_str());		// decode reference descriptor

		// read mode id
		if (query.getModeID() != query_mode)
			throw CdvsException(string("wrong mode id in query descriptor ").append(q_fname));

		if (reference.getModeID() != ref_mode)
			throw CdvsException(string("wrong mode id in reference descriptor ").append(r_fname));

		/* perform descriptor matching: */
		if (localization_test)
		{
			string q_bbox_fn = fm_matching.replaceExt(query_name, ".bbox");
			string r_bbox_fn = fm_matching.replaceExt(ref_name, ".bbox");
			/* read bounding boxes: */
			matching_pair.q_bbox.read(q_bbox_fn.c_str());
			matching_pair.r_bbox.read(r_bbox_fn.c_str());

#ifdef USE_OLD_LOCALIZATION
			read_dimensions (query_name.c_str(), &matching_pair.q_height, &matching_pair.q_width);
#endif
			matchingPairs = cdvsserver->match(query, reference, matching_pair.r_bbox.getAddress(), pr_bbox.getAddress(), matchType);
		}
		else
			matchingPairs = cdvsserver->match(query, reference, NULL, NULL, matchType);

		/* stop timer: */
		timer.stop();

		traceMgr.matchResults(matchingPairs, localization_test? pr_bbox.getAddress(): NULL, i);	// log data to trace file

		traceMgr.stop("pair");

		/* compute duration: */
		duration = timer.elapsed();
		if (duration > max_duration) max_duration = duration;
		total_duration += duration;

		/* analyze score: */
		if (matchingPairs.score > 0.5)  TP ++; /* true positive */
		else              FN ++; /* false negative */

		/* additional analysis and reports: */
		if (localization_test) {

		  /* compute overlap ratio: */
#ifdef USE_OLD_LOCALIZATION
		  if (localization_test == 1)
			  overlap_ratio = BoundingBox::cfind_overlap (matching_pair.q_width, matching_pair.q_height, matching_pair.q_bbox.getAddress(), pr_bbox.getAddress());
		  else if (localization_test == 2)
#endif
		  overlap_ratio = matching_pair.q_bbox.find_overlap(pr_bbox);

		  acc_overlap_ratio += overlap_ratio;

		  if (matchingPairs.score > 0.5)
			  acc_tp_overlap_ratio += overlap_ratio;

		  if ((matchingPairs.score > 0.5) && (matchingPairs.hasLocalizationInliers()))
		  {
			  acc_local_overlap_ratio += overlap_ratio;
			  acc_local_overlap_counter++;
		  }

		  /* report including localization ratio: */
		  //fprintf (stdout, " -> %g [%g], %g [s]\n", matchingPairs.score, overlap_ratio, duration);
		} else {
		  /* report: */
		  //fprintf (stdout, " -> %g, %g [s]\n", matchingPairs.score, duration);
		}
	  }

	  /* report localization precision / complexity numbers: */
	  if (localization_test) {
		average_duration = total_duration / (double) n_matching_pairs;
		average_overlap_ratio = acc_overlap_ratio / (double) n_matching_pairs;
		double average_local_ratio = acc_local_overlap_counter > 0 ? acc_local_overlap_ratio / (double) acc_local_overlap_counter : 0.0;
		average_tp_overlap_ratio = (!TP)? 0.: acc_tp_overlap_ratio / (double) TP;

		/*printf ("Localization test results:\n");
		printf ("Average (for true positives) overlap ratio: %g\n", average_tp_overlap_ratio);
		printf ("Average (for pairs matched by local descriptors) overlap ratio: %g\n", average_local_ratio);
		printf ("Average (for all matching pairs) overlap ratio: %g\n", average_overlap_ratio);

		printf ("Average descriptor matching & localization time: %g [s]\n", average_duration);
		printf ("Worst-case descriptor matching & localization time: %g [s]\n", max_duration);*/
	  }

	  traceMgr.stop("matching_pairs");

	  /*
	   * Process non-matching pairs:
	   */
	  //cout << "Processing non-matching pairs: (" << n_non_matching_pairs << " pairs, list: " << non_matching_pairs << ")" << endl;
	  //traceMgr.start("non_matching_pairs");

	  //for (size_t i=0; i<n_non_matching_pairs; i++)
	  //{
		/* read descriptors: */
		//string query_name = fm_non_matching.getQueryName(i);
		//string ref_name = fm_non_matching.getReferenceName(i);
		//string q_fname = fm_non_matching.replaceExt(query_name, ext);
		//string r_fname = fm_non_matching.replaceExt(ref_name, ref_ext);

		/* progress report: */
		//cout << "[" << i+1 << "/" << n_non_matching_pairs << "]: " << q_fname << " vs "<< r_fname;
		//traceMgr.start("pair");
		//traceMgr.matchPair(q_fname.c_str(), r_fname.c_str());

		/* start timer: */
		//timer.start();

		//CdvsDescriptor query, reference;
		//cdvsserver->decode(query, q_fname.c_str());			// decode query descriptor
		//cdvsserver->decode(reference, r_fname.c_str());		// decode reference descriptor

		// read mode id
		//if (query.getModeID() != query_mode)
		//	throw CdvsException(string("wrong mode id in query descriptor ").append(q_fname));

		//if (reference.getModeID() != ref_mode)
		//	throw CdvsException(string("wrong mode id in reference descriptor ").append(r_fname));

		/* perform descriptor matching: */
		//PointPairs matchingPairs = cdvsserver->match(query, reference, NULL,NULL, matchType);

		/* stop timer: */
		//timer.stop();

		//traceMgr.matchResults(matchingPairs, NULL, i);		// log data to trace file
		//traceMgr.stop("pair");

		/* compute duration: */
		//duration = timer.elapsed();
		//if (duration > max_duration) max_duration = duration;
		//total_duration += duration;

		/* analyze score: */
		//if (matchingPairs.score > 0.5)  FP ++; /* false positive */
		//else              TN ++; /* true negative */

		/* report: */
		//fprintf (stdout, "-> %g, %g [s]\n", matchingPairs.score, duration);
	  //}

	  /* compute average time / descriptor: */
	  //average_duration = total_duration / (double)(n_matching_pairs + n_non_matching_pairs);

	  //traceMgr.stop("non_matching_pairs");
	  //traceMgr.stop("trace");

	  delete cdvsserver;		// destroy the CDVS Server instance
	  delete cdvsconfig;		// destroy the CDVS configuration instance

	  /*
	   * Produce report:
	   */
	  /*printf ("Matching accuracy results:\n");
	  printf ("True positive rate: %g\n", (double)TP / (double)(TP+FN));
	  printf ("False positive rate: %g\n", (double)FP / (double)(FP+TN));
	  printf ("True negative rate: %g\n", (double)TN / (double)(FP+TN));
	  printf ("\n");
	  printf ("Average descriptor matching time: %g [s]\n", average_duration);
	  printf ("Worst-case descriptor matching time: %g [s]\n", max_duration);*/
	  return 0;
}

//  ----- main ------

int main(int argc, char *argv[]) {
	try {
		run_match(argc, argv);		// run "match" catching any exception
	}
	catch(exception & ex)				// catch any exception, including CdvsException
	{
	    cerr << argv[0] << " exception: " << ex.what() << endl;
	}
	return 0;
}

/* match.cpp -- end of file */
