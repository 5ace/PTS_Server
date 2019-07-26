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


#include "DistratEigen.h"
#include <string.h>
#include <float.h>
#include <math.h>

#ifdef _WIN32
    #define ISNAN _isnan
#elif _WIN64
    #define ISNAN _isnan
#elif __MACH__
	#define ISNAN std::isnan
# elif defined(__MINGW32__) || defined(__CYGWIN__)
#  define ISNAN(x) (std::isnan(x))
#else
    #define ISNAN isnan
#endif

#ifndef MIN
#define MIN(a,b)  ((a) > (b) ? (b) : (a))
#endif

#ifndef MAX
#define MAX(a,b)  ((a) < (b) ? (b) : (a))
#endif


//definition general field parameters

#define MINNUMPOINTS 5
#define SAMPLES_ASYMPTOTE 5000
#define USE_STANDARD_C_COMPUTATION false		// TODO: change to true and adjust wmThreshold according to False Positive Rate

// private constants
const float	DistratEigen::samplingStep 		=  0.2f;		// Sampling step for histogram computation
const float DistratEigen::maxScaling			=  2.5f;		// Max scaling for histogram computation
const float DistratEigen::minScaling			= -2.5f;		// Min scaling for histogram computation
const float DistratEigen::logImageDiagSize		=  7.0f;		// logarithm of the maximum image diagonal size

