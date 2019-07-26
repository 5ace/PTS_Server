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

#include "AlpOctave.h"
#include "ImageBuffer.h"
#include <cstddef>
#include <cstring>   // std::memcpy, std::memcmp, std::memset, std::memchr
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <string>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include "CdvsException.h"
#include "vl/sift.h"		// vl_feat library

using namespace std;
using namespace mpeg7cdvs;

const double AlpOctave::magnification = 2.64;	// magnification factor
const double AlpOctave::sigma0 = 1.6;			// sigma of the first gaussian filtered image (G1)
const double AlpOctave::sigman = 0.5;			// sigma of the original image
const double AlpOctave::sigmak = pow(2.0, (1.0 / nlevels));
const double AlpOctave::dsigma0 = sigma0 * sigmak * sqrt (1.0 - 1.0 / (sigmak*sigmak)) ;

const Filter AlpOctave::o1g1_filter(sqrt(sigma0*sigma0 - sigman*sigman));	// static filter initialization
const Filter AlpOctave::g2_filter(dsigma0);									// static filter initialization
const Filter AlpOctave::g3_filter(dsigma0*sigmak);							// static filter initialization
const Filter AlpOctave::g4_filter(dsigma0*sigmak*sigmak);					// static filter initialization

const float AlpOctave::sigmas[] = {
		sigma0, sigma0 * sigmak, sigma0*sigmak*sigmak, sigma0*sigmak*sigmak*sigmak 	// same as vl_feat with levels = 2
};

const float AlpOctave::inter_sigmas[] = {
		1.90273138f, 2.69086853f, 3.80546277f
};

const float AlpOctave::peakThreshold = 0.4f;
const float AlpOctave::lowSigma = 1.7f;
const float AlpOctave::highSigma = 4.0f;
const float AlpOctave::sigmaThreshold = 0.4f;
const float AlpOctave::curvRatioThreshold = 12.0f;
const float AlpOctave::sigmaMerge = 0.4f;
const float AlpOctave::radiusMerge = 2.0f;
const float AlpOctave::maxDisplacement = 1.0f;

const int AlpOctave::overlapLines = floor(magnification * highSigma * 3.536 + 0.5);

const float AlpOctave::qcoeff[4][54] =	{
	//	Values of the qcoeff matrix when sigma = 1.600000
	{
	0.098214f,    0.310104f,    0.098214f,    -0.195494f,    -0.620398f,    -0.195494f,    0.098214f,    0.310104f,    0.098214f,
	0.098214f,    -0.195494f,    0.098214f,    0.310104f,    -0.620398f,    0.310104f,    0.098214f,    -0.195494f,    0.098214f,
	0.278622f,    -0.000000f,    -0.278622f,    -0.000000f,    0.000000f,    -0.000000f,    -0.278622f,    -0.000000f,    0.278622f,
	-0.113412f,    -0.311714f,    -0.113412f,    -0.000000f,    -0.000000f,    -0.000000f,    0.113412f,    0.311714f,    0.113412f,
	-0.113412f,    0.000000f,    0.113412f,    -0.311714f,    -0.000000f,    0.311714f,    -0.113412f,    0.000000f,    0.113412f,
	-0.027805f,    0.068460f,    -0.027805f,    0.068460f,    0.844390f,    0.068460f,    -0.027805f,    0.068460f,    -0.027805f,
	},

	//	Values of the qcoeff matrix when sigma = 2.262742
	{
	0.095647f,    0.311924f,    0.095647f,    -0.190786f,    -0.624384f,    -0.190786f,    0.095647f,    0.311924f,    0.095647f,
	0.095647f,    -0.190786f,    0.095647f,    0.311924f,    -0.624384f,    0.311924f,    0.095647f,    -0.190786f,    0.095647f,
	0.264138f,    -0.000000f,    -0.264138f,    0.000000f,    0.000000f,    0.000000f,    -0.264138f,    0.000000f,    0.264138f,
	-0.107186f,    -0.304575f,    -0.107186f,    -0.000000f,    0.000000f,    -0.000000f,    0.107186f,    0.304575f,    0.107186f,
	-0.107186f,    0.000000f,    0.107186f,    -0.304575f,    -0.000000f,    0.304575f,    -0.107186f,    0.000000f,    0.107186f,
	-0.028542f,    0.063217f,    -0.028542f,    0.063217f,    0.863039f,    0.063217f,    -0.028542f,    0.063217f,    -0.028542f,
	},

	//	Values of the qcoeff matrix when sigma = 3.200000
	{
	0.094511f,    0.312611f,    0.094511f,    -0.188729f,    -0.625674f,    -0.188729f,    0.094511f,    0.312611f,    0.094511f,
	0.094511f,    -0.188729f,    0.094511f,    0.312611f,    -0.625674f,    0.312611f,    0.094511f,    -0.188729f,    0.094511f,
	0.257040f,    0.000000f,    -0.257040f,    0.000000f,    -0.000000f,    0.000000f,    -0.257040f,    0.000000f,    0.257040f,
	-0.104562f,    -0.300359f,    -0.104562f,    -0.000000f,    0.000000f,    -0.000000f,    0.104562f,    0.300359f,    0.104562f,
	-0.104562f,    -0.000000f,    0.104562f,    -0.300359f,    0.000000f,    0.300359f,    -0.104562f,    -0.000000f,    0.104562f,
	-0.028356f,    0.059829f,    -0.028356f,    0.059829f,    0.874571f,    0.059829f,    -0.028356f,    0.059829f,    -0.028356f,
	},

	//	Values of the qcoeff matrix when sigma = 4.525483
	{
	0.093707f,    0.313394f,    0.093707f,    -0.187276f,    -0.627030f,    -0.187276f,    0.093707f,    0.313394f,    0.093707f,
	0.093707f,    -0.187276f,    0.093707f,    0.313394f,    -0.627030f,    0.313394f,    0.093707f,    -0.187276f,    0.093707f,
	0.253510f,    0.000000f,    -0.253510f,    0.000000f,    -0.000000f,    0.000000f,    -0.253510f,    0.000000f,    0.253510f,
	-0.102943f,    -0.298825f,    -0.102943f,    -0.000000f,    0.000000f,    -0.000000f,    0.102943f,    0.298825f,    0.102943f,
	-0.102943f,    -0.000000f,    0.102943f,    -0.298825f,    0.000000f,    0.298825f,    -0.102943f,    -0.000000f,    0.102943f,
	-0.029147f,    0.059807f,    -0.029147f,    0.059807f,    0.877473f,    0.059807f,    -0.029147f,    0.059807f,    -0.029147f,
	},
};


