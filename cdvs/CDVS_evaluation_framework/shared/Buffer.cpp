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
#include "Buffer.h"
#include <cstddef>
#include <cstring>   // std::memcpy, std::memcmp, std::memset, std::memchr
#include <cstdio>
#include <cmath>
#include <string>
#include <algorithm>
#include "CdvsException.h"

using namespace mpeg7cdvs;

#define MyAbs(a) (a>=0)?(a):(-a)

Buffer::Buffer():mydata(NULL),mysize(0)
{
}

Buffer::Buffer(unsigned char * newdata, size_t newsize):mydata(NULL),mysize(0)
{
	assign(newdata, newsize);
}

Buffer::~Buffer()
{
	if (mydata != NULL)
    delete[] mydata;
}


Buffer::Buffer (size_t size):mydata(NULL),mysize(0)
{
	if (size > 0)
	{
		mydata = new unsigned char [size];
		mysize = size;		
	}
}

Buffer::Buffer (const Buffer& buffer):mydata(NULL),mysize(0)
{
	if (buffer.size() > 0)
	{
		mydata = new unsigned char [buffer.size()];
		mysize = buffer.size();
		memcpy (mydata, buffer.mydata, buffer.mysize);
	}
}

Buffer& Buffer::operator= (const Buffer& x)
{	
	if (&x != this)
	{
		if (x.mysize == 0)
		{
			if (mydata != NULL)
				delete[] mydata;
			mydata = NULL;
			mysize = 0;
			return *this;
		}
		
		if (x.mysize > mysize)
		{
		  if (mydata != NULL)
			delete[] mydata;

		  mydata = new unsigned char[x.mysize];
		  mysize = x.mysize;
		}

		if (x.mysize > 0)
		  memcpy (mydata, x.mydata, x.mysize);

		mysize = x.mysize;
	}

	return *this;	
}

void Buffer::swap (Buffer& x)
{
  unsigned char* d  = x.mydata;
  size_t s  = x.mysize;

  x.mydata = mydata;
  x.mysize = mysize;
 
  mydata = d;
  mysize = s;
}


void Buffer::fill (unsigned char value)
{
	if (mysize > 0)
		memset (mydata, value, mysize);
}

size_t Buffer::size () const
{
	return mysize;
}

bool Buffer::resize (size_t newsize)
{
  if (newsize <= 0)
	  return false;
	  
  if (newsize <= mysize)	// size decrease request
  {
	  mysize = newsize;
	  return true;
  }
  
  unsigned char* d = new unsigned char[newsize];

  if (mydata != NULL)
    delete[] mydata;

  mydata = d;
  mysize = newsize;
 
  return true;
}
 
bool Buffer::empty () const
{
	return (mysize == 0);
}

void Buffer::clear ()
{
	if (mydata != NULL)
	{
		delete[] mydata;
		mydata = NULL;
	}
	mysize = 0;
}

bool Buffer::assign(const unsigned char * newdata, size_t newsize)
{
	if ((newsize <= 0) || (newdata == NULL))
		return false;
	
	if (newsize > mysize)
	{
	  if (mydata != NULL)
		delete[] mydata;

	  mydata = new unsigned char[newsize];
	}

	memcpy (mydata, newdata, newsize);
	mysize = newsize;
	return true;
}

bool Buffer::equals(Buffer & buffer)
{
	if (buffer.mysize != mysize)
		return false;
	
	return (memcmp (data(), buffer.data(), buffer.size()) == 0);		
}

unsigned char* Buffer::data ()
{
	return mydata;
}

const unsigned char* Buffer::data () const
{
	return mydata;
}


/*
 * Read a file content into the buffer.
 */
void Buffer::read(const char * fname)
{
	FILE *pfile;

	if ((pfile = fopen (fname, "rb")) == NULL)
	{
		throw CdvsException(std::string("Cannot read file ").append(fname));
	}

	fseek(pfile, 0, SEEK_END);			// set the file pointer to end of file
	long filesize = ftell(pfile);		// get the file size
	rewind(pfile);						// rewind to start of file

	resize(filesize);

	if (fread (mydata, filesize, 1, pfile) != 1)
	{
		fclose (pfile);
		throw CdvsException(std::string("Error reading ").append(fname));
	}

	fclose (pfile);
}

void Buffer::write(const char * fname) const
{
	FILE * pfile;
	if ((pfile = fopen (fname, "wb")) == NULL)
	{
		throw CdvsException(std::string("Cannot write file ").append(fname));
	}

	if (fwrite (data(), size(), 1, pfile) != 1)
	{
		fclose(pfile);
		throw CdvsException(std::string("Error writing ").append(fname));
	}

	fclose (pfile);
}

int Buffer::compare(const Buffer & other) const
{
	size_t minsize = std::min(mysize, other.mysize);
	const unsigned char * a = data();
	const unsigned char * b = other.data();
	int count = 0;

	for (size_t k=0; k<minsize; ++k)
	{
		if (*(a++) != *(b++))
				count++;
	}

	if (mysize != other.mysize)
		count += MyAbs((int)(mysize - other.mysize));

	return count;
}

bool Buffer::operator== (const Buffer& other) const
{
	return (compare(other) == 0);
}