// look-up tables
const float DistratEigen::LUTchiSquare99[] = {6.5858f, 9.2205f, 11.3691f, 13.3057f, 15.1171f, 16.8433f, 18.5067f, 20.1213f, 21.6966f, 23.2394f, 24.7545f, 26.2460f, 27.7167f, 29.1692f, 30.6054f, 32.0269f, 33.4352f, 34.8314f, 36.2165f, 37.5914f, 38.9570f, 40.3138f, 41.6625f, 43.0035f, 44.3375f, 45.6648f, 46.9857f, 48.3007f, 49.6101f, 50.9141f};
const float DistratEigen::LUTchiSquare98[] = {5.3220f, 7.7912f,  9.8220f, 11.6605f, 13.3854f, 15.0331f, 16.6242f, 18.1713f, 19.6830f, 21.1654f, 22.6231f, 24.0595f, 25.4773f, 26.8788f, 28.2657f, 29.6396f, 31.0015f, 32.3527f, 33.6941f, 35.0263f, 36.3502f, 37.6662f, 38.9751f, 40.2771f, 41.5728f, 42.8626f, 44.1467f, 45.4256f, 46.6994f, 47.9685f};
const float DistratEigen::LUTchiSquare97[] = {4.6107f, 6.9658f,  8.9172f, 10.6904f, 12.3583f, 13.9547f, 15.4987f, 17.0019f, 18.4724f, 19.9158f, 21.3363f, 22.7373f, 24.1210f, 25.4897f, 26.8450f, 28.1882f, 29.5205f, 30.8428f, 32.1560f, 33.4609f, 34.7581f, 36.0481f, 37.3314f, 38.6085f, 39.8798f, 41.1455f, 42.4062f, 43.6619f, 44.9131f, 46.1598f};
const float DistratEigen::LUTchiSquare96[] = {4.1195f, 6.3849f,  8.2744f,  9.9973f, 11.6213f, 13.1784f, 14.6863f, 16.1560f, 17.5951f, 19.0089f, 20.4012f, 21.7752f, 23.1331f, 24.4769f, 25.8082f, 27.1282f, 28.4379f, 29.7384f, 31.0304f, 32.3146f, 33.5916f, 34.8618f, 36.1259f, 37.3841f, 38.6369f, 39.8846f, 41.1274f, 42.3657f, 43.5997f, 44.8296f};
const float DistratEigen::LUTchiSquare95[] = {3.7468f, 5.9369f,  7.7750f,  9.4560f, 11.0439f, 12.5686f, 14.0468f, 15.4891f, 16.9024f, 18.2918f, 19.6610f, 21.0129f, 22.3497f, 23.6732f, 24.9848f, 26.2858f, 27.5772f, 28.8598f, 30.1344f, 31.4017f, 32.6622f, 33.9163f, 35.1646f, 36.4075f, 37.6452f, 38.8780f, 40.1064f, 41.3304f, 42.5505f, 43.7666f};
const float DistratEigen::LUTchiSquare40[] = {0.2853f, 1.0411f, 1.8881f, 2.7701f, 3.6711f, 4.5844f, 5.5064f, 6.4348f, 7.3684f, 8.3061f, 9.2473f, 10.1915f, 11.1382f, 12.0871f, 13.0380f, 13.9907f, 14.9449f, 15.9006f, 16.8576f, 17.8157f, 18.7750f, 19.7353f, 20.6965f, 21.6586f, 22.6216f, 23.5853f, 24.5497f, 25.5148f, 26.4806f, 27.4470f};
const float DistratEigen::LUTchiSquare50[] = {0.4705f, 1.4047f, 2.3815f, 3.3697f, 4.3625f, 5.3577f, 6.3543f, 7.3517f, 8.3497f, 9.3480f, 10.3467f, 11.3456f, 12.3447f, 13.3439f, 14.3432f, 15.3425f, 16.3420f, 17.3415f, 18.3411f, 19.3407f, 20.3404f, 21.3400f, 22.3398f, 23.3395f, 24.3392f, 25.3390f, 26.3388f, 27.3386f, 28.3384f, 29.3383f};
const float DistratEigen::LUTchiSquare60[] = {0.7222f, 1.8443f, 2.9541f, 4.0501f, 5.1357f, 6.2134f, 7.2851f, 8.3518f, 9.4144f, 10.4736f, 11.5299f, 12.5837f, 13.6352f, 14.6848f, 15.7326f, 16.7788f, 17.8235f, 18.8670f, 19.9092f, 20.9503f, 21.9904f, 23.0295f, 24.0677f, 25.1051f, 26.1417f, 27.1776f, 28.2128f, 29.2473f, 30.2812f, 31.3145f};
const float DistratEigen::LUTchiSquare70[] = {1.0768f, 2.4070f, 3.6612f, 4.8734f, 6.0586f, 7.2249f, 8.3770f, 9.5179f, 10.6498f, 11.7742f, 12.8922f, 14.0047f, 15.1123f, 16.2158f, 17.3155f, 18.4117f, 19.5049f, 20.5954f, 21.6832f, 22.7687f, 23.8520f, 24.9333f, 26.0127f, 27.0904f, 28.1664f, 29.2409f, 30.3139f, 31.3856f, 32.4559f, 33.5250f};
const float DistratEigen::LUTchiSquare80[] = {1.6203f, 3.1985f, 4.6222f, 5.9702f, 7.2718f, 8.5414f, 9.7874f, 11.0149f, 12.2276f, 13.4279f, 14.6179f, 15.7989f, 16.9721f, 18.1385f, 19.2987f, 20.4534f, 21.6032f, 22.7484f, 23.8896f, 25.0269f, 26.1607f, 27.2913f, 28.4188f, 29.5435f, 30.6656f, 31.7852f, 32.9024f, 34.0174f, 35.1304f, 36.2413f};
const float DistratEigen::LUTchiSquare90[] = {2.6390f, 4.5590f, 6.2139f, 7.7472f, 9.2078f, 10.6188f, 11.9933f, 13.3395f, 14.6630f, 15.9677f, 17.2566f, 18.5318f, 19.7952f, 21.0481f, 22.2917f, 23.5270f, 24.7547f, 25.9755f, 27.1901f, 28.3989f, 29.6024f, 30.8009f, 31.9948f, 33.1845f, 34.3701f, 35.5519f, 36.7302f, 37.9051f, 39.0769f, 40.2457f};


// gaussian filter taps
const float DistratEigen::m_GaussianKernel[] = {0.0404f, 0.9192f, 0.0404f};