/*
 * Constructor of the Filter class, which initializes the filter kernel.
 */
Filter::Filter(double sigmapar)
{
	sigma = sigmapar;
	int num = (int) ceil(4.0 * sigma);
	ntaps = 2 * num + 1;
	if (ntaps > maxsize)
		throw CdvsException("Filter has too many taps");

	double dfilter[maxsize];
	double acc = 0 ;

	for (int j = 0 ; j < ntaps ; ++j)
	{
		double d = (j - num) / sigma ;
		dfilter[j] =  exp (- 0.5 * (d*d)) ;
		acc += dfilter[j] ;
	}

	for (int j = 0 ; j < ntaps ; ++j)
	{
		kernel[j] = (float) (dfilter[j] / acc) ;
	}
}


void Filter::print() const
{
	cout << "sigma: " << sigma << endl;
	cout << "ntaps: " << ntaps << " (num: " << ntaps / 2 << ")" << endl;
	cout << "max taps: " << maxsize << endl;

	for (int i = 0; i < ntaps; ++i)
	{
		cout << setw(10) << kernel[i] << " ";
	}
	cout << endl << endl;
}

std::ostream & operator << (std::ostream& outstr, const FeatureAlp & f)
{
	int rescaler = (1 << f.octave); 	//  rescale factor for x and y coordinates

	outstr << "( x:" << rescaler*f.x << ", y:" << rescaler*f.y << ", ix:" << rescaler*f.ix << ", iy:" << rescaler*f.iy
		   << ", scale:" << rescaler*f.sigma << ", orient:" << f.orientation << ", peak:" << f.peak
		   << ", cr:" << f.curvRatio << ", cs:" <<f.curvSigma << ", is:" << f.iscale << ", octave:" << f.octave << ")";

	if (f.spatialIndex == -1)
		outstr << "[DUPLICATE]";

	return outstr;
}

float FeatureAlp::distL1(const FeatureAlp & other) const
{
	int rescaler = (1 << octave); 	//  rescale factor for x and y coordinates.
	int orescaler = (1 << other.octave);
	float dist = abs(rescaler*x - orescaler*other.x);
	dist += abs(rescaler*y - orescaler*other.y);
	return dist;
}


AlpOctave::AlpOctave():height(0),width(0),octave(0),keypoints(),extraTopLines(0),extraBottomLines(0),capacity(0),fast_mode(false) 
{
	G1 = G2 = G3 = G4 = L1 = L2 = L3 = L4 = A = B = C = D = tmp = nextG1 = NULL;
	minResponse = maxResponse = minScale = maxScale = NULL;
	for (int k=0; k<4; ++k)
		gradTheta[k] = gradMod[k] = NULL;
}


AlpOctave::~AlpOctave()
{
	clear();
}


/*
 * Process the first octave.
 * If the image is greater than a certain limit, it is split into four parts to reduce memory usage.
 */
bool AlpOctave::processFirst(unsigned char * data, int width, int height, FeatureList & featurelist)
{
	if (min(width, height) <= minSize)	// check minimum size
		return false;

	int next_size = ((width+1)/2) * ((height+1)/2);				// next octave size

	if (height <= 4*overlapLines)	// decide if this image must be split
	{
		allocate(width, height, false);		// false ==> not split
		if (init(data, width, height))
		{
			detect();				// detect keypoints in this octave
			getKeypoints(featurelist.features, true, true);
			subsampleImage(G3, nextG1, width, height);
		}
		else
			return false;
	}
	else		// split the first octave image into 4 parts to reduce allocated memory
	{
		allocate(width, height, true);	// true ==> split

		float * next_dst = nextG1;			// the first destination address is nextG1
		bool skip_next_line = false;		// the first time do not skip the first line while subsampling

		int splitImageLines = height/4;
		int splitLines[4] = {splitImageLines, splitImageLines, splitImageLines, (height - 3*splitImageLines)};

		int splitHeight[4] = {
				splitLines[0] + overlapLines,		// only bottom overlap
				splitLines[1] + 2*overlapLines, 		// top and bottom overlap
				splitLines[2] + 2*overlapLines, 		// top and bottom overlap
				splitLines[3] + overlapLines};		// only top overlap

		int startLine[4] = { 0, splitImageLines - overlapLines, 2*splitImageLines - overlapLines, 3*splitImageLines - overlapLines};
		int extraTopLines[4] = {0, overlapLines, overlapLines, overlapLines};
		int extraBottomLines[4] = {overlapLines, overlapLines, overlapLines, 0};

		for (int sk = 0; sk < 4; ++sk)		// loop 4 times over the split image (on the first octave only)
		{
			if (init(data + width*startLine[sk], width, splitHeight[sk], extraTopLines[sk], extraBottomLines[sk]))		// input: the original image split into 4 parts
			{
				detect();						// detect keypoints in this part of the octave
				getKeypoints(featurelist.features, true, true, startLine[sk]);

				next_dst = subsampleImage(G3 + width * extraTopLines[sk], next_dst, width, splitLines[sk], skip_next_line);
				if (splitLines[sk] & 1)
					skip_next_line = !skip_next_line; 	// if odd, invert the skipping logic
			}
			else
				return false;		// init() failed
		}

		this->width = width;		// restore the real width
		this->height = height;		// restore the real height

	}

	return true;
}

/*
 * Process the next octave.
 * Returns true if successful, false if the resampled image is too small.
 */
bool AlpOctave::processNext(FeatureList & featurelist)
{
	if (init())
	{
		detect();				// detect keypoints in this octave
		detectDuplicates(featurelist); // detect duplicates
		getKeypoints(featurelist.features, true, true);
		subsampleImage(G3, nextG1, width, height);
		return true;
	}

	return false;
}


