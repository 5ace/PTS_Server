/*
 * This software module was originally developed by:
 *
 *   Joint Open Lab VISIBLE (Telecom Italia)
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


#pragma once
#include "Parameters.h"
#include "CdvsDescriptor.h"
#include "CdvsPoint.h"
#include "Buffer.h"

/**
 * @namespace mpeg7cdvs
 * Namespace used to encapsulate all MPEG-7 CDVS declarations that are visible when
 * the CDVS Library headers (in particular CdvsInterface.h) are included.
 */
namespace mpeg7cdvs
{

	/**
	 * @class CdvsConfiguration
	 * Interface to all configuration parameters for clients and servers.
	 * @author  Massimo Balestri
	 * @date 2014
	 */
	class CdvsConfiguration {
	public:
		virtual ~CdvsConfiguration() {};

		/**
		 * Create an instance of a CDVS configuration containing all default coding/decoding parameters.
		 * The configuration instance can be used to initialize a client or a server CDVS instance.
		 * The configuration can be modified using the setParameters method.
		 * The calling entity takes ownership of the instance (i.e. must delete the instance when not used anymore).
		 * @param configfile a file containing some or all parameters replacing the default values.
		 * @return a CdvsConfiguration instance
		 */
		static CdvsConfiguration * cdvsConfigurationFactory(const char * configfile = NULL);

		/**
		 * Get one of the Parameters instances (note that this class keeps an instance of all parameters for all modes).
		 * @param mode the mode for which parameters are requested.
		 * @return a read-only instance of the parameters.
		 */
		virtual const Parameters & getParameters(int mode) const = 0;

		/**
		 * Set some Parameters value for a specific mode.
		 * @param mode the mode for which parameters are requested.
		 * @return a modifiable instance of the parameters.
		 */
		virtual Parameters & setParameters(int mode) = 0;

		/**
		 * Get the mode ID corresponding to a specific descriptor length.
		 * The relation between length and mode ID is provided according to the MPEG CDVS specification:
		 *  - mode 1:   512 bytes
		 *  - mode 2:  1024 bytes
		 *  - mode 3:  2048 bytes
		 *  - mode 4:  4096 bytes
		 *  - mode 5:  8192 bytes
		 *  - mode 6: 16384 bytes
		 *  @param descLen the descriptor length (in bytes)
		 *  @return the corresponding mode
		 */
		static int getMode(int descLen);
	};

	/**
	 * @class CdvsClient
	 * Interface to the client-side functionality of the CDVS Library.
	 * @author  Massimo Balestri
	 * @date 2014
	 */
	class CdvsClient {
	public:
		virtual ~CdvsClient() {};

		/**
		 * Create an instance of a CDVS Client producing descriptors according to the indicated mode.
		 * The calling entity takes ownership of the instance (i.e. must delete the instance when not used anymore).
		 * @param config the parameter configuration that will be used to produce descriptors.
		 * @param mode mode of the descriptors produced by the client instance.
		 * @return a pointer to the Cdvs Client instance
		 */
		static CdvsClient * cdvsClientFactory(const CdvsConfiguration * config, int mode);

		/**
		 * Encode the luminance component of an image producing a CDVS descriptor.
		 * @param output the output CDVS descriptor
		 * @param width width of the image
		 * @param height height of the image
		 * @param input the buffer containing the luminance component of the image (Y component, 8 bit per pixel)
		 * @return the actual size of the encoded CDVS descriptor
		 */
		virtual unsigned int encode(CdvsDescriptor & output, int width, int height, const unsigned char * input)  const = 0;

	};


	/**
	 * @class CdvsServer
	 * Interface to the server-side functionality of the CDVS Library.
	 * @author  Massimo Balestri
	 * @date 2014
	 */
	class CdvsServer {
	public:

		virtual ~CdvsServer() {};

		/**
		 * Create an instance of a CDVS Server for matching and retrieval of CDVS descriptors.
		 * The calling entity takes ownership of the instance (i.e. must delete the instance when not used anymore).
		 * @param config the configuration that will be used to produce descriptors.
		 * @param twoWayMatch select one-way or two-way matching; default is two-way.
		 * @return a pointer to the Cdvs Server instance
		 */
		static CdvsServer * cdvsServerFactory(const CdvsConfiguration * config, bool twoWayMatch = true);

		/**
		 * Decode a compressed query descriptor stored in a file.
		 * @param fname the input file name
		 * @param output the decoded CdvsDescriptor
		 * @return the size of the consumed descriptor (bytes).
		 */
		virtual size_t decode(CdvsDescriptor & output, const char * fname) const = 0;

		/**
		 * Decode a compressed reference descriptor stored either in the bitstream parameter or in the CdvsDescriptor input/output Buffer.
		 * @param output the decoded CdvsDescriptor
		 * @param bitstream a buffer containing an encoded CdvsDescriptor bitstream (optional parameter; if missing, the "buffer" member variable of CdvsDescriptor will be used instead)
		 * @param size size in bytes of the bitstream buffer (must be specified only if bitstream is not null)
		 * @return the size of the consumed descriptor (bytes).
		 */
		virtual size_t decode(CdvsDescriptor & output, const unsigned char * bitstream = NULL, int size = 0) const = 0;

