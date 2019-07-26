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
 * Copyright (c) ISO/IEC 2012.
 *
 */

#pragma once

#include <vector>
#include <algorithm>
#include "FeatureList.h"
#include "Parameters.h"
#include "BitOutputStream.h"
#include "BitInputStream.h"


// #define USE_WEIGHT_TABLE

namespace mpeg7cdvs
{
	// define global constants
	static const float gama =  0.3f;
	static const int num_bit_selection = 24;
	static const int PCASiftLength = 32;					///< number of principal components in the centroid space
	static const int numberCentroids = 512;					///< number of centroids of the codebook

	/**
	 * @class LookUpTable
	 * A simple look up table implementation, to perform a bit count very quickly.
	 */
	class LookUpTable
	{
	public:
		char f[(1<<16)];	///< the look up table

		LookUpTable();		// constructor
	};

	/**
	 * @class SCFVSignature
	 * Container class for a Scalable Fisher Vector binary signature; allows reading/writing from/to a bitstream,
	 * fetching/storing from/into a file, and comparing a signature with another.
	 */
	class SCFVSignature
	{
	private:
		bool bHasVar;
		bool bHasBitSelection;
		float fNorm;
		unsigned int m_numVisited;

		SCFVSignature();	// must be private to force using the constructor with parameters

		void write_bitselection(BitOutputStream & out) const;	///< write the binary signature into the given output stream
		void read_bitselection(BitInputStream & in);			///< read the binary signature from the given input stream

	public:
		static const unsigned int table_bit_selection[];		///< subset of bits used in low bitrate applications
		
		unsigned int m_vWordBlock[numberCentroids];				///< Scalable Fisher Vector binary signature
		unsigned int m_vWordVarBlock[numberCentroids];			///< Scalable Fisher Vector binary variance

		/**
		 * Constructor declaring if this signature contains variance information and bit selection.
		 * @param hasVar true if this signature contains variance information (used normally at high bitrates)
		 * @param hasBitSelection true if this signature performs bit selection (used normally at very low bitrates)
		 */
		SCFVSignature(bool hasVar, bool hasBitSelection);

		void clear();				///< clear all data
		size_t size() const;		///< get the size of the binary signature (uncompressed)
		int compressedNumBits() const;		///< get the number of bits of the encoded signature (compressed)

		void write(BitOutputStream & out) const;	///< write the binary signature into the given output stream
		void read(BitInputStream & in);				///< read the binary signature from the given input stream

		unsigned int getVisited() const;		///< get the number of visited words 
		void setVisited();						///< compute and store the correct number of visited words

		float getNorm() const;				///< get the norm of this signature
		void  setNorm();						///< compute and store the correct norm for this signature

		bool hasVar() const;					///< tell if this signature has variance information
		void hasVar(bool value);				///< set this signature as one containing variance information (if value is true)

		bool hasBitSelection() const;		///< tell if this signature performs bit selection
		void hasBitSelection(bool value);	///< set this signature as one performing bit selection (if value is true)

		int compare(const SCFVSignature & other) const;		///< compare two signatures (only for debugging)

		void toFile(FILE * file) const;			///< write the signature to file
		void fromFile(FILE * file);				///< read the signature from file

		void print() const;					///< print a summary of the signature data
	};

	/**
	 * @class SCFVIndex
	 * A class to manage an indexed list of SCFV signatures. 
	 * Includes methods to read/write/append SCFV signatures, to use a signature as a query, and to match two signatures. 
	 */
	class SCFVIndex
	{
	private:
		// define local constants
		static const int M = 2;
		static const int h_t = 3;
		static const float beta;
		static const int mbit_speedup = 3;

		static const LookUpTable lut;

		struct Node
		{
			int ID;
			float val;
		};

		static bool cmp(Node A , Node B);
		static unsigned int originalTocompress(unsigned int a , int nCentroid);
		static unsigned int compressToOriginal(unsigned int a , int nCentroid);
		static unsigned int inv_bits(unsigned int a);

	public:

		SCFVIndex();	// constructor

		//	default destructor, copy-constructor, assignment op are ok

		void append(const SCFVSignature & scfvSignature);		///< append the given SCFV signature to the current index

		void replace(size_t index, const SCFVSignature & scfvSignature);		///< replace the given SCFV signature with the given one at the given index

		void write(std::string sIndexName) const;		///< write the SCFV index to file 

