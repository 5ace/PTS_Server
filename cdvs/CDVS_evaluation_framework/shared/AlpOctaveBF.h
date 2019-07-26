/*
 * This software module was originally developed by:
 *
 *   Peking University
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
 * Peking University retain full rights to modify and use the code for their own
 * purposes, assign or donate the code to a third party and to inhibit third parties
 * from using the code for products that do not conform to MPEG-related
 * ITU Recommendations and/or ISO/IEC International Standards.
 *
 * This copyright notice must be included in all copies or derivative works.
 * Copyright (c) ISO/IEC 2014.
 *
 */

#pragma once
#include "Feature.h"
#include "Parameters.h"
#include <ostream>
#include "AlpOctave.h"
#include "fftw3.h"
#include <cstring>

namespace mpeg7cdvs
{

//#define PADDING_MIRROR
//#define PADDING_ZERO
#define PADDING_REPEAT

//#define SPATIAL_FILTERING		//switch for using spatial domain filtering

#define BLOCK_WIDTH 96			/*The Block Size*/
#define MAX_FILTER_WIDTH 33		/*The Maximum Width of the Filters*/
#define PAD_WIDTH 16			/*The pad width*/
#define FBLOCK_WIDTH 128
#define FHWIDTH 65
#define BLOCK_SIZE 9216

#define MODE_LEFT	1
#define MODE_TOP	2
#define MODE_RIGHT	4
#define MODE_BOTTOM	8
#define MODE_FINISH	16

#define LOG_OF_2 0.693147180559945f
#define PI 3.141592653589793f
#define PI2 6.283185307179586f
#define HALF_PI 1.5707963267948965
#define QUTER_PI 0.78539816339744831f
#define THR_QUTER_PI 2.3561944902f
#define BPO_P_PI2 1.273239544735162686151f
#define EPSILON_F 1.19209290E-07F
#define EPSILON_D 2.220446049250313e-16
#define MIN(x,y) (((x)<(y))?(x):(y))
#define MAX(x,y) (((x)>(y))?(x):(y))
#define SHIFT_LEFT(x,n) (((n)>=0)?((x)<<(n)):((x)>>-(n)))

#define FILTER_LOG 0
#define FILTER_GAUSSIAN 1

typedef float input_type;
typedef float gs_type;
typedef float element_type; /*The LoG response image and Scaled image element type */
typedef float filter_type;	/*Filters' coefficient type*/

/**
 * @class Filter
 * A class containing a separable Gaussian and LoG filter kernel in frequency domain.
 */
class FrequencyFilter
{
private:

	typedef struct _node
	{
		const filter_type * data;		//no-zero data of the filter
		const unsigned char * mask;		//mask where 1 indicates the no-zero value
		const int * mask_pos;			//mask position for different scale
		const int * pos;				//data position for different scale
	} node;

	//LoG filters for the first octave
	static const int g_flog_maskpos0[];
	static const unsigned char g_flog_mask0[];
	static const int g_flog_pos0[];
	static const filter_type g_flog_data0[];
	//Gaussian filters for the first octave
	static const int g_fgaussian_maskpos0[];
	static const unsigned char g_fgaussian_mask0[];
	static const int g_fgaussian_pos0[];
	static const filter_type g_fgaussian_data0[];
	//LoG filters for the second octave
	static const int g_flog_maskpos1[];
	static const unsigned char g_flog_mask1[];
	static const int g_flog_pos1[];
	static const filter_type g_flog_data1[];
	//Gaussian filters for the second octave
	static const int g_fgaussian_maskpos1[];
	static const unsigned char g_fgaussian_mask1[];
	static const int g_fgaussian_pos1[];
	static const filter_type g_fgaussian_data1[];//*/

	static const unsigned char bits[8];

	static const int fblock_size;
	static const int imageSize;
	static const float fft_scale_factor;
	static const int total_memory;

	static void complex_mutil(fftwf_complex *srcA, const node srcB, fftwf_complex *dest,size_t size, int fwidth, float factor);

	void Init();
	void Release();

public:
	FrequencyFilter();
	virtual ~FrequencyFilter();

	float * inmat;
	fftwf_complex *fblock;			/**< current frequency block data. */
	fftwf_complex *fblock_flog;		/**< current frequency block log data. */

	fftwf_plan fftplan;				/**< pointer to plans used for fft*/
	fftwf_plan ifftplan;			/**< pointer to plans used for ifft*/
	
	void Convolution(int o_cur, int type, int loc);

	static void open_lib();		///< to be called once at the beginning of main
	static void close_lib();		///< to be called once at the end of main

};

/**
 * @class FeatureAlpBF
 * Inherits all member variables declared in FeatureAlp and adds two new member variables.
 */
class FeatureAlpBF: public FeatureAlp
{
public:
	char mode;			/**< point mode */
	char block_id;		/**< detected in the block id */
};

/**
 * @class AlpOctaveBF
 * A container class for a single octave of an image, at a given scale, used
 * to detect and extract ALP key points.
 * @author Massimo Balestri
 * @date 2013
 */
class AlpOctaveBF
{
private:
	static const int minSize = 20;					// minimum size of the octave
	static const int nsigmas = 4;					// number of different filtered images
	static const int nlevels = nsigmas - 2;		// intermediate levels between filtered images
	static const double magnification;			// feature size magnification factor
	static const double sigma0;					// base sigma
	static const double sigman;					// sigma of the original image
	static const double sigmak;					// ratio between sigmas
	static const double dsigma0;					// base differential sigma
	static const float sigmas[];					// sigma values for G1,G2,G3,G4
	static const float inter_sigmas[];			// intermediate sigma values