bool AlpOctave::init(unsigned char * data, int width_par, int height_par, int extra_top_lines, int extra_bottom_lines, bool fast)
{
	if (min(width_par, height_par) <= minSize)
		return false;

	octave = 0;				// this is the first octave
	extraTopLines = extra_top_lines;
	extraBottomLines = extra_bottom_lines;
	width = width_par;
	height = height_par;
	fast_mode = fast; 
	int size = width_par * height_par;

	allocate(width, height, false);		// allocate memory (if needed) 
	
	// copy the source image

	float * dest = G1;					// set destination
	unsigned char * source = data;		// set source
	for (int k=0; k < size; ++k)
		*(dest++) = *(source++);		// copy and convert from char to float

	// gaussian filtering

	conv2(G1, o1g1_filter, G1);	// now G1 contains the filtered image
	conv2(G1, g2_filter, G2);	// now G2 contains the filtered image
	conv2(G2, g3_filter, G3);	// now G3 contains the filtered image
	conv2(G3, g4_filter, G4);	// now G4 contains the filtered image

	return true;
}

float * AlpOctave::subsampleImage(const float * src, float * dst, int src_width, int src_height, bool skip_first_line)
{
	int delta = skip_first_line? src_width: 0;
	int height = skip_first_line? (src_height/2) : ((src_height + 1)/2);
	int width = src_width/2;
	float * dest;

	for (int i=0; i<height; ++i)
	{
		dest = dst + i*width;					// set destination
		const float * source = src + 2*i*src_width + delta;	// set source

		for (int k=0; k < width; ++k)
		{
			*(dest++) = *source;		// copy and subsample
			source += 2;
		}
	}
	return dest;
}

bool AlpOctave::init(const AlpOctave & previous)
{
	if (min(previous.width, previous.height) <= 2*minSize)
		return false;

	width = previous.width / 2;
	height = previous.height / 2;
	octave = previous.octave + 1;
	fast_mode = previous.fast_mode; 

	int size = width * height;
	allocate(width, height, false);		// allocate memory (if needed)

	// get data from previous G3 (subsampling)

	for (int i=0; i<height; ++i)
	{
		float * dest = G1 + i*width;				// set destination
		float * source = previous.G3 + 2*i*previous.width;	// set source

		for (int k=0; k < width; ++k)
		{
			*(dest++) = *source;		// copy and subsample
			source += 2;
		}
	}							// now G1 contains a subsampled image

	conv2(G1, g2_filter, G2);	// now G2 contains the filtered image
	conv2(G2, g3_filter, G3);	// now G3 contains the filtered image
	conv2(G3, g4_filter, G4);	// now G4 contains the filtered image

	return true;
}


bool AlpOctave::init()
{
	if (min(width, height) <= 2*minSize)
		return false;

	width /= 2;
	height /= 2;
	octave += 1;
	extraTopLines = 0;
	extraBottomLines = 0;

	int size = width * height;

	memcpy(G1, nextG1, sizeof(float)*size);		// get data from next G1 (just copy data)

	// now G1 contains a subsampled image

	conv2(G1, g2_filter, G2);	// now G2 contains the filtered image
	conv2(G2, g3_filter, G3);	// now G3 contains the filtered image
	conv2(G3, g4_filter, G4);	// now G4 contains the filtered image

	return true;
}


void AlpOctave::allocate(int width, int height, bool split) 
{
	// allocate buffers according to the split/not split parameter
	int size = 0;
	if (split)
	{
		int splitImageLines = height/4;
		size = width * (splitImageLines + 2*overlapLines);
	}
	else
	{
		size = width * height;
	}

	if (size > capacity)
	{
		clear();	// free allocated memory

		capacity = size;

		G1 = new float [size];
		G2 = new float [size];
		G3 = new float [size];
		G4 = new float [size];
		tmp = new float [size];

		// prepare buffer to contain the next octave G1

		int next_size = ((width+1)/2) * ((height+1)/2);
		nextG1 = new float [next_size];

		// low memory implementation: allocate only 3 lines of the source image

		L1 = new float [3*width];
		L2 = new float [3*width];
		L3 = new float [3*width];
		L4 = new float [3*width];

		A = new float [3*width];
		B = new float [3*width];
		C = new float [3*width];
		D = new float [3*width];

		minResponse = new float [3*width];
		maxResponse = new float [3*width];
		minScale = new float [3*width];
		maxScale = new float [3*width];

		// all gradient buffers reuse previously allocated buffers
		// we keep G3 because it will be subsampled and used as next G1 in the next octave

		gradTheta[0] = NULL;		// prepare buffers to be reused as gradient angle
		gradTheta[1] = G1;
		gradTheta[2] = G4;
		gradTheta[3] = NULL;

		gradMod[0] = NULL;			// prepare buffers to be reused as gradient module
		gradMod[1] = tmp;

		if(fast_mode) // Added by ETRI : for faster gradient computation, Gaussian and gradient buffers should be available at the same time
			gradMod[2] = new float[size];
		else
			gradMod[2] = G2;			// once gradTheta[1] and gradMod[1] have been computed, we can reuse G2

		gradMod[3] = NULL;

	}
}

bool AlpOctave::empty () const
{
	return (capacity == 0);
}