		void read(std::string sIndexName);				///< read the SCFV index from file

		/**
		 * Use a binary SCFV signature as a query to retrieve a ranked list of signatures matching the given one.
		 * @param querySignature the query signature
		 * @param vImageScoresNumbers the output ordered list of images matching the query
		 * @param numRankedOuput the number of maximum output images required 
		 */
		void query(const SCFVSignature& querySignature, std::vector< std::pair<double,unsigned int> >& vImageScoresNumbers, size_t numRankedOuput) const;

		/**
		 * Use a subset of a binary SCFV signature as a query to retrieve a ranked list of signatures matching the given one.
		 * @param querySignature the query signature
		 * @param vImageScoresNumbers the output ordered list of images matching the query
		 * @param numRankedOuput the number of maximum output images required
		 */
		void query_bitselection(const SCFVSignature& querySignature, std::vector< std::pair<double,unsigned int> >& vImageScoresNumbers, size_t numRankedOuput) const;
		
		/**
		 * Produces an optional table of weights to reduce the importance of features that are too common. 
		 * @param querySignature the query image signature
		 * @param W2_log (output) the logarithmic weight for mean values
		 * @param W2_log_var (output) the logarithmic weight for variance values
		 * @param weight_base the basic weight from which the table is produced.
		 */
		void generateWeight(const SCFVSignature& querySignature , float *W2_log , float *W2_log_var , float weight_base) const;

		/**
		 * Get the number of images (actually signatures) contained in this index.
		 * @return the number of images.
		 */
		size_t numberImages() const
		{
			return m_signatures.size();
		}
		
		/**
		 * Get the SCFV signature of a specific image.
		 * @param index index of the image in the database of images.
		 * @return the image signature
		 */
		const SCFVSignature & getImage(unsigned int index) const
		{
			return m_signatures[index];
		}

		/**
		 * Resize the index to num elements.
		 * @param num the number of elements required to be in the index 
		 */
		void resize (size_t num)
		{
			m_signatures.resize(num, SCFVSignature(false, false));
		}

		/**
		 * Reserve memory for the given number of signatures.
		 * @param num the number of signatures to be reserved in the index.
		 */
		void reserve (size_t num)
		{
			m_signatures.reserve(num);
		}

		/**
		 * Clear all signatures.
		 */
		void clear()
		{
			m_signatures.clear();
		}

		/**
		 * Match two signatures and return a matching score.
		 * @param signature1 the first SCFV signature
		 * @param signature2 the second SCFV signature
		 * @param pNumWords1 the visited number of words of signature1
		 * @param pNumWords2 the visited number of words of signature2
		 * @param overlap unused
		 * @return the matching score
		 */
		float matchImages(const SCFVSignature& signature1, const SCFVSignature& signature2, unsigned int* pNumWords1, unsigned int* pNumWords2, unsigned int* overlap) const;

		/**
		 * Match two signatures applying bit selection and return a matching score.
		 * @param signature1 the first SCFV signature
		 * @param signature2 the second SCFV signature
		 * @param pNumWords1 the visited number of words of signature1
		 * @param pNumWords2 the visited number of words of signature2
		 * @param overlap unused
		 * @return the matching score
		 */
		float matchImages_bitselection(const SCFVSignature& signature1, const SCFVSignature& signature2, unsigned int* pNumWords1, unsigned int* pNumWords2, unsigned int* overlap) const;

		/**
		 * Initialize the index with Hamming distance weights.
		 */
		void loadHammingWeight();


		// Function MBIT struct
#ifdef USE_MBIT
		int mbit_m;
		int mbit_r;
		double candidate_size;
		void initMBIT();
		void queryMBIT(const SCFVSignature& querySignature, std::vector< std::pair<double,unsigned int> >& vImageScoresNumbers, size_t numRankedOuput, size_t MBIT_Threshold) const;

		void initMBIT(int modeID);
		void initMBIT_bit_selection(int modeID);
		void queryMBIT_sort(const SCFVSignature& querySignature, std::vector< std::pair<double,unsigned int> >& vImageScoresNumbers, size_t numRankedOuput, size_t MBIT_Threshold, int &num_candidate) const;
		void queryMBIT_sort_bit_selection(const SCFVSignature& querySignature, std::vector< std::pair<double,unsigned int> >& vImageScoresNumbers, size_t numRankedOuput, size_t MBIT_Threshold, int &num_candidate) const;


