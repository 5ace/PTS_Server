/*
The MIT License (MIT)

Copyright (c) 2014 Mark Thomas Nelson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

 This code was written to illustrate the article:
 Data Compression With Arithmetic Coding
 by Mark Nelson
 published at: http://marknelson.us/2014/10/19/data-compression-with-arithmetic-coding

*/

/*
 * Change Log:
 *
 * September, 2011 by Giovanni Cordara (Telecom Italia)
 * Original code wrapped into C++ classes.
 *
 * October, 2011 by Massimo Balestri (Telecom Italia)
 * Input/output bitstream from/to BitInputStream/BitOutputStream instead of FILE *fp.
 *
 * June, 2012 by Massimo Balestri (Telecom Italia)
 * Original code wrapped again to restore backward compatibility with the Fred Weeler's implementation.
 * This version passes the test contained in the acdemo.c code (with small adaptation to the C++ syntax)
 *
 * January 2013 by Massimo Balestri (Telecom Italia)
 * All malloc/free replaced by new/delete
 *
 * January 2014 by Massimo Balestri (Telecom Italia)
 * Reimplemented using code by Mark Nelson, wrapped again into the AC_model, AC_encoder, AC_decoder classes for backward compatibility.
 */

#pragma once

#include "BitInputStream.h"
#include "BitOutputStream.h"

namespace mpeg7cdvs
{
  // do not change the following constants

  typedef unsigned long long CODE_VALUE;

  static const int PRECISION            = 64;
  static const int CODE_VALUE_BITS      = 18;
  static const int FREQUENCY_BITS       = CODE_VALUE_BITS - 2;
  static const CODE_VALUE MAX_CODE      = (1 << CODE_VALUE_BITS) - 1;
  static const CODE_VALUE MAX_FREQ      = (1 << FREQUENCY_BITS) - 1;
  static const CODE_VALUE TOP_VALUE		= MAX_CODE;
  static const CODE_VALUE ONE_FOURTH    = 1 << (CODE_VALUE_BITS - 2);
  static const CODE_VALUE ONE_HALF      = 2 * ONE_FOURTH;
  static const CODE_VALUE THREE_FOURTHS = 3 * ONE_FOURTH;

  // the following must be true to avoid overflow or underflow:

  // PRECISION >= CODE_VALUE_BITS + FREQUENCY_BITS
  // CODE_VALUE_BITS >= FREQUENCY_BITS + 2

/**
 * @class AC_model
 * Arithmetic Coding model to be used when encoding or decoding a symbol.
 * @author Giovanni Cordara, Massimo Balestri
 * @date 2012
 */
class AC_model {
private:
	int nsym;
	CODE_VALUE * cfreq;
	bool adapt;

public:
	AC_model();			// default constructor

	virtual ~AC_model();	// destructor


	/**
	 * Initialize an Arithmetic Coder model using the given accumulated frequency values.
	 * Note that adaptation is disabled in this case.
	 * @param nsym number of symbols to be encoded/decoded
	 * @param cfreq the cumulative frequency of all symbols
	 */
	void init(int nsym, long *cfreq);

	/**
	 * Initialize an Arithmetic Coder model using the given input frequency values.
	 * @param nsym number of symbols to be encoded/decoded;
	 * @param ifreq the input frequency table, if NULL a frequency of one for all elements will be assumed;
	 * @param adapt flag indicating if dynamic adaptation must be performed during encoding/decoding
	 */
	void init (int nsym, int *ifreq, int adapt);

	/**
	 * update model with the given symbol
	 * @param symbol the symbol to update
	 */
	void update(int symbol);

	/**
	 * Terminate using this Arithmetic Coder model.
	 */
	void done ();

	/**
	 * Print the model constants.
	 */
	void print();

	/**
	 * Check if the symbol belongs to the symbol range of this model.
	 * @param symbol the symbol to check
	 * @throws CdvsException
	 */
	void check(int symbol) const;

	/**
	 * Get the higher accumulated frequency of this symbol.
	 * @param symbol the input symbol
	 * @return the higher value of the symbol
	 */
	int high(int symbol) const
	{
		return cfreq[symbol+1];
	}

	/**
	 * Get the lower accumulated frequency of this symbol.
	 * @param symbol the input symbol
	 * @return the lower value of the symbol
	 */
	int low(int symbol) const
	{
		return cfreq[symbol];
	}

	/**
	 * Get the total accumulated frequency.
	 * @return the total accumulated frequency
	 */
	int count() const
	{
		return cfreq[nsym];
	}

	/**
	 * Get the symbol corresponding to a specific code value.
	 * @param cum the accumulated code value 
	 * @return the symbol
	 */
	int getSymbol(CODE_VALUE cum) const
	{
		CODE_VALUE low = 0;
		CODE_VALUE high = nsym;

		while ((high - low) > 1)
		{
			CODE_VALUE mid = (low + high) >> 1;
			if (cum < cfreq[mid])
				high = mid;
			else
				low = mid;
		}

		return (int)low;
	}

	/**
	 * Get the current mode.
	 * @return true if currently in adaptative mode
	 */
	bool adaptative() const
	{
		return adapt;
	}
};

/**
 * @class AC_encoder
 * The encoder using the Arithmetic Coding model.
 * @author Giovanni Cordara, Massimo Balestri
 * @date 2012
 */
class AC_encoder {
private:
	CODE_VALUE low;
	CODE_VALUE high;
	long fbits;
	long total_bits;
	BitOutputStream * writer;

	void output_bit (int bit);
	void bit_plus_follow (int bit);

public:
	/**
	 * Initialize the decoder using the given output buffer writer.
	 * @param writer the output buffer writer.
	 */
	void init(BitOutputStream & writer);

	/**
	 * Encode a symbol using the given Arithmetic Coding model.
	 * @param model the decoding model to be used.
	 * @param symbol the symbol to encode.
	 */
	void encode_symbol(AC_model & model, int symbol);

	/**
	 * Get the number of bits currently encoded.
	 * @return the number of bits written into the bitstream.
	 */
	long bits() const;

	/**
	 * Terminate writing into the output buffer.
	 */
	void done ();
};

/**
 * @class AC_decoder
 * The decoder using the Arithmetic Coder Model.
 * @author Giovanni Cordara, Massimo Balestri
 * @date 2012
 */
class AC_decoder {
private:
	CODE_VALUE value;
	CODE_VALUE low;
	CODE_VALUE high;
	long total_bits;
	int garbage_bits;				// Number of bits past end-of-file
	BitInputStream * reader;		// external reader
	BitInputStream myReader;		// local reader;

	int input_bit ();				// reads one bit

public:

	/**
	 * Initialize the decoder using the given input buffer reader.
	 * @param reader the input buffer reader
	 */
	void init (BitInputStream & reader);

	/**
	 * Decode a symbol using the given Arithmetic Coding model.
	 * @param model the decoding model to be used.
	 * @return the decoded symbol.
	 */
	int decode_symbol(AC_model & model);

	/**
	 * Get the number of bits currently decoded.
	 * @return the number of bits read from the bitstream.
	 */
	long bits() const;

	/**
	 * Terminate reading from the input buffer
	 */
	void done ();
};

} 	// end of namespace
