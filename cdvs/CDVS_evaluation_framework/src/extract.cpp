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

#include <cassert>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include "CdvsInterface.h"
#include "FileManager.h"
#include "HiResTimer.h"		// high-resolution timer
#include "CdvsException.h"
#include <jpeglib.h>

using namespace mpeg7cdvs;
using namespace std;

// simple version of extract - uses defaults for all options.

#if defined(_WIN32) || defined(_WIN64)
  extern "C" void vl_constructor_lib();
  extern "C" void vl_destructor_lib();
#else
  static void vl_constructor_lib() {};
  static void vl_destructor_lib() {};
#endif


/**
 * @class JpegReader
 * Helper class for the JPEG library.
 */
class JpegReader
{
private:

	// Define the level of warning messages that will be printed to cerr (0=all, 1=some, 2=few, ... 5=none)
	static const int JPEG_WARNING_LEVEL = 5;

	// Redefine some of the standard JPEG error handlers. We don't
	// want any output or the application to stop.

	// Called when jpeg encounters an error.
	// This prevents the application from stopping on an error
	static void local_error_exit (jpeg_common_struct* cinfo)
	{
		cinfo->err->output_message (cinfo);
		throw CdvsException ("JPEG decoding failed");
	}

	// Conditionally emit a trace or warning message
	static void local_emit_message (jpeg_common_struct* cinfo, int msg_level)
	{
		if (msg_level >= JPEG_WARNING_LEVEL)
		{
			cinfo->err->output_message(cinfo);
		}
	}

	// Routine that actually outputs a trace or error message
	static void local_output_message (jpeg_common_struct* cinfo)
	{
		char jmessage[JMSG_LENGTH_MAX];
		cinfo->err->format_message(cinfo, jmessage);
		fprintf(stderr, "[JPEG] %s\n", jmessage);
	}

	public:

	//
	// -------  read a JPEG image ignoring the color components  ------------
	//
	static unsigned char * readJpeg(const char *fname, int & width, int & height)
	{
		FILE *file;
		if ((file = fopen (fname, "rb")) == NULL)
			throw CdvsException(string("ImageBuffer.read: cannot open image ").append(fname));

		struct jpeg_decompress_struct cinfo;		// JPEG decoder object
		struct jpeg_error_mgr jerr;				// error handling

		// initialize JPEG decompression object
		cinfo.err = jpeg_std_error(&jerr);      // use normal JPEG error routines

		jerr.error_exit     = local_error_exit;	// replace error handlers
		jerr.emit_message   = local_emit_message;
		jerr.output_message = local_output_message;

		jpeg_create_decompress(&cinfo);		// create the cinfo structure
		jpeg_stdio_src(&cinfo, file);		// specify data source
		jpeg_read_header(&cinfo, TRUE);		// read file parameters

		// copy image parameters

		height = cinfo.image_height;
		width = cinfo.image_width;

		// check colorspace

		if (cinfo.jpeg_color_space != JCS_CMYK)
		{
		  cinfo.out_color_space = JCS_GRAYSCALE;	// convert to grayscale

		  // start decompressor
		  jpeg_start_decompress(&cinfo);

		  // check if provided buffer is large enough
		  size_t row_stride = cinfo.output_width * cinfo.output_components;
		  size_t image_size = row_stride * cinfo.output_height;

		  unsigned char * buf = new unsigned char [image_size];	// set the correct size

		  // main loop
		  while (cinfo.output_scanline < cinfo.output_height) {
			JSAMPROW row_pointer[1];
			row_pointer[0] = buf + cinfo.output_scanline * row_stride;
			jpeg_read_scanlines (&cinfo, row_pointer, 1);
		  }

		  jpeg_finish_decompress(&cinfo);	// finish decompression
		  jpeg_destroy_decompress(&cinfo);	// free memory
		  fclose(file);
		  return buf;
		}
		else
		{
		  jpeg_destroy_decompress(&cinfo);	// free memory
		  fclose (file);					// close file
		  throw CdvsException(string("CMYK color space not supported"));
		}
	}

};		// end class JpegReader

/**
 * check if a file exists and is not empty
 */
bool fexists(const char *filename)
{
  ifstream ifile(filename, ios::in | ios::binary);
  return (ifile.peek() != std::ifstream::traits_type::eof());
}


void usage()
{
    cout <<
      "CDVS descriptor extraction module.\n"
      "Usage:\n"
	  "  extract <images> <mode> <dataset path> <annotation path>  [-p parameters] [-help]\n"
      "where:\n"
      "  images - image files to process (text file, one file name per line)\n"
	  "  mode (0..6) - sets the encoding mode to use\n"
      "  dataset path - the root dir of the CDVS dataset of images\n"
      "  annotation path - the root dir of the CDVS annotation files\n"
      "options:\n"
      "  -p parameters: text file containing initialization parameters for all modes\n"
	  "  -help or -h: help\n";
    exit (EXIT_FAILURE);
}

/**
 * @file 
 * extract.exe: CDVS descriptor extraction module.
 * Extracts CDVS descriptors from all given images, 
 * and stores them as files having the same image pathname but different extension.
 * @verbatim

    Usage:
	    extract <images> <mode> <dataset path> <annotation path>  [-p parameters] [-help]
    where:
        images - image files to process (text file, one file name per line)
        mode (0..n) - sets the encoding mode to use
        dataset path - the root dir of the CDVS dataset of images
        annotation path - the root dir of the CDVS annotation files
    options:
    	-p parameters: text file containing initialization parameters for all modes
	    -help or -h: help

 @endverbatim
 */
