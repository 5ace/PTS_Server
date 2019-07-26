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

#include <cmath>
#include <limits>
#include <cassert>
#include <iostream>
#include <cstring>
#include "fisher.h"
#include "simd_math.h"

fisher::fisher( fisher_param &_param )
  : param(_param), gmm(0), iwgh(0), istd(0)
{
  ngrad = (int)param.grad_weights + (int)param.grad_means + (int)param.grad_variances;
  assert( (param.alpha>0.0) && (param.alpha<=1.0) ); 
}

fisher::~fisher()
{
  gmm=0;

  delete[] iwgh;
  iwgh=0;

  delete[] istd;
  istd = 0;
}

bool fisher::equal( T a, T b )
  {
    if( fabs((T)a-(T)b)<std::numeric_limits<T>::epsilon() )
      return true;
    return false;
  }


void
fisher::set_model(const gaussian_mixture &_gmm )
{
  gmm = &_gmm;
  ndim = gmm->n_dim();
  ngauss = gmm->n_gauss();

  fkdim = 0;
  if( param.grad_weights )
  {
    fkdim += ngauss;
  }
  if( param.grad_means )
  {
    fkdim += ngauss*ndim;
  }
  if( param.grad_variances )
  {
    fkdim += ngauss*ndim;
  }

  delete[] iwgh;

  // precompute inverse weights
  iwgh = new T[ngauss];
  for( int j=0; j<ngauss; ++j )
  {
    assert( gmm->coef[j]>0.0 );
    iwgh[j] = 1.0f/gmm->coef[j];
  } 

  // precompute inverse standard deviations
  if( param.grad_means || param.grad_variances )
  {
    delete[] istd;
    istd = new T[ngauss*ndim];

    for( int j=0; j<ngauss; ++j ) 
    {
      T *var_j = gmm->var[j];
      T *istd_j = istd+j*ndim;
      for( int k=ndim; k--; ) 
      {
        assert( var_j[k]>0.0 );
        istd_j[k] = (T)1.0/(T)sqrtf( (float)var_j[k] );
      }
    }    
  }
}


int
fisher::compute( std::vector<T*> &x, T *fk )
{
  std::vector<T> wghx( x.size(), 1.0 );  
  return compute( x, wghx, fk );
}