DistratEigen::DistratEigen(const float *x1, const float *x2, const float *y1, const float *y2, int size):
		m_x1(x1), m_x2 (x2), m_y1 (y1), m_y2 (y2), m_nPoints(size), DA(size, size), DB(size, size), m_nFeatures(0)
{
	// initialization
	LDR = NULL;
	m_c = 0;

	// Initialization configuration parameters
	minNumPoints = MINNUMPOINTS;

	// creation of bins for histogram computation (independent on the numeric values of the processed data)
	m_nBins = maxNumBins;
	for (int i=0;i<m_nBins; i++)
		m_Bins[i] = minScaling + i*samplingStep;
}

DistratEigen::~DistratEigen(void)
{
	if (LDR != NULL)
		delete [] LDR;
}

int DistratEigen::estimateInliers(bool useParametric, bool computeInliers, unsigned int percentile, int * inliersIndexes)
{
	if (m_nPoints < minNumPoints)
		return 0;

	coord2dist(m_x1, m_x2, m_nPoints, DA, true);
	coord2dist(m_y1, m_y2, m_nPoints, DB, true);


	/****** Initialization ******/

	//compute statistics of the input vectors
	float stdx1, stdx2, stdy1, stdy2;
	float meanx1, meanx2, meany1, meany2;
	vectorStatistics(m_x1, m_nPoints, &meanx1, &stdx1);
	vectorStatistics(m_x2, m_nPoints, &meanx2, &stdx2);
	vectorStatistics(m_y1, m_nPoints, &meany1, &stdy1);
	vectorStatistics(m_y2, m_nPoints, &meany2, &stdy2);

	m_stdA = (stdx1 + stdx2)/2;
	m_stdB = (stdy1 + stdy2)/2;

	if(useParametric)
		prepareParametric();
	else
		prepareNonParametric();

	int nInliers = 0;

	// Goodness of fit
	m_bFitIsGood = goodnessOfFit(percentile);
	if(!m_bFitIsGood && computeInliers)
	{
		nInliers = MLCoherence(inliersIndexes);
		//nInliers = retainGoodInliers();
	}
	return nInliers;
}



/* DistratEigen initialization

In this function the numeric information exploited by the following goodness of fit and ML coherence for the parametric case are computed:
FVaues, histograms and LogDistanceRatios. */

void DistratEigen::prepareParametric()
{
	/****** DISTRAT PART I : data preparation PARAMETRIC******/

    m_diagScaling =  m_stdA/m_stdB;

	//Computation of F-distribution according to the computed diagScaling

	logRootF(m_Bins, m_Fvalues, m_nBins, m_diagScaling);

	//Compute possible resampling factor
	float nDistPerBin = 0.4f;
	m_nSamples = m_nPoints*(m_nPoints-1)/2;

	//gli intervalli (bin) dell'istogramma devono essere
    // piu' larghi se ci sono poche feature

    // il valore atteso minimo di distanze per bin
    // implica un certo intervallo di campionamento

	int resFacA, resFacB;
	int index = 0;
	float partialSum = 0;
	while(partialSum<nDistPerBin)
	{
		partialSum += m_Fvalues[index]*m_nSamples;
		index++;
	}
	resFacA = index;

	index = 0;
	partialSum = 0;
	while(partialSum<nDistPerBin)
	{
		partialSum += m_Fvalues[m_nBins-index-1]*m_nSamples;
		index++;
	}
	resFacB = index;

	int resamplingFactor = MIN(10, MAX(resFacA, resFacB));

	// DEBUG
	// printf("RESAMPLING FACTOR = %d\n", resamplingFactor);

	float stepsize = samplingStep;

	if (resamplingFactor != 1)
	{
		m_nBins = m_nBins/resamplingFactor;

		for (int i=0;i<m_nBins; i++)
			m_Bins[i] = minScaling+((i+1)*samplingStep*resamplingFactor-samplingStep);

		logRootF(m_Bins, m_Fvalues, m_nBins, m_diagScaling);
		stepsize = m_Bins[1]- m_Bins[0];	//Per evitare problemi numerici m_Bins[1]-m_Bins[0];
	}


	// Matrix of log distance ratio
	LDR = new float [m_nSamples];

	// Compute log distance ratio - the values are divided by two since the coor2dist values are computed at a squared value
	// Optimization to avoid the sqrt in the Coord2dist function

	// Rapporto delle distanze

	int counter=0;
	for (int i=0; i< m_nPoints; i++)
	{
		for (int j=0; j< i; j++)
		{
			LDR[counter] = 0.5f * log((DA(i,j) + 1e-6f)  /   (DB(i,j) + 1e-9f)); 		// Added small numbers to avoid Inf
			counter++;
		}
	}

	// istogramma per i valori campionati

	for (int i=0; i<m_nBins; i++ )
	{
		m_Edges[i] =  m_Bins[i]-(stepsize/2);
	}
	m_Edges[m_nBins] = m_Edges[m_nBins-1] + stepsize;

	computeHist(m_Hist,LDR,m_Edges,m_nSamples,m_nBins+1);

	for(int i=0; i<m_nBins; i++)
	{
		m_Fvalues[i] = m_Fvalues[i]*stepsize;
	}

	return;
}


