/*
 * adjacency.cc -- adjacency matrices for Click routers
 * Eddie Kohler
 *
 * Copyright (c) 1999 Massachusetts Institute of Technology.
 *
 * This software is being provided by the copyright holders under the GNU
 * General Public License, either version 2 or, at your discretion, any later
 * version. For more information, see the `COPYRIGHT' file in the source
 * distribution.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "adjacency.hh"
#include "routert.hh"
#include <stdio.h>

AdjacencyMatrix::AdjacencyMatrix(RouterT *r, RouterT *type_model = 0)
  : _x(0)
{
  init(r, type_model);
}

AdjacencyMatrix::~AdjacencyMatrix()
{
  delete[] _x;
}

void
AdjacencyMatrix::init(RouterT *r, RouterT *type_model = 0)
{
  int n = _n = r->nelements();
  delete[] _x;
  _default_match.clear();
  _x = new int[n*n];
  
  for (int i = 0; i < n*n; i++)
    _x[i] = 0;

  Vector<int> new_type;
  for (int i = 0; i < n; i++) {
    int t = r->etype(i);
    if (type_model)
      t = type_model->type_index(r->type_name(t));
    if (t == RouterT::TUNNEL_TYPE) {
      new_type.push_back(t);
      _default_match.push_back(-2);
    } else {
      new_type.push_back(t);
      _x[i + n*i] = t*100;
      _default_match.push_back(-1);
    }
  }
  
  const Vector<Hookup> &hf = r->hookup_from();
  const Vector<Hookup> &ht = r->hookup_to();
  for (int i = 0; i < hf.size(); i++)
    _x[ hf[i].idx + n*ht[i].idx ] += hf[i].port + 10*ht[i].port;
}

void
AdjacencyMatrix::print() const
{
  for (int i = 0; i < _n; i++) {
    for (int j = 0; j < _n; j++)
      fprintf(stderr, "%3d ", _x[_n*i + j]);
    fprintf(stderr, "\n");
  }
  fprintf(stderr, "\n");
}

bool
AdjacencyMatrix::next_subgraph_isomorphism(const AdjacencyMatrix *input,
					   Vector<int> &match) const
{
  int pat_n = _n;
  int *pat_x = _x;
  int input_n = input->_n;
  int *input_x = input->_x;
  
  int match_idx;
  int direction;
  if (match.size() == 0) {
    match = _default_match;
    match_idx = 0;
    direction = 1;
  } else {
    match_idx = pat_n - 1;
    direction = -1;
  }

  //print();
  //fprintf(stderr, "input:\n");
  //input->print();
  
  while (match_idx >= 0 && match_idx < pat_n) {
    int rover = match[match_idx] + 1;
    if (rover < 0) {
      match_idx += direction;
      continue;
    }

    while (rover < input_n) {
      match[match_idx] = rover;
    
      // S_{k,k}(input) =? S_{k,n}(P) * M * (S_{k,n}(P))^T
      // test only the new border
      for (int i = 0; i <= match_idx; i++)
	if (match[i] >= 0 &&
	    pat_x[ i*pat_n + match_idx ] != input_x[ match[i]*input_n + match[match_idx] ])
	  goto failure;
      for (int j = 0; j < match_idx; j++)
	if (match[j] >= 0 &&
	    pat_x[ match_idx*pat_n + j ] != input_x[ match[match_idx]*input_n + match[j] ])
	  goto failure;
      break;
      
     failure: rover++;
    }
    
    if (rover < input_n) {
      match_idx++;
      direction = 1;
    } else {
      match[match_idx] = -1;
      match_idx--;
      direction = -1;
    }
  }

  //for (int i = 0; i < pat_n; i++) fprintf(stderr,"%d ", match[i]);fputs("\n",stderr);
  return (match_idx >= 0 ? true : false);
}


bool
check_subgraph_isomorphism(const RouterT *pat, const RouterT *input,
			   const Vector<int> &match)
{
  const Vector<Hookup> &hf = pat->hookup_from();
  const Vector<Hookup> &ht = pat->hookup_to();
  int nh = hf.size();
  for (int i = 0; i < nh; i++) {
    if (match[hf[i].idx] < 0 || match[ht[i].idx] < 0)
      continue;
    if (!input->has_connection(Hookup(match[hf[i].idx], hf[i].port),
			       Hookup(match[ht[i].idx], ht[i].port)))
      return false;
  }
  return true;
}
