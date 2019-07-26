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
 * Copyright (c) ISO/IEC 2013.
 */

#include "SiftDetector.h"
#include <iostream>
#include <cmath>
#include <cassert>
#include <vector>
#include <algorithm>
#include <cstring>
#include "vl/sift.h"		// vl_feat library

#define VL_PI 3.141592653589793

// exceptions
#include "CdvsException.h"

#ifdef max
	#undef max
	#undef min
#endif

using namespace std;
using namespace mpeg7cdvs;

SiftDetector::SiftDetector()
{
	verbose = 0;

	for (int k=0; k<nFilters; ++k)
	{
		filters[k] = NULL;
	}
}

SiftDetector::~SiftDetector()
{
	for (int k=0; k<nFilters; ++k)
	{
		if (filters[k] != NULL)
			vl_sift_delete ((VlSiftFilt*)filters[k]) ;
	}
}


void SiftDetector::detect(FeatureList & featurelist, const Parameters &params)
{
	/* algorithm default parameters */

	int O		=  -1 ;
	int S		=   3 ;
	int o_min	=   0 ;

	double edge_thresh = -1 ;
	double peak_thresh = -1 ;
	double norm_thresh = -1 ;
	double magnif      = -1 ;

	int err                 	= VL_ERR_OK ;
	vl_bool force_output        = 0 ;

	verbose				=  params.debugLevel ;

	/* ------------------------------------------------------------------
	*                                                       Process image
	* --------------------------------------------------------------- */

	float     *fdata = 0;

	VlSiftFilt      *filt = 0 ;
	int          q ;
	int              i ;

	unsigned char * data = buffer.data();

	/* allocate buffer */
	fdata = new float [width * height] ;

	if (!data || !fdata) {
		err = VL_ERR_ALLOC ;
		goto done ;
	}

	/* convert data type */
	for (q = 0 ; q < height; q++) {
		for (int p = 0 ; p < width; p++) {
			fdata [p*height + q] = data [q*width+p];
		}
	}

	/* ...............................................................
	*                                                     Make filter
	* ............................................................ */

	filt = vl_sift_new (height, width, O, S, o_min) ;

	if (edge_thresh >= 0) vl_sift_set_edge_thresh (filt, edge_thresh) ;
	if (peak_thresh >= 0) vl_sift_set_peak_thresh (filt, peak_thresh) ;
	if (norm_thresh >= 0) vl_sift_set_norm_thresh (filt, norm_thresh) ;
	if (magnif      >= 0) vl_sift_set_magnif      (filt, magnif) ;

	if (verbose > 1) {
		printf("vl_sift: filter settings:\n") ;
		printf("vl_sift:   octaves      (O)      = %d\n",
			vl_sift_get_noctaves      (filt)) ;
		printf("vl_sift:   levels       (S)      = %d\n",
			vl_sift_get_nlevels       (filt)) ;
		printf("vl_sift:   first octave (o_min)  = %d\n",
			vl_sift_get_octave_first  (filt)) ;
		printf("vl_sift:   edge thresh           = %g\n",
			vl_sift_get_edge_thresh   (filt)) ;
		printf("vl_sift:   peak thresh           = %g\n",
			vl_sift_get_peak_thresh   (filt)) ;
		printf("vl_sift:   norm thresh           = %g\n",
			vl_sift_get_norm_thresh   (filt)) ;
		printf("vl_sift:   window size           = %g\n",
			vl_sift_get_window_size   (filt)) ;
	}


	if (!filt) {
		err = VL_ERR_ALLOC ;
		goto done ;
	}

	/* ...............................................................
	*                                             Process each octave
	* ............................................................ */
	i     = 0 ;
	for (int filterIterator = 0; filterIterator < nFilters; ++filterIterator) {

		/* calculate the GSS for the next octave .................... */
		if (filterIterator == 0) {
			err = vl_sift_process_first_octave (filt, fdata) ;
			delete [] fdata;	// release image data - it has been copied into the filter
			fdata = NULL ;
		} else {
			err = vl_sift_process_next_octave  (filt) ;
		}

		if (err) {
			err = VL_ERR_OK ;
			break ;
		}

		/* run detector ............................................. */
		vl_sift_detect (filt) ;

		VlSiftKeypoint const *keys  = vl_sift_get_keypoints     (filt) ;
		int nkeys = vl_sift_get_nkeypoints (filt) ;

		if (verbose > 1) {
			printf ("vl_sift: detected %d (unoriented) keypoints\n", nkeys) ;
		}

		/* for each keypoint ........................................ */
		for (i = 0; i < nkeys ; ++i) {
			double                angles [4] ;
			int                   nangles ;
			VlSiftKeypoint const *k ;

			/* obtain keypoint orientations ........................... */

			k = keys + i ;
			nangles = vl_sift_calc_keypoint_orientations(filt, angles, k);

			/* For each orientation ................................... */
			for (q = 0 ; q < nangles ; ++q) {
				Feature d;
				float orientation = (float)(VL_PI / 2 - angles [q]);
				d.orientation = orientation;
				d.y = k->x;			// matrix is transposed, so exchange X with Y
				d.x = k->y;
				d.scale = k->sigma;
				d.peak = k->peak;
				d.iscale = k->is;

				assert(filterIterator == k->o);

				d.octave = k->o;
				featurelist.features.push_back(d);
				if (verbose > 1) {
					printf("vl_sift point: or=%.4f, x=%.4f, y=%.4f, scale=%.4f, iscale=%d, peak=%.4f\n", d.orientation, d.x, d.y, d.scale, d.iscale, d.peak);
				}
			}
		}    // end foreach keypoint

		filters[filterIterator] = vl_save_gradient (filt);		// save filt

	}	// end main loop

	/* ...............................................................
	*                                                       Finish up
	* ............................................................ */
done :

	/* release filter */
	if (filt) {
		vl_sift_delete (filt) ;
		filt = NULL ;
	}

	/* release image data */
	if (fdata) {
		delete [] fdata;
		fdata = NULL ;
	}

	if (err != 0) 		// if bad print error message
	{
		string msg = "";
		switch (err)
		{

			case 0: break;		// all right
			case VL_ERR_OVERFLOW: msg = "Buffer overflow"; break;
			case VL_ERR_ALLOC: msg = "Memory allocation failed"; break;
			case VL_ERR_BAD_ARG: msg = "Bad argument or illegal data"; break;
			case VL_ERR_IO: msg = "Input/output failed"; break;
			case VL_ERR_EOF: msg = "End-of-file or end-of-sequence detected"; break;
			default: msg = "Unknown error"; break;
		}
		throw CdvsException(string("ImageBuffer.detect: ").append(msg));
	}

	if (verbose > 1) {
		printf("vl_sift: total point num = %d\n", (int) featurelist.features.size());
	}

	// sort keypoints in descending order of importance

	sortPdf(featurelist);

	if (params.debugLevel > 0)
		print(featurelist.features, "SIFT");
}