void DistratEigen::convolution (const float *x, float *y, const float *h, int sampleCount, int kernelCount)
{
	for ( int i = 0; i < sampleCount+kernelCount-1; i++ )
	{
		y[i] = 0;                       // set to zero before sum
		for ( int j = 0; j < kernelCount; j++ )
		{
			if((i-j)>=0 && (i-j)<sampleCount)
				y[i] += x[i - j] * h[j];    // convolve: multiply and accumulate
		}
	}
}

/*
 * Distrat initialization
 * In this function the numeric information exploited by the following goodness of fit and ML coherence for the parametric case are computed:
 * FVaues, histograms and LogDistanceRatios.
 */
void DistratEigen::prepareNonParametric()
{
	// DISTRAT PART I : data preparation NON PARAMETRIC

	//Compute log distance ratio - the values are divided by two since the coor2dist values are computed at a squared value
	//Optimization to avoid the sqrt in the Coord2dist function
	m_nSamples = m_nPoints*(m_nPoints-1)/2;
	delete [] LDR;
	LDR = new float [m_nSamples];

	float *distEnum = new float [m_nSamples];
	float *distDenom = new float [m_nSamples];

	// calcola il logaritmo delle distanze

	int counter = 0;
	for (int i=0; i< m_nPoints; i++)
	{
		for (int j=0; j< i; j++)
		{
			distEnum[counter] = 0.5f * log(DA(i,j) + 1e-6f); 							//Added small numbers to avoid Inf
			distDenom[counter] = 0.5f * log(DB(i,j) + 1e-9f);
			LDR[counter] = distEnum[counter] - distDenom[counter];
			counter++;
		}
	}

    //  la pdf modello
	float stepsize = samplingStep;
	int numElForT = (int)ceil(logImageDiagSize/stepsize);
	float *histEnum = new float [numElForT+1];
	float *histDenom = new float [numElForT+1];
	float * edges = new float [numElForT+1];

	// Computation edges for first histogram

	float *t = new float [numElForT];
	for (int i=0; i<numElForT; i++)
	{
		t[i] = stepsize*i;
		edges[i] =  t[i]-(stepsize/2);

	}
  	edges[numElForT] = edges[numElForT-1] + stepsize;

	//Compute histogram
	computeHist(histEnum,distEnum,edges,m_nSamples,numElForT+1);
	computeHist(histDenom,distDenom,edges,m_nSamples,numElForT+1);

	// Smussamento dela funzione modello (sembra apportare miglioramenti,
    // provato con i dataset CDVS). Diminuisce anche la probabilita' che
    // qualche elemento di modelDensity sia uguale a zero (il coefficiente c
    // di accostamento diventerebbe di conseguenza infinito).

	float * modelDensity = new float[2*numElForT-1];
	float * histDenomI = new float[numElForT];
	float * histEnumI = new float[numElForT];

	for(int i=0; i<numElForT;i++)
	{
		histDenomI[i] = histDenom[numElForT-i-1];
		histEnumI[i] = histEnum[i];
	}

	convolution(histEnumI, modelDensity, histDenomI, numElForT, numElForT);

	float sumDensity = 0;
	for(int i = 0; i<2*numElForT-1;i++)
	{
		modelDensity[i] += 1e-8f;
		sumDensity += modelDensity[i];
	}

	for(int i = 0; i<2*numElForT-1;i++)
	{
		modelDensity[i] = modelDensity[i]/(sumDensity*stepsize);
	}

	// Smussamento funzione modello
	float *filteredModelDensity = new float[2*numElForT-1 + gaussianFilterDim -1];
	convolution(modelDensity, filteredModelDensity, m_GaussianKernel, 2*numElForT-1, gaussianFilterDim);

	// l'istogramma dei log distance ratio
	int samplesToKeep = m_nBins / 2;
	m_nBins -= 1;
	int halfIndex = m_nBins/2;

	m_Bins [halfIndex] = 0;
	for(int i=1; i< samplesToKeep; i++)
	{
		m_Bins[halfIndex+i] = t[i];
		m_Bins[halfIndex-i] = -t[i];
	}

	for(int i=0; i<m_nBins; i++)
	{
		m_Fvalues[i] = filteredModelDensity[numElForT-halfIndex+i]*stepsize;
		m_Edges[i] = m_Bins[i] -stepsize/2;
	}
	m_Edges[m_nBins] = m_Edges[m_nBins-1] + stepsize;

	computeHist(m_Hist,LDR,m_Edges,m_nSamples,m_nBins+1);

	delete [] distDenom;
	delete [] distEnum;
	delete [] histDenom;
	delete [] histDenomI;
	delete [] histEnum;
	delete [] histEnumI;
	delete [] t;
	delete [] filteredModelDensity;
	delete [] modelDensity;
	delete [] edges;
	return;
}