	static const float peakThreshold;				// minimum peak value
	static const Filter o1g1_filter;				// filter for G1 of octave 1
	static const Filter g2_filter;					// filter for G2 of all octaves
	static const Filter g3_filter;					// filter for G3 of all octaves
	static const Filter g4_filter;					// filter for G4 of all octaves

	static const float lowSigma;					// min allowed sigma
	static const float highSigma;					// max allowed sigma
	static const float sigmaThreshold;			// threshold for the second derivative
	static const float curvRatioThreshold;		// curvature ratio threshold
	static const float sigmaMerge;				// sigma merge threshold
	static const float radiusMerge;				// radius merge threshold
	static const float maxDisplacement;			// maximum displacement of the x and y coordinate
	static const float qcoeff[4][54];				// coefficients of the qcoeff matrix for all sigmas

	AlpOctaveBF (const AlpOctaveBF&);					// disallow copy
	AlpOctaveBF & operator= (const AlpOctaveBF&);		// disallow assignment

	void conv2(const float * imagein, const Filter & filter, float * imageout) const;	 // same as MATLAB conv2 function
	static void print(const Feature & f, float ratio);
	bool isMax(int k, float * R) const;
	bool isMin(int k, float * R) const;
	static int getClosestIndex(float scale);
	bool computeCurvRatioAndCoordinates(FeatureAlp & d) const;
	void computeResponse(int startLineSrc, int startLineDst, int nLines);
	void shiftUp();
	static bool isDuplicate(const Feature & a);		// is the keypoint marked as duplicate?

	// member variables
	int height;				// height of this octave
	int width;				// width of this octave
	int octave;				// octave number
	int capacity;			// the size of the allocated memory buffers

	float * G1;		// first Gaussian
	float * G2;		// second Gaussian
	float * G3;		// third Gaussian
	float * G4;		// fourth Gaussian

	float * L1;		// first Laplacian
	float * L2;		// second Laplacian
	float * L3;		// third Laplacian
	float * L4;		// fourth Laplacian

	float * A;		// first coefficient
	float * B;		// second coefficient
	float * C;		// third coefficient
	float * D;		// fourth coefficient

	float * tmp;			// temporary buffer

	float * minResponse;	// temporary buffer
	float * maxResponse;	// temporary buffer
	float * minScale; 		// temporary buffer
	float * maxScale;		// temporary buffer

	float * grad;					//gradient (magnitude and orientation) of the keypoin's neiboring patch
	float * log_gradient_buffer;	//buffers for block-based LoG and the gradient map
	gs_type * block_buffer;			//block buffer to store the blocks for descriptor computing
	input_type * next_octave_input;	//buffer to store the next octave input image
	

public:

	AlpOctaveBF();					///< constructor

	~AlpOctaveBF();					///< destructor

	std::vector<FeatureAlpBF> keypoints;					///< raw key points detected in this octave (without orientation) // Keundong Lee, moved to public


	bool empty () const;				///< return true if the AlpOctave is empty

	/**
	 * Detect all ALP keypoints from this octave.
	 * @throws CdvsException in case of error
	 */
	void detect();

	/**
	 * Low-memory version of detect duplicates.
	 * @param featurelist the list of keypoints already detected in the previous octaves.
	 */
	void detectDuplicates(FeatureList& featurelist);

	/**
	 * Export the height value (read only)
	 * @return the height of this octave
	 */
	int getHeight() const {
		return height;
	}

	/**
	 * Export the width value (read only)
	 * @return the width of this octave
	 */
	int getWidth() const {
		return width;
	}

	/**
	 * Export the octave value (read only), the meaning of which is defined in the vl_sift library:
	 * By convention, the octave of index 0
	 * starts with the image full resolution. Specifying an index greater
	 * than 0 starts the scale space at a lower resolution (e.g. 1 halves the resolution).
	 * @return the octave number
	 */
	int getOctave() const {
		return octave;
	}

	void Allocate(int Image_width, int Image_height, int Block_width, int Block_height);
	void Clear();
	void SpatialFiltering();
	void FrequencyFiltering();
	void getBlockInput(unsigned char * data = NULL);
	
	int block_row_n;			//block number in a row
	int block_col_n;			//block number in a coln
	int block_buffer_len;		//block buffer number for one scale
	int block_row_remain_width;	//block remaining width in case of the image boundary
	int block_col_remain_height;//block remaining width in case of the image boundary
	int block_map[64];			//block map to map the whole block to the block buffer
	FrequencyFilter * filters;	//filters in frequency domain
	
	int block_width;			//the block width without overlap
	int block_height;			//the block height without overlap
	int extrema_w, extrema_h, extrema_sx, extrema_sy;	//the LoG block region
	int padWidth;				//the padding width
	int cur_block_id;			//the current block id under processing
	int h_mode, v_mode;			//the mode of the block
	int windowSize;
	int sx, sy;
	int w2, h2;
	bool processOctave(FeatureList & featurelist, unsigned char * data = NULL, int width = 0, int height = 0);
	void computeDescriptor(FeatureList & featurelist, float xper);
};

} 	// end of namespace