//
// free allocated memory; do not touch other variables.
//
void AlpOctave::clear ()
{
	if (empty())
			return;

	if (G1 != NULL) delete[] G1;
	if (G2 != NULL) delete[] G2;
	if (G3 != NULL) delete[] G3;
	if (G4 != NULL) delete[] G4;
	if (nextG1 != NULL) delete[] nextG1;
	if (L1 != NULL) delete[] L1;
	if (L2 != NULL) delete[] L2;
	if (L3 != NULL) delete[] L3;
	if (L4 != NULL) delete[] L4;
	if (A != NULL) delete[] A;
	if (B != NULL) delete[] B;
	if (C != NULL) delete[] C;
	if (D != NULL) delete[] D;
	if (tmp != NULL) delete[] tmp;
	if (minResponse != NULL) delete[] minResponse;
	if (maxResponse != NULL) delete[] maxResponse;
	if (minScale != NULL) delete[] minScale;
	if (maxScale != NULL) delete[] maxScale;

	G1 = G2 = G3 = G4 = nextG1 = L1 = L2 = L3 = L4 = A = B = C = D = NULL;
	minResponse = maxResponse = minScale = maxScale = NULL;

	// Added by ETRI : for faster gradient computation, gradient buffers are allocated
	if(fast_mode && (gradMod[2]!=NULL)) delete[] gradMod[2]; 

	// all four gradient buffers were shared
	gradTheta[1]=gradTheta[2]=gradMod[1] = gradMod[2] = NULL;
	
	capacity = 0;
}

void AlpOctave::conv2(const float * imagein, const Filter & filter, float * imageout) const
{
	// this operation needs a temporary image of size [width, height]
	// filter must contain a 2D separable filter
	// use vl_feat filtering function

	alp_smooth( imageout, tmp, imagein, width, height, filter.kernel, filter.ntaps);

}

void AlpOctave::laplacian(const float * src, float sigma, float * dst, int w, int h)
{
	// laplacian =
	//		0  1  0
	//		1 -4  1
	//		0  1  0
	//  ----   indices:  ----
	//			     k-w
	//	dst[k] = k-1, k, k+1
	//				 k+w

	int size = w * h;
	float squareSigma = sigma * sigma;			// compute square sigma

	// compute laplacian

	for (int k = 0; k < size; ++k)
		   dst[k] = squareSigma * (src[k - w] + src[k-1] - 4.0f*src[k] + src[k+1] + src[k + w]);

}

/*
 * tell if point k is the minimum value in the surrounding window of k;
 * offset to use: k-w-1, k-w, k-w+1, k-1, (NOT use k), k+1, k+w-1, k+w, k+w+1 (not necessarily in this order)
 */
inline bool AlpOctave::isMin(int k, float * R) const
{
	float val = R[k];

	return (val < -peakThreshold) && (val < R[k-1]) && (val < R[k+1]) &&
			(val < R[k-width-1]) && (val < R[k-width]) && (val < R[k-width+1]) &&
			(val < R[k+width-1]) && (val < R[k+width]) && (val < R[k+width+1]);
}

/*
 * tell if point k is the maximum value in the surrounding window of k;
 * offset to use: k-w-1, k-w, k-w+1, k-1, (NOT use k), k+1, k+w-1, k+w, k+w+1 (not necessarily in this order)
 */
inline bool AlpOctave::isMax(int k, float * R) const
{
	float val = R[k];
	return (val > peakThreshold) && (val > R[k-1]) && (val > R[k+1]) &&
			(val > R[k-width-1]) && (val > R[k-width]) && (val > R[k-width+1]) &&
			(val > R[k+width-1]) && (val > R[k+width]) && (val > R[k+width+1]);
}
void AlpOctave::shiftUp()	// shift one line up in memory buffers
{
	size_t size = width * sizeof(float);

	memcpy(L1, L1 + width, size);
	memcpy(L1 + width, L1 + 2 * width, size);
	memcpy(L2, L2 + width, size);
	memcpy(L2 + width, L2 + 2 * width, size);
	memcpy(L3, L3 + width, size);
	memcpy(L3 + width, L3 + 2 * width, size);
	memcpy(L4, L4 + width, size);
	memcpy(L4 + width, L4 + 2 * width, size);

	memcpy(A, A + width, size);
	memcpy(A + width, A + 2 * width, size);
	memcpy(B, B + width, size);
	memcpy(B + width, B + 2 * width, size);
	memcpy(C, C + width, size);
	memcpy(C + width, C + 2 * width, size);
	memcpy(D, D + width, size);
	memcpy(D + width, D + 2 * width, size);

	memcpy(minResponse, minResponse + width, size);
	memcpy(minResponse + width, minResponse + 2 * width, size);
	memcpy(maxResponse, maxResponse + width, size);
	memcpy(maxResponse + width, maxResponse + 2 * width, size);
	memcpy(minScale, minScale + width, size);
	memcpy(minScale + width, minScale + 2 * width, size);
	memcpy(maxScale, maxScale + width, size);
	memcpy(maxScale + width, maxScale + 2 * width, size);

}

void AlpOctave::computeResponse(int startLineSrc, int startLineDst, int nLines)
{
	int start_offset = startLineSrc * width;
	int dest_offset  = startLineDst * width;

	laplacian(G1 + start_offset, sigmas[0], L1 + dest_offset, width, nLines);		// now L1 contains the Laplacian of G1
	laplacian(G2 + start_offset, sigmas[1], L2 + dest_offset, width, nLines);		// now L2 contains the Laplacian of G2
	laplacian(G3 + start_offset, sigmas[2], L3 + dest_offset, width, nLines);		// now L3 contains the Laplacian of G3
	laplacian(G4 + start_offset, sigmas[3], L4 + dest_offset, width, nLines);		// now L4 contains the Laplacian of G4

	for (int k = dest_offset; k < (dest_offset + nLines * width); ++k)
	{
 		A[k] = -0.246388f*L1[k] + 0.493379f*L2[k] + -0.271662f*L3[k] + 0.013998f*L4[k];
 		B[k] = 2.502135f*L1[k] + -4.563607f*L2[k] + 2.010828f*L3[k] + 0.154854f*L4[k];
 		C[k] = -8.200728f*L1[k] + 12.982370f*L2[k] + -4.044904f*L3[k] + -1.056472f*L4[k];
 		D[k] = 8.643160f*L1[k] + -10.842436f*L2[k] + 2.120433f*L3[k] + 1.388641f*L4[k];
	}

	// Values at octave boundaries:
	// L = a*octaveRange(1).^3 + b*octaveRange(1).^2 + c*octaveRange(1) + d;
	// R = a*octaveRange(end).^3 + b*octaveRange(end).^2 + c*octaveRange(end) + d;

	const float * L = L1;
	const float * R = L4;

	for (int k = dest_offset; k < (dest_offset + nLines*width); ++k)		// compute extrema
	{
		minResponse[k] = min(L[k], R[k]);		// initialize minResponse
		maxResponse[k] = max(L[k], R[k]);		// initialize maxResponse
		minScale[k] = 0;
		maxScale[k] = 0;

		if (std::abs(A[k]) > 0)
		{
			float ap = 2*3*A[k];
			float bp = 2*B[k];
			float n = bp * bp - 2*ap*C[k];
			if (n >= 0)
			{
				n = sqrt(n);

				float S1 = (-bp + n)/ap;
				float S2 = (-bp - n)/ap;

				// check range
				if ((S1 >= sigmas[0]) && (S1 <= sigmas[3]))
				{
					float R1 = (float) (A[k]*S1*S1*S1 + B[k]*S1*S1 + C[k]*S1 + D[k]);	// compute response value at the S1 x-value

					if (R1 < minResponse[k])
					{
						minResponse[k] = R1;
						minScale[k] = S1;
					}
				}

				if ((S2 >= sigmas[0]) && (S2 <= sigmas[3]))
				{
					float R2 = (float) (A[k]*S2*S2*S2 + B[k]*S2*S2 + C[k]*S2 + D[k]);
					// find maximum
					if (R2 > maxResponse[k])
					{
						maxResponse[k] = R2;
						maxScale[k] = S2;
					}
				}
			}
		}
	} // end loop; minResponse, maxResponse, minScale, maxScale are now set.
}

