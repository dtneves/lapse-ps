#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/matrix_proxy.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/generator_iterator.hpp>
#include <boost/numeric/ublas/matrix_sparse.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/timer/timer.hpp>

typedef boost::numeric::ublas::coordinate_matrix<double, boost::numeric::ublas::row_major> SparseMatrix;

using namespace std;


/** Calculate l2 regularization for the local part of W and one part of H */
double l2(std::vector<double> &w, std::vector<double> &h, uint h_start_index, uint h_block_num_scalars) {
  double regularization = 0.;
  // w: worker adds reg for local block of w
  for (uint z=0; z!=w.size(); z++) {
    regularization += w[z] * w[z];
  }

  // h: worker i adds reg for i'th block of h
  for (uint z=h_start_index; z!=h_start_index+h_block_num_scalars; z++) {
    regularization += h[z] * h[z];
  }

  return regularization;
}


/** Calculate non-zero squared loss with L2 regularization for local part of data */
double loss_Nzsl_L2(mf::DataPart &data, std::vector<double> &w, std::vector<double> &h, const double lambda, const uint mf_rank, int pos) {
  double loss = 0;
  auto vals_per_block = data.num_cols_per_block() * mf_rank;

  // iterate over local part of the data
  for (unsigned long z=0; z!=data.num_nnz(); ++z) {
    double ip = 0;
    auto x = data.data()[z].x;
    auto i_pos = (data.data()[z].i - data.start_row()) * mf_rank;
    auto j_pos = (data.data()[z].j) * mf_rank;

    for (uint r=0; r!=mf_rank; ++r) {
      ip += w[i_pos+r] * h[j_pos+r];
    }

    double diff = x - ip;

    /* LLOG("Loss for " << x << ": " << diff*diff << ", wh: " << ip << ", h[0]: " << h[j_pos+0] << ", w[0]: " << w[i_pos+0]); */
    loss += diff * diff;
  }

  // add L2 regularization
  if (lambda > 0.) {
    double l2l = l2(w,h,pos*vals_per_block,vals_per_block);
    loss += lambda * l2l;
    /* LLOG("L2 loss: " << lambda * l2l); */
  }

  return loss;
}
