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

#include "AlpDetector.h"
#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>

// exceptions
#include "CdvsException.h"

using namespace mpeg7cdvs;
using namespace std;

AlpDetector::AlpDetector():alpKeypoints()
{
	verbose = 0;
}

AlpDetector::~AlpDetector()
{}

/*
 * Detect keypoints using the ALP algorithm.
 *
 * ALP: A Low-degree Polynomial detector
 *
 * keypoint information includes:
 * - Horizontal coordinate
 * - Vertical coordinate
 * - Scale
 * - Orientation
 * - Peak
 * - Second derivative in sigma
 * - Ratio of the spatial curvatures
 */
void AlpDetector::detect(FeatureList & featurelist, const Parameters &params)
{
	int noctaves = 0;
	verbose = params.debugLevel;

	if (octaves[0].init(buffer.data(), width, height))		// input: the original image
	{
		octaves[0].detect();			// detect keypoints in this octave
		noctaves++;
	}

	for (int k = 1; k < maxNumberOctaves; ++k)
	{
		if (octaves[k].init(octaves[k - 1]))					// input: the previous octave
		{
			octaves[k].detect();			// detect keypoints in this octave
			octaves[k].detectDuplicates(octaves[k - 1]);	// detect duplicates using previous octave
			noctaves++;							// count the number of octaves
		}
		else
			break;				// end loop
	}

    // get key points at all octaves

	for (int k=0; k<noctaves; ++k)
	{
		octaves[k].getAlpKeypoints(alpKeypoints);	// get the detected key points
	}	// end for all octaves


	// sort keypoints in descending order of importance

	sortPdf(alpKeypoints, width, height);
}

void AlpDetector::extract(FeatureList & featurelist, size_t num) const
{
	if (featurelist.features.size() >= num)
		return;			// nothing to do: the required number of features have already been extracted

//	#pragma omp parallel for default(shared)

	for(std::vector<FeatureAlp>::const_iterator d = alpKeypoints.begin() ; (d < alpKeypoints.end()) && (featurelist.features.size() < num); ++d)
	{
		octaves[d->octave].computeDescriptor(featurelist, *d, true, num - featurelist.features.size());
	}

	if (verbose > 0)
	{
		printHeader("AlpDetector.cpp", featurelist.features.size());
		for(std::vector<Feature>::const_iterator d=featurelist.features.begin(); d<featurelist.features.end(); ++d)
			printDescr(*d);
	}
}

// implementation of sortPdf() for this type of keypoints (ALP)
// statistics optained removing the limit on the number of keypoints stored in the cache files
// so that this feature selection does not depend from any previous version of the same

const float AlpDetector::DistC[] = {
    22.844617f, 44.558786f, 61.568107f, 76.155380f, 89.323812f, 101.485196f, 112.896232f, 123.680915f, 133.983693f, 143.868350f, 153.427612f, 162.703518f, 171.741960f, 180.603609f, 189.372927f, 198.027656f, 206.587537f, 215.062502f, 223.453165f, 231.781280f, 240.120547f, 248.814285f, 257.790032f, 267.037077f, 276.582451f, 286.435909f, 296.567667f, 306.995723f, 317.908285f, 331.220671f, 347.660617f, 369.732523f
};

const float AlpDetector::DistP[] = {
    0.361944f, 0.355746f, 0.347353f, 0.336356f, 0.327837f, 0.318998f, 0.308817f, 0.299182f, 0.289585f, 0.280994f, 0.268077f, 0.256938f, 0.247710f, 0.237395f, 0.225750f, 0.214016f, 0.203422f, 0.191898f, 0.182628f, 0.167823f, 0.165221f, 0.163733f, 0.156702f, 0.148728f, 0.141236f, 0.131043f, 0.119838f, 0.106638f, 0.090266f, 0.083220f, 0.067291f, 0.043126f
};

const float AlpDetector::ScaleC[] = {
    2.042040f, 2.669125f, 3.721873f, 5.093062f, 7.450777f, 10.927115f, 18.254845f, 37.934578f
};

const float AlpDetector::ScaleP[] = {
    0.129539f, 0.209122f, 0.253507f, 0.300881f, 0.309653f, 0.328560f, 0.288677f, 0.230131f
};

const float AlpDetector::PeakC[] = {
    1.232293f, 3.041529f, 5.324250f, 7.999865f, 11.004862f, 14.324853f, 17.965569f, 21.953289f, 26.315967f, 31.156428f, 36.613176f, 42.914120f, 50.496989f, 60.219579f, 74.433250f, 99.991624f
};