void AlpOctave::detect()
{
	static const int border = 4;		// ignore borders as key point candidates

	int border_top = max(border, extraTopLines);
	int border_bottom = max(border, extraBottomLines);

	computeResponse(border_top - 1, 0, 3);		// process 3 lines starting from border_top - 1 up to border top + 1

	// detect extrema in minResponse, maxResponse

	float curvSigma = 0;

	for (int base_i = border_top; base_i < (height - border_bottom); ++base_i)
	{
		static const int i = 1;		// the central line

		// detect keypoints in the current central line
		for (int j = border; j < (width - border); ++j)
		{
			int k = i*width + j;
			if ((minScale[k] >= lowSigma) && (minScale[k] <= highSigma)		// check if sigma is in the [lowSigma,highSigma] range
				&& (isMin(k, minResponse))	    							// look for a minimum
				&& ((curvSigma = abs(6*A[k]*minScale[k] + 2*B[k])) > sigmaThreshold))		// test on second derivative
			{
				FeatureAlp point;	    			// save point
				point.ix = j ;
				point.iy = base_i ;
				point.x = j ;
				point.y = base_i ;
				point.octave = octave;
				point.sigma = minScale[k];
				point.iscale = getClosestIndex(point.sigma);
				point.peak = minResponse[k];
				point.orientation = 0; 			// will be set later on
				point.spatialIndex = k;
				point.curvSigma = curvSigma;
				if (computeCurvRatioAndCoordinates(point))		// curvature ratio test
				{
					keypoints.push_back(point);
				}
			}
			else if ((maxScale[k] >= lowSigma) && (maxScale[k] <= highSigma) 	// test for existence
				&& (isMax(k, maxResponse))	    								// look for a maximum
				&& ((curvSigma = abs(6*A[k]*maxScale[k] + 2*B[k])) > sigmaThreshold))			// test on second derivative
			{
				FeatureAlp point;	    			// save point
				point.ix = j;
				point.iy = base_i;
				point.x = j ;
				point.y = base_i ;
				point.octave = octave;
				point.sigma = maxScale[k];
				point.iscale = getClosestIndex(point.sigma);
				point.peak = maxResponse[k];
				point.orientation = 0; 			// will be set later on
				point.spatialIndex = k;
				point.curvSigma = curvSigma;
				if (computeCurvRatioAndCoordinates(point))		// curvature ratio test
				{
					keypoints.push_back(point);
				}
			}
		}	// end for

		shiftUp();								// shift up one line for next iteration
		computeResponse(base_i + 2 , 2, 1);		// compute response of last line for next iteration

	}
}



/*
 * This version uses only the central scales 1 and 2 (like vl_feat).
 */
int AlpOctave::getClosestIndex(float sigma)
{
	if (sigma <= inter_sigmas[1])
		return 1;
	else
		return 2;
}

/*
 * detectDuplicates() -
 */
void AlpOctave::detectDuplicates(AlpOctave & previous)
{
	// mark duplicates - using a full search algorithm

	for(std::vector<FeatureAlp>::iterator p1=previous.keypoints.begin(); p1<previous.keypoints.end(); ++p1)
	{
		for(std::vector<FeatureAlp>::iterator p2 = keypoints.begin(); p2<keypoints.end(); ++p2)
		{
			// p2 belongs to a smaller octave - normalize before using coordinates and sigma
			// to normalize, just multiply by 2 the coordinates of the smaller octave

			// this test is slightly different from what is described in m31369 and reported in the CD text.
			// In the aforementioned documents the two distances (spatial and scale) are checked separately;
			// here they are checked in a single comparison.
			// This most probably does not make any difference but it has to be verified.

			float dist = abs(2*p2->x - p1->x) + abs(2*p2->y - p1->y) + abs(2*p2->sigma - p1->sigma);

			if (dist < (radiusMerge + sigmaMerge))
			{
				// suppress the key point having the smaller peak (if both positive) or the greater peak (if both negative).
				// do not suppress the key point if the two peaks have different sign

				if ((p1->peak > 0) && (p2->peak > 0))
				{
					if (p1->peak < p2->peak)
						p1->spatialIndex  = -1;				// this flag indicates a duplicate key point
					else
						p2->spatialIndex  = -1;				// this flag indicates a duplicate key point
				}

				if ((p1->peak < 0) && (p2->peak < 0))
				{
					if (p1->peak > p2->peak)
						p1->spatialIndex  = -1;				// this flag indicates a duplicate key point
					else
						p2->spatialIndex  = -1;				// this flag indicates a duplicate key point
				}

				break;			// no other point can be so close, so skip to the next iteration
			}
		}
	}
}


