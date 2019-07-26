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

/*
 * CdvsServerImpl.h
 *
 *  Created on: Jun 10, 2014
 *      Author: massimo
 */
#pragma once

#include "CdvsInterface.h"
#include "Database.h"
#include <cstring>
#include <vector>
#include <utility>

namespace mpeg7cdvs
{

/**
 * @class CdvsServerImpl
 * Implementation of the high level interface to the server-side functionality of the CDVS Library.
 * @author  Massimo Balestri
 * @date 2014
 */
class CdvsServerImpl : public CdvsServer {
private:
	ParameterSet parset;
	Database db;
	SCFVIndex scfvIdx;
	bool useTwoWayMatch;

	static bool descending_float_score(const RetrievalData & i, const RetrievalData & j) {
	  return (i.fScore > j.fScore);
	}

	static bool cmpDoubleUintAscend(const std::pair<double,unsigned int> & pair1, const std::pair<double,unsigned int> & pair2) {
	  return pair1.first < pair2.first;
	}

	static const int LOC_INTERSECTION_THRESHOLD = 8;					// The threshold used in localization


	PointPairs matchCompressed(const CompressedFeatureList & queryCFL, const CompressedFeatureList & refCFL, const CDVSPOINT *r_bbox, CDVSPOINT *proj_bbox, int matchType,
			unsigned int queryMode, unsigned int refMode, const SCFVSignature & querySignature, const SCFVSignature & refSignature) const;


public:
	CdvsServerImpl(const CdvsConfiguration * config, bool twoWayMatch = true);

	virtual ~CdvsServerImpl();


	virtual size_t decode(CdvsDescriptor & output, const char * fname) const;

	virtual size_t decode(CdvsDescriptor & output, const unsigned char * bitstream, int size) const;

	virtual PointPairs match(const CdvsDescriptor & queryDescriptor, const CdvsDescriptor & refDescriptor, const CDVSPOINT *r_bbox, CDVSPOINT *proj_bbox, int matchType) const;

	virtual PointPairs match(const CdvsDescriptor & queryDescriptor, unsigned int index, const CDVSPOINT *r_bbox, CDVSPOINT *proj_bbox, int matchType) const;

	virtual void createDB(int mode, int reserve);

	virtual unsigned int addDescriptorToDB(const CdvsDescriptor & refDescriptor, const char * referenceImageId);

	virtual bool isDescriptorInDB(const char * referenceImageId) const;

	virtual bool replaceDescriptorInDB(const CdvsDescriptor & refDescriptor, const char * referenceImageId, const char * oldImageId);

	virtual void clearDB();

	virtual void storeDB(const char * localname, const char * globalname) const;

	virtual void loadDB(const char * localname, const char * globalname);

	virtual size_t sizeofDB() const;

	virtual int retrieve(std::vector<RetrievalData> & results, const CdvsDescriptor & cdvsDescriptor, unsigned int max_matches) const;

	virtual std::string getImageId(unsigned int index) const;

	virtual void commitDB();
};

}  // end namespace