int 
fisher::compute( std::vector<T*> &x, std::vector<T> &wghx, T *fk )
{  

  assert(gmm);

  assert( x.size()==wghx.size() );

  int nsamples = (int)wghx.size();

  T wghsum=0.0;
//#pragma omp parallel for reduction(+:wghsum)
  for( int i=0; i<nsamples; ++i ) 
  {
    wghsum += wghx[i];
  }

  assert( wghsum>0 );

  T *s0, **s1, **s2;
  int ngauss = gmm->n_gauss();
  int ndim = gmm->n_dim();
  {
    s0 = new T[ngauss];
    memset( s0, 0, ngauss*sizeof(T));
    s1 = new T*[ngauss];
    for( int k=0; k<ngauss; ++k)
    {
      s1[k] = new T[ndim];
      memset( s1[k], 0, ndim*sizeof(T));
    }
    s2 = new T*[ngauss];
    for( int k=0; k<ngauss; ++k)
    {
      s2[k] = new T[ndim];
      memset( s2[k], 0, ndim*sizeof(T));
    }
    for( int i=0; i<nsamples; ++i )
    {
      gmm->accumulate_statistics( x[i], true, param.grad_means||param.grad_variances, param.grad_variances,
				  s0, s1, s2 );
    }
  }

  T *p=fk;

  // Gradient w.r.t. the mixing weights
  // without the constraint \sum_i pi_i=1 => Soft-BoV
  if( param.grad_weights )
  {
    for( int j=0; j<ngauss; ++j )
    {        
      p[j] = s0[j] / ( wghsum*(T)sqrtf((float)iwgh[j]) );
    } 
    p += ngauss;
  }

  // Gradient w.r.t. the means
  if( param.grad_means )
  {
//#pragma omp parallel for
    for( int j=0; j<ngauss; j++ ) 
    {
      T *s1_j = s1[j];
      T *mean_j = gmm->mean[j];
      T *istd_j = istd+j*ndim;
      T *p_j = p+j*ndim;
      T mc = (T)sqrtf((float)iwgh[j])/wghsum;

      for( int k=0; k<ndim; ++k )
      {
        p_j[k] = mc * ( s1_j[k] - mean_j[k] * s0[j] ) * istd_j[k];
      }      
    }
    p += ngauss*ndim;     
  }

  // Gradient w.r.t. the variances
  if( param.grad_variances )
  {
//#pragma omp parallel for
    for( int j=0; j<ngauss; j++ ) 
    {
      T *s1_j = s1[j];
      T *s2_j = s2[j];
      T *mean_j = gmm->mean[j];
      T *var_j = gmm->var[j];
      T *p_j = p+j*ndim;
      T vc = (T)sqrtf(0.5f*(float)iwgh[j])/wghsum;

      for( int k=0; k<ndim; ++k)
      {
        p_j[k] = vc * ( ( s2_j[k] + mean_j[k] * ( mean_j[k]*s0[j] - (T)2.0*s1_j[k] ) ) / var_j[k] - s0[j] );
      }   
    }
  } 

  p = fk;
  if( param.grad_weights )
	  p += ngauss;

  int size = ngauss*ndim;
  if( param.grad_variances )
  {
	  alpha_and_lp_normalization(p, size);
	  alpha_and_lp_normalization(p+size, size);
  }
  else
	  alpha_and_lp_normalization(p, size);
  
  // deallocate s0, s1, s2
  {
      delete[] s0;

      for( int k=0; k<ngauss; ++k)
      {
    	  delete[] s1[k];
    	  delete[] s2[k];
      }
      delete[] s1;
      delete[] s2;
    }

  return 0;
}



void
fisher::alpha_and_lp_normalization( T *fk , int fkdim)
{
  // alpha normalization
  if( !equal(param.alpha,1.0f) )
  {
    if( equal(param.alpha,0.5f) )
    {
//#pragma omp parallel for
      for( int i=0; i<fkdim; i++ )
      {
        if( fk[i]<0.0 )
          fk[i] = -std::sqrt(-fk[i]);
        else
          fk[i] = std::sqrt(fk[i]);
      }
    }
    else
    {
//#pragma omp parallel for
      for( int i=0; i<fkdim; i++ )
      {
        if( fk[i]<0.0 )
          fk[i] = -std::pow(-fk[i],(T)param.alpha);
        else
          fk[i] = std::pow(fk[i],(T)param.alpha);
      }
    }
  }

  // Lp normalization
  if( !equal(param.pnorm,(float)0.0) )
  {
    T pnorm=0;
    if( equal(param.pnorm,(float)1.0) )
    {
//#pragma omp parallel for reduction(+:pnorm)
      for( int i=0; i<fkdim; ++i )
      {
        pnorm += std::fabs(fk[i]);
      }
    }
    else if( equal(param.pnorm,2.0) )
    {
      pnorm = sqrt( simd_math::dot_product( fkdim, fk, fk ) );
    }
    else
    {
//#pragma omp parallel for reduction(+:pnorm)
      for( int i=0; i<fkdim; ++i )
      {
        pnorm += std::pow( std::fabs(fk[i]), (T)param.pnorm );
      }
      pnorm = std::pow( pnorm, 1.0f/(T)param.pnorm );
    }

    if( pnorm>0.0 )
    {
      simd_math::scale( fkdim, fk, (T)(1.0/pnorm) );
    }
  }
}

/// \brief print
/// 
/// \param none
///
/// \return none
///
/// \author Jorge Sanchez
/// \date    August 2009

void
fisher_param::print()
{
  std::cout << "  grad_weights = " << grad_weights << std::endl;
  std::cout << "  grad_means = " << grad_means << std::endl;
  std::cout << "  grad_variances = " << grad_variances << std::endl;
  std::cout << "  alpha = " << alpha << std::endl;
  std::cout << "  pnorm = " << pnorm << std::endl;
}

