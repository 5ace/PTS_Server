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

#include <iostream>
#include <math.h>
#include <assert.h>

#include "ArithmeticCoding.h"
#include "CdvsException.h"

using namespace std;
using namespace mpeg7cdvs;

#define DEBUGLEVEL 0

// --------------- AC_model class implementation ----------------- //

AC_model::AC_model()
{
	nsym = 0;
	adapt = false;
	cfreq = NULL;
	assert(PRECISION >= CODE_VALUE_BITS + FREQUENCY_BITS);
	assert(CODE_VALUE_BITS >= FREQUENCY_BITS + 2);
}

void AC_model::check(int symbol) const
{
	if ((symbol<0)||(symbol >= nsym))
		throw CdvsException("symbol out of range");
}

void AC_model::init (int nsym, int *ifreq, int adapt)
{
	this->nsym = nsym;
	this->adapt = (adapt > 0);
	this->cfreq = new CODE_VALUE[nsym + 1];

	if (ifreq)
	{
		while(true)
		{
			cfreq[0] = 0;
			for ( int i = 0 ; i < nsym ; ++i )
			{
				cfreq[i+1] = cfreq[i] + ifreq[i];
			}

			if (count() <= MAX_FREQ)
				return;

			for ( int i = 0 ; i < nsym ; ++i )
			{
				ifreq[i] = (ifreq[i] > 1) ? ifreq[i] >> 1 :  1;	// frequency cannot be less than 1
			}
		}
	}
	else
	{
	   for ( int i = 0 ; i <= nsym ; ++i )
	      cfreq[i] = i;
	}

    if (count() > MAX_FREQ)
      throw CdvsException("arithmetic coder model max frequency exceeded");
}

void AC_model::init (int nsym, long *cumfreq)
{
    // convert to frequency

    int * freq = new int[nsym];
    freq[0] = (int) cumfreq[0];
	for (int i=1; i<nsym; ++i)
		freq[i] = (int) (cumfreq[i] - cumfreq[i-1]);

	// call init

    init(nsym, freq, 0);

    delete[] freq;
}

AC_model::~AC_model()
{
	if (cfreq != NULL)
		delete [] cfreq;
}

void AC_model::update(int sym)
{
	if (adapt)
	{
		for ( int i = sym + 1 ; i <= nsym ; ++i )
		  cfreq[i]++;

		if ( count() >= MAX_FREQ )
		  adapt = false;
	}
}

void AC_model::print()
{
    cout << "Model constants:\n"
      << "CODE_VALUE with precition of " << PRECISION << " bits\n"
      << "CODE_VALUE_BITS " << CODE_VALUE_BITS << " bits giving MAX_CODE of "      << MAX_CODE << "\n"
      << "FREQUENCY_BITS "  << FREQUENCY_BITS  << " bits giving MAX_FREQUENCY of " << MAX_FREQ << "\n"
      << "TOP_VALUE: "      << TOP_VALUE     << " (0x" << std::hex << TOP_VALUE      << std::dec << ")\n"
      << "ONE_FOURTH: "     << ONE_FOURTH    << " (0x" << std::hex << ONE_FOURTH    << std::dec << ")\n"
      << "ONE_HALF: "       << ONE_HALF      << " (0x" << std::hex << ONE_HALF      << std::dec << ")\n"
      << "THREE_FOURTHS: "  << THREE_FOURTHS << " (0x" << std::hex << THREE_FOURTHS << std::dec << ")\n";

}

void AC_model::done ()
{
  nsym = 0;
  adapt = false;
  if (cfreq != NULL)
		delete [] cfreq;
  cfreq = NULL;
}


// --------------- AC_encoder class implementation ----------------- //

void AC_encoder::output_bit (int bit)
{
	writer->write(bit);
	total_bits++;
#if DEBUGLEVEL > 0
	cout << bit ;
#endif
	return;
}


