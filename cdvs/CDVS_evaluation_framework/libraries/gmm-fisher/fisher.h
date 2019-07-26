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

/// \class    fisher fisher.h "fisher.h"
///  
/// \brief
///
/// \version  1.0
/// \author   Jorge A. Sanchez
/// \date     02/08/2010

#pragma once

#include "gaussian_mixture.h"

// -------------------------------------------------------------------------
// Fisher Vector

struct fisher_param {
  fisher_param() :
    grad_weights(false), 
    grad_means(true), 
    grad_variances(true),
    alpha(0.5), 
    pnorm(2.0) { }
  bool grad_weights;
  bool grad_means;
  bool grad_variances;
  float alpha;
  float pnorm;
  float gamma_eps;
  void print();
};

// -------------------------------------------------------------------------

class fisher
{

public:
  
  fisher( fisher_param &_param );
  ~fisher( );
  
  void set_model(const gaussian_mixture &_gmm );

  // unweighted
  int compute( std::vector<T*> &x, T *fk );

  // weighted
  int compute( std::vector<T*> &x, std::vector<T> &wgh, T *fk);

  int dim(){ return fkdim; }

private:

  bool equal( T a, T b );

  void alpha_and_lp_normalization( T *fk, int fkdim );

protected:

  fisher_param param;

  int ndim, ngauss, ngrad, fkdim;

  const gaussian_mixture *gmm;

  T *iwgh, *istd;
};

