/* map.h -- Mean Average Precision computation */

#ifndef _MAP_H_
#define _MAP_H_ 1

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * Query -> Retrieval results associations (QRAs): 
 */
#define MAX_MATCHES 500    
#define MAX_FILENAME_LENGTH 128
typedef char FILENAME[MAX_FILENAME_LENGTH];

/**
 * Structure containing the name of a query image and the list of names of matching reference pictures.
 */
typedef struct {
  FILENAME query;					///< query image
  int n_matches;					///< number of matching images
  FILENAME matches[MAX_MATCHES];	///< list of matches
} QRA;

/* 
 * Retrieval precision measures (mean average precision and success rate for top match).
 * Parameters:
 *  truth - ground truth correspondences
 *  results - ranked lists of results
 *  n - number of records available (assumed to be the same in both lists)
 */
double mean_average_precision (QRA **truth, QRA **results, int n);
double success_rate_for_top_match (QRA **truth, QRA **results, int n);
double success_rate_for_top_match_2 (QRA **truth, QRA **results, int nt, int nr);
int read_qra_list (char *file, QRA **list, int max_n);
void write_qra_list (char *file, QRA **list, int n);
void alloc_qra_list (QRA **list, int n);
void free_qra_list (QRA **list, int n);

#ifdef __cplusplus
}
#endif

#endif // _MAP_H_