bool AlpOctave::isDuplicate(const Feature & a)
{
	return (a.spatialIndex == -1);
}

/*
 * This method erases the duplicate keypoints from featurelist, using the erase-remove idiom.
 */
void AlpOctave::detectDuplicates(FeatureList & featurelist)
{
	// mark duplicates - using a full search algorithm

	int rescaler = (1 << octave); 	// 2, 4, 8, 16 ...
	float threshold = (radiusMerge + sigmaMerge) * (1 << (octave - 1));

	for(std::vector<Feature>::iterator p1=featurelist.features.begin(); p1<featurelist.features.end(); ++p1)
	{
		if(p1->octave == octave-1) 		// ignore octaves other than the previous one
		{
			for(std::vector<FeatureAlp>::iterator p2 = keypoints.begin(); p2<keypoints.end(); ++p2)
			{
				float dist = abs(rescaler*p2->x - p1->x) + abs(rescaler*p2->y - p1->y) + abs(rescaler*p2->sigma - p1->scale);

				if (dist < threshold)
				{
					// suppress the key point having the smaller peak (if both positive) or the greater peak (if both negative).
					// do not suppress the key point if the two peaks have different sign

					if ((p1->peak > 0) && (p2->peak > 0))
					{
						if (p1->peak < p2->peak)
							p1->spatialIndex  = -1;				// this flag indicates a duplicate key point
						else
							p2->spatialIndex  = -1;				// this flag indicates a duplicate key point
					}

					if ((p1->peak < 0) && (p2->peak < 0))
					{
						if (p1->peak > p2->peak)
							p1->spatialIndex  = -1;				// this flag indicates a duplicate key point
						else
							p2->spatialIndex  = -1;				// this flag indicates a duplicate key point
					}

					break;			// no other point can be so close, so skip to the next iteration
				}
			}
		}
	}

	// now remove all marked items from the "features" vector
	// (NB: marked elements in "keypoints" will be ignored, so there is no need to remove them)

	featurelist.features.erase( std::remove_if(featurelist.features.begin(), featurelist.features.end(), isDuplicate), featurelist.features.end() );
}

bool AlpOctave::computeCurvRatioAndCoordinates(FeatureAlp & d) const
{
	// p11 p12 p13
	// p21 p22 p23
	// p31 p32 p33

	int k = d.spatialIndex;
	float s = d.sigma;
	float ss = s*s;
	float sss = s*ss;
	float p11 = A[k-width-1]*sss + B[k-width-1]*ss + C[k-width-1]*s + D[k-width-1];
	float p12 = A[k-width]*sss + B[k-width]*ss + C[k-width]*s + D[k-width];
	float p13 = A[k-width+1]*sss + B[k-width+1]*ss + C[k-width+1]*s + D[k-width+1];
	float p21 = A[k-1]*sss + B[k-1]*ss + C[k-1]*s + D[k-1];
	float p22 = d.peak;
	float p23 = A[k+1]*sss + B[k+1]*ss + C[k+1]*s + D[k+1];
	float p31 = A[k+width-1]*sss + B[k+width-1]*ss + C[k+width-1]*s + D[k+width-1];
	float p32 = A[k+width]*sss + B[k+width]*ss + C[k+width]*s + D[k+width];
	float p33 = A[k+width+1]*sss + B[k+width+1]*ss + C[k+width+1]*s + D[k+width+1];

	float gxx = p21 - 2*p22 + p23;
	float gyy = p12 - 2*p22 + p32;
	float gxy = 0.25f * (p11 + p33 - p31 - p13);
	float TrH = gxx + gyy;
	float DetH = gxx * gyy - gxy * gxy;
	float ratio = abs((TrH*TrH)/DetH);

	d.curvRatio = ratio;

	if (ratio > curvRatioThreshold)
		return false;						// drop the point (nothing else to do in this case)

	const float * qc = qcoeff[d.iscale];			// get the correct coefficients

	float w1 = qc[0]*p11 + qc[1]*p21 + qc[2]*p31 + qc[3]*p12 + qc[4]*p22 + qc[5]*p32 + qc[6]*p13 + qc[7]*p23 + qc[8]*p33;
	qc += 9;	// next row
	float w2 = qc[0]*p11 + qc[1]*p21 + qc[2]*p31 + qc[3]*p12 + qc[4]*p22 + qc[5]*p32 + qc[6]*p13 + qc[7]*p23 + qc[8]*p33;
	qc += 9;
	float w3 = qc[0]*p11 + qc[1]*p21 + qc[2]*p31 + qc[3]*p12 + qc[4]*p22 + qc[5]*p32 + qc[6]*p13 + qc[7]*p23 + qc[8]*p33;
	qc += 9;
	float w4 = qc[0]*p11 + qc[1]*p21 + qc[2]*p31 + qc[3]*p12 + qc[4]*p22 + qc[5]*p32 + qc[6]*p13 + qc[7]*p23 + qc[8]*p33;
	qc += 9;
	float w5 = qc[0]*p11 + qc[1]*p21 + qc[2]*p31 + qc[3]*p12 + qc[4]*p22 + qc[5]*p32 + qc[6]*p13 + qc[7]*p23 + qc[8]*p33;

	float deltaY =  (w3*w4 - 2*w1*w5) / (4*w1*w2 - w3*w3);
	float deltaX = (-w4 - w3*deltaY)/(2*w1);

	//cout << "x = " << d.x << " y = "<< d.y << " deltaX = " << setw(10) << deltaX << " deltaY = " << setw(10) << deltaY << endl;

	if ((abs(deltaX) > maxDisplacement) || (abs(deltaY) > maxDisplacement))
		return false;			// drop the point

	d.x += deltaX;			// correct coordinates
	d.y += deltaY;			// correct coordinates

	return true;	// save this point
}