int run_extract(int argc, char *argv[])
{
	// when run with no arguments, argc value is 1
	// at least 4 parameters must be given
	if (argc <= 4) {
		  usage();
	}

	// read the first 4 parameters

	const char * imagelist = argv[1];
	int mode = atoi(argv[2]);
	const char * datasetPath = argv[3];
	const char * annotationPath = argv[4];
	const char * paramfile = NULL;

	if (mode > 6)
		usage();

	// set default values

	bool read_from_cache = false;
	bool write_to_cache = false;

	argv += 4;	// skip the first 4 params
	argc -= 4;	// skip the first 4 params

	// parse other params
	while ((argc > 1) && (argv[1][0] == '-'))
	{
		int n = 1;
		switch (argv[1][1])
		{
			case 'h': usage(); break;
			case 'p': paramfile = argv[2]; n = 2; break;
			default : cerr << "wrong argument: " << argv[1] << endl; usage(); break;
		}
		argv += n;
		argc -= n;
	}

	FileManager manager;
	manager.setDatasetPath(datasetPath);					// Set paths
	manager.setAnnotationPath(annotationPath);				// Set paths
	size_t n_images = manager.readAnnotation(imagelist);	// read file list


	// declare and clear time/length accumulators:

	double max_duration = 0.0;
	double total_duration = 0.0;
	size_t max_descriptor_length = 0;
	size_t total_descriptor_length = 0;

	int processed_images = 0;			// Added by TI - take into account that skipped images do not count

	CdvsConfiguration * cdvsconfig = CdvsConfiguration::cdvsConfigurationFactory(paramfile);	// if paramfile is NULL use default values
	CdvsClient * cdvsclient = CdvsClient::cdvsClientFactory(cdvsconfig, mode);		// get a CDVS client instance

	// MAIN LOOP: scan all files in the list: //

	#pragma omp parallel default(shared)
	{
		double local_max_duration = 0.0;
		size_t local_max_dsc_len = 0;
		ostringstream osserr;

		#pragma omp for reduction(+:total_duration,total_descriptor_length,processed_images)
		for (int i=0; i<n_images; i++)
		{
			HiResTimer timer;
			string image = manager.getAbsolutePathname(i);
			string cdvs_descriptor = manager.replaceExt(image, cdvsconfig->getParameters(mode).modeExt);

			try			// catch all errors locally
			{
				int width, height;
				CdvsDescriptor output;
				unsigned char * input = JpegReader::readJpeg(image.c_str(), width, height);	// read JPEG image

				timer.start();							// start timer
				size_t descriptor_length = cdvsclient->encode(output, width, height, input);
				output.buffer.write(cdvs_descriptor.c_str());
				timer.stop();							// stop timer

				delete [] input;				// free memory

				double duration = timer.elapsed();		// compute duration
				if (duration > local_max_duration)
					local_max_duration = duration;
				total_duration += duration;

				processed_images++;						// this image has been processed

				// update length stats:
				if (descriptor_length > local_max_dsc_len) local_max_dsc_len = descriptor_length;
				total_descriptor_length += descriptor_length;

				#pragma omp critical
				{
					cout << "[" << i + 1 << "/" << n_images << "]: " << image
						 << " -> " << descriptor_length << " [bytes], " << duration << " [s], " << output.getNumberOfLocalDescriptors() << " [features]"<< endl;
				}
			}
			catch (exception & ex)
			{
					osserr << "[" << i + 1 << "/" << n_images << "]: " << image
					 << " -> Error decoding JPEG image: " << ex.what()  << std::endl;
			}
		}		// end parallel for

		#pragma omp critical
		{
			cerr << osserr.str();		// output all errors at the end of processing
			max_duration = std::max(max_duration, local_max_duration);
			max_descriptor_length = std::max(max_descriptor_length, local_max_dsc_len);
		}
	}		// end parallel (only if compiled with -fopenmp)

	delete cdvsconfig;		// destroy the CDVS configuration instance
	delete cdvsclient;		// destroy the CDVS client instance

	double average_duration = 0.0;
	double average_descriptor_length = 0.0;

	if (processed_images > 0)	// avoid a divide by zero exception
	{
	  average_duration = (double) total_duration / (double) processed_images;
	  average_descriptor_length = (double) total_descriptor_length / (double) processed_images;
	}

	// output length/timing results:

	cout << endl << processed_images << " images processed:" << endl << endl;
	cout << "Average descriptor length: " << average_descriptor_length << " [bytes]" << endl;
	cout << "Maximal descriptor length: " << max_descriptor_length << " [bytes]" << endl << endl;
	cout << "Average extraction time: " << average_duration << " [s]" << endl;
	cout << "Maximal extraction time: " << max_duration << " [s]" << endl << endl;
	return 0;
}

/**
 * initialize any library that needs a global init.
 */
void open_libs()
{
	vl_constructor_lib();			// initialize the vlfeat library
}

/**
 * free memory and close any library that needs a global init.
 */
void close_libs()
{
	vl_destructor_lib();			// free memory and exit
}
//  ----- main ------

int main(int argc, char *argv[])
{
	open_libs();

	try {
		run_extract(argc, argv);		// run "extract" catching any exception
	}
	catch(exception & ex)				// catch any exception, including CdvsException
	{
	    cerr << argv[0] << " exception: " << ex.what() << endl;
	}

	close_libs();
	return 0;
}

/* extract.cpp - end of file */