//   test di accostamento tra istogramma e densita` ipotizzata
//   histogram       istogramma
//   modelDensity    densita` ipotizzata
//   cThreshold      chisquare_(1-alfa,k-1)
//   nSamplesAsymptote (optional) altera il numero di campioni
//   Larsen, Marx:
//   An Introduction to Mathematical Statistics and its Applications,
//   pagina 402, capitolo 9.3
bool DistratEigen::goodnessOfFit(unsigned int percentile)
{
	//Chi Square test
	int histogramLength = m_nBins;
	int nDegrees = m_nBins - 1;

	float c= 0;

	// Values computed using the chisquare implementation in Matlab, varying the number of degrees of freedom from 1 to 30, at a given chisquare percentile
	// In matlab: chisquare(95,1:30);

	const float *LUTchiSquare;
	switch(percentile)
	{
		case 99:	LUTchiSquare = LUTchiSquare99;
					break;
		case 98:	LUTchiSquare = LUTchiSquare98;
					break;
		case 97:	LUTchiSquare = LUTchiSquare97;
					break;
		case 96:	LUTchiSquare = LUTchiSquare96;
					break;
		case 95:	LUTchiSquare = LUTchiSquare95;
					break;
		case 90:	LUTchiSquare = LUTchiSquare90;
					break;
		case 80:	LUTchiSquare = LUTchiSquare80;
					break;
		case 70:	LUTchiSquare = LUTchiSquare70;
					break;
		case 60:	LUTchiSquare = LUTchiSquare60;
					break;
		case 50:	LUTchiSquare = LUTchiSquare50;
					break;
		case 40:	LUTchiSquare = LUTchiSquare40;
					break;
		default:	LUTchiSquare = LUTchiSquare99;
					break;
	}


	float threshold = LUTchiSquare[nDegrees-1]; // one degree of freedom less than the number of elements

	//Goodness of fit
	m_nFeatures = 0;
	for(int i=0; i<histogramLength; i++)
	{
		m_nFeatures+= m_Hist[i];
	}

	if(USE_STANDARD_C_COMPUTATION)
	{
		// Metodo standard per il calcolo di c
		//np computation
		for(int i=0; i<histogramLength; i++)
		{
			m_Np [i] = m_Fvalues[i]*m_nFeatures;
		}

		//Computation of normalized error
		for(int i=0; i<histogramLength; i++)
		{
			float diff = (m_Hist[i]-m_Np [i]);
			c+=((diff * diff)/m_Np [i]);
		}
	}
	else
	{
		// La sequente e' una modifica rispetto alla teoria e serve per alterare il
		// comportamento in presenza di grandi numeri di campioni o con statistiche
		// strane (lenta convergenza alla funzione modello).
		// All'aumentare del numero di campioni, il test originario richiede che
		// histogram e modelDensity debbano assomigliarsi sempre di piu' per essere
		// considerati simili.
		int upperLimit = SAMPLES_ASYMPTOTE;
		float newN = upperLimit*(1-exp(-m_nFeatures/upperLimit));
		float *newHist = new float [histogramLength];

		//np computation
		for(int i=0; i<histogramLength; i++)
		{
			newHist[i] = m_Hist[i]*newN/m_nFeatures;
			m_Np [i] = m_Fvalues[i]*newN;
		}

		//Computation of normalized error
		for(int i=0; i<histogramLength; i++)
		{
			float diff = (newHist[i]-m_Np[i]);
			c+=((diff * diff)/m_Np [i]);
		}

		delete [] newHist;
	}

	if(ISNAN(c))
	{
		c = 0;
	}

	m_c = c;
	m_GoFThreshold = threshold;

	if(c >= threshold /*|| kl >= 0.1*/)
		return false;
	else
		return true;
}