void AC_encoder::bit_plus_follow (int bit)
{
  output_bit (bit);
  while (fbits > 0)  {
    output_bit (!bit);
    fbits -= 1;
  }

  return;
}

void AC_encoder::done ()
{
#if DEBUGLEVEL > 0
	cout << endl <<  "symbol: EOF = ";
#endif

  fbits += 1;
  if (low < ONE_FOURTH)
    bit_plus_follow (0);
  else
    bit_plus_follow (1);

#if DEBUGLEVEL > 0
	cout << endl;
#endif

  return;
}

void AC_encoder::init(BitOutputStream & outw)
{
	writer = &outw;
	low = 0;
	high = TOP_VALUE;
	fbits = 0;
	total_bits = 0;
	return;
}


long AC_encoder::bits() const
{
  return total_bits;
}


void AC_encoder::encode_symbol (AC_model & acm, int sym)
{
#if DEBUGLEVEL > 0
	cout << endl <<  "symbol: " << sym << " = ";
#endif
#if DEBUGLEVEL > 1
    cout << "(high: " << acm.high(sym) << " low: " << acm.low(sym) << " count: 0x" << acm.count() << ") " << dec;
#endif

	acm.check(sym);

	CODE_VALUE range = high - low + 1;

    high = low + (range * acm.high(sym) / acm.count()) - 1;
    low = low + (range * acm.low(sym) / acm.count());

	for (;;) {
		if ( high < ONE_HALF )
			bit_plus_follow(0);
		else if ( low >= ONE_HALF )
			bit_plus_follow(1);
		else if ( low >= ONE_FOURTH && high < THREE_FOURTHS ) {
			fbits++;			// pending bits
			low -= ONE_FOURTH;
			high -= ONE_FOURTH;
		} else
			break;

		high = ((high << 1) | 1) & MAX_CODE;
		low = (low << 1) & MAX_CODE;
	}

	acm.update(sym);
}

// --------------- AC_decoder class implementation ----------------- //

int AC_decoder::input_bit ()
{
	total_bits++;
	return myReader.read();
}

void AC_decoder::init (BitInputStream & inreader)
{
	reader = &inreader;	// save a reference to the real reader
	myReader = inreader;  // make a copy of the real reader
	value = myReader.read(CODE_VALUE_BITS);	//look ahead next n bits
	garbage_bits = 0;
	low = 0;
	high = TOP_VALUE;
	total_bits = 2;
}

void AC_decoder::done ()
{
	reader->skip(bits());		// skip all read bits
	return;
}

long AC_decoder::bits () const
{
  return total_bits;
}

int AC_decoder::decode_symbol (AC_model & acm)
{
	CODE_VALUE range = high - low + 1;
	CODE_VALUE cum =  ((value - low + 1) * acm.count() - 1 ) / range;
	int sym = acm.getSymbol(cum);
	acm.check (sym);

	high = low + ((range*acm.high(sym))   / acm.count()) - 1;
	low  = low + ((range*acm.low(sym)) / acm.count());

	for( ; ; ) {
		if ( high < ONE_HALF ) {
			//do nothing, bit is a zero
		} else if ( low >= ONE_HALF ) {
			value -= ONE_HALF;  //subtract one half from all three code values
			low -= ONE_HALF;
			high -= ONE_HALF;
		} else if ( low >= ONE_FOURTH && high < THREE_FOURTHS ) {
			value -= ONE_FOURTH;
			low -= ONE_FOURTH;
			high -= ONE_FOURTH;
		} else
			break;

		high = ((high << 1) | 1) & MAX_CODE;
		low = (low << 1) & MAX_CODE;

		if(myReader.eof())
		{
			value <<= 1;
			garbage_bits++;
			assert( garbage_bits <= CODE_VALUE_BITS - 2);
			if (garbage_bits > (CODE_VALUE_BITS - 2))
				throw CdvsException("too many bits after eof");
		}
		else
		{
	    	value = (value << 1) + input_bit();
		}
	}

	acm.update(sym);

	return (sym);
}