		void makingWeightFunction();
#endif


	private:

		static const float CorrelationWeights[];
		static const float VarCorrelationWeights[];

		static const float CorrelationWeights_bit_selection[];
		static const float VarCorrelationWeights_bit_selection[];

		static void dfs(std::vector<unsigned int> &vec , unsigned int var , int level , int dis);

		static std::vector<unsigned int> getKNN( unsigned int var );


		static bool cmpDoubleUintAscend(const std::pair<double,unsigned int> & pair1, const std::pair<double,unsigned int> & pair2) 
		{
			return pair1.first < pair2.first;
		}

		static bool cmpDoubleUintDescend(const std::pair<double,unsigned int> & pair1, const std::pair<double,unsigned int> & pair2)
		{
			return pair1.first > pair2.first;
		}

		inline unsigned char quantizeByte(float fVal) {
			return (unsigned char)(std::max<double>(0.0, std::min<double>(255.0, 6400.0*fVal + 0.5)));
		}

		inline float dequantizeByte(unsigned char nVal) {
			if ((int)nVal == 255)
				return 0.045f;
			else if ((int)nVal == 0)
				return 0.00004f;
			else
				return (float)nVal / 6400.0f;
		}

		inline unsigned short int quantize2Bytes(float fVal) {
			return (unsigned short int)(std::max<double>(0.0, std::min<double>(65535.0, 65536.0*fVal + 32768 + 0.5)));
		}

		inline float dequantize2Bytes(unsigned short int nVal) {
			if (nVal == 32768)
				return 0.00001f;
			else
				return ((float)nVal - 32768.0f) / 65536.0f;
		}

		std::vector<SCFVSignature> m_signatures;
		static const float fCorrTable[PCASiftLength + 1];
		static const float fVarCorrTable[PCASiftLength + 1];
		static const float fCorrTableBitSelection[num_bit_selection + 1];
		static const float fVarCorrTableBitSelection[num_bit_selection + 1];

#ifdef  USE_WEIGHT_TABLE
		int weight[numberCentroids][M][1<<(PCASiftLength / M)];
		int weight_var[numberCentroids][M][1<<(PCASiftLength / M)];
#endif

#ifdef USE_MBIT
		//Data for MBIT struct
		std::vector< unsigned int > bitsDiff;
		std::vector< unsigned char > numDiff;
		
		std::vector<int> *vec;
		int ***MBIT;
		int **SizeOfMBIT;

		int **W2[numberCentroids];
		int **W2_var[numberCentroids];
#endif
	};


	/**
	 * @class SCFVFactory
	 * A class to produce SCFV signatures.
	 */
	class SCFVFactory
	{
	private:
		static const int UNCOMPRESSED_SIFT_DESC_LENGTH = 128;
		static const int SCFV_MAX_NUMBER_FEATURES = 300;
		static const float GMM_W[];
		static const float GMM_MU[];
		static const float GMM_SIGMA[];
		static const float PCA_SIFT_POWER_LAW;
		static const float SCFV_POWER_LAW;

		static const float PCA_SIFT_MU[];
		static const float PCA_SIFT_EIGEN[];
		
		unsigned int generateSFV(const FeatureList& featureList, float* pFisherVector, bool vWordVisited[], int nNumFeatures) const;

		bool m_bHasVar;
		bool m_bHasBitSelection;
		float m_fSCFVThreshold;

	public:
		SCFVFactory();		// constructor

		//	default destructor, copy-constructor, assignment op are ok

		/**
		 * Initialize the class with the correct set of parameters
		 * @param params the input parameters to use
		 */
		void init(const Parameters & params);

		/**
		 * Generate a global descriptor signature using the given feature list
		 * @param featureList the key points to use as input information
		 * @param signature the output signature
		 * @param nNumFeatures the number of features to encode
		 */
		void generateSCFV(const FeatureList& featureList, SCFVSignature & signature, int nNumFeatures) const;

		/**
		 *  Indicates if using the variance information of the Gaussian function
		 * @return true if using the variance information
		 */
		bool hasVariance() const {
			return m_bHasVar;
		}

		/**
		 *  Indicates if using the bit selection information
		 * @return true if using the bit selection information
		 */
		bool hasBitSelection() const {
			return m_bHasBitSelection;
		}
	};
}

