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

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <fstream>
#include <algorithm>
#include <vector>

#define Shen 1

#if defined(_WIN32) || defined(_WIN64)
#include <io.h>
#pragma warning(disable : 4996) // ignore MSVC's concerns about safety of IO library
#endif

/* mean average precision & related definitions: */
#include "map.h"
#include "FileManager.h"
#include "HiResTimer.h"		/* high-resolution timer: */
#include "CdvsException.h"
#include "CdvsInterface.h"

//#ifdef _OPENMP
//  #include <omp.h>		// use OpenMP multithreading if supported
//#endif

using namespace std;
using namespace mpeg7cdvs;


/* 
 * Query -> Retrieval results lists:
 *  QRA structure is defined in file libraries/map/map.h 
 */

/* ground truth & actual resuls: */
QRA **ground_truth;        // ground truth
QRA **retrieval_results;   // actual ranked lists of results that we retrieve
int nqueries = 0;

struct merchandise
{
	string name;
	int freq;
};

bool struct_cmp_by_freq(merchandise a, merchandise b)
{
    return a.freq > b.freq;
}


void usage()
{
    cout <<
      "CDVS retrieval module.\n"
      "Usage:\n"
	  "  retrieve <index> <ground-truth-annotations> <mode> <datasetPath> <annotationPath> [-o] [-p paramfile] [-t tracefile]\n"
      "Where:\n"
      "  index - database index file to be used for retrieval\n"
      "  ground-truth-annotations - query images and corresponding (ground truth)\n"
      "        matches in the database; this is a text file, containing per each line,\n"
      "        the following records:\n"
      "           <query image> <matching image 1> ... <matching image N>\n"
      "  mode (0..n) - sets the encoding mode to use (described in the parameters file)\n"
      "  dataset path - the root dir of the CDVS dataset of images\n"
      "  annotation path - the root dir of the CDVS annotation files\n"
      "Options:\n"
      "  -o -oneway: use one-way matching (instead of two-way matching which is the default)\n"
      "  -p paramfile: text file containing initialization parameters for all modes\n"
	  "  -t -tracefile: trace all results in a trace file containing \n"
      "         ranked lists of results retrieved per each query;\n"
      "         it must be in same format as ground truth data.\n"
 	  "  -help or -h: help\n";
    exit (EXIT_FAILURE);
}

/**
 * @file 
 * retrieve.exe: CDVS Retrieval module.
 * Uses the CDVS database to retrieve images similar to that given in input as query image.
 * Repeats the process for all query images given in input.
 * @verbatim   

   usage:
	  retrieve <index> <ground-truth-annotations> <mode> <datasetPath> <annotationPath> [-o] [-p paramfile] [-t tracefile]
   where:
      index - database index file to be used for retrieval
      ground-truth-annotations - query images and corresponding (ground truth)
			matches in the database; this is a text file, containing per each line
			the following records:
                 <query image> <matching image 1> ... <matching image N>
      mode (0..n) - sets the encoding mode to use (described in the parameters file)
      dataset path - the root dir of the CDVS dataset of images
      annotation path - the root dir of the CDVS annotation files
      options:
      -o -oneway: use one-way matching (instead of two-way matching which is the default)
      -p paramfile: text file containing initialization parameters for all modes
      -t -tracefile: trace all results in a trace file containing
                ranked lists of results retrieved per each query;
                it must be in same format as ground truth data.
      -help or -h: help

 @endverbatim
 */
