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

#include "stat.h"
#include <iostream>
#include <cstring>
#include <cmath>
#include <cassert>
#include <limits>

/// \bief sample mean
/// 
/// \param samples sample list (input)
/// \param mean mean (output)
/// \param dim dimension of the input samples
///
/// \return none
///
/// \author Jorge Sanchez
/// \date    August 2009

void
sample_mean( const std::vector<T*> &samples, T *mean, int dim )
{
  int N=samples.size();
  assert(N>0);

  double *acc = new double[dim];
  memset( acc, 0, dim*sizeof(double) );

  for( int n=N; n--; )  
  {
    for( int i=dim; i--; )
    {
      acc[i] += (double)samples[n][i];
    }
  }

  double iN = 1.0/double(N);
  for( int i=dim; i--; )
  {
    mean[i] = (T)(acc[i]*iN);
  } 

  delete[] acc;
}


/// \bief sample variance (over dimensions)
/// 
/// \param samples sample list (input)
/// \param mean precomputed sample mean (input)
/// \param variance variance (output)
/// \param dim dimension of the input samples
///
/// \return none
///
/// \author Jorge Sanchez
/// \date    August 2009

void
sample_variance( const std::vector<T*> &samples, const T *mean, T *variance, int dim )
{
  int N=samples.size();
  assert(N>1);

  double *acc = new double[dim];
  memset( acc, 0, dim*sizeof(double) );

  for( int n=N; n--; )  
  {
    for( int i=dim; i--; )
    {
      T dm = samples[n][i]-mean[i];
      acc[i] += (double)(dm*dm);
    }
  }

  double iN1 = 1.0/double(N-1);
  for( int i=dim; i--; )
  {
    variance[i] = (T)(acc[i]*iN1);
  }

  delete[] acc;
}

/// \bief sample variance (over dimensions)
/// 
/// \param samples sample list (input)
/// \param variance variance (output)
/// \param dim dimension of the input samples
///
/// \return none
///
/// \author Jorge Sanchez
/// \date    August 2009

void
sample_variance( const std::vector<T*> &samples, T *variance, int dim )
{
  T *mean = new T[dim];
  sample_mean( samples, mean, dim );
  sample_variance( samples, mean, variance, dim );
  delete[] mean;
  return;
}

/// \bief Sample standardization (-> zero mean, unit variance)
/// 
/// \param samples sample list (input) -> standarized samples (output)
/// \param mean sample mean (output)
/// \param variance sample variance (output)
/// \param dim dimension of the input samples
///
/// \return none
///
/// \author Jorge Sanchez
/// \date    August 2009

void 
standardize( std::vector<T*> &samples, T *mean, T *variance, int dim )
{

  sample_mean( samples, mean, dim );
  sample_variance( samples, mean, variance, dim );  

  //T i_stddev[dim];
  T *i_stddev = new T[dim];
  for( int i=dim; i--; )
  {
    if( variance[i]>0.0 )
      i_stddev[i] = 1.0/sqrt(variance[i]);
    else
      i_stddev[i] = 0.0;
  }

//#pragma omp parallel for
  for( int n=0; n<(int)samples.size(); n++ )
  {
    for( int i=0; i<dim; i++ )   
    {
      samples[n][i] = (samples[n][i]-mean[i])*i_stddev[i];
    }
  }
  delete [] i_stddev;
}

/// \bief Samples de-standardization (zero mean, unit variance -> original mean and var)
/// 
/// \param samples standardized sample list (input) -> original standarized samples (output)
/// \param mean original sample mean (input)
/// \param variance original sample variance (input)
/// \param dim dimension of the input samples
///
/// \return none
///
/// \author Jorge Sanchez
/// \date    August 2009

void
destandardize( std::vector<T*> &samples, const T *mean, const T *variance, int dim )
{
  assert(mean && variance);

  //T stddev[dim];
  T *stddev = new T[dim];
  for( int i=0; i<dim; i++ )
  {
    stddev[i] = sqrt(variance[i]);
  }

//#pragma omp parallel for
  for( int n=0; n<(int)samples.size(); n++ )
  {
    for( int i=0; i<dim; i++ )   
    {
      samples[n][i] = samples[n][i]*stddev[i]+mean[i];
    }
  }
  delete []stddev;
}

