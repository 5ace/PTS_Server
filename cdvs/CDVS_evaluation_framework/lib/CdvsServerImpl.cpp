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
 * CdvsServer.cpp
 *
 *  Created on: Jun 10, 2014
 *      Author: massimo
 */

#include "CdvsServerImpl.h"
#include "CdvsException.h"
#include "DistratEigen.h"
#include "Projective2D.h"
#include "Buffer.h"

using namespace std;
using namespace Eigen;
using namespace mpeg7cdvs;

CdvsServerImpl::CdvsServerImpl(const CdvsConfiguration * config, bool twoWayMatch):useTwoWayMatch(twoWayMatch)
{
	for (int k = 0; k < Parameters::nModes; ++k)
	{
		parset[k] = config->getParameters(k);
	}
}

CdvsServerImpl::~CdvsServerImpl()
{}

void CdvsServerImpl::createDB(int mode, int reserve)
{
	if ((mode < 0) || (mode >= Parameters::nModes))
		throw CdvsException("Required mode does not exists");

	db.modeId = mode;
	scfvIdx.reserve(reserve);		// reserve memory space for n descriptors
}

unsigned int CdvsServerImpl::addDescriptorToDB(const CdvsDescriptor & refDescriptor, const char * referenceImageId)
{
	 scfvIdx.append(refDescriptor.scfvSignature);						// Add to global database index
	 return (unsigned int) db.addImage(refDescriptor.featurelist, referenceImageId);	// Add to local database index
}

bool CdvsServerImpl::isDescriptorInDB(const char * referenceImageId) const
{
	return (db.find(referenceImageId) != NOT_FOUND);
}

bool CdvsServerImpl::replaceDescriptorInDB(const CdvsDescriptor & refDescriptor, const char * referenceImageId, const char * oldImageId)
{
	size_t index;
	if (oldImageId == NULL)
		index = db.find(referenceImageId);
	else
		index = db.find(oldImageId);

	if (index != NOT_FOUND)
	{
		scfvIdx.replace(index, refDescriptor.scfvSignature);						// replace global
		db.replaceImage(index, refDescriptor.featurelist, referenceImageId);		// replace local
	}
	return (index != NOT_FOUND);
}

void CdvsServerImpl::clearDB()
{
	db.clear();
	scfvIdx.clear();
}

void CdvsServerImpl::storeDB(const char * localname, const char * globalname) const
{
	db.writeToFile(localname);		// write local DB
	scfvIdx.write(globalname);		// write global DB

	if (db.size() != scfvIdx.numberImages())		// check the number of images
		throw CdvsException("Global and local DB contain a different number of images");
}

void CdvsServerImpl::loadDB(const char * localname, const char * globalname)
{
	scfvIdx.read(globalname);		// read global DB
	db.readFromFile(localname);		// read local DB

	if (db.size() != scfvIdx.numberImages())		// check the number of images
		throw CdvsException("Global and local DB contain a different number of images");
	scfvIdx.loadHammingWeight();		// initialize global DB for retrieval
}

size_t CdvsServerImpl::sizeofDB() const
{
	return db.size();
}


size_t CdvsServerImpl::decode(CdvsDescriptor & output, const char * fname) const
{
	output.buffer.read(fname);
	return output.decode(parset);		// decode query descriptor
}

size_t CdvsServerImpl::decode(CdvsDescriptor & output, const unsigned char * bitstream, int size) const
{
	if (bitstream != NULL)
		output.buffer.assign(bitstream, size);

	return output.decode(parset);		// decode query descriptor
}


