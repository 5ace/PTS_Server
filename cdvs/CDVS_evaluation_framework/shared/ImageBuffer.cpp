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
 *
 */

#include "ImageBuffer.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <cassert>
#include <vector>
#include <algorithm>

// Additional include needed to use the resampler library
#define STBI_HEADER_FILE_ONLY
#include "resampler.h"
#include "stb_image.c"

// exceptions
#include "CdvsException.h"

#ifdef max
	#undef max
	#undef min
#endif

using namespace std;
using namespace mpeg7cdvs;

ImageBuffer::ImageBuffer():buffer()
{
}

ImageBuffer::~ImageBuffer()
{
}

//
// Read a planar luminance image from a buffer.
//
void ImageBuffer::read(int ext_width, int ext_height, const unsigned char * ext_buffer)
{
	if ((ext_width <= 0) || (ext_height <= 0))		// check size
		return;

	height = originalHeight = ext_height;					// set height
	width = originalWidth = ext_width;						// set width

	size_t image_size = width * height;		// compute size
	buffer.assign(ext_buffer, image_size);				// resize buffer and copy all data
}


bool ImageBuffer::resize(int newheight, int newwidth)
{
	height = newheight;
	width  = newwidth;
	return buffer.resize(width * height);
}


void ImageBuffer::resample(double rfactor)
{
	int resampled_height = (int) (height *  rfactor + 0.5);
	int resampled_width = (int) (width *  rfactor + 0.5);

	Buffer resampled(resampled_height * resampled_width);
	resampleImage(buffer.data() , width, height, 1,
			resampled.data(), resampled_width, resampled_height, "lanczos3", 0);

	// swap buffer and copy the resampled dimensions

	buffer.swap(resampled);
	height = resampled_height;
	width = resampled_width;
}

void ImageBuffer::swap (ImageBuffer & other)
{
	std::swap(height, other.height);
	std::swap(width, other.width);
	std::swap(originalHeight, other.originalHeight);
	std::swap(originalWidth, other.originalWidth);
	buffer.swap(other.buffer);
}

void ImageBuffer::resample(ImageBuffer & dest) const
{
	resampleImage(buffer.data() , width, height, 1,
				dest.buffer.data(), dest.width, dest.height, "lanczos3", 0);
}

void ImageBuffer::resampleIfGreater(int maxSize)
{
	int maxOriginalSize = (width > height? width : height);
	if (maxOriginalSize > maxSize)
	{
		double rfactor = (double) maxSize / (double) maxOriginalSize;
		resample(rfactor);	// resample image
	}
}


void ImageBuffer::resampleImage(
   const unsigned char* pSrc_image,
   int src_width,
   int src_height,
   int n,			// num of components
   unsigned char* dst_image,
   int dst_width,
   int dst_height,
   const char* pFilter,
   int debugLevel)
{

   if ((std::min(dst_width, dst_height) < 1) || (std::max(dst_width, dst_height) > RESAMPLER_MAX_DIMENSION))
   {
	   throw CdvsException("ImageBuffer.resampleImage: invalid output width/height");
   }

   if (!pSrc_image)
   {
	   throw CdvsException("ImageBuffer.resampleImage: failed loading image");
   }

   if (debugLevel >= 2)
		cout << "Resolution: " << src_width << "x" << src_height << ", Channels: " << n << endl;

   const int max_components = 4;

   if ((std::max(src_width, src_height) > RESAMPLER_MAX_DIMENSION) || (n > max_components))
   {
	   throw CdvsException("ImageBuffer.resampleImage: Image is too large");
   }

   // Partial gamma correction looks better on mips. Set to 1.0 to disable gamma correction.
   //const float source_gamma = 1.75f;
   const float source_gamma = 1.0f;


   // Filter scale - values < 1.0 cause aliasing, but create sharper looking mips.
   const float filter_scale = 1.0f;//.75f;

   float srgb_to_linear[256];
   for (int i = 0; i < 256; ++i)
      srgb_to_linear[i] = (float)pow(i * 1.0f/255.0f, source_gamma);

   const int linear_to_srgb_table_size = 4096;
   unsigned char linear_to_srgb[linear_to_srgb_table_size];

   const float inv_linear_to_srgb_table_size = 1.0f / linear_to_srgb_table_size;
   const float inv_source_gamma = 1.0f / source_gamma;

   for (int i = 0; i < linear_to_srgb_table_size; ++i)
   {
      int k = (int)(255.0f * pow(i * inv_linear_to_srgb_table_size, inv_source_gamma) + .5f);
      if (k < 0) k = 0; else if (k > 255) k = 255;
      linear_to_srgb[i] = (unsigned char)k;
   }

   Resampler* resamplers[max_components];
   std::vector<float> samples[max_components];

   resamplers[0] = new Resampler(src_width, src_height, dst_width, dst_height, Resampler::BOUNDARY_CLAMP, 0.0f, 1.0f, pFilter, NULL, NULL, filter_scale, filter_scale);
   samples[0].resize(src_width);
   for (int i = 1; i < n; i++)
   {
      resamplers[i] = new Resampler(src_width, src_height, dst_width, dst_height, Resampler::BOUNDARY_CLAMP, 0.0f, 1.0f, pFilter, resamplers[0]->get_clist_x(), resamplers[0]->get_clist_y(), filter_scale, filter_scale);
      samples[i].resize(src_width);
   }

   const int src_pitch = src_width * n;
   const int dst_pitch = dst_width * n;
   int dst_y = 0;

   if (debugLevel >= 2)
	   cout << "Resampling to " << dst_width << "x" << dst_height << endl;

   for (int src_y = 0; src_y < src_height; src_y++)
   {
      const unsigned char* pSrc = &pSrc_image[src_y * src_pitch];

      for (int x = 0; x < src_width; x++)
      {
         for (int c = 0; c < n; c++)
         {
            if ((c == 3) || ((n == 2) && (c == 1)))
               samples[c][x] = *pSrc++ * (1.0f/255.0f);
            else
               samples[c][x] = srgb_to_linear[*pSrc++];
         }
      }

      for (int c = 0; c < n; c++)
      {
         if (!resamplers[c]->put_line(&samples[c][0]))
         {
        	 throw CdvsException("ImageBuffer.resampleImage: Out of memory");
         }
      }

      for ( ; ; )
      {
         int c;
         for (c = 0; c < n; c++)
         {
            const float* pOutput_samples = resamplers[c]->get_line();
            if (!pOutput_samples)
               break;

            const bool alpha_channel = (c == 3) || ((n == 2) && (c == 1));
            assert(dst_y < dst_height);
            unsigned char* pDst = &dst_image[dst_y * dst_pitch + c];

            for (int x = 0; x < dst_width; x++)
            {
               if (alpha_channel)
               {
                  int c = (int)(255.0f * pOutput_samples[x] + .5f);
                  if (c < 0) c = 0; else if (c > 255) c = 255;
                  *pDst = (unsigned char)c;
               }
               else
               {
                  int j = (int)(linear_to_srgb_table_size * pOutput_samples[x] + .5f);
                  if (j < 0) j = 0; else if (j >= linear_to_srgb_table_size) j = linear_to_srgb_table_size - 1;
                  *pDst = linear_to_srgb[j];
               }

               pDst += n;
            }
         }
         if (c < n)
            break;

         dst_y++;
      }
   }

   for (int i = 0; i < n; i++)
   {
      delete (resamplers[i]);
   }
}



