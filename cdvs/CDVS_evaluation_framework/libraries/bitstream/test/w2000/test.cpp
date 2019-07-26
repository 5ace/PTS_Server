// prova di uso delle API per lettura/scrittura bitstream in Network byte order

#include "BitInputStream.h"
#include "BitOutputStream.h"
#include <windows.h>		// needed for timeGetTime()
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <assert.h>

//#define ALTERNATE_TEST			// test the fancy methods




#ifdef ALTERNATE_TEST

#define SKIP_START_BIT		511
#define SKIP_END_BIT		(8*369)
#define BUFFER_SIZE			(SKIP_END_BIT/8 + 437)		// buffer size in bytes 

unsigned char in_buffer[BUFFER_SIZE];
unsigned char out_buffer[BUFFER_SIZE];

/*
 * ------------------------ ALTERNATE TEST ---------------------------
 * this test checks the fancy methods like skip(), reset(), getPointer(), bookmarks, jumpTo();
 * Write the first SKIP_START_BIT bits, then skip to the SKIP_END_BIT bit, write up to the end.
 * Go back to the SKIP_START_BIT byte, write up to the SKIP_END_BIT.
 * Finally check that the two buffers contain the same bits.
 */
int main()
{

	srand( timeGetTime() );			// Seed the random-number generator with current time 

	for( int i = 0;   i < BUFFER_SIZE; i++ )	// initialize the input buffer
		in_buffer[i] = (char) rand() ;			// copy the last 8 bits only
	
	memset(out_buffer,0,BUFFER_SIZE);			// initialize the output buffer

	BitInputStream in(in_buffer, BUFFER_SIZE);
	BitOutputStream out(out_buffer, BUFFER_SIZE);				// attach input and output
	
	// test vector read/write
	
	assert(SKIP_START_BIT > 128 );		// se no il test non funziona

	{ 
		unsigned int val = in.read(7);	// read the first 16 bits
		out.write(val, 7);
		val = in.read(9);
		out.write(val, 9);
	}

	unsigned char tmpbuf[128/8];

	in.read(tmpbuf, 128);				// test block reading
	out.write(tmpbuf, 128);				// test block writing

	in.read(tmpbuf, 48);
	out.write(tmpbuf, 48);

	// Write the first SKIP_START_BIT bits,
	
	while ( out.produced() < SKIP_START_BIT )
	{
		assert(! in.eof());
		assert(! out.eof());
		unsigned int nbits = 1 + (0x1F & rand());	// number of bits to read/write (must be 1..32 bits)
		if ((nbits + out.produced()) > SKIP_START_BIT)
			nbits = SKIP_START_BIT - out.produced();		// do not try to write more than SKIP_START_BIT!

		unsigned int value = in.read(nbits);		// read
		out.write(value, nbits);					// write
		assert(in.available() == out.available());
		assert(in.consumed() == out.produced());
	}

	// bookmark the current position

	BitInputStream markin = in;				// use default copy constructor
	BitOutputStream markout = out;

	// jump to SKIP_END_BIT
	in.jumpTo(SKIP_END_BIT);
	out.jumpTo(SKIP_END_BIT);

	// write from SKIP_END_BIT to the end
	while (! in.eof() )
	{
		assert(! out.eof());
		unsigned int nbits = 1 + (0x1F & rand());	// number of bits to read/write (must be 1..32 bits)
		
		if (nbits > in.available())
			nbits = in.available();		// do not try to read more than available!

		if (nbits == 1)					// test the single bit read/write
		{
			unsigned int value = in.read();
			out.write(value);
		}
		else							// test the multiple bit read/write
		{
			unsigned int value = in.read(nbits);
			out.write(value, nbits);
		}

		assert(in.available() == out.available());
		assert(in.consumed() == out.produced());

	}
	in.close();				// terminate reading
	out.close();			// terminate writing

	// now write from mark to SKIP_END_BIT

	while (markout.produced() < SKIP_END_BIT)
	{
		assert(! markin.eof());
		assert(! markout.eof());
		unsigned int nbits = 1 + (0x1F & rand());	// number of bits to read/write (must be 1..32 bits)
		if ((nbits + markout.produced()) > SKIP_END_BIT)
			nbits = SKIP_END_BIT - markout.produced();		// do not try to write more than SKIP_END_BIT!

		unsigned int value = markin.read(nbits);		// read
		markout.write(value, nbits);					// write
		assert(markin.available() == markout.available());
		assert(markin.consumed() == markout.produced());
	}
	markin.close();
	markout.close();					// flush output

	if (memcmp(in_buffer, out_buffer, BUFFER_SIZE) == 0)
		printf("test passed.\n");
	else
	{
		printf("test not passed.\n");
		
		int counter = 0;
		for (int i = 0; i < BUFFER_SIZE; i++)		// check the difference
		{
			if (in_buffer[i] != out_buffer[i])
			{
				counter++;
				printf("byte number %d differ: input = %x, output = %x\n", i + 1, (int) in_buffer[i], (int) out_buffer[i]);
			}
		}

		printf("%d bytes differ.\n", counter);

	}

	return 0;
}

	
#else			// end of ALTERNATE_TEST