// DISTRAT PART 3 : estimation of number of inliers and identification on inliers when computeInliers == true
// if the inliersIndexes parameter is null only the number of inliers is provided as output.
int DistratEigen::MLCoherence(int * inliersIndexes)
{
	//Check to evaluate the amount of the histogram that can be represented my the model function
	int histogramLength = m_nBins;

	float *modelHistogram = new float [histogramLength];
	float *quantizedDistRatio = new float [m_nSamples];
	MatrixXf G(m_nPoints,m_nPoints);

	for(int i=0; i<histogramLength; i++)
	{
		modelHistogram[i] = m_Fvalues[i] * m_nFeatures;
	}

	//**** Optimized - to be verified that the function is monotonic
	float coeff, coeff1 = 0, coeff2 = 0;

	for(int i=0; i<histogramLength; i++)
	{
		coeff1 += m_Hist[i]*modelHistogram[i];
		coeff2 += modelHistogram[i]*modelHistogram[i];
	}
	coeff = coeff1/coeff2;

	// l'istogramma featureHistogram e' la somma pesata di due termini
	for(int i=0; i<histogramLength; i++)
	{
		m_differenceCurve[i] = m_Hist[i] -coeff*modelHistogram[i];
	}

	// la matrice G contiene valori che permettono il calcolo degli inlier
	// mediante autovalore e autovettore dominante
	//Quantizzazione uniforme
	uniformQuantize(LDR, m_Edges, m_nSamples, m_nBins, quantizedDistRatio);

	for(int i=0; i< m_nSamples; i++)
	{
		quantizedDistRatio[i] = m_differenceCurve[(int)quantizedDistRatio[i] -1];
	}


	//Creazione di G
	int counter=0;
	for (int i=0; i < m_nPoints; i++)
	{
		G(i,i) = 0.0f;
	}
	for (int i=0; i< m_nPoints; i++)
	{
		for (int j=0; j< i; j++)
		{
			G(j,i) = quantizedDistRatio[counter];
			G(i,j) = quantizedDistRatio[counter];
			counter++;
		}
	}

	// cerchiamo l'autovettore dominante di G
	VectorXf u(m_nPoints);
	float lambda = 0.0f;

	eigPowIteration(G, u, lambda, 4);	// compute dominant eigenvector of G

	float *pin = new float [histogramLength];

	float pinsum = 0;
	float max = 0;
	for(int i = 0; i < histogramLength; i++)
	{
		if (m_differenceCurve[i]> 0)
		{
			pin[i] = m_differenceCurve[i];
			pinsum += m_differenceCurve[i];
			if (m_differenceCurve[i] > max)
				max = m_differenceCurve[i];
		}
		else
			pin[i] = 0;
	}

	// Estimate inliers
	int nEstimatedInliers = (int)floor(lambda/max)+1;
	nEstimatedInliers = MIN(nEstimatedInliers, m_nPoints); // E' molto raro ma e' capitato che il numero stimato fosse maggiore del numero di punti

	if (inliersIndexes == NULL)
		return nEstimatedInliers;		// if no output vector is provided, we can stop here the processing

	// gli inlier corrisponderanno agli nEstimatedInliers elementi maggiori dell'autovettore dominante

	int indBig = 0;
	float valueBig = 0;
	for (int i = 0; i < m_nPoints; i++)
	{
		float val = fabs(u(i));
		if(val > valueBig)
		{
			indBig = i;
			valueBig = val;
		}
	}

	int sign = (u(indBig) < 0) ? -1 : 1;

	for (int i = 0; i<m_nPoints; i++)
	{
		u(i) = u(i)*sign;
	}

	int *indexes = new int[m_nPoints];
	for (int i = 0; i<m_nPoints; i++)
	{
		indexes[i] = i;
	}

	quicksort(u, 0, m_nPoints-1, indexes);

	//retrieval n first numbers
	for(int i=0; i<nEstimatedInliers; i++)
	{
		inliersIndexes[i] = indexes[m_nPoints-1-i];
	}

	delete [] indexes;
	delete [] quantizedDistRatio;
	delete [] pin;
	delete [] modelHistogram;

	return nEstimatedInliers;
}