		/**
		 * Pair-wise descriptor matching & localization function.
		 * @param queryDescriptor the query descriptor
		 * @param refDescriptor the reference descriptor
		 * @param r_bbox bounding box of object of interest in the second (reference) image;  replaced by the full image coordinates if NULL.
		 * @param proj_bbox buffer to contain parameters of bounding box for a match projected in the coordinate system of the first (query) image; ignored if NULL.
		 * @param matchType type of matching; may be MATCH_TYPE_DEFAULT, MATCH_TYPE_BOTH, MATCH_TYPE_LOCAL, MATCH_TYPE_GLOBAL. Default is MATCH_TYPE_DEFAULT in this case.
		 * @return an instance of PointPairs which contains all matching points, plus local and global scores.
		 */
		virtual PointPairs match(const CdvsDescriptor & queryDescriptor, const CdvsDescriptor & refDescriptor, const CDVSPOINT *r_bbox=NULL, CDVSPOINT *proj_bbox=NULL, int matchType = MATCH_TYPE_DEFAULT) const = 0;

		/**
		 * Pair-wise descriptor matching & localization using a DB image as reference.
		 * This method can be called after a retrieval operation, to localize the retrieved object(s) in the query image.
		 * The default match type in this case is MATCH_TYPE_LOCAL.
		 * @param queryDescriptor the query descriptor
		 * @param index index of the reference descriptor in the DB
		 * @param r_bbox bounding box of object of interest in the DB image; replaced by the full image coordinates if NULL.
		 * @param proj_bbox buffer to contain parameters of bounding box for a match projected in the coordinate system of the query image; ignored if NULL.
		 * @param matchType type of matching; may be MATCH_TYPE_DEFAULT, MATCH_TYPE_BOTH, MATCH_TYPE_LOCAL, MATCH_TYPE_GLOBAL. Default is MATCH_TYPE_LOCAL in this case.
		 * @return an instance of PointPairs which contains all matching points, plus local and global scores.
		 */
		virtual PointPairs match(const CdvsDescriptor & queryDescriptor, unsigned int index, const CDVSPOINT *r_bbox=NULL, CDVSPOINT *proj_bbox=NULL, int matchType = MATCH_TYPE_LOCAL) const = 0;

		/**
		 * Create a Database of CDVS Descriptors for retrieval.
		 * @param mode the mode identifier of all descriptors that will be stored in the DB;
		 * @param reserve the estimate number of CDVS Descriptor that will constitute the DB; the code will reserve a corresponding space in the DB.
		 */
		virtual void createDB(int mode, int reserve) = 0;

		/**
		 * Add the given reference descriptor to the Data Base of reference images.
		 * @param refDescriptor the reference descriptor
		 * @param referenceImageId the string that identifies this image; may be a pathname or a numeric ID but must be expressed as text.
		 * @return the index of the reference image in the DB
		 */
		virtual unsigned int addDescriptorToDB(const CdvsDescriptor & refDescriptor, const char * referenceImageId) = 0;

		/**
		 * Verify if a given image is stored in the DB.
		 * @param referenceImageId the string that identifies this image; may be a pathname or a numeric ID but must be expressed as text.
		 * @return true if the image is present.
		 */
		virtual bool isDescriptorInDB(const char * referenceImageId) const = 0;

		/**
		 * Replace a given image in the DB with another one. If the image is not present no operation is performed.
		 * @param refDescriptor the reference descriptor of the new image
		 * @param referenceImageId the string that identifies the new image
		 * @param oldImageId the string that identifies the old image to be replaced; if NULL, referenceImageId will be also used as name of the image to replace
		 * @return true if the image was present (and its descriptor has been replaced).
		 */
		virtual bool replaceDescriptorInDB(const CdvsDescriptor & refDescriptor, const char * referenceImageId, const char * oldImageId = NULL) = 0;

		/**
		 * Clear the DB removing all images.
		 */
		virtual void clearDB() = 0;

		/**
		 * Commit all changes into the DB.
		 */
		virtual void commitDB() = 0;

		/**
		 * Store the Data Base permanently into a pair of files.
		 * @param localname the name of the local descriptors file;
		 * @param globalname the name of the global descriptors file;
		 */
		virtual void storeDB(const char * localname, const char * globalname) const = 0;


		/**
		 * Load the Data Base from a pair of files.
		 * @param localname the name of the local descriptors file;
		 * @param globalname the name of the global descriptors file;
		 */
		virtual void loadDB(const char * localname, const char * globalname) = 0;

		/**
		 * Get the number of descriptors currently stored in the retrieval Data Base.
		 * @return the number of descriptors in the DB.
		 */
		virtual size_t sizeofDB() const = 0;

		/**
		 * Retrieval function.
		 *	  Notes:
		 *	  - it is assumed that database index is already pre-loaded and available through globals.
		 *	  - query descriptor is also pre-loaded and passed via input parameters
		 *	  - the task of this function is to produce a list of matching images in the database using only query descriptor, index, and descriptors of images stored in the database (included in the index)
		 * @param results vector of information data about matching images (in order of relevance)
		 * @param queryDescriptor the query descriptor to be used as input query data of the retrieval operation
		 * @param max_matches - maximum number of matches to include in the list of results
		 * @return number of matches found
		 */
		virtual int retrieve(std::vector<RetrievalData> & results, const CdvsDescriptor & queryDescriptor, unsigned int max_matches) const = 0;

		/**
		 * Get the id corresponding to the given image index in the DB.
		 * @param index the index in the DB of the image
		 * @return a string containing the identifier of the image
		 */
		virtual std::string getImageId(unsigned int index) const = 0;

	};

}  // namespace mpeg7cdvs