PointPairs CdvsServerImpl::matchCompressed(const CompressedFeatureList & compressedQuery, const CompressedFeatureList & compressedReference, const CDVSPOINT *r_bbox, CDVSPOINT *proj_bbox, int matchType,
		unsigned int queryMode, unsigned int refMode,
		const SCFVSignature & querySignature, const SCFVSignature & refSignature) const
{
	const Parameters & query_params = parset[queryMode];
	const Parameters & ref_params = parset[refMode];

	// Determine the minimum weight threshold mixing the values of query and reference
	double wmThreshold = useTwoWayMatch? query_params.wmThreshold2Way : query_params.wmThreshold;
	double globalThreshold = query_params.gdThreshold;
	bool isMixed = (query_params.descLength != ref_params.descLength);		// mixed case?
	if (isMixed){
		wmThreshold = useTwoWayMatch? query_params.wmMixed2Way : query_params.wmMixed;
		globalThreshold = query_params.gdThresholdMixed;
	}

	// Determine the ratio threshold by taking the minimum of query and reference
	float ratioThreshold = min(query_params.ratioThreshold, ref_params.ratioThreshold);

	// determine the chi-square percentile
	unsigned int chiSquarePercentile = max(query_params.chiSquarePercentile,ref_params.chiSquarePercentile);

	PointPairs pairs(compressedQuery.nFeatures() + compressedReference.nFeatures());

	pairs.local_threshold = wmThreshold;		// set current local thresholds
	pairs.global_threshold = globalThreshold;	// set current global thresholds

	if(matchType != MATCH_TYPE_GLOBAL)		// if not global-only...
	{
		if (useTwoWayMatch)
			pairs.nMatched = compressedQuery.matchDescriptors_twoWay(pairs, compressedReference, ratioThreshold);		// Compare descriptors
		else
			pairs.nMatched = compressedQuery.matchDescriptors_oneWay(pairs, compressedReference, ratioThreshold);		// Compare descriptors

		if ((pairs.nMatched >= 5) && (pairs.getTotalWeight() >= wmThreshold))			// 5 is the minimum number of points needed by DISTRAT
		{
			DistratEigen distrat(pairs.x1, pairs.x2, pairs.y1, pairs.y2, pairs.nMatched);
			pairs.nInliers = distrat.estimateInliers(false, true, chiSquarePercentile, pairs.inlierIndexes);
			pairs.local_score = pairs.getInlierWeight();
			pairs.score = pairs.local_score / (pairs.local_score + wmThreshold);		// return a value between 0 and 1
		}
	}

	if(matchType != MATCH_TYPE_LOCAL) 		// if not local-only...
	{
		if (pairs.score < 0.5 || matchType == MATCH_TYPE_BOTH) 		// matchType 1 forces the global comparison
		{
			// Match global descriptors
			unsigned int n_q_words = 0, n_r_words = 0, n_words_overlap = 0;

			// now call the predictor
			if(query_params.hasBitSelection)
			{
				pairs.global_score = scfvIdx.matchImages_bitselection(querySignature, refSignature, &n_q_words, &n_r_words, &n_words_overlap);
			}
			else
			{
				pairs.global_score = scfvIdx.matchImages(querySignature, refSignature, &n_q_words, &n_r_words, &n_words_overlap);
			}

			if (pairs.global_score > globalThreshold)
				pairs.score = 1.0;	// force match

		}
	}

	// rescale coordinates to the original image actual size

	int query_max_size = std::max(compressedQuery.imageHeight, compressedQuery.imageWidth);
	int reference_max_size = std::max(compressedReference.imageHeight, compressedReference.imageWidth);
	int query_orig_size = std::max(compressedQuery.originalHeight, compressedQuery.originalWidth);
	int reference_orig_size = std::max(compressedReference.originalHeight, compressedReference.originalWidth);

	pairs.toFullResolution(query_max_size, query_orig_size, reference_max_size, reference_orig_size);

	// return coordinates of projected bounding box - implemented by Telecom Italia

	if (proj_bbox != NULL)
	{
		CDVSPOINT default_bbox[4];	// default bounding box composed by the entire reference image

		if (r_bbox == NULL)
		{
			default_bbox[0].x = 0;
			default_bbox[0].y = 0;
			default_bbox[1].x = compressedReference.originalWidth - 1;
			default_bbox[1].y = 0;
			default_bbox[2].x = compressedReference.originalWidth - 1;
			default_bbox[2].y = compressedReference.originalHeight - 1;
			default_bbox[3].x = 0;
			default_bbox[3].y = compressedReference.originalHeight - 1;

			r_bbox = default_bbox;		// set r_bbox to default values
		}

		if (pairs.hasLocalizationInliers())
		{
			Projective2D pp;
			float zoom;
			if (query_orig_size > query_max_size)
				zoom = (float)query_orig_size/(float)query_max_size;
			else
				zoom = (float)query_orig_size/(float) query_params.resizeMaxSize;

			if (useTwoWayMatch)
			{
				// Localization is performed on the intersection inliers if #intersection inliers >= 8, else on the union inliers
				int num_inliers_bidir = 0;
				bool use_intersection_only = true;
				for (int k=0; k<pairs.nInliers; k++)
				{
					int kinlier = pairs.inlierIndexes[k];
					if(pairs.match_dirs[kinlier] == match_2way_INTERSECTION)
						num_inliers_bidir++;
				}
				if(num_inliers_bidir < LOC_INTERSECTION_THRESHOLD)
				{
					// use union inliers
					num_inliers_bidir = pairs.nInliers;
					use_intersection_only = false;
				}
				Point2DArray refPoints(num_inliers_bidir,2);
				Point2DArray queryPoints(num_inliers_bidir,2);
				int l = 0;
				for (int k=0; k<pairs.nInliers; k++)
				{
					int kinlier = pairs.inlierIndexes[k];
					if(use_intersection_only && pairs.match_dirs[kinlier] != match_2way_INTERSECTION) continue;
					queryPoints(l,0)	= pairs.x1[kinlier];
					queryPoints(l,1)	= pairs.x2[kinlier];
					refPoints(l,0)		= pairs.y1[kinlier];
					refPoints(l,1)		= pairs.y2[kinlier];
					l++;
				}
				pp.ransac(refPoints,  queryPoints, query_params.ransacNumTests, zoom * query_params.ransacThreshold);
			}
			else		// one way match
			{
				Point2DArray refPoints(pairs.nInliers,2);
				Point2DArray queryPoints(pairs.nInliers,2);

				for (int k=0; k<pairs.nInliers; k++)
				{
					int kinlier = pairs.inlierIndexes[k];
					queryPoints(k,0)	= pairs.x1[kinlier];
					queryPoints(k,1)	= pairs.x2[kinlier];
					refPoints(k,0)		= pairs.y1[kinlier];
					refPoints(k,1)		= pairs.y2[kinlier];
				}
				pp.ransac(refPoints,  queryPoints, query_params.ransacNumTests, zoom * query_params.ransacThreshold);
			}
			// end one way or two way match

			Point2DArray Mproj_bbox(4,2);
			// get the coordinates of the bounding box:
			Point2DArray Mr_bbox(4,2);
			for (int k=0; k<4; k++)
			{
				Mr_bbox(k,0) = r_bbox[k].x;
				Mr_bbox(k,1) = r_bbox[k].y;
			}

			// use the homography to get the new bounding box
			pp.moveByHomography(Mproj_bbox, Mr_bbox);

			// put the coordinates of Mproj_bbox into proj_bbox
			for (int k=0; k<4; k++)
			{
				proj_bbox[k].x = Mproj_bbox(k,0);
				proj_bbox[k].y = Mproj_bbox(k,1);
			}
		}	// end if (pairs.hasLocalizationInliers())
		else
		{
			// return the entire query image coordinates
			proj_bbox[0].x = 0;
			proj_bbox[0].y = 0;
			proj_bbox[1].x = compressedQuery.originalWidth - 1;
			proj_bbox[1].y = 0;
			proj_bbox[2].x = compressedQuery.originalWidth - 1;
			proj_bbox[2].y = compressedQuery.originalHeight - 1;
			proj_bbox[3].x = 0;
			proj_bbox[3].y = compressedQuery.originalHeight - 1;

		}
	}

	return pairs;	// return the result
}