void DistratEigen::eigPowIteration (const MatrixXf &G, VectorXf &u, float &lambda, int maxIterations)
{
	size_t nPoints = G.rows();
	static const float zerof = 1e-6f;		// floating point threshold for "zero"

	// Metodo delle potenze per calcolare il primo autovettore e relativo autovettore

	float changeThreshold = 1e-3f;

	for (unsigned int i = 0; i<nPoints; i++)
	{
		float acc = 0;
		for (unsigned int j = 0; j<nPoints; j++)
		{
			acc += G(i,j);
		}
		u(i) = acc;
	}

	lambda = u.norm();

	if (lambda < zerof)		// se la norma e' praticamente zero, esco con 1,0,0,0...
	{
		u.Zero(u.rows());
		u(0) = 1.0f;
		return;
	}

	VectorXf unew(nPoints);
	VectorXf diff(nPoints);

	u.normalize();

	for (int i=0; i<maxIterations; i++)
	{
		unew = G * u; 							// unew = G*u;
		lambda = unew.norm();					// lambda = norm(unew);
		unew.normalize();						// normalized unew

		diff = unew - u;						// diff = unew - u
		u = unew;								// u = unew
		if (diff.norm() < changeThreshold)	//  if norm(diff) < changeThreshold
		{
			break;
		}

	}
}

void DistratEigen::coord2dist(const float *v1, const float *v2, int nPoints, MatrixXf & result, bool squared)
{
	Matrix<float, 4, Dynamic> A(4,nPoints);
	Matrix<float, 4, Dynamic> B(4,nPoints);

	for (int j = 0; j < nPoints; j++)
	{
		A(0,j) = (v1[j] * v1[j]) + (v2[j] * v2[j]);
	}

	for (int j =0; j <nPoints; j++)
	{
		A(1,j) = 1;
		A(2,j) = -2*v1[j];
		A(3,j) = -2*v2[j];
		B(0,j) = 1;
		B(1,j) = A(0, j);
		B(2,j) = v1[j];
		B(3,j) = v2[j];
	}

	result = A.transpose() * B;

	//Set zero to the diagonal elements
	for (int i=0; i < nPoints; i++)
		result(i, i)= 0.0f;

	if (!squared)
	{
		result = result.array().sqrt().matrix();
	}

	return;
}