void AlpOctave::getAlpKeypoints(std::vector<FeatureAlp> & outKeypoints, bool rescale)
{
	int size = width * height;

	float * srcGaussian[] = {G1, G2, G3, G4};

	// check if each gradient is needed for at least one keypoint

	bool needGradient[4] = {false, false, false, false};
	for(std::vector<FeatureAlp>::const_iterator d = keypoints.begin() ; d < keypoints.end(); ++d)
		if (d->spatialIndex != -1)	// skip duplicates
			needGradient[d->iscale] = true;

	// compute gradient - assuming that only gradients for scale 1 and 2 are needed

	if (needGradient[0] || needGradient[3])		// check if assumption on gradients has been violated
		throw CdvsException("Basic assumption on gradients has been violated");

	for (int scale = 1; scale <= 2; ++scale)
	{
		if (needGradient[scale])
		{
			// call vl_feat update_gradient()
			alp_update_gradient (gradMod[scale], gradTheta[scale], srcGaussian[scale], width, height);
		}
	}

	for(std::vector<FeatureAlp>::iterator e = keypoints.begin() ; e < keypoints.end(); ++e)
	{
		if (e->spatialIndex != -1)	// skip duplicates
		{
			// normalize octaves if needed
			if (rescale && (e->octave > 0))
			{
				int rescaler = (1 << e->octave); 	//  rescale factor for x and y coordinates.
				e->x = rescaler * e->x;
				e->y = rescaler * e->y;
				e->sigma = rescaler * e->sigma;		// in CDVS scale contains sigma with no sqrt factor
			}

			outKeypoints.push_back(*e);
		}
	}
	keypoints.clear();	// local keypoints can be cleared now

}

void AlpOctave::getKeypoints(std::vector<Feature> & outKeypoints, bool calcDescriptor, bool rescale, int base_Y)
{
	int size = width * height;

	float * srcGaussian[] = {G1, G2, G3, G4};

	// check if each gradient is needed for at least one keypoint

	bool needGradient[4] = {false, false, false, false}; 
	for(std::vector<FeatureAlp>::const_iterator d = keypoints.begin() ; d < keypoints.end(); ++d)
		if (d->spatialIndex != -1)	// skip duplicates
			needGradient[d->iscale] = true;

	// compute gradient - assuming that only gradients for scale 1 and 2 are needed

	if (needGradient[0] || needGradient[3])		// check if assumption on gradients has been violated
		throw CdvsException("Basic assumption on gradients has been violated");

	for (int scale = 1; scale <= 2; ++scale)
	{
		if (needGradient[scale])
		{
			// call vl_feat update_gradient()
			alp_update_gradient (gradMod[scale], gradTheta[scale], srcGaussian[scale], width, height);
		}
	}

	// compute orientation of all key points (excluding duplicate key points)
	int *nangles = new int[keypoints.size()];
	double **angles = new double*[keypoints.size()];

//	#pragma omp parallel for default(shared)
	for(int i=0; i<keypoints.size(); ++i)
	{
		FeatureAlp *d = &(keypoints[i]);

		// call alp_keypoint_orientations in vl_feat library
		angles[i] = new double[4];
		nangles[i] = alp_keypoint_orientations (angles[i], width, height, d->sigma,
					d->x, d->y, gradMod[d->iscale], gradTheta[d->iscale], magnification);
	}

	for(int i=0; i<keypoints.size(); ++i)
	{
		FeatureAlp *d = &(keypoints[i]);

		if (d->spatialIndex != -1)	// skip duplicates
		{
			Feature e;				  // convert FeatureAlp into Feature
			e.x = d->x;				  // the X coordinate of the ALP keypoint
			e.y = d->y;	  			  // the Y coordinate of the ALP keypoint
			e.scale = d->sigma;		  // the sigma of the Gaussian filter used to detect this point
			e.peak = d->peak;		  // the peak of the ALP keypoint
			e.curvRatio = d->curvRatio;		  // the ratio of the curvatures
			e.curvSigma = d->curvSigma;		  // the curvature at sigma
			e.pdf = d->pdf;				  // probability of this point to be matched
			e.octave = d->octave;				// octave of this feature
			e.iscale = d->iscale;				// int scale

			// normalize octaves if needed
			if (rescale && (e.octave > 0))
			{
				int rescaler = (1 << e.octave); 	//  rescale factor for x and y coordinates.
				e.x = rescaler * d->x;
				e.y = rescaler * d->y;
				e.scale = rescaler * d->sigma;		// in CDVS scale contains sigma with no sqrt factor
			}

			for (int q = 0; q < nangles[i] ; ++q)
			{
				e.orientation = angles[i][q];				// set orientation
				if (calcDescriptor)
					computeDescriptor(e, rescale);			// compute SIFT descriptor --> either here or in ImageBuffer.extract()

				outKeypoints.push_back(e);

				if (base_Y > 0)
					outKeypoints.back().y += base_Y;			// use base offset if required
			}
		}

		delete []angles[i];
	}

	delete []nangles;
	delete []angles;

	keypoints.clear();	// local keypoints can be cleared now
	
}