/*
 * Pair-wise descriptor matching & localization function.
 */
PointPairs CdvsServerImpl::match(const CdvsDescriptor & queryDescriptor, const CdvsDescriptor & refDescriptor, const CDVSPOINT *r_bbox, CDVSPOINT *proj_bbox, int matchType) const
{
	// check descriptor size

	if ((queryDescriptor.getNumberOfLocalDescriptors() == 0) || (refDescriptor.getNumberOfLocalDescriptors() == 0))
	{
		PointPairs emptypairs;
		return emptypairs;			// no match possible with an empty descriptor
	}

	CompressedFeatureList compressedQuery(queryDescriptor.featurelist);
	CompressedFeatureList compressedReference(refDescriptor.featurelist);

	return matchCompressed(compressedQuery, compressedReference, r_bbox, proj_bbox, matchType,
			queryDescriptor.getModeID(), refDescriptor.getModeID(),
			queryDescriptor.scfvSignature, refDescriptor.scfvSignature);
}
/*
 * DB descriptor matching & localization function.
 * This method matches a query descriptor with a reference stored in the DB; it is supposed to be called after a retrieval operation, even though this is not essential.
 */
PointPairs CdvsServerImpl::match(const CdvsDescriptor & queryDescriptor, unsigned int index, const CDVSPOINT *r_bbox, CDVSPOINT *proj_bbox, int matchType) const
{
	// check descriptor size

	if ((queryDescriptor.getNumberOfLocalDescriptors() == 0) || (index >= db.images.size()))
	{
		PointPairs emptypairs;
		return emptypairs;			// no match possible with an empty descriptor or an index out of range
	}

	CompressedFeatureList compressedQuery(queryDescriptor.featurelist);

	return matchCompressed(compressedQuery, db.images[index], r_bbox, proj_bbox, matchType,
		queryDescriptor.getModeID(), db.modeId,
		queryDescriptor.scfvSignature, scfvIdx.getImage(index));
}

bool descending_float_score(const RetrievalData & i, const RetrievalData & j) {
  return (i.fScore > j.fScore);
}


bool cmpDoubleUintAscend(const std::pair<double,unsigned int> & pair1, const std::pair<double,unsigned int> & pair2) {
  return pair1.first < pair2.first;
}


