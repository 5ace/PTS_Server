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

#pragma once

#include <vector>
#include "template.h"		// define T as either float or double

struct em_param 
{
  em_param(): 
    max_iter(100), 
    alpha(100.0f),
    llh_diff_thr(0.001f),
    min_gamma(1e-4f),
    variance_floor(1.0e-9f),
    variance_floor_factor(0.01f) {}
  int max_iter;         // max. number of EM iterations
  float alpha;          // Dirichlet prior on mixture weights: alpha_k=alpha (for all k)
  float llh_diff_thr;   // average Log-Likelihood difference threshold
//  float grow_factor;    // growing factor for split
  float min_gamma;      // min. posterior prob. for a sample
  float variance_floor; // hard variance floor
  float variance_floor_factor; // factor for the adaptive flooring
  void print();
};

/// \class    gaussian_mixture gaussian_mixture.h "gaussian_mixture.h"
///  
/// \brief    Gaussian (diagonal covariances) Mixture Model using EM-algorithm
///
/// \author   Jorge Sanchez
/// \date     29/07/2009

class gaussian_mixture
{
public:
  friend class fisher;

  gaussian_mixture( const char* modelfile );

  gaussian_mixture( int n_gauss, int n_dim );

  gaussian_mixture( int n_gauss, int n_dim, const float* fcoef, const float * fmean, const float * fvar );

  gaussian_mixture( em_param &p, int n_gauss, int n_dim );

  ~gaussian_mixture();

  void set( std::vector<T*> &_mean, std::vector<T*> &_var, std::vector<T>  &_coef );
  void random_init( std::vector<T*> &samples, int seed=-1 );

  void em( std::vector<T*> &samples );

  T posterior( T* sample, T *pst ) const;

  T log_likelihood( std::vector<T*> &samples );

  int n_dim() const { return ndim; }
  int n_gauss() const { return ngauss; }

  int load( const char* filename );
  int load( const float * fcoef, const float * fmean, const float * fvar);

  int save( const char* filename );
  void print( bool _coef=true, bool _mean=false, bool _var=false );
  
  inline T* get_mean( int idx ) { return mean[idx]; }
  inline T* get_variance( int idx ) { return var[idx]; }
  inline T get_mixing_coefficients( int idx ) { return coef[idx]; }

  inline em_param get_params() { return param; }

  void set_mean( std::vector<T*> &_mean );
  void set_variance( std::vector<T*> &_var );
  void set_mixing_coefficients( std::vector< T > &_coef );

protected:

  T **mean, **var, *coef;

  void reset_stat_acc();
  T accumulate_statistics( T* sample, bool _s0=true, bool _s1=true, bool _s2=true,
		  T* s0_ext=0, T** s1_ext=0, T** s2_ext=0 ) const;
  T *s0, **s1, **s2;

  em_param param;

  void init();

  void clean();

  void compute_variance_floor( std::vector<T*> &x );

  void precompute_aux_var();

  void update_model();

  T log_p( std::vector<T*> &samples );

  T **i_var, *var_floor, *log_coef;

  double *log_var_sum; // accumulate as double

  int ngauss, ndim, nsamples;

  T ndim_log_2pi;

  T log_p( T* x, T *log_pst=0 ) const;

  T log_gauss( int k, T* x ) const;

};

