/*
This code is part of enceval-toolkit, which is distributed under the following BSD license:

Copyright (c) 2011-2013, Ken Chatfield, University of Oxford
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
 * Neither the name of the project nor the names of its contributors may be
   used to endorse or promote products derived from this software without
   specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

///
/// \file	simd_math.h
/// \brief	Math routines using SIMD vector extensions
///
/// \author	Jorge Sanchez
/// \date	31/08/10 
///

#pragma once


class simd_math
{
#if defined(__GNUC__) && defined(__SSE2__)
  typedef float  v4sf __attribute__ ((vector_size(16)));
  typedef double v2df __attribute__ ((vector_size(16)));
#endif

public:
  simd_math(){};

  // sum_i a_i*b_i
  static float dot_product( int ndim, const float *a, const float *b );
  static double dot_product( int ndim, const double *a, const double *b);

  // a_i *= c
  static void scale( int ndim, float *a, const float c );
  static void scale( int ndim, double *a, const double c );

  // a_i += c
  static void offset( int ndim, float *a, const float c );
  static void offset( int ndim, double *a, const double c );

  // a_i -= b_i
  static void sub( int ndim, float *a, const float *b );
  static void sub( int ndim, double *a, const double *b );

  // a_i -= b_i^2
  static void sub2( int ndim, float *a, const float *b );
  static void sub2( int ndim, double *a, const double *b );

  // a_i += b_i
  static void add( int ndim, float *a, const float *b );
  static void add( int ndim, double *a, const double *b );

  // a_i += c * b_i
  static void add( int ndim, float *a, const float *b, const float c );
  static void add( int ndim, double *a, const double *b, const double c );

  // a_i += c * b_i^2
  static void add2( int ndim, float *a, const float *b, const float c );
  static void add2( int ndim, double *a, const double *b, const double c );

  // a_i *= b_i
  static void mult( int ndim, float *a, const float *b );  
  static void mult( int ndim, double *a, const double *b );

  // sum_i (a_i-b_i)^2
  static float l2_sq( int ndim, const float *a, const float *b );
  static double l2_sq( int ndim, const double *a, const double *b );

  // sum_i c_i*(a_i-b_i)^2
  static float weighted_l2_sq( int ndim, const float *a, const float *b, const float *c );
  static double weighted_l2_sq( int ndim, const double *a, const double *b, const double *c );

  // s1_i += x_i
  // s2_i += x_i * x_i
  static void accumulate_stat( int ndim, float *s1, float *s2, const float *x );
  static void accumulate_stat( int ndim, double *s1, double *s2, const double *x );

  // s1_i += weight * x_i
  // s2_i += weight * x_i * x_i
  static void accumulate_stat( int ndim, float *s1, float *s2, const float *x, const float weight );
  static void accumulate_stat( int ndim, double *s1, double *s2, const double *x, const double weight );

  // s1_i += x_i
  static void accumulate_stat( int ndim, float *s1, const float *x );
  static void accumulate_stat( int ndim, double *s1, const double *x );

  // s1_i += weight * x_i
  static void accumulate_stat( int ndim, float *s1, const float *x, const float weight );
  static void accumulate_stat( int ndim, double *s1, const double *x, const double weight );

  // s2_i += weight * ( x_i - mu_i )^2
  static void accumulate_stat_centered( int ndim, float *s2, const float *x, const float *mu, const float weight );
  static void accumulate_stat_centered( int ndim, double *s2, const double *x, const double *mu, const double weight );

  // y_i = (x_i-m_i)*istd_i
  static void standardize( int ndim, float *y, const float *x, const float *m, const float *istd );
  static void standardize( int ndim, double *y, const double *x, const double *m, const double *istd );


};
