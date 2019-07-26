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

#ifndef __BITOUTPUTSTREAM_H__
#define __BITOUTPUTSTREAM_H__

#include <stdlib.h>

namespace mpeg7cdvs
{

/**
 * @class BitOutputStream
 * This class represents an output stream of bits. 
 * @author Massimo Balestri, Andrea Varesio, Marco Vecchietti
 * @date 2002
 */
class BitOutputStream
{
private:
	unsigned char * startBuffer;		// pointer to the start of the write buffer
	unsigned char * writePtr;			// pointer to the current position in the write buffer
	unsigned char * endBuffer;			// pointer to the end of the buffer
	unsigned bitBuffer;					// 32-bit buffer used to collect bits
	unsigned availableBits;				// number of empty bits in the buffer above
	bool isOpen;						// indicates if the output buffer has been attached
	
	void store();						// a private method that stores 32 bits into the write buffer

public:
	/**
	 * Creates an empty object. The open method must be first called to actually use the object.
	 */
	BitOutputStream();
	
	/**
	 * Create and initialize a BitOutputStream.
	 */
	BitOutputStream(unsigned char * buf, size_t size);

	/**
	 * Closes the stream, if not yet done, and destroys the object.
	 */
	virtual ~BitOutputStream();
	
	/**
	 * Attaches the object to a buffer of known size. This will be the destination in 
	 * which data will be written in all subsequent operations.
	 * @param buf the buffer in which the data will be written
	 * @param size the size of the buffer in bytes
	 */
	void open(unsigned char * buf, size_t size);

	/**
	 * Close this input stream.
	 * This method flushes data and releases any resources associated with the stream.
	 * @return the total number of produced bits.
	 */
	size_t close();

	/**
	 * Align to the next byte boundary and flushes this output stream forcing any buffered output bits to be written in the destination buffer.
	 * @param fill the value to be used in order to fill the missing bits (must be 0 or 1).
	 */
	void flush(unsigned int fill);

	/**
	 * writes one bit into the output stream. The operation fails if eof() is true.
	 * @param bit the bit to write (must be 0 or 1) 
	 */
	void write(unsigned int bit);

	/**
	 * Writes the specified number of bits into the input stream. The operation fails if eof() is true.
	 * @param value the value to be written into  stream.
	 * @param nbits the number of bits to write (in the range 1..32)
	 */
	 void write(unsigned int value, unsigned int nbits);

	/**
	 * Writes the specified number of bits from the source buffer into the output stream, 
	 * assuming that the output is byte-aligned. The operation fails if eof() is true.
	 * @param source the source buffer (MUST be unsigned char, do not cast int or short arrays!)
	 * @param nbits the number of bits to be copied from the source into this output stream (8*n, assuming n>0)
	 */
	void write(unsigned char * source, unsigned int nbits);

	/**
	 * Reposition the write pointer at the beginning of the stream.
	 */
	void reset();

	/**
	 * Skip the next n bits while writing into the current position.
	 * This operation first flushes any buffered bits, then jumps to the new location.
	 * If the end of the buffer is reached or even surpassed, eof() will return true.
	 * @param nbits the number of bits to skip (0..MAXINT)
	 */
	void skip(unsigned int nbits);


	/**
	 * Informs about the write cursor position.
	 * @return true if the end of the output buffer has been reached.
	 */
	bool eof () const;

	/**
	 * Align the write pointer to the closest byte boundary. 
	 * If the write pointer is already aligned, the write pointer is not changed.
	 * @param fill the value to be used in order to fill the missing bits (must be 0 or 1).
	 */
	void align(unsigned int fill);

	/**
	 * Returns the number of bits that can be written into this stream starting
	 * form the current position.
	 * @return the number of bits that can be written.
	 */
	size_t available() const;

	/**
	 * Returns the number of bits that have been written so far.
	 * @return the number of bits written so far.
	 */
	size_t produced() const;

	/**
	 * returns the current write pointer, assuming it is byte-aligned.
	 */
	unsigned char * getPointer() const;

	/**
	 * Get the size of the attached buffer.
	 * @return the size in bytes.
	 */
	size_t getSize() const;

	/**
	 * Jump to the indicated absolute position (in bits).
	 * The absolute position must be byte-aligned.
	 * @param position the number of bits to skip from the start of the buffer.
	 */
	void jumpTo(size_t position);
};

}	// end of namespace

#endif