#define BUFFER_SIZE			1937487		// buffer size in bytes 

unsigned char in_buffer[BUFFER_SIZE];
unsigned char out_buffer[BUFFER_SIZE];


/*
 * ------------------------ MAIN TEST ---------------------------
 * this test fills a buffer with random numbers, 
 * then reads from the buffer a random number of bits, 
 * write the same number of bits on another buffer, and finally
 * checks that output buffer == input buffer.
 */
int main()
{

	srand( timeGetTime() );			// Seed the random-number generator with current time 

	for( int i = 0;   i < BUFFER_SIZE; i++ )	// initialize the input buffer
		in_buffer[i] = (char) rand() ;			// copy the last 8 bits only
	
	memset(out_buffer,0,BUFFER_SIZE);			// initialize the output buffer

	DWORD start = timeGetTime();				// time in milliseconds

	BitInputStream in;
	in.open(in_buffer, BUFFER_SIZE);
	BitOutputStream out;
	out.open(out_buffer, BUFFER_SIZE);				// attach input and output

	while (! in.eof() )
	{
		assert(! out.eof());
		unsigned int nbits = 1 + (0x1F & rand());	// number of bits to read/write (must be 1..32 bits)
		
		if (nbits > in.available())
			nbits = in.available();		// do not try to read more than available!

		if (nbits == 1)					// test the single bit read/write
		{
			unsigned int value = in.read();
			out.write(value);
		}
		else							// test the multiple bit read/write
		{
			unsigned int value = in.read(nbits);
			out.write(value, nbits);
		}

		assert(in.available() == out.available());
		assert(in.consumed() == out.produced());

	}
	in.close();				// terminate reading
	out.close();			// terminate writing

	DWORD stop = timeGetTime();					// time in milliseconds

	{											// performance check
		double bitrate = 1.0;
		bitrate = bitrate * (0.008 *BUFFER_SIZE/(stop-start));
		printf ("Binary read/write time %d ms, bitrate %12.6f Mbit/s\n", stop-start, bitrate);
	}

	if (memcmp(in_buffer, out_buffer, BUFFER_SIZE) == 0)
		printf("test passed.\n");
	else
	{
		printf("test not passed.\n");
		
		int counter = 0;
		for (int i = 0; i < BUFFER_SIZE; i++)		// check the difference
		{
			if (in_buffer[i] != out_buffer[i])
			{
				counter++;
				printf("byte number %d differ: input = %x, output = %x\n", i + 1, (int) in_buffer[i], (int) out_buffer[i]);
			}
		}

		printf("%d bytes differ.\n", counter);

	}

	return 0;
}

#endif