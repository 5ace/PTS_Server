/*
 * MPEG Compact Desctiptors for Visual Search (CDVS) evaluation framework.
 *
 * map.c -- Mean Average Precision computation
 *
 ***********************************
 *
 * This software module was originally developed by:
 *
 *   <list of original developers>
 *
 * in the course of development of ISO/IEC <number> (Compact Descriptors for Visual 
 * Search) standard for reference purposes and its performance may not have been 
 * optimized. This software module includes implementation of one or more tools as 
 * specified by the ISO/IEC <number> standard.
 *
 * ISO/IEC gives you a royalty-free, worldwide, non-exclusive, copyright license to copy, 
 * distribute, and make derivative works of this software module or modifications thereof 
 * for use in implementations of the ISO/IEC <number> standard in products that satisfy 
 * conformance criteria (if any).
 *
 * Those intending to use this software module in products are advised that its use may 
 * infringe existing patents. ISO/IEC have no liability for use of this software module 
 * or modifications thereof.
 *
 * Copyright is not released for products that do not conform to audiovisual and image-
 * coding related ITU Recommendations and/or ISO/IEC International Standards.
 *
 ****** Section to be removed when the standard is published **************************
 *
 * Assurance that the originally developed software module can be used
 *  (1) in the ISO/IEC <number> standard once this standard has been adopted; and
 *  (2) to develop the ISO/IEC <number> standard:
 *
 * <Original developers> grant ISO/IEC all rights necessary to include the originally 
 * developed software module or modifications thereof in the ISO/IEC <number> standard 
 * and to permit ISO/IEC to offer You a royalty-free, worldwide, non-exclusive, copyright 
 * license to copy, distribute, and make derivative works for use in implementations of 
 * the ISO/IEC <number> standard in products that satisfy conformance criteria (if any), 
 * and to the extent that such originally developed software module or portions of it 
 * are included in the ISO/IEC <number> standard.
 *
 * To the extent that <original developers> own patent rights that would be required 
 * to make, use, or sell the originally developed software module or portions thereof 
 * included in the ISO/IEC <number> standard in a conforming product, the <original 
 * developers> will assure the ISO/IEC that they are willing to negotiate licenses under 
 * reasonable and non-discriminatory terms and conditions with applicants throughout 
 * the world.
 *
 * ISO/IEC gives You a free license to this software module or modifications thereof 
 * for the sole purpose of developing the ISO/IEC <number> standard.
 *
 ****** End of section to be removed when the standard is published *******************
 *
 * <original developers> retain full rights to modify and use the code for their own 
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
#include <string.h>
#include <math.h>


#if defined(_WIN32) || defined(_WIN64)
#define strcasecmp _stricmp // use _stricmp() in Windows
#endif

#include "map.h"
/*#include <string>
using namespace std;*/

/**
 * Success rate for top match.
 * @param truth ground truth
 * @param results ranked lists of results
 * @param n number of records (queries)
 * @return the success rate
 */

double success_rate_for_top_match (QRA **truth, QRA **results, int n)
{
  double success_rate;
  int i, k, relevant = 0, queries = 0;
  /* scan queries: */
  for (i=0; i<n; i++) {
    /* sanity check: */
    if (!strcasecmp(truth[i]->query, results[i]->query)) {  // _strcasecmp ignores case in comparison
      /* do we have at least one match?	*/
      if (results[i]->n_matches >= 1) {
        /* check if top match is relevant: */
        for (k=0; k<truth[i]->n_matches; k++) {
          if (!strcasecmp(results[i]->matches[0], truth[i]->matches[k])) {
            relevant ++;
            break;
          }
        }
      }
      queries ++;
    }
  }
  /* compute success rate: */
  success_rate = (double) relevant;
  if (queries) success_rate /= (double) queries;
  return success_rate; 
}

/**
 * Mean Average Precision (simplified version)
 * @param truth ground truth
 * @param results ranked lists of results
 * @param n number of records (queries)
 * @return the mean average precision value
 */
double mean_average_precision (QRA **truth, QRA **results, int n)
{
  int relevant, queries = 0;
  double average_precision, MAP = 0.0;
  int i, j, k;

  /* scan records: */
  for (i=0; i<n; i++) {
    /* sanity check: */
    if (!strcasecmp(truth[i]->query, results[i]->query)) {  // _strcasecmp ignores case in comparison
      relevant = 0; 
      average_precision = 0.;
      /* scan results	*/
      for (j=0; j<results[i]->n_matches; j++) {
        /* scan ground truth */
        for (k=0; k<truth[i]->n_matches; k++) {
          if (!strcasecmp(truth[i]->matches[k], results[i]->matches[j])) {
            /* ground truth match found */
            relevant ++;
            average_precision += (double)relevant / (double) (j+1); // j+1 = rank
            break;
          }
        }
      }
      /* normalize average precision and add it to MAP:  */
      if (truth[i]->n_matches) average_precision /= truth[i]->n_matches;
      MAP += average_precision;
      queries ++;
    }
  }

  /* normalize and return MAP value: */
  if (queries) MAP /= (double) queries;
  return MAP;
}

/**
 * Mean Average Precision (allows results and annotations to be presented in arbitrary order / completeness)
 * @param truth ground truth
 * @param results ranked lists of results
 * @param n_truth number of records available in ground truth
 * @param n_results number of records available in retrieval result
 * @return the mean average precision value
 */
double mean_average_precision_2 (QRA **truth, QRA **results, int n_truth, int n_results)
{
  double average_precision, MAP = 0.;
  int relevant, queries = 0;
  int i, j, k, l;

  /* scan query images that present in both lists: */
  for (i=0; i<n_results; i++) for (j=0; j<n_truth; j++) {
    if (!strcasecmp(truth[j]->query, results[i]->query)) {
      relevant = 0;
      average_precision = 0.0;
      /* scan results	*/
      for (l=0; l<results[i]->n_matches; l++) {
        /* scan ground truth */
        for (k=0; k<truth[j]->n_matches; k++) {
          if (!strcasecmp(truth[j]->matches[k], results[i]->matches[l])) {
            /* ground truth match found */
            relevant ++;
            average_precision += (double) relevant / (double) (l+1);  // l+1 = rank
            break;
          }
        }
      }
      /* normalize average precision and add it to MAP:  */
      if (relevant) average_precision /= truth[j]->n_matches;
      MAP += average_precision;
      queries ++;
    }
  }
  /* normalize and return MAP value: */
  if (queries) MAP /= (double) queries;
  return MAP;
}

/*!
 * \brief Read annotations:
 * \param fn - name of file with annotations
 * \param list - pointer to a structure to contain annotations
 * \param max_n - max number of records to read
 * \return the number of records read
*/
int read_qra_list (char *fn, QRA **list, int max_n)
{
  static char buf[(MAX_MATCHES+1)*(MAX_FILENAME_LENGTH+1)];   // line buffer
  FILE *file; char *p, *q, query[300], *tmp;
  int i, j, num;

  /* open file: */
  if ((file = fopen (fn, "rt")) == NULL) {
    fprintf (stderr, "Cannot open file: %s\n", fn);
    exit (1);
  }

  /* scan records: */
  for (i=0; i<max_n; i++) {

skip:
    /* read text line: */
    if (fgets(buf, sizeof(buf), file) == NULL)
      break;

    /* skip empty lines */
    if (!strlen(buf) || !strcmp(buf,"\n"))
      goto skip;

    /* allocate QRA structure: */
    if (!list[i]) {
      if (!(list[i] = (QRA*) malloc (sizeof(QRA)))) {
        fprintf (stderr, "Out of memory\n");
        exit (1);
      }
    }

    /* extract query descriptor name */
    if ((q = strpbrk(buf, ",;\t")) != NULL) *q = ' '; // allow comma, semicolon, and tab- separators
    if (sscanf (buf, "%[^\t\n]", list[i]->query) != 1) {
      fprintf (stderr, "Cannot read query descriptor name in line %d, file %s\n", i, fn);
      exit (1);
    }

	//myself
	strcpy(query, list[i]->query);
	tmp = query;
	num = 0;
	while (*tmp != '.')
	{
		num++;
		tmp++;
	}
	num += 3;
	query[num+1] = '\0';
	strcpy(list[i]->query, query);

	/*string tmp = list[i]->query;
	tmp = tmp.substr(0, tmp.find_first_of(".")+4);
	strcpy(list[i]->query, tmp.c_str());*/
	//fprintf (stderr, "list[i]->query: %s/%d", list[i]->query, num); //????
    /* scan matching files: */
    p = strstr(buf,list[i]->query) + strlen(list[i]->query);
    for (j=0; j<MAX_MATCHES; j++) {
      if ((q = strpbrk(p, ",;\t")) != NULL) *q = ' '; // allow comma, semicolon, and tab- separators
      if (sscanf (p, "%s", list[i]->matches[j]) != 1) break;
      p = strstr(p,list[i]->matches[j]) + strlen(list[i]->matches[j]); // shift pointer
    }

    /* save number of matches */
    list[i]->n_matches = j;
//  if (j == 0 || j == MAX_MATCHES) {	// MB: this check breaks the 4_retrieval experiment! commented out by TI
    if (j == 0) {
      fprintf (stderr, "Cannot read (all) matches listed in line %d, file %s\n", i, fn);
      exit (1);
    }
  }
  /* close file: */
  fclose(file);

  /* return # of records read: */
  if (i > max_n)
    fprintf(stderr, "Can't read all records in %s\n", fn);
  return i;
}

/*!
 * \brief Write retrieval results:
 * \param fn - file name
 * \param list - list of results
 * \param n - number of queries / results
 */
void write_qra_list (char *fn, QRA **list, int n)
{
  FILE *file; int i, j;

  /* open text file: */
  if ((file = fopen (fn, "wt+")) == NULL) {
    fprintf (stderr, "Can't open/create output file: %s\n", fn);
    exit (1);
  }

  /* write things: */
  for (i=0; i<n; i++) {
    fprintf (file, "%s\t", list[i]->query);
    for (j=0; j<list[i]->n_matches; j++)
      fprintf (file, "%s\t", list[i]->matches[j]);
    fprintf (file, "\n");
  }

  /* close file and exit: */
  fclose(file);
}

/*!
 * \brief allocate FRA lists:
 * \param list - list of results
 * \param n - number of queries / results
 */
void alloc_qra_list (QRA **list, int n)
{
  int i;
  for (i=0; i<n; i++) {
    if (!(list[i] =(QRA*) calloc (1, sizeof(QRA)))) {
      fprintf (stderr, "Out of memory\n");
      exit (1);
    }
  }
}

/*!
 * \brief FRA list:
 * \param list - list of results
 * \param n - number of queries / results
 */
void free_qra_list (QRA **list, int n)
{
  int i;
  for (i=0; i<n; i++) {
    if (list[i]) {
      free(list[i]);
      list[i] = 0;
    }
  }
}

/* map.c -- end of file */