void AlpOctave::getKeypoints_fast_mode(std::vector<Feature> & outKeypoints,float minpdf, bool calcDescriptor, bool rescale, int base_Y) 
{
	int size = width * height;

	float * srcGaussian[] = {G1, G2, G3, G4};

	bool needGradient[4] = {false, true, true, false}; 

	// initialize gradient map
	for (int scale = 1; scale <= 2; ++scale)  
	{
		memset(gradMod[scale],0,sizeof(float)*size); 
		memset(gradTheta[scale],0,sizeof(float)*size); // initialized to zero
	}

	// compute orientation of all key points (excluding duplicate key points or keypoints which pdf is less than minpdf (preliminary feature selection)

	for(std::vector<FeatureAlp>::const_iterator d = keypoints.begin() ; d < keypoints.end(); ++d)
	{
		if (d->spatialIndex != -1 && d->pdf >= minpdf)	// skip duplicates and keypoints which pdf is less than minpdf (in order to skip less important keypoints in orientation assignment)
		{
			// call alp_keypoint_orientations in vl_feat library

			double angles[4];
			int nangles = alp_keypoint_orientations_fast_mode (angles, width, height, d->sigma,
				d->x, d->y, gradMod[d->iscale], gradTheta[d->iscale], magnification, srcGaussian[d->iscale]);//  gradients are computed on the fly if necessary or just read the valued if already computed

			Feature e;				  // convert FeatureAlp into Feature
			e.x = d->x;				  // the X coordinate of the ALP keypoint
			e.y = d->y;	  			  // the Y coordinate of the ALP keypoint
			e.scale = d->sigma;		  // the sigma of the Gaussian filter used to detect this point
			e.peak = d->peak;		  // the peak of the ALP keypoint
			e.curvRatio = d->curvRatio;		  // the ratio of the curvatures
			e.curvSigma = d->curvSigma;		  // the curvature at sigma
			e.pdf = d->pdf;				  // probability of this point to be matched
			e.octave = d->octave;				// octave of this feature
			e.iscale = d->iscale;				// int scale

			// normalize octaves if needed
			if (rescale && (e.octave > 0))
			{
				int rescaler = (1 << e.octave); 	//  rescale factor for x and y coordinates.
				e.x = rescaler * d->x;
				e.y = rescaler * d->y;
				e.scale = rescaler * d->sigma;		// in CDVS scale contains sigma with no sqrt factor
			}

			for (int q = 0; q < nangles ; ++q)
			{
				e.orientation = angles[q];				// set orientation
				if (calcDescriptor)
					computeDescriptor(e, rescale);			// compute SIFT descriptor --> either here or in ImageBuffer.extract()

				outKeypoints.push_back(e);

				if (base_Y > 0)
					outKeypoints.back().y += base_Y;			// use base offset if required
			}
		}
	}

	keypoints.clear();	// local keypoints can be cleared now



}

void AlpOctave::computeDescriptor(Feature & keypoint, bool rescale) const
{
	double x = keypoint.x;
	double y = keypoint.y;
	double sigma = keypoint.scale;
	double angle0 = keypoint.orientation;
	float * descr = keypoint.descr;

	if ((rescale) && (keypoint.octave > 0))
	{
		int rescaler = (1 << keypoint.octave); 	//  rescale factor for x and y coordinates.
		x /= rescaler;
		y /= rescaler;
		sigma /= rescaler;
	}

  // call alp_keypoint_descriptor in vl_feat library
  alp_keypoint_descriptor(descr, x, y, sigma, angle0, width, height,  gradMod[keypoint.iscale],  gradTheta[keypoint.iscale], magnification);

  // export data
   for (int k = 0 ; k < 128 ; ++k)
   {
   	float exportval = 512.0f * descr[k];
   	descr[k] = (unsigned int) ((exportval < 255.0f) ? exportval : 255.0f);
   }
}

/*
 * -----------------------------------------------------------------------------
 * This variant of computeDescriptor() computes both orientation and descriptor.
 * -----------------------------------------------------------------------------
 */
void AlpOctave::computeDescriptor(FeatureList & featurelist, const FeatureAlp & in, bool rescale, size_t missing) const
{
	Feature out;
	out.x = in.x;				  // the X coordinate of the ALP keypoint
	out.y = in.y;	  			  // the Y coordinate of the ALP keypoint
	out.scale = in.sigma;		  // the sigma of the Gaussian filter used to detect this point
	out.peak = in.peak;		  // the peak of the ALP keypoint
	out.curvRatio = in.curvRatio;		  // the ratio of the curvatures
	out.curvSigma = in.curvSigma;		  // the curvature at sigma
	out.pdf = in.pdf;				  // probability of this point to be matched
	out.octave = in.octave;				// octave of this feature
	out.iscale = in.iscale;				// int scale

	double x = in.x;
	double y = in.y;
	double sigma = in.sigma;
	float * descr = out.descr;

	if ((rescale) && (in.octave > 0))
	{
		int rescaler = (1 << in.octave); 	//  rescale factor for x and y coordinates.
		x /= rescaler;
		y /= rescaler;
		sigma /= rescaler;
	}

	// compute orientation of each keypoint (up to four orientations are possible)
	double angles[4];
	int nangles = alp_keypoint_orientations (angles, width, height, sigma, x, y, gradMod[in.iscale], gradTheta[in.iscale], magnification);

	if (nangles > missing)			// check if we are going to produce more keypoints than required:
		nangles = (int) missing;	// in this case reduce the number of keypoints accordingly

	// compute a descriptor for each orientation
	for (int k = 0; k < nangles; ++k)
	{
		out.orientation = angles[k];	// save orientation

		// call alp_keypoint_descriptor in vl_feat library
		alp_keypoint_descriptor(descr, x, y, sigma, angles[k], width, height,  gradMod[in.iscale],  gradTheta[in.iscale], magnification);

		// export data
		for (int k = 0 ; k < 128 ; ++k)
		{
			float exportval = 512.0f * descr[k];
			descr[k] = (unsigned int) ((exportval < 255.0f) ? exportval : 255.0f);
		}

		featurelist.features.push_back(out);		// store output keypoint into featurelist
	}
}

void AlpOctave::computeDescriptor_fast_mode(Feature & keypoint, bool rescale) const 
{
	double x = keypoint.x;
	double y = keypoint.y;
	double sigma = keypoint.scale;
	double angle0 = keypoint.orientation;
	float * descr = keypoint.descr;
	
	float * srcGaussian[] = {G1, G2, G3, G4}; // for faster gradient computation

	if ((rescale) && (keypoint.octave > 0))
	{
		int rescaler = (1 << keypoint.octave); 	//  rescale factor for x and y coordinates.
		x /= rescaler;
		y /= rescaler;
		sigma /= rescaler;
	}

	// call alp_keypoint_descriptor_fast_mode in vl_feat library , gradients are computed on the fly if necessary or just read the valued if already computed
	alp_keypoint_descriptor_fast_mode(descr, x, y, sigma, angle0, width, height,  gradMod[keypoint.iscale],  gradTheta[keypoint.iscale], magnification, srcGaussian[keypoint.iscale]);

	// export data
	for (int k = 0 ; k < 128 ; ++k)
	{
		float exportval = 512.0f * descr[k];
		descr[k] = (unsigned int) ((exportval < 255.0f) ? exportval : 255.0f);
	}
}