void SiftDetector::extract(FeatureList & featurelist, size_t num) const
{
	if (num > featurelist.features.size())
		num = featurelist.features.size();	// cannot extract more points than those available!

	vl_sift_pix  buf [128] ;
	vl_sift_pix rbuf [128] ;

	for(std::vector<Feature>::iterator d=featurelist.features.begin(); d<featurelist.features.end(); ++d)
	{
		VlSiftKeypoint ik ;

		double orientation = VL_PI / 2 - d->orientation;
		ik.x = d->y;	// matrix is transposed, so exchange X with Y
		ik.y = d->x;
		ik.sigma = d->scale;
		ik.peak = d->peak;
		ik.o = d->octave;
		ik.is = d->iscale;

		vl_sift_calc_keypoint_descriptor((VlSiftFilt*)filters[d->octave], buf, &ik, orientation);

		transpose_descriptor (rbuf, buf) ;

		for (int j = 0 ; j < 128 ; ++j) {
			float x = 512.0F * rbuf [j] ;
			x = (x < 255.0F) ? x : 255.0F ;
			d->descr[j] = (vl_uint8) x ;		// return uint8
//			d->descr[j] = x ;					// return float
		}

		if (verbose > 0)
			printDescr(*d);
	}
}


void SiftDetector::transpose_descriptor (float* dst, float* src)
{
	int const BO = 8 ;  /* number of orientation bins */
	int const BP = 4 ;  /* number of spatial bins     */
	int i, j, t ;

	for (j = 0 ; j < BP ; ++j) {
		int jp = BP - 1 - j ;
		for (i = 0 ; i < BP ; ++i) {
			int o  = BO * i + BP*BO * j  ;
			int op = BO * i + BP*BO * jp ;
			dst [op] = src[o] ;
			for (t = 1 ; t < BO ; ++t)
				dst [BO - t + op] = src [t + o] ;
		}
	}
}