int CdvsServerImpl::retrieve(vector<RetrievalData> & results, const CdvsDescriptor & cdvsDescriptor, unsigned int max_matches) const
{
	if (cdvsDescriptor.getNumberOfLocalDescriptors() == 0)			// this special case happens when no features are extracted from the image by vlfeat
		return 0;

	const Parameters & query_params = parset[cdvsDescriptor.getModeID()];
	const Parameters & param_db = parset[db.getMode()];

	// Compute scores with global signature
	vector< pair<double,unsigned int> > imageScoresNumbersTop;

	if(query_params.hasBitSelection)
	{
		scfvIdx.query_bitselection(cdvsDescriptor.scfvSignature, imageScoresNumbersTop, query_params.retrievalLoops);
	}
	else
	{
		scfvIdx.query(cdvsDescriptor.scfvSignature, imageScoresNumbersTop, query_params.retrievalLoops);
	}


	// Rerank with image neighbors (if required by parameter settings)
	if ((db.recallGraph.size() > 0) && (param_db.queryExpansionLoops > 0))
	{
		int nTop1Limit = std::min((size_t) 35, imageScoresNumbersTop.size());		// avoid out-of-range access
		int nTop2Limit = std::min((size_t) 2000, imageScoresNumbersTop.size());	// avoid out-of-range access
		for (int nTop1 = 0; nTop1 < nTop1Limit; nTop1++)
		{
			unsigned int nDatabaseImage = imageScoresNumbersTop[nTop1].second;
			bool bReranked = false;
			for (int nTop2 = nTop1+1; nTop2 < nTop2Limit; nTop2++)
			{
				unsigned int nDatabaseImageOther = imageScoresNumbersTop[nTop2].second;
				for (recallGraphNode_t::const_iterator node = db.recallGraph[nDatabaseImage].begin();
						node < db.recallGraph[nDatabaseImage].end(); node++)
				{
					unsigned int val = *node;
					if (val == nDatabaseImageOther)
					{
						bReranked = true;
						imageScoresNumbersTop[nTop2].first = imageScoresNumbersTop[nTop1].first + 0.001;
					}
				} // node
			} // nTop2
			if (bReranked) break;
		} // nTop1Limit

		sort(imageScoresNumbersTop.begin(), imageScoresNumbersTop.end(), cmpDoubleUintAscend);
	}


	// Computation of the number of loops to use in the reranking stage
	unsigned int nLoops = query_params.retrievalLoops;

	// Number of loops should not be greater than the number of images in the DB
	if(nLoops > imageScoresNumbersTop.size())
		nLoops = imageScoresNumbersTop.size();

	// Reduction of the number of keypoints contained in the query (only if the relevance bit is present)
	// use a special copy constructor of CompressedFeatureList to sort by relevance the keypoints.

	CompressedFeatureList query_db(cdvsDescriptor.featurelist, cdvsDescriptor.getRelevanceBitsPresent());

	// Reranking by means of euclidean matching and geometric verification of the top matches

	results.reserve(nLoops);

	// Vectors that contain the coordinates of the features matched by the method matchCompressedDescriptors

	PointPairs pairs(query_params.selectMaxPoints + param_db.selectMaxPoints);
	pairs.local_threshold = useTwoWayMatch? query_params.wmRetrieval2Way: query_params.wmRetrieval;		// set current local thresholds

	for(unsigned int i=0; i<nLoops; ++i)		// first loop - get only images passing the DISTRAT check
	{
		RetrievalData vip;		// very important pictures
		vip.index = imageScoresNumbersTop[i].second;
		vip.gScore = imageScoresNumbersTop[i].first;

		vip.nMatched = useTwoWayMatch?db.matchCompressedDescriptors_twoWay(pairs, query_db, vip.index, param_db.ratioThreshold):
				db.matchCompressedDescriptors_oneWay(pairs, query_db, vip.index, param_db.ratioThreshold);

		// Geometric consistency check using DISTRAT
		double weight = 0.0;
		vip.nInliers = 0;
		vip.fScore = 0;
		if (vip.nMatched >= 5)			// 5 is the minimum number of points needed by DISTRAT
		{
			DistratEigen distrat(pairs.x1, pairs.x2, pairs.y1, pairs.y2, vip.nMatched);
			vip.nInliers = pairs.nInliers = distrat.estimateInliers(false, true, query_params.chiSquarePercentile, pairs.inlierIndexes);
			weight = pairs.getInlierWeight();

			if (weight >= pairs.local_threshold)
				vip.fScore = (float) weight;
		}

		results.push_back(vip);
	}

	stable_sort(results.begin(), results.end(), descending_float_score);		// Sorting of the results

	// Keep a number of images <= max_matches
	if(results.size() > max_matches)
		results.resize(max_matches);

	/* return number of matches found: */
	return results.size();
}


std::string CdvsServerImpl::getImageId(unsigned int index) const
{
	return db.getImageName(index);
}

void CdvsServerImpl::commitDB()
{
	scfvIdx.loadHammingWeight();
}
