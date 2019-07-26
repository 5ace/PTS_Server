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

#ifndef __BITINPUTSTREAM_H__
#define __BITINPUTSTREAM_H__

#include <stdlib.h>

namespace mpeg7cdvs
{


/**
 * @class BitInputStream
 * This class represents an input stream of bits. 
 * @author Massimo Balestri, Andrea Varesio, Marco Vecchietti
 * @date 2002
 */
class BitInputStream
{
private:
	bool isOpen;						// status of this stream (opened/closed) 
	unsigned int m_uNumOfBitsInBuffer;	// the remaining bits in the bit buffer
	unsigned int m_iBitBuffer;			// the 32-bit buffer (left-aligned)
	const unsigned char* pBuf;			// current read position in the input buffer
	const unsigned char* pStartBuf;		// pointer to the start of the buffer
	const unsigned char* pEndBuf;		// pointer to the end of the buffer

	void fetch();						// fetch 32 bits from the bitstream into the bit buffer. 

public:
	
	/**
	 * Create an empty object. The open method must be first called to actually use the object.
	 */	
	BitInputStream();
	
	/**
	 * Create and initialize a BitInputStream.
	 * Attach the buffer from which data will be read in all subsequent operations.
	 * @param buf the buffer from which the data will be read
	 * @param size the size of the buffer in bytes (minimum size is 4 bytes)
	 */	
	BitInputStream(const unsigned char * buf, size_t size);

	/**
	 * Close the stream, if not yet done, and destroy the object.
	 */
	virtual ~BitInputStream();
	
	/**
	 * Attach the buffer from which data will be read in all subsequent operations.
	 * @param buf the buffer from which the data will be read
	 * @param size the size of the buffer in bytes (minimum size is 4 bytes)
	 */
	void open(const unsigned char * buf, size_t size);


	/**
	 * closes this input stream and releases any resources associated with the stream
	 */
	void close();

	/**
	 * Reads the next bit from the input stream.The operation fails if eof() is true.
	 * @return the next bit from the input stream.
	 */
	unsigned int read();

	/**
	 * Reads the specified number of bits from the input stream. The operation fails if eof() is true.
	 * @param nbits the number of bits to read (in the range 1..32)
	 * @return the next n bits from the input stream.
	 */
	unsigned int read(unsigned int nbits);

	/**
	 * Reads the specified number of bits from the input stream into the destination buffer, 
	 * assuming that the input is byte-aligned. The operation fails if eof() is true.
	 * @param destination the destination buffer
	 * @param nbits the number of bits to read (8*n, assuming n>0)
	 */
	void read(unsigned char * destination, unsigned int nbits);

	/**
	 * Skip the next n bits while reading from the current position.
	 * If the end of the buffer is reached or even surpassed, eof() will return true.
	 * @param nbits the number of bits to skip (0..MAXINT)
	 */
	void skip(unsigned int nbits);


	/**
	 * Informs about the read cursor position.
	 * @return true if the end of the input buffer has been reached.
	 */
	bool eof () const;

	/**
	 * Reposition the read pointer at the beginning of the stream.
	 */
	void reset();

	/**
	 * Align the read pointer to the closest byte boundary. 
	 * If the read pointer is already aligned, the read pointer is not changed.
	 * @return the number of skipped bits
	 */
	unsigned int align();

	/**
	 * Returns the number of bits that can be read from this stream starting 
	 * form the current position.
	 * @return the number of bits that can be read.
	 */
	size_t available() const;

	/**
	 * Returns the number of bits that have been read so far.
	 * @return the number of bits read so far.
	 */
	size_t consumed() const;

	/**
	 * Return the current read pointer, assuming it is byte-aligned.
	 * @return the read pointer
	 */
	const unsigned char * getPointer() const;

	/**
	 * Get the size of the attached buffer.
	 * @return the size in bytes.
	 */
	size_t getSize() const;

	/**
	 * Jump to the indicated absolute position (in bits).
	 * @param position the number of bits to skip from the start of the buffer.
	 */
	void jumpTo(size_t position);

};

}	// end of namespace;

#endif