//
// save the vlSiftFilt gradient (and associated parameters)
//
void * SiftDetector::vl_save_gradient (const void *_fin)
{
   VlSiftFilt *fin = (VlSiftFilt*) _fin;
   VlSiftFilt *f = vl_sift_new (fin->width, fin->height, fin->O, fin->S, fin->o_min) ;

   f->sigman  = fin->sigman;       		/**< nominal image smoothing. */
   f->sigma0  = fin->sigma0;       		/**< smoothing of pyramid base. */
   f->sigmak  = fin->sigmak;	       	/**< k-smoothing */
   f->dsigma0 = fin->dsigma0;      		/**< delta-smoothing. */

   f->s_min   = fin->s_min; 	        /**< minimum level index. */
   f->s_max   = fin->s_max;             /**< maximum level index. */
   f->o_cur   = fin->o_cur;             /**< current octave. */

   f->octave_width  = fin->octave_width;    /**< current octave width. */
   f->octave_height = fin->octave_height;   /**< current octave height. */

   f->gaussFilterSigma = fin->gaussFilterSigma ;   /**< current Gaussian filter std */
   f->gaussFilterWidth = fin->gaussFilterWidth;    /**< current Gaussian filter width */

   f->nkeys = fin->nkeys ;          	 /**< number of detected keypoints. */
   f->keys_res = fin->keys_res ;        /**< size of the keys buffer. */

   f->peak_thresh = fin->peak_thresh ;  /**< peak threshold. */
   f->edge_thresh = fin->edge_thresh;  /**< edge threshold. */
   f->norm_thresh = fin->norm_thresh;  /**< norm threshold. */
   f->magnif = fin->magnif ;       /**< magnification factor. */
   f->windowSize = fin->windowSize;   /**< size of Gaussian window (in spatial bins) */

   f->grad_o = fin->grad_o ;          /**< GSS gradient data octave. */

  // copy gradient

  if (fin->grad == NULL)
	  f->grad = NULL;
  else
  {
	  size_t size = sizeof(vl_sift_pix) * fin->octave_width * fin->octave_height * 2 * (fin->s_max - fin->s_min - 2);
	  memcpy(f->grad, fin->grad, size);
  }

  return f ;
}

// implementation of sortPdf() for this type of keypoints (SIFT)

const float SiftDetector::DistC[] = {
		236.6142425537f, 228.2534637451f, 211.4427642822f, 219.8725585938f, 202.9744262695f, 245.5096893311f, 194.4166564941f, 185.7111358643f, 254.7625732422f, 159.0561828613f,
		177.0009307861f, 264.3377075195f, 168.1789245605f, 274.1414184570f, 284.2149963379f, 149.8331451416f, 316.0087585449f, 294.4043579102f, 304.9752197266f, 140.3899230957f,
		130.6065673828f, 120.4344406128f, 109.8616027832f,  98.7058868408f,  86.8511657715f, 329.2155761719f,  74.0838470459f,  60.0149383545f, 345.9376220703f,  43.5756835938f,
		22.4543666840f, 369.2438964844f
};

const float SiftDetector::DistP[] = {
		 0.0220398754f,  0.0238624662f,  0.0276106168f,  0.0261496734f,  0.0288298205f,  0.0219966639f,  0.0306943282f,  0.0331666134f,  0.0213320162f,  0.0372393429f,  // 10
		 0.0342464559f,  0.0202504732f,  0.0362297818f,  0.0194976330f,  0.0186509714f,  0.0393599831f,  0.0127053130f,  0.0170345809f,  0.0152353840f,  0.0412544832f,  // 20
		 0.0429147556f,  0.0442708395f,  0.0463888831f,  0.0474954657f,  0.0487268977f,  0.0119834812f,  0.0506855324f,  0.0524494164f,  0.0103358822f,  0.0545994230f,  // 30
		 0.0559572875f,  0.0068056579f
};

const float SiftDetector::ScaleC[] = {
		 2.0037319660f,   2.5626039505f,   3.3531446457f,   4.6013789177f,   6.6070671082f,  10.1078004837f,  16.8908882141f,  33.2625312805f
};