unsigned int ImageBuffer::scalarQuantize(float value, const float *data, size_t size)
{
	unsigned int bt;

	if(size>0)
	{
		float minDist = 1e10f;

		for(int i=0; i<size; ++i)
		{
			float dist = fabs(value - data[i]);
			if(dist<minDist)
			{
				bt = i;
				minDist = dist;
			}
		}
	}
	else
	{
		throw CdvsException("FeatureList::quantize, Error: no data");
	}

	return bt;
}

float ImageBuffer::fastScalarQuantize(float value, const float *data,const float *output, size_t size)
{
	int first = 0;
	int count = (int) size;
	while (count > 0)
	{
		int step = count / 2;
		int k = first + step;
		if (value < data[k])
			count = step;
		else
		{
			first = k + 1;
			count -= step + 1;
		}
	}

	// first and last are special cases

	if (first == 0)
		return output[0];

	if (first == (int) size)
		return output[first - 1];

	// all other cases are decided here

	if ((data[first] - value) < (value - data[first - 1]))
		return output[first];
	else
		return output[first - 1];
}

float ImageBuffer::fastInterpolate(float value, const float *data, const float *output, size_t size)
{
	int first = 0;
	int count = (int) size;
	while (count > 0)
	{
		int step = count / 2;
		int k = first + step;
		if (value < data[k])
			count = step;
		else
		{
			first = k + 1;
			count -= step + 1;
		}
	}

	// first and last are special cases

	if (first == 0)
		return output[0];

	if (first == (int) size)
		return output[first - 1];

	// all other cases are interpolated here

	float norm = data[first] - data[first - 1];
	float weight1 = data[first] - value;			// distance from higher value ==> weight for lower value
	float weight2 = value - data[first - 1];		// distance from lower value ==> weight for higher value

	return (weight2*output[first] + weight1*output[first - 1]) / norm;
}

void ImageBuffer::print(const vector<Feature> & keypoints, const char * source)
{
	printHeader(source, keypoints.size());
	for(std::vector<Feature>::const_iterator d=keypoints.begin(); d<keypoints.end(); ++d)
		printDescr(*d);
}

void ImageBuffer::printHeader(const char * source, size_t npoints)
{
	cout << "------- " << npoints << " keypoints detected by " << source << ": --------"<< endl;
	cout << "x y scale orientation peak curvRatio curvSigma iscale octave" << endl;
}

void ImageBuffer::printDescr(const Feature & d)
{
	cout << "keypoint: " << d.x << " " << d.y << " " << d.scale << " " << d.orientation << " " << d.peak << " "
				<< d.curvRatio << " " << d.curvSigma << " " << d.iscale << " " << d.octave << endl;

	cout << "descr: " << d.descr[0];
	for (int k=1; k < 128; ++k) {
		cout << "," << d.descr[k] ;
	}
	cout << endl;
}

bool ImageBuffer::sortPdfPredicate(const Feature &f1, const Feature &f2)
{
  return f1.pdf > f2.pdf;
}

bool ImageBuffer::sortBySigmaPredicate(const Feature &f1, const Feature &f2)
{
  return f1.scale < f2.scale;
}

bool ImageBuffer::sortPredicate(const float & a, const float & b) 
{
	return a > b;
}


void ImageBuffer::writeBMP(const char * filename, const float * source, int w, int h)
{
	int size = w*h;
	unsigned char * tmpimage = new unsigned char [size];

	unsigned char * dest = tmpimage;		// set destination
	for (int i=0; i < size; ++i)
		*(dest++) = *(source++);		// copy and convert from float to char

	stbi_write_bmp(filename, w, h, 1, tmpimage);

	delete [] tmpimage;
}

void ImageBuffer::writeRawData(const char * filename, const float * source, int w, int h)
{
	int size = w*h;
	FILE * fp = fopen(filename, "wb");
	size_t fout;
	fout = fwrite(&h, sizeof(h), 1, fp);
	fout = fwrite(&w, sizeof(w), 1, fp);
	fout = fwrite(source, sizeof(float), size, fp);
	fclose(fp);
}

