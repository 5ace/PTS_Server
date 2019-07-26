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
#pragma once
#include <cstddef>   // std::size_t

namespace mpeg7cdvs
{


/**
 * @class Buffer
 * A container class for a byte array, intended to replace all malloc() and new() instructions in the main code.
 * This class properly deallocates memory when an exception is thrown.
 * @author Massimo Balestri
 * @date 2013
 */
class Buffer
{
public:
  Buffer();
  virtual ~Buffer();
  Buffer (size_t size);								///< create a buffer of the given size
  Buffer (unsigned char * data, size_t size);		///< copy the given array into this Buffer

  Buffer (const Buffer&);					///< copy the given Buffer into this Buffer
  Buffer& operator= (const Buffer&);		///< assign a Buffer to another

  void swap (Buffer& x);					///< swap the content of two Buffer(s)
  void fill (unsigned char value = 0);		///< fill a Buffer with the given value

  size_t size () const;						///< return the current size of the Buffer
  
  bool resize (size_t newsize);				///< change buffer size; content is lost if newsize if less than the current size
 
  bool empty () const;				///< return true if the Buffer is empty

  void clear ();					///< clear the Buffer

  bool assign(const unsigned char * data, size_t size);		///< assign the given data to Buffer

  bool equals(Buffer & buffer);			///< compare if two Buffer(s) are equal (i.e. if they have the same size and contain the same data)

  unsigned char* data ();				///< access to Buffer's data (writable)
  
  const unsigned char* data () const;	///< access to Buffer's data (read only)

  void read(const char * fname);		///< read Buffer from a file

  void write(const char * fname) const;	///< write Buffer to file

  /**
   * Compare this buffer with another; return the number of different bytes.
   * @param other the other Buffer
   * @return the number of differences; zero if no difference is found.
   */
  int compare(const Buffer & other) const;

  bool operator== (const Buffer& other) const;		///< compare if two Buffer(s) are equal (i.e. if they have the same size and contain the same data) 

private:
  unsigned char* mydata;
  size_t mysize;
};


} // end of namespace