const float SiftDetector::ScaleP[] = {
		 0.1809079796f,  0.2700507343f,  0.3348271549f,  0.4203188419f,  0.4220668674f,  0.4392355382f,  0.3710923493f,  0.3111107647f
};

const float SiftDetector::OrientC[] = {
		 -1.5368843079f, 1.4983994961f, -4.6412000656f, -0.0169844721f, -1.6649979353f, -3.1754138470f,  0.1449248195f, -3.0298063755f, -1.3944710493f,  1.3232766390f,
		 -3.3388459682f,-4.4705252647f, -1.8167932034f, -0.1744092405f, -1.2183139324f,  0.3518850803f, -2.8577301502f,  1.1060111523f, -4.2607388496f, -3.5427241325f,
		 -1.9989310503f,-0.3603723645f, -1.0142878294f, -4.0234694481f,  0.8552971482f,  0.5963328481f, -2.6501891613f, -3.7760767937f, -2.2037825584f, -0.5725694895f,
		 -2.4249286652f,-0.7950484753f
};

const float SiftDetector::OrientP[] = {
		 0.2052661479f,  0.2003512084f,  0.2017586380f,  0.1751748621f,  0.1990270615f,  0.1747285426f,  0.1745616049f,  0.1758822352f,  0.1916013956f,  0.1793494225f,  // 10
		 0.1792997271f,  0.1788771898f,  0.1834550649f,  0.1745595038f,  0.1807001084f,  0.1745832562f,  0.1684573591f,  0.1673879176f,  0.1615773141f,  0.1694945097f,  // 20
		 0.1749896258f,  0.1648799777f,  0.1693636328f,  0.1672000736f,  0.1666287184f,  0.1736336648f,  0.1676661670f,  0.1665415317f,  0.1717979163f,  0.1691450924f,  // 30
		 0.1697088629f,  0.1677394360f
};

const float SiftDetector::PeakC[] = {
		  0.2547715306f, 0.7862316966f,  1.4677816629f,  2.2217512131f,  3.0260436535f,  3.8845551014f,  4.7957448959f,  5.7748422623f,  6.8275876045f,  7.9892172813f,
		  9.2980575562f,10.8110599518f, 12.6346817017f, 14.9174613953f, 18.1477069855f, 23.8209209442f

};

const float SiftDetector::PeakP[] = {
		 0.0372311398f,  0.0892269239f,  0.1461775154f,  0.1787834764f,  0.1981789768f,  0.2148804218f,  0.2268953770f,  0.2374276817f,  0.2490262240f,  0.2592820823f,  // 10
		 0.2710254490f,  0.2913467288f,  0.3080614507f,  0.3262100518f,  0.3563414216f,  0.3579748571f
};


void SiftDetector::sortPdf(FeatureList & featurelist) const
{
	if(featurelist.features.size()>0)
	{
		float distFromCenter;
		float halfImageWidth = (float) featurelist.imageWidth/2;
		float halfImageHeight = (float) featurelist.imageHeight/2;

		for(std::vector<Feature>::iterator p=featurelist.features.begin(); p<featurelist.features.end(); ++p)
		{
			distFromCenter = sqrt(pow(p->x - halfImageWidth, 2)+pow(p->y - halfImageHeight, 2));
			p->pdf = DistP[scalarQuantize(distFromCenter, DistC, sizeof(DistC)/sizeof(float))];

			p->pdf = p->pdf * ScaleP[scalarQuantize(p->scale, ScaleC, sizeof(ScaleC)/sizeof(float))];

			p->pdf = p->pdf * OrientP[scalarQuantize(p->orientation, OrientC, sizeof(OrientC)/sizeof(float))];

			p->pdf = p->pdf * PeakP[scalarQuantize(p->peak, PeakC, sizeof(PeakC)/sizeof(float))];
		}

		std::stable_sort(featurelist.features.begin(), featurelist.features.end(), sortPdfPredicate);
	}
}

void SiftDetector::printC()
{
	std::cout.precision(10);

	for (int i=0; i<32; ++i)
	{
		if (OrientC[i] >=0)
			cout << OrientC[i] << "f, ";
		else
			cout << 2*VL_PI + OrientC[i] << "f, ";

		if ((i+1)%10 == 0)
			cout << endl;
	}

	cout << endl;
}