void DistratEigen::vectorStatistics (const float *arr, int n, float *average, float *std_dev)
{
  float    sum = 0;           /* The sum of all elements so far. */
  float    sum_std = 0;      /* The sum of their squares. */
  float    variance;
  int       i;

  for (i = 0; i < n; i++)
    sum += arr[i];

  /* Compute the average and variance,*/
    *average = sum / (float)n;

  for (i = 0; i < n; i++)
  {
	  float x = (arr[i]-*average);
	  sum_std += (x * x);
  }

  variance = sum_std / (float)(n-1);

  /* Compute the standard deviation. */
  *std_dev = sqrt(variance);

  return;
}

void DistratEigen::logRootF(const float *input, float *output, int nValues, float scalingFactor)
{
	float square_sf = scalingFactor * scalingFactor;
	for (int i=0; i < nValues; i++)
	{
		float iexp = exp(input[i]);
		float square_iexp = iexp * iexp;
		float val = scalingFactor*iexp/(square_iexp + square_sf);
		output[i] = 2 * val * val ;

		//output[i] = 2 * pow((scalingFactor*exp(input[i])/(exp(2*input[i]) + square_sf)),2);

	}
}

void DistratEigen::computeHist(float *hist, float *input, float *edges, long nValues, int nEdges)
{
	int half = (int)(nEdges/2);
	int j;

	for (int i=0; i < (nEdges - 1); i++)
		hist[i] = 0.0f;

	for (int i=0; i < nValues; i++)
	{
		float value = input[i]+1e-5f;
		if(value>edges[half-1])
		{
			for (j = half; j<nEdges; j++)
				if(value<edges[j])
				{
					j--;
					break;
				}
		}
		else{
			for (j = half-2; j>=0; j--)
			{
				if(value>=edges[j])
					break;
			}
		}

		if(j>=0 && j<nEdges)
			hist[j] += 1.0f;

	}
}

void DistratEigen::uniformQuantize(float *distRatios,float *samplingGrid, long nElem, int nEdges, float *results)
{

	float stepSize = samplingGrid[1]-samplingGrid[0];
	float step = (samplingGrid[0]);

	for(int i=0; i< nElem; i++)
	{
		if (ISNAN(distRatios[i]))
			results[i] = 1;
		else
		{
			results[i] = ceil(((distRatios[i]+1e-5f)-step)/stepSize);
			if(results[i] < 1)
				results[i] = 1;
			if (results[i] > nEdges)
				results[i] = (float)nEdges;
		}
	}
}

void DistratEigen::quicksort(VectorXf & vector, int beg, int end, int *indexes)
{

    int  l,r,p;

    while (beg<end)    // This while loop will substitude the second recursive call
    {
        l = beg; p = (beg+end)/2; r = end;

        float piv = vector[p];
		int ind = indexes[p];

        while (1)
        {
            while ( (l<=r) && ( vector[l]<= piv )) l++;
            while ( (l<=r) && ( vector[r]> piv  )) r--;

            if (l>r) break;

            float tmp=vector[l]; vector[l]=vector[r]; vector[r]=tmp;
			int tmpInd =indexes[l]; indexes[l]=indexes[r]; indexes[r]=tmpInd;

            if (p==r) p=l;

            l++; r--;
        }

        vector[p]=vector[r]; vector[r]=piv;
		indexes[p] = indexes[r];
		indexes[r] = ind;
        r--;

        // Select the shorter side & call recursion. Modify input param. for loop
        if ((r-beg)<(end-l))
        {
            quicksort(vector, beg, r, indexes);
            beg=l;
        }
        else
        {
            quicksort(vector, l, end, indexes);
            end=r;
        }
    }
}