int run_retrieve (int argc, char *argv[])
{
	  // argv 0      1          2          3         4              5
	  // retrieve <index> <groundtruth> <mode> <datasetPath> <annotationPath> [-h]

	const char * trace = NULL;		// default: no trace file
	const char * paramfile = NULL;	// default: no parameters file

	/* query descriptor length stats: */
	size_t max_descriptor_length, total_descriptor_length;
	double average_descriptor_length;

	/* timing variables: */
	double max_duration, average_duration;

	/* check if sufficient # of arguments were provided: */
	if (argc < 8) { // original : argc < 6
	  usage();
	}

	/* read mandatory arguments */

	const char * dbname = argv[1];
	const char * annotationname = argv[2];
	int mode = atoi(argv[3]);
	const char * datasetPath = argv[4];
	const char * annotationPath = argv[5];
	const string top = argv[6];
	const string threshold = argv[7];
	const int IsNumberOne = atoi(argv[8]);
	bool useTwoWayMatching = true;

	argv += 8;	// skip the first 5 params 
	argc -= 8;	// skip the first 5 params 

	// parse other arguments

	while ((argc > 1) && (argv[1][0] == '-'))
	{
		int n = 1;
		switch (argv[1][1])
		{
			case 'h': usage(); break;
			case 't': trace = argv[2]; n = 2; break;
			case 'p': paramfile = argv[2]; n = 2; break;
			case 'o': useTwoWayMatching = false; break;
			default : cerr << "wrong argument: " << argv[1] << endl; usage(); break;
		}
		argv += n;
		argc -= n;
	}

	/* read mandatory parameters */

	FileManager manager;
	manager.setAnnotationPath(annotationPath);
	nqueries = (int) manager.readAnnotation(annotationname);

	manager.setDatasetPath(datasetPath);

	/* create an instance of CdvsServer */
	CdvsConfiguration * cdvsconfig = CdvsConfiguration::cdvsConfigurationFactory(paramfile);	// if paramfile == NULL use default values
	CdvsServer * cdvsserver = CdvsServer::cdvsServerFactory(cdvsconfig, useTwoWayMatching);

	/* get extension: */
	const char * ext = cdvsconfig->getParameters(mode).modeExt;

	// allocate ground truth and result vectors
	ground_truth = (QRA **) calloc (nqueries , sizeof(QRA *));
	retrieval_results = (QRA **) calloc (nqueries , sizeof(QRA *));

	/* read ground truth annotations: */
	char annotationpathname[512];
	sprintf(annotationpathname, "%s/%s", annotationPath, annotationname);
	nqueries = read_qra_list (annotationpathname, ground_truth, nqueries);

	/* allocate structures for retrieval results: */
	alloc_qra_list (retrieval_results, nqueries);

	// read DB header - to get the DB mode id
	char databasename[512];
	sprintf(databasename, "%s/%s.local", datasetPath, dbname);

	/* load database index (do it after all checks, it's a long read): */
	sprintf(databasename, "%s/%s", datasetPath, dbname);
	cout << "Reading the database..." << endl;
	cdvsserver->loadDB(string(databasename).append(".local").c_str(), string(databasename).append(".global").c_str());
	cout << cdvsserver->sizeofDB() << " images loaded." << endl;

	/* zero counters: */
	max_duration = average_duration = 0.;
	max_descriptor_length = total_descriptor_length = 0;

	/*
	 * Execute query requests:
	 */
	printf ("Executing %d queries (given by %s)\n", nqueries, annotationname);
	fflush (stdout);

	// omp_set_num_threads(1);
	#pragma omp parallel default(shared)
	{
	double local_max_duration = 0.0;
	size_t local_max_dsc_len = 0;
	#pragma omp for reduction(+:average_duration,total_descriptor_length)
	for (int i=0; i<nqueries; i++) {
	  try {			// exceptions must also be handled inside each thread

		  /* timing variables: */
		  HiResTimer timer;
		  double duration;

		  string image = manager.getAbsolutePathname(i);
		  string dsc_fname = manager.replaceExt(image, ext);

		  CdvsDescriptor query;
		  vector<RetrievalData> results;

		  /* start timer: */
		  timer.start();

		  size_t qsize = cdvsserver->decode(query, dsc_fname.c_str());
		  int n = cdvsserver->retrieve(results, query, MAX_MATCHES);

		  /* stop timer: */
		  timer.stop();

		  // Write the results in retrieval_results

		  for(unsigned int k=0; k<results.size(); ++k)
		  {
			  string imgname = cdvsserver->getImageId(results[k].index);
			  imgname.copy(retrieval_results[i]->matches[k], imgname.size());
		  }


		  /* compute duration: */
		  duration = timer.elapsed();

//#if !defined (_OPENMP)
		  if (duration > local_max_duration) local_max_duration = duration;		// under race condition in the parallel version
		  if (qsize > local_max_dsc_len) local_max_dsc_len = qsize;
//#endif

		  /* update stats: */
		  average_duration += duration;
		  total_descriptor_length += qsize;

		  /* progress report: */
		  fprintf (stdout, "[%4d/%4d]: %s -> %d matches found, %g [s]\n", i+1, nqueries, ground_truth[i]->query, n, duration);

		  /* save query image and # of matches found */
		  strcpy(retrieval_results[i]->query, ground_truth[i]->query);
		  retrieval_results[i]->n_matches = n;
	  }
	  catch(exception & ex)				// catch any exception, including CdvsException
	  {
		  cerr << argv[0] << " exception: " << ex.what() << endl;
	  }
  }		// end parallel for
  #pragma omp critical 
  {
	  max_duration = std::max(max_duration, local_max_duration);
	  max_descriptor_length = std::max(max_descriptor_length, local_max_dsc_len);
  }
  }		// end parallel (only if compiled with -fopenmp)

  /* compute average stats: */
  average_duration /= (double) nqueries;
  average_descriptor_length = (double) total_descriptor_length / (double) nqueries;

  delete cdvsserver;
  delete cdvsconfig;

  /* save retrieval results:  */
  if (trace != NULL)
	  write_qra_list ((char *) trace, retrieval_results, nqueries);
  

  vector<merchandise> merchandiseClass;

  //execute match.exe
  //load predict(trace) file
  ifstream fp_predict(trace);
  string data, buf;

  int topNum = atoi(top.c_str());
  double threshold_d = atof(threshold.c_str());

  /* 2017/11/22 update */
  /* version 1 - find No.1 in single query then voting on all queries. */
  if ( IsNumberOne )
  {
	  string file_predictScore = "predictWithScore.txt";
	  ofstream fp_predictScore(file_predictScore.c_str());
	  //fp_predictScore.open(file_predictScore, std::ofstream::out | std::ofstream::app);
	  //fp_predictScore << "----------------------";
	  //fp_predictScore << "Show the top " << topNum << " of result and threshold is " << threshold_d << "." << "----------------------" << endl << endl;
  
	  while (getline(fp_predict, data))
	  {
		  stringstream ss(data); 
		  vector<string> imgName; 
		  while (ss >> buf)
			  imgName.push_back(buf);

		  //write matching-pair file
		  const string file_score = "score.txt";
		  const string file_matchPair = "matching_pair.txt";
		  ofstream fp_matchPair(file_matchPair.c_str());

		  for (int i = 1; i < topNum+1; i++)
			  fp_matchPair << imgName[0] << " " << imgName[i] << endl;

		  system(("match " + file_matchPair + " " + file_matchPair + " 6 " + datasetPath + " " + annotationPath + " -t " + file_score + " -m 0").c_str());
		  fp_matchPair.close();

		  ifstream fp_score(file_score.c_str());

		  getline(fp_score, data); //throw
		  getline(fp_score, data); //throw

		  fp_predictScore << imgName[0] << "/";

		  float max_score = threshold_d;
		  int max_index = -1;
		  for (int i = 0; i < topNum; i++)
		  {
			  getline(fp_score, data); // get useful info
			  stringstream ss(data); 
			  vector<string> score; 
			  while (ss >> buf)
				  score.push_back(buf);

			  if (atof(score[7].c_str()) >= max_score)
			  {
				  max_index = i;
				  max_score = atof(score[7].c_str());
			  }
		  }

		  if ( max_index == -1 )
			fp_predictScore << "NotFound" << endl;
		  else
			fp_predictScore << imgName[max_index + 1] << endl;

		  fp_score.close();
	  }
	  fp_predict.close();
	  fp_predictScore.close();

	  const string file_pWs = "predictWithScore.txt";
	  ifstream fp_pWs(file_pWs.c_str());
 
	  while (getline(fp_pWs, data))
	  {
		  string c = data.substr(data.find("/")+1);
		  c = c.substr(0, c.find("_"));
		  int i;
		  for (i=0; i<merchandiseClass.size(); i++)
			  if ( !c.compare(merchandiseClass[i].name) )
				  break;
		  if ( i >= merchandiseClass.size() )
		  {
			  merchandiseClass.push_back(merchandise());
			  merchandiseClass[i].name = c;
			  merchandiseClass[i].freq = 1;
		  }
		  else
			  merchandiseClass[i].freq++;
	  }

	  //get the most possible product name
	  int max = 0;
	  for (int i=1; i<merchandiseClass.size(); i++)
		  if ( merchandiseClass[i].freq > merchandiseClass[max].freq )
			  max = i;

	  string result_class;
	  if ( !merchandiseClass[max].name.compare("NotFound") )
		  result_class = "NotFound";
	  else
	  {
		  result_class = merchandiseClass[max].name.substr(0, merchandiseClass[max].name.find("_"));
		  //result_class = result_class.substr(0, result_class.find("-"));
	  }
	  ofstream fp_result_class("result_class");
	  fp_result_class << result_class;
  }
  /* version 2 - voting on top xx each query */
  else
  {
		//chenca
		//const string predict_image = "predict_image.txt";
		//ofstream fp_predict_im(predict_image.c_str());
		  
		const string image_score = "image_score.txt";
		ofstream fp_resultimage(image_score.c_str());  
	    /*end*/
	  while (getline(fp_predict, data))
	  {
		  stringstream ss(data); 
		  vector<string> imgName; 
		  while (ss >> buf)
			  imgName.push_back(buf);

		  //write matching-pair file
		  const string file_score = "score.txt";
		  const string file_matchPair = "matching_pair.txt";
		  ofstream fp_matchPair(file_matchPair.c_str());

		  for (int i = 1; i < topNum+1; i++){
			  fp_matchPair << imgName[0] << " " << imgName[i] << endl;
			  //fp_predict_im << imgName[i] <<" "; //chenca
		  }

		  system(("match " + file_matchPair + " " + file_matchPair + " 6 " + datasetPath + " " + annotationPath + " -t " + file_score + " -m 0").c_str());
		  fp_matchPair.close();

		  ifstream fp_score(file_score.c_str());

		  getline(fp_score, data); //throw
		  getline(fp_score, data); //throw

		  for (int i = 0; i < topNum; i++)
		  {
			  getline(fp_score, data); // get useful info
			  stringstream ss(data); 
			  vector<string> score; 
			  while (ss >> buf)
				  score.push_back(buf);

			  if (atof(score[7].c_str()) >= threshold_d)
			  {
				  string c = imgName[i+1];
				  fp_resultimage << c <<" "<<score[7]<< endl;/*chenca*/
				  c = c.substr(0, c.find("_"));
				  int i;
				  for (i=0; i<merchandiseClass.size(); i++)
					  if ( !c.compare(merchandiseClass[i].name) )
						  break;
				  if ( i >= merchandiseClass.size() )
				  {
					  merchandiseClass.push_back(merchandise());
					  merchandiseClass[i].name = c;
					  merchandiseClass[i].freq = 1;
				  }
				  else
					  merchandiseClass[i].freq++;
			  }
		  }
	  }
	  fp_predict.close();
	  fp_resultimage.close();/*chenca*/
	  //fp_predict_im.close();/*chenca*/
	  //////////////////////////////////////////////////////////////////////////////////////////
	  //get the most possible product name
	  string result_class;
	  if ( merchandiseClass.size() == 0 )
		  result_class = "NotFound";
	  else
	  {
		  int max = 0;
		  for (int i=1; i<merchandiseClass.size(); i++)
			  if ( merchandiseClass[i].freq > merchandiseClass[max].freq )
				  max = i;

			result_class = merchandiseClass[max].name.substr(0, merchandiseClass[max].name.find("_"));
			//result_class = result_class.substr(0, result_class.find("-"));
	  }

	  /*ofstream fp_result_class("result_class");
	  fp_result_class << result_class;*/
  }
  	/*
	if ( merchandiseClass.size() )
	{
		ofstream fp_class_freq("class_freq");
		sort(merchandiseClass.begin(), merchandiseClass.end(), struct_cmp_by_freq);

		int total = 0;
		for (int i=0; i<merchandiseClass.size(); i++)
			total += merchandiseClass[i].freq;
		for (int i=0; i<merchandiseClass.size(); i++)
			fp_class_freq << merchandiseClass[i].name << " / " << merchandiseClass[i].freq*1.0/total << endl; 
	}
	*/
  /*
   * Produce report:
   */
  /*printf ("Retrieval results for query list %s:\n", annotationname);
  printf ("\n");
  printf ("%d queries executed:\n", nqueries);
  printf ("Average query descriptor length: %g [bytes]\n", average_descriptor_length);
  cout << "Maximal query descriptor length: "<< max_descriptor_length << " [bytes]" << endl;
  printf ("\n");
  printf ("Mean Average Precision: %g\n", mean_average_precision (ground_truth, retrieval_results, nqueries));
  printf ("Success Rate for Top Match: %g\n", success_rate_for_top_match (ground_truth, retrieval_results, nqueries));
  printf ("\n");
  printf ("Average retrieval time: %g [s]\n", average_duration);
  printf ("Worst-case retrieval time: %g [s]\n", max_duration);*/

  free_qra_list(retrieval_results, nqueries);
  free_qra_list(ground_truth, nqueries);
  free(ground_truth);
  free(retrieval_results);
  return 0;
}

//  ----- main -------

int main(int argc, char *argv[])
{
	try {
		run_retrieve(argc, argv);		// run "extract" catching any exception
	}
	catch(exception & ex)				// catch any exception, including CdvsException
	{
	    cerr << argv[0] << " exception: " << ex.what() << endl;
	}
	return 0;
}

/* retrieve.cpp -- end of file */