const float AlpDetector::PeakP[] = {
    0.048659f, 0.107536f, 0.154757f, 0.187244f, 0.211478f, 0.230324f, 0.246366f, 0.261054f, 0.273547f, 0.288059f, 0.302603f, 0.316947f, 0.335577f, 0.358025f, 0.386192f, 0.399048f
};

const float AlpDetector::CurvSigmaC[] = {
    0.768991f, 1.808803f, 3.081451f, 4.526539f, 6.117329f, 7.866999f, 9.796742f, 11.937961f, 14.331533f, 17.042964f, 20.178996f, 23.935241f, 28.668670f, 34.965996f, 44.775371f, 65.189828f
};

const float AlpDetector::CurvSigmaP[] = {
    0.061990f, 0.128899f, 0.168087f, 0.193768f, 0.213368f, 0.228705f, 0.242957f, 0.256926f, 0.269282f, 0.282173f, 0.293957f, 0.308877f, 0.322290f, 0.334719f, 0.350858f, 0.357814f
};

const float AlpDetector::CurvRatioC[] = {
    4.091766f, 4.303375f, 4.543137f, 4.811809f, 5.111757f, 5.446847f, 5.820148f, 6.234557f, 6.693328f, 7.200291f, 7.759710f, 8.376632f, 9.055713f, 9.801916f, 10.619244f, 11.515857f
};

const float AlpDetector::CurvRatioP[] = {
    0.208944f, 0.210027f, 0.211018f, 0.212091f, 0.214323f, 0.214273f, 0.214968f, 0.214793f, 0.213825f, 0.211447f, 0.208663f, 0.203317f, 0.195487f, 0.183629f, 0.169702f, 0.152641f
};

void AlpDetector::sortPdf(FeatureList & featurelist)
{
	if(featurelist.features.size()>0)
	{
		float distFromCenter;
		float diffx, diffy;
		float halfImageWidth = (float) featurelist.imageWidth/2;
		float halfImageHeight = (float) featurelist.imageHeight/2;

		for(std::vector<Feature>::iterator p=featurelist.features.begin(); p<featurelist.features.end(); ++p)
		{
			diffx = p->x - halfImageWidth;
			diffy = p->y - halfImageHeight;
			distFromCenter = sqrt(diffx*diffx + diffy*diffy);

			p->pdf = fastScalarQuantize(distFromCenter, DistC, DistP, sizeof(DistC)/sizeof(float))
				   * fastScalarQuantize(p->scale, ScaleC, ScaleP, sizeof(ScaleC)/sizeof(float))
				   * fastScalarQuantize(fabs(p->peak), PeakC, PeakP, sizeof(PeakC)/sizeof(float))
				   * fastScalarQuantize(fabs(p->curvRatio), CurvRatioC, CurvRatioP, sizeof(CurvRatioC)/sizeof(float))
				   * fastScalarQuantize(fabs(p->curvSigma), CurvSigmaC, CurvSigmaP, sizeof(CurvSigmaC)/sizeof(float));
		}

		std::stable_sort(featurelist.features.begin(), featurelist.features.end(), sortPdfPredicate);
	}
}

bool AlpDetector::sortAlpPredicate(const FeatureAlp &f1, const FeatureAlp &f2)
{
  return f1.pdf > f2.pdf;
}

void AlpDetector::sortPdf(std::vector<FeatureAlp> & alpFeatures, float imageWidth, float imageHeight)
{
	if(alpFeatures.size()>0)
	{
		float distFromCenter;
		float diffx, diffy;
		float halfImageWidth = 0.5 * imageWidth;
		float halfImageHeight = 0.5 * imageHeight;

		for(std::vector<FeatureAlp>::iterator p=alpFeatures.begin(); p<alpFeatures.end(); ++p)
		{
			diffx = p->x - halfImageWidth;
			diffy = p->y - halfImageHeight;
			distFromCenter = sqrt(diffx*diffx + diffy*diffy);

			p->pdf = fastScalarQuantize(distFromCenter, DistC, DistP, sizeof(DistC)/sizeof(float))
				   * fastScalarQuantize(p->sigma, ScaleC, ScaleP, sizeof(ScaleC)/sizeof(float))
				   * fastScalarQuantize(fabs(p->peak), PeakC, PeakP, sizeof(PeakC)/sizeof(float))
				   * fastScalarQuantize(fabs(p->curvRatio), CurvRatioC, CurvRatioP, sizeof(CurvRatioC)/sizeof(float))
				   * fastScalarQuantize(fabs(p->curvSigma), CurvSigmaC, CurvSigmaP, sizeof(CurvSigmaC)/sizeof(float));
		}

		std::stable_sort(alpFeatures.begin(), alpFeatures.end(), sortAlpPredicate);
	}
}
