/**
 * \file mat_are_solver.hpp
 * 
 * This library provides function templates to solve Algebraic Riccati Equations (AREs) of 
 * different kinds.
 * 
 * \author Sven Mikael Persson <mikael.s.persson@gmail.com>
 * \date July 2012
 */

/*
 *    Copyright 2012 Sven Mikael Persson
 *
 *    THIS SOFTWARE IS DISTRIBUTED UNDER THE TERMS OF THE GNU GENERAL PUBLIC LICENSE v3 (GPLv3).
 *
 *    This file is part of ReaK.
 *
 *    ReaK is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    ReaK is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with ReaK (as LICENSE in the root folder).  
 *    If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef REAK_MAT_ARE_SOLVER_HPP
#define REAK_MAT_ARE_SOLVER_HPP

#include "mat_alg.hpp"
#include "mat_num_exceptions.hpp"

#include "mat_householder.hpp"
#include "mat_hess_decomp.hpp"
#include "mat_schur_decomp.hpp"

namespace ReaK {
  


namespace detail {


template <typename Matrix1, typename Matrix2>
typename mat_traits<Matrix1>::value_type
  get_norm_of_eigens_impl(const Matrix1& A, const Matrix2& B) {
  typedef typename mat_traits<Matrix1>::value_type ValueType;
  typedef typename mat_traits<Matrix1>::size_type SizeType;
  using std::fabs;
  using std::sqrt;
  SizeType N = A.get_row_count();
  
  if(N == 1) {
    ValueType l = fabs(A(0,0));
    ValueType tmp = fabs(B(0,0));
    if(tmp < std::numeric_limits<ValueType>::epsilon() * l)
      return std::numeric_limits<ValueType>::infinity();
    else
      return l / tmp;
  } else {
    ValueType l = fabs(A(0,0) * A(1,1) - A(1,0) * A(0,1));
    ValueType tmp = fabs(B(0,0) * B(1,1) - B(1,0) * B(0,1));
    if(tmp < std::numeric_limits<ValueType>::epsilon() * l)
      return std::numeric_limits<ValueType>::infinity();
    else
      return sqrt(l / tmp);
  };
};

template <typename Matrix1, typename Matrix2>
typename mat_traits<Matrix1>::value_type
  get_real_val_of_eigens_impl(const Matrix1& A, const Matrix2& B) {
  typedef typename mat_traits<Matrix1>::value_type ValueType;
  typedef typename mat_traits<Matrix1>::size_type SizeType;
  using std::fabs;
  using std::sqrt;
  SizeType N = A.get_row_count();
  
  if(N == 1) {
    ValueType l = A(0,0);
    ValueType tmp = B(0,0);
    if(fabs(tmp) < std::numeric_limits<ValueType>::epsilon() * fabs(l))
      return (l < 0 ? -std::numeric_limits<ValueType>::infinity() 
                    :  std::numeric_limits<ValueType>::infinity());
    else
      return l / tmp;
  } else {
    ValueType l = fabs(A(0,0) * A(1,1) - A(1,0) * A(0,1));
    ValueType tmp = fabs(B(0,0) * B(1,1) - B(1,0) * B(0,1));
    if(tmp < std::numeric_limits<ValueType>::epsilon() * l)
      return std::numeric_limits<ValueType>::infinity();
    else {
      ValueType mu = A(0,0) / B(0,0);
      ValueType a_22 = A(1,1) - mu * B(1,1);
      ValueType p = ValueType(0.5) * (a_22 / B(1,1) - (B(0,1) * A(1,0)) / (B(0,0) * B(1,1)));
      return mu + p;
    };
  };
};


struct lesser_norm_eigen_first {
  
  template <typename Matrix1, typename Matrix2, 
            typename Matrix3, typename Matrix4>
  int operator()(const Matrix1& A1, const Matrix2& B1, 
                 const Matrix1& A2, const Matrix2& B2) {
    typedef typename mat_traits<Matrix1>::value_type ValueType;
    ValueType l1 = get_norm_of_eigens_impl(A1,B1);
    ValueType l2 = get_norm_of_eigens_impl(A2,B2);
    if( l1 < l2 )
      return 1;
    else if( l1 == l2 )
      return 0;
    else
      return -1;
  };
  
};

struct greater_norm_eigen_first {
  
  template <typename Matrix1, typename Matrix2, 
            typename Matrix3, typename Matrix4>
  int operator()(const Matrix1& A1, const Matrix2& B1, 
                 const Matrix1& A2, const Matrix2& B2) {
    typedef typename mat_traits<Matrix1>::value_type ValueType;
    ValueType l1 = get_norm_of_eigens_impl(A1,B1);
    ValueType l2 = get_norm_of_eigens_impl(A2,B2);
    if( l1 > l2 )
      return 1;
    else if( l1 == l2 )
      return 0;
    else
      return -1;
  };
  
};


struct lesser_real_val_eigen_first {
  
  template <typename Matrix1, typename Matrix2, 
            typename Matrix3, typename Matrix4>
  int operator()(const Matrix1& A1, const Matrix2& B1, 
                 const Matrix1& A2, const Matrix2& B2) {
    typedef typename mat_traits<Matrix1>::value_type ValueType;
    ValueType l1 = get_real_val_of_eigens_impl(A1,B1);
    ValueType l2 = get_real_val_of_eigens_impl(A2,B2);
    if( l1 < l2 )
      return 1;
    else if( l1 == l2 )
      return 0;
    else
      return -1;
  };
  
};

struct greater_real_val_eigen_first {
  
  template <typename Matrix1, typename Matrix2, 
            typename Matrix3, typename Matrix4>
  int operator()(const Matrix1& A1, const Matrix2& B1, 
                 const Matrix1& A2, const Matrix2& B2) {
    typedef typename mat_traits<Matrix1>::value_type ValueType;
    ValueType l1 = get_real_val_of_eigens_impl(A1,B1);
    ValueType l2 = get_real_val_of_eigens_impl(A2,B2);
    if( l1 > l2 )
      return 1;
    else if( l1 == l2 )
      return 0;
    else
      return -1;
  };
  
};


struct stable_eigen_first {
  
  template <typename Matrix1, typename Matrix2, 
            typename Matrix3, typename Matrix4>
  int operator()(const Matrix1& A1, const Matrix2& B1, 
                 const Matrix1& A2, const Matrix2& B2) {
    typedef typename mat_traits<Matrix1>::value_type ValueType;
    ValueType l1 = get_norm_of_eigens_impl(A1,B1);
    ValueType l2 = get_norm_of_eigens_impl(A2,B2);
    if( l1 < ValueType(1.0) && l2 >= ValueType(1.0) )
      return 1;
    else if( l2 < ValueType(1.0) && l1 >= ValueType(1.0) )
      return -1;
    else
      return 0;
  };
  
};

struct unstable_eigen_first {
  
  template <typename Matrix1, typename Matrix2, 
            typename Matrix3, typename Matrix4>
  int operator()(const Matrix1& A1, const Matrix2& B1, 
                 const Matrix1& A2, const Matrix2& B2) {
    typedef typename mat_traits<Matrix1>::value_type ValueType;
    ValueType l1 = get_norm_of_eigens_impl(A1,B1);
    ValueType l2 = get_norm_of_eigens_impl(A2,B2);
    if( l1 < ValueType(1.0) && l2 >= ValueType(1.0) )
      return -1;
    else if( l2 < ValueType(1.0) && l1 >= ValueType(1.0) )
      return 1;
    else
      return 0;
  };
  
};



/*
 * This function performs the swapping of two schur blocks in the real schur pencil (A,B).
 * This is the Case I in Van Dooren (1981), where the blocks to be swapped both have dimension 1.
 */
template <typename Matrix1, typename Matrix2, 
          typename Matrix3, typename Matrix4>
void swap_schur_blocks11_impl(Matrix1& A, Matrix2& B, Matrix3* Q, Matrix4* Z,
                              typename mat_traits<Matrix1>::size_type p,  // upper-left diagonal element.
                              typename mat_traits<Matrix1>::size_type row_offset) {
  typedef typename mat_traits<Matrix1>::value_type ValueType;
  typedef typename mat_traits<Matrix1>::size_type SizeType;
  
  SizeType N = A.get_row_count();
  
  givens_rot_matrix<ValueType> G;
  
  SizeType q = row_offset + p;
  
  bool reduce_A = fabs(B(q+1,p+1)) < fabs(A(q+1,p+1));
  
  ValueType x1 = A(row_offset + p + 1, p+1) * B(row_offset + p, p) 
               - B(row_offset + p + 1, p+1) * A(row_offset + p, p);
  ValueType x2 = A(row_offset + p + 1, p+1) * B(row_offset + p, p+1) 
               - B(row_offset + p + 1, p+1) * A(row_offset + p, p+1);
  
  G.set(-x2, x1);
  G = transpose(G);
  
  mat_sub_block<Matrix2> subB1(B, q + 2, 2, 0, p);
  givens_rot_prod(subB1,G); // B * G^T
  
  mat_sub_block<Matrix1> subA1(A, q + 2, 2, 0, p);
  givens_rot_prod(subA1,G); // A * G^T
  
  if(Z) {
    mat_sub_block<Matrix4> subZ(*Z, Z->get_row_count(), 2, 0, p);
    givens_rot_prod(subZ,G); // Q_prev * G^T
  };
  
  if(reduce_A)
    G.set(A(q,p),A(q+1,p));
  else
    G.set(B(q,p),B(q+1,p));
  
  mat_sub_block<Matrix1> subA2(A, 2, A.get_col_count()-p, q, p);
  givens_rot_prod(G,subA2); // G * A
  
  mat_sub_block<Matrix2> subB2(B, 2, B.get_col_count()-p, q, p);
  givens_rot_prod(G,subB2); // G * B
  
  if(Q) {
    mat_sub_block<Matrix3> subQ(*Q, Q->get_row_count(), 2, 0, q);
    givens_rot_prod(subQ,transpose(G)); // Q_prev * G^T
  };
  
};



/*
 * This function performs the swapping of two schur blocks in the real schur pencil (A,B).
 * This is the Case II in Van Dooren (1981), where the two blocks to be swapped both have 
 * dimension 2 and 1, in that order.
 */
template <typename Matrix1, typename Matrix2, 
          typename Matrix3, typename Matrix4>
void swap_schur_blocks21_impl(Matrix1& A, Matrix2& B, Matrix3* Q, Matrix4* Z,
                              typename mat_traits<Matrix1>::size_type p,  // upper-left diagonal element.
                              typename mat_traits<Matrix1>::size_type row_offset) {
  typedef typename mat_traits<Matrix1>::value_type ValueType;
  typedef typename mat_traits<Matrix1>::size_type SizeType;
  
  SizeType N = A.get_row_count();
  
  givens_rot_matrix<ValueType> G;
  
  SizeType q = row_offset + p;
  
  bool reduce_A = fabs(B(q+2,p+2)) < fabs(A(q+2,p+2));
  
  ValueType a33 = A(row_offset + p + 2, p + 2);
  ValueType b33 = B(row_offset + p + 2, p + 2);
  
  {
  ValueType x11 =   a33 * B(row_offset + p, p) 
                  - b33 * A(row_offset + p, p);
  ValueType x21 = - b33 * A(row_offset + p + 1, p);

  G.set(x11,x21);
  
  mat_sub_block<Matrix1> subA(A, 2, A.get_col_count()-p, q, p);
  givens_rot_prod(G,subA); // G * A
  
  mat_sub_block<Matrix2> subB(B, 2, B.get_col_count()-p, q, p);
  givens_rot_prod(G,subB); // G * B
  
  if(Q) {
    mat_sub_block<Matrix3> subQ(*Q, Q->get_row_count(), 2, 0, q);
    givens_rot_prod(subQ,transpose(G)); // Q_prev * G^T
  };
  };
  
  
  
  // Annihilate x1 in R' * H
  {
  ValueType x1 = a33 * B(row_offset + p + 1, p + 1) 
               - b33 * A(row_offset + p + 1, p + 1);
  ValueType x2 = a33 * B(row_offset + p + 1, p + 2) 
               - b33 * A(row_offset + p + 1, p + 2);
  
  G.set(-x2, x1);
  G = transpose(G);
  
  mat_sub_block<Matrix2> subB(B, q + 3, 2, 0, p+1);
  givens_rot_prod(subB,G); // B * G^T
  
  mat_sub_block<Matrix1> subA(A, q + 3, 2, 0, p+1);
  givens_rot_prod(subA,G); // A * G^T
  
  if(Z) {
    mat_sub_block<Matrix4> subZ(*Z, Z->get_row_count(), 2, 0, p+1);
    givens_rot_prod(subZ,G); // Q_prev * G^T
  };
  };
  
  
  {
  G.set(B(q+1,p+1),B(q+2,p+1));
  
  mat_sub_block<Matrix1> subA(A, 2, A.get_col_count()-p-1, q+1, p+1);
  givens_rot_prod(G,subA); // G * A
  
  mat_sub_block<Matrix2> subB(B, 2, B.get_col_count()-p-1, q+1, p+1);
  givens_rot_prod(G,subB); // G * B
  
  if(Q) {
    mat_sub_block<Matrix3> subQ(*Q, Q->get_row_count(), 2, 0, q+1);
    givens_rot_prod(subQ,transpose(G)); // Q_prev * G^T
  };
  };
  
  
  {
  // Annihilate x_2 (is x1 here) in R' * H
  ValueType x1 = a33 * B(row_offset + p, p) 
               - b33 * A(row_offset + p, p);
  ValueType x2 = a33 * B(row_offset + p, p + 1) 
               - b33 * A(row_offset + p, p + 1);
  
  G.set(-x2, x1);
  G = transpose(G);
  
  mat_sub_block<Matrix2> subB(B, q + 2, 2, 0, p);
  givens_rot_prod(subB,G); // B * G^T
  
  mat_sub_block<Matrix1> subA(A, q + 2, 2, 0, p);
  givens_rot_prod(subA,G); // A * G^T
  
  if(Z) {
    mat_sub_block<Matrix4> subZ(*Z, Z->get_row_count(), 2, 0, p);
    givens_rot_prod(subZ,G); // Q_prev * G^T
  };
  };
  
  
  {
  if(reduce_A)
    G.set(A(q,p),A(q+1,p));
  else
    G.set(B(q,p),B(q+1,p));
  
  mat_sub_block<Matrix1> subA(A, 2, A.get_col_count()-p, q, p);
  givens_rot_prod(G,subA); // G * A
  
  mat_sub_block<Matrix2> subB(B, 2, B.get_col_count()-p, q, p);
  givens_rot_prod(G,subB); // G * B
  
  if(Q) {
    mat_sub_block<Matrix3> subQ(*Q, Q->get_row_count(), 2, 0, q);
    givens_rot_prod(subQ,transpose(G)); // Q_prev * G^T
  };
  };
  
};



/*
 * This function performs the swapping of two schur blocks in the real schur pencil (A,B).
 * This is the Case II in Van Dooren (1981), where the two blocks to be swapped both have 
 * dimension 1 and 2, in that order.
 */
template <typename Matrix1, typename Matrix2, 
          typename Matrix3, typename Matrix4>
void swap_schur_blocks12_impl(Matrix1& A, Matrix2& B, Matrix3* Q, Matrix4* Z,
                              typename mat_traits<Matrix1>::size_type p,  // upper-left diagonal element.
                              typename mat_traits<Matrix1>::size_type row_offset) {
  typedef typename mat_traits<Matrix1>::value_type ValueType;
  typedef typename mat_traits<Matrix1>::size_type SizeType;
  
  SizeType N = A.get_row_count();
  
  givens_rot_matrix<ValueType> G;
  
  SizeType q = row_offset + p;
  
  bool reduce_A = fabs(B(q+2,p+2)) < fabs(A(q+2,p+2));
  
  ValueType a11 = A(q, p);
  ValueType b11 = B(q, p);
  
  {
  ValueType x33 =   a11 * B(q + 2, p + 2) 
                  - b11 * A(q + 2, p + 2);
  ValueType x32 = - b11 * A(q + 2, p + 1);

  G.set(-x33, x32);
  G = transpose(G);
  
  mat_sub_block<Matrix2> subB(B, q + 3, 2, 0, p+1);
  givens_rot_prod(subB,G); // B * G^T
  
  mat_sub_block<Matrix1> subA(A, q + 3, 2, 0, p+1);
  givens_rot_prod(subA,G); // A * G^T
  
  if(Z) {
    mat_sub_block<Matrix4> subZ(*Z, Z->get_row_count(), 2, 0, p+1);
    givens_rot_prod(subZ,G); // Q_prev * G^T
  };
  };
  
  
  {
  // Annihilate x1 in R' * H  [ x1; x2 ]
  ValueType x1 = a11 * B(q, p + 1) 
               - b11 * A(q, p + 1);
  ValueType x2 = a11 * B(q + 1, p + 1) 
               - b11 * A(q + 1, p + 1);
  
  G.set(x1,x2);
  
  mat_sub_block<Matrix1> subA(A, 2, A.get_col_count()-p, q, p);
  givens_rot_prod(G,subA); // G * A
  
  mat_sub_block<Matrix2> subB(B, 2, B.get_col_count()-p, q, p);
  givens_rot_prod(G,subB); // G * B
  
  if(Q) {
    mat_sub_block<Matrix3> subQ(*Q, Q->get_row_count(), 2, 0, q);
    givens_rot_prod(subQ,transpose(G)); // Q_prev * G^T
  };
  };
  
  
  {
  G.set(-B(q+1,p+1),B(q+1,p));
  G = transpose(G);
  
  mat_sub_block<Matrix2> subB(B, q + 2, 2, 0, p);
  givens_rot_prod(subB,G); // B * G^T
  
  mat_sub_block<Matrix1> subA(A, q + 2, 2, 0, p);
  givens_rot_prod(subA,G); // A * G^T
  
  if(Z) {
    mat_sub_block<Matrix4> subZ(*Z, Z->get_row_count(), 2, 0, p);
    givens_rot_prod(subZ,G); // Q_prev * G^T
  };
  };
  
  
  
  {
  // Annihilate x2 in R' * H  [ x1; x2 ]
  ValueType x1 = a11 * B(q + 1, p + 2) 
               - b11 * A(q + 1, p + 2);
  ValueType x2 = a11 * B(q + 2, p + 2) 
               - b11 * A(q + 2, p + 2);
  
  G.set(x1,x2);
  
  mat_sub_block<Matrix1> subA(A, 2, A.get_col_count()-p-1, q+1, p+1);
  givens_rot_prod(G,subA); // G * A
  
  mat_sub_block<Matrix2> subB(B, 2, B.get_col_count()-p-1, q+1, p+1);
  givens_rot_prod(G,subB); // G * B
  
  if(Q) {
    mat_sub_block<Matrix3> subQ(*Q, Q->get_row_count(), 2, 0, q+1);
    givens_rot_prod(subQ,transpose(G)); // Q_prev * G^T
  };
  };
  
  
  {
  if(reduce_A)
    G.set(-A(q+2,p+2),A(q+2,p+1));
  else
    G.set(-B(q+2,p+2),B(q+2,p+1));
  G = transpose(G);
  
  mat_sub_block<Matrix2> subB(B, q + 3, 2, 0, p+1);
  givens_rot_prod(subB,G); // B * G^T
  
  mat_sub_block<Matrix1> subA(A, q + 3, 2, 0, p+1);
  givens_rot_prod(subA,G); // A * G^T
  
  if(Z) {
    mat_sub_block<Matrix4> subZ(*Z, Z->get_row_count(), 2, 0, p+1);
    givens_rot_prod(subZ,G); // Q_prev * G^T
  };
  };
  
};




/*
 * This function performs the swapping of two schur blocks in the real schur pencil (A,B).
 * This is the Case II in Van Dooren (1981), where the two blocks to be swapped both have 
 * dimension 2 and 2, in that order.
 */
template <typename Matrix1, typename Matrix2, 
          typename Matrix3, typename Matrix4>
void swap_schur_blocks22_impl(Matrix1& A, Matrix2& B, Matrix3* Q, Matrix4* Z,
                              typename mat_traits<Matrix1>::size_type p,  // upper-left diagonal element.
                              typename mat_traits<Matrix1>::size_type row_offset,
                              typename mat_traits<Matrix1>::value_type NumTol) {
  typedef typename mat_traits<Matrix1>::value_type ValueType;
  typedef typename mat_traits<Matrix1>::size_type SizeType;
  using std::fabs;
  
  SizeType N = A.get_row_count();
  
  givens_rot_matrix<ValueType> G;
  
  SizeType q = row_offset + p;
  
  // before anything else, record the elements that determine lamba-1
  ValueType b_mm = B(q,p);
  ValueType b_nn = B(q+1,p+1);
  ValueType b_mn = B(q,p+1);
  ValueType a_mm = A(q,p);
  ValueType a_nm = A(q+1,p);
  ValueType a_mn = A(q,p+1);
  ValueType a_nn = A(q+1,p+1);
  
  
  // first, do a random (e.g. 45 degree) Givens rotation on the middle of the 4x4 pencil.
  //  the point of this is to force a non-zero element on the middle sub-diagonal.
  
  // Q23
  {
  G.set(1.0, 1.0);
  
  mat_sub_block<Matrix1> subA(A, 2, A.get_col_count()-p, q+1, p);
  givens_rot_prod(G,subA); // G * A
  
  mat_sub_block<Matrix2> subB(B, 2, B.get_col_count()-p-1, q+1, p+1);
  givens_rot_prod(G,subB); // G * B
  
  if(Q) {
    mat_sub_block<Matrix3> subQ(*Q, Q->get_row_count(), 2, 0, q+1);
    givens_rot_prod(subQ,transpose(G)); // Q_prev * G^T
  };
  };
  
  //  then, a typical QZ procedure is used to retrieve a hess-tri form.
  
  // Z23
  {
  G.set(-B(q+2,p+2),B(q+2,p+1));
  G = transpose(G);
  
  mat_sub_block<Matrix2> subB(B, q + 3, 2, 0, p+1);
  givens_rot_prod(subB,G); // B * G^T
  
  mat_sub_block<Matrix1> subA(A, q + 4, 2, 0, p+1);
  givens_rot_prod(subA,G); // A * G^T
  
  if(Z) {
    mat_sub_block<Matrix4> subZ(*Z, Z->get_row_count(), 2, 0, p+1);
    givens_rot_prod(subZ,G); // Q_prev * G^T
  };
  };
  
  // Q23
  {
  G.set(A(q+1,p),A(q+2,p));
  
  mat_sub_block<Matrix1> subA(A, 2, A.get_col_count()-p, q+1, p);
  givens_rot_prod(G,subA); // G * A
  
  mat_sub_block<Matrix2> subB(B, 2, B.get_col_count()-p-1, q+1, p+1);
  givens_rot_prod(G,subB); // G * B
  
  if(Q) {
    mat_sub_block<Matrix3> subQ(*Q, Q->get_row_count(), 2, 0, q+1);
    givens_rot_prod(subQ,transpose(G)); // Q_prev * G^T
  };
  };
  
  // Z23
  {
  G.set(-B(q+2,p+2),B(q+2,p+1));
  G = transpose(G);
  
  mat_sub_block<Matrix2> subB(B, q + 3, 2, 0, p+1);
  givens_rot_prod(subB,G); // B * G^T
  
  mat_sub_block<Matrix1> subA(A, q + 4, 2, 0, p+1);
  givens_rot_prod(subA,G); // A * G^T
  
  if(Z) {
    mat_sub_block<Matrix4> subZ(*Z, Z->get_row_count(), 2, 0, p+1);
    givens_rot_prod(subZ,G); // Q_prev * G^T
  };
  };
  
  // Q34
  {
  G.set(A(q+2,p+1),A(q+3,p+1));
  
  mat_sub_block<Matrix1> subA(A, 2, A.get_col_count()-p-1, q+2, p+1);
  givens_rot_prod(G,subA); // G * A
  
  mat_sub_block<Matrix2> subB(B, 2, B.get_col_count()-p-2, q+2, p+2);
  givens_rot_prod(G,subB); // G * B
  
  if(Q) {
    mat_sub_block<Matrix3> subQ(*Q, Q->get_row_count(), 2, 0, q+2);
    givens_rot_prod(subQ,transpose(G)); // Q_prev * G^T
  };
  };
  
  // Z34
  {
  G.set(-B(q+3,p+3),B(q+3,p+2));
  G = transpose(G);
  
  mat_sub_block<Matrix2> subB(B, q + 4, 2, 0, p+2);
  givens_rot_prod(subB,G); // B * G^T
  
  mat_sub_block<Matrix1> subA(A, q + 4, 2, 0, p+2);
  givens_rot_prod(subA,G); // A * G^T
  
  if(Z) {
    mat_sub_block<Matrix4> subZ(*Z, Z->get_row_count(), 2, 0, p+2);
    givens_rot_prod(subZ,G); // Q_prev * G^T
  };
  };
  
  
  // second, apply a double-shift QZ-step using the original lamba-1 as shifts.
  ValueType a20 = (A(q+1,p+1) / B(q+1,p+1) - A(q,p) / B(q,p)) 
                - (A(q+1,p) / B(q,p)) * (B(q,p+1) / B(q+1,p+1))
                - (a_mm / b_mm - A(q,p) / B(q,p))
                - (a_nn / b_nn - A(q,p) / B(q,p))
                + (a_nm / b_mm) * (b_mn / b_nn);
  ValueType a30 = A(q+2,p+1) / B(q+1,p+1);
  
  // Q23
  {
  G.set(a20,a30);
  
  mat_sub_block<Matrix1> subA(A, 2, A.get_col_count()-p, q+1, p);
  givens_rot_prod(G,subA); // G * A
  
  mat_sub_block<Matrix2> subB(B, 2, B.get_col_count()-p-1, q+1, p+1);
  givens_rot_prod(G,subB); // G * B
  
  if(Q) {
    mat_sub_block<Matrix3> subQ(*Q, Q->get_row_count(), 2, 0, q+1);
    givens_rot_prod(subQ,transpose(G)); // Q_prev * G^T
  };
  };
  
  // Z23
  {
  G.set(-B(q+2,p+2),B(q+2,p+1));
  G = transpose(G);
  
  mat_sub_block<Matrix2> subB(B, q + 3, 2, 0, p+1);
  givens_rot_prod(subB,G); // B * G^T
  
  mat_sub_block<Matrix1> subA(A, q + 4, 2, 0, p+1);
  givens_rot_prod(subA,G); // A * G^T
  
  if(Z) {
    mat_sub_block<Matrix4> subZ(*Z, Z->get_row_count(), 2, 0, p+1);
    givens_rot_prod(subZ,G); // Q_prev * G^T
  };
  };
  
  
  
  ValueType a10 = ((a_mm / b_mm - A(q,p) / B(q,p)) * (a_nn / b_nn - A(q,p) / B(q,p))
                 - (a_mn / b_nn) * (a_nm / b_mm)
                 + (a_nm / b_mm) * (b_mn / b_nn) * (A(q,p) / B(q,p))) * (B(q,p) / A(q+1,p))
                 + A(q,p+1) / B(q+1,p+1) - (A(q,p) / B(q,p)) * (B(q,p+1) / B(q+1,p+1));
  a20 = (A(q+1,p+1) / B(q+1,p+1) - A(q,p) / B(q,p)) 
      - (A(q+1,p) / B(q,p)) * (B(q,p+1) / B(q+1,p+1))
      - (a_mm / b_mm - A(q,p) / B(q,p))
      - (a_nn / b_nn - A(q,p) / B(q,p))
      + (a_nm / b_mm) * (b_mn / b_nn);
  
  // Q12
  {
  G.set(a10,a20);
  
  mat_sub_block<Matrix1> subA(A, 2, A.get_col_count()-p, q, p);
  givens_rot_prod(G,subA); // G * A
  
  mat_sub_block<Matrix2> subB(B, 2, B.get_col_count()-p, q, p);
  givens_rot_prod(G,subB); // G * B
  
  if(Q) {
    mat_sub_block<Matrix3> subQ(*Q, Q->get_row_count(), 2, 0, q);
    givens_rot_prod(subQ,transpose(G)); // Q_prev * G^T
  };
  };
  
  // Z12
  {
  G.set(-B(q+1,p+1),B(q+1,p));
  G = transpose(G);
  
  mat_sub_block<Matrix2> subB(B, q + 2, 2, 0, p);
  givens_rot_prod(subB,G); // B * G^T
  
  mat_sub_block<Matrix1> subA(A, q + 4, 2, 0, p);
  givens_rot_prod(subA,G); // A * G^T
  
  if(Z) {
    mat_sub_block<Matrix4> subZ(*Z, Z->get_row_count(), 2, 0, p);
    givens_rot_prod(subZ,G); // Q_prev * G^T
  };
  };
  
  
  
  // third, reduce back to hess-tri form.
  
  // Q34
  {
  G.set(A(q+2,p),A(q+3,p));
  
  mat_sub_block<Matrix1> subA(A, 2, A.get_col_count()-p, q+2, p);
  givens_rot_prod(G,subA); // G * A
  
  mat_sub_block<Matrix2> subB(B, 2, B.get_col_count()-p-2, q+2, p+2);
  givens_rot_prod(G,subB); // G * B
  
  if(Q) {
    mat_sub_block<Matrix3> subQ(*Q, Q->get_row_count(), 2, 0, q+2);
    givens_rot_prod(subQ,transpose(G)); // Q_prev * G^T
  };
  };
  
  // Z34
  {
  G.set(-B(q+3,p+3),B(q+3,p+2));
  G = transpose(G);
  
  mat_sub_block<Matrix2> subB(B, q + 4, 2, 0, p+2);
  givens_rot_prod(subB,G); // B * G^T
  
  mat_sub_block<Matrix1> subA(A, q + 4, 2, 0, p+2);
  givens_rot_prod(subA,G); // A * G^T
  
  if(Z) {
    mat_sub_block<Matrix4> subZ(*Z, Z->get_row_count(), 2, 0, p+2);
    givens_rot_prod(subZ,G); // Q_prev * G^T
  };
  };
  
  // Q23
  {
  G.set(A(q+1,p),A(q+2,p));
  
  mat_sub_block<Matrix1> subA(A, 2, A.get_col_count()-p, q+1, p);
  givens_rot_prod(G,subA); // G * A
  
  mat_sub_block<Matrix2> subB(B, 2, B.get_col_count()-p-1, q+1, p+1);
  givens_rot_prod(G,subB); // G * B
  
  if(Q) {
    mat_sub_block<Matrix3> subQ(*Q, Q->get_row_count(), 2, 0, q+1);
    givens_rot_prod(subQ,transpose(G)); // Q_prev * G^T
  };
  };
  
  // Z23
  {
  G.set(-B(q+2,p+2),B(q+2,p+1));
  G = transpose(G);
  
  mat_sub_block<Matrix2> subB(B, q + 3, 2, 0, p+1);
  givens_rot_prod(subB,G); // B * G^T
  
  mat_sub_block<Matrix1> subA(A, q + 4, 2, 0, p+1);
  givens_rot_prod(subA,G); // A * G^T
  
  if(Z) {
    mat_sub_block<Matrix4> subZ(*Z, Z->get_row_count(), 2, 0, p+1);
    givens_rot_prod(subZ,G); // Q_prev * G^T
  };
  };
  
  // Q34
  {
  G.set(A(q+2,p+1),A(q+3,p+1));
  
  mat_sub_block<Matrix1> subA(A, 2, A.get_col_count()-p-1, q+2, p+1);
  givens_rot_prod(G,subA); // G * A
  
  mat_sub_block<Matrix2> subB(B, 2, B.get_col_count()-p-2, q+2, p+2);
  givens_rot_prod(G,subB); // G * B
  
  if(Q) {
    mat_sub_block<Matrix3> subQ(*Q, Q->get_row_count(), 2, 0, q+2);
    givens_rot_prod(subQ,transpose(G)); // Q_prev * G^T
  };
  };
  
  // Z34
  {
  G.set(-B(q+3,p+3),B(q+3,p+2));
  G = transpose(G);
  
  mat_sub_block<Matrix2> subB(B, q + 4, 2, 0, p+2);
  givens_rot_prod(subB,G); // B * G^T
  
  mat_sub_block<Matrix1> subA(A, q + 4, 2, 0, p+2);
  givens_rot_prod(subA,G); // A * G^T
  
  if(Z) {
    mat_sub_block<Matrix4> subZ(*Z, Z->get_row_count(), 2, 0, p+2);
    givens_rot_prod(subZ,G); // Q_prev * G^T
  };
  };
  
  
  // finally, if the middle sub-diagonal element is not back to zero, then perform QZ-steps
  // with lambda-1 as initial shifts until it converges to zero (this should be quick)
  
  // loop until it works.
  while( fabs(A(q+2,p+1)) > NumTol * ( fabs(A(q+1,p+1)) + fabs(A(q+2,p+2)) ) ) {
    
    householder_matrix< vect<ValueType,3> > hhm;
    
    vect<ValueType,3> v;
    
    v[0] = ((a_mm / b_mm - A(q,p) / B(q,p)) * (a_nn / b_nn - A(q,p) / B(q,p))
           - (a_mn / b_nn) * (a_nm / b_mm)
           + (a_nm / b_mm) * (b_mn / b_nn) * (A(q,p) / B(q,p))) * (B(q,p) / A(q+1,p))
         + A(q,p+1) / B(q+1,p+1) - (A(q,p) / B(q,p)) * (B(q,p+1) / B(q+1,p+1));
    v[1] = (A(q+1,p+1) / B(q+1,p+1) - A(q,p) / B(q,p)) 
         - (A(q+1,p) / B(q,p)) * (B(q,p+1) / B(q+1,p+1))
         - (a_mm / b_mm - A(q,p) / B(q,p))
         - (a_nn / b_nn - A(q,p) / B(q,p))
         + (a_nm / b_mm) * (b_mn / b_nn);
    v[2] = A(q+2,p+1) / B(q+1,p+1);
    
    for(SizeType k = 0; k < 2; ++k) {
      hhm.set(v,NumTol);
      
      mat_sub_block< Matrix1 > subA1(A,3,A.get_col_count() - p, q + k, p);
      householder_prod(hhm,subA1);
      
      mat_sub_block< Matrix2 > subB1(B,3,B.get_col_count() - p, q + k, p);
      householder_prod(hhm,subB1);
      
      if(Q) {
        mat_sub_block< Matrix3 > subQ(*Q,Q->get_row_count(), 3, 0, q + k);
        householder_prod(subQ,hhm); // Q * Q_k ^T
      };
      
      
      vect<ValueType,3> v2(B(q + k + 2, p + k + 2),
                           B(q + k + 2, p + k + 1),
                           B(q + k + 2, p + k));
      hhm.set(v2,NumTol);
      
      mat_sub_block< Matrix1 > subA2(A,A.get_row_count(),3,0,p + k);
      householder_prod(subA2,hhm);
      
      mat_sub_block< Matrix2 > subB2(B,B.get_row_count(),3,0,p + k);
      householder_prod(subB2,hhm);
      
      if(Z) {
        mat_sub_block<Matrix4> subZ(*Z,Z->get_row_count(),3,0,p + k);
        householder_prod(subZ,hhm); // Q_prev * P
      };
      
      
      householder_matrix< vect<ValueType,2> > hhm2;
      hhm2.set( vect<ValueType,2>(B(q + k + 1, p + k + 1),
                                  B(q + k + 1, p + k)), NumTol );
    
      mat_sub_block< Matrix1 > subA3(A,A.get_row_count(),2,0,p + k);
      householder_prod(subA3,hhm2);
      
      mat_sub_block< Matrix2 > subB3(B,B.get_row_count(),2,0,p + k);
      householder_prod(subB3,hhm2);
      
      if(Z) {
        mat_sub_block<Matrix4> subZ(*Z,Z->get_row_count(),2,0,p + k);
        householder_prod(subZ,hhm2); // Q_prev * P
      };
      
      v[0] = A(q + k + 1, p + k);
      v[1] = A(q + k + 2, p + k);
      if(k == 0)
        v[2] = A(q + k + 3, p + k);
    };
    
    householder_matrix< vect<ValueType,2> > hhm3(vect<ValueType,2>(v[0],v[1]),NumTol);
    
    mat_sub_block< Matrix1 > subA4(A,2,A.get_col_count()-p,q + 2,p);
    householder_prod(hhm3,subA4);
    
    mat_sub_block< Matrix2 > subB4(B,2,B.get_col_count()-p,q + 2,p);
    householder_prod(hhm3,subB4);
    
    if(Q) {
      mat_sub_block< Matrix3 > subQ(*Q,Q->get_row_count(), 2, 0, q + 2);
      householder_prod(subQ,hhm3); // Q * Q_k ^T
    };
    
    hhm3.set( vect<ValueType,2>(B(q + 3, p + 3),
                                B(q + 3, p + 2)), NumTol );
      
    mat_sub_block< Matrix1 > subA5(A,A.get_row_count(),2,0,p + 2);
    householder_prod(subA5,hhm3);
      
    mat_sub_block< Matrix2 > subB5(B,B.get_row_count(),2,0,p + 2);
    householder_prod(subB5,hhm3);
      
    if(Z) {
      mat_sub_block<Matrix4> subZ(*Z,Z->get_row_count(),2,0,p + 2);
      householder_prod(subZ,hhm3); // Q_prev * P
    };
    
  };
  
  
};



template <typename Matrix1, typename Matrix2, 
          typename Matrix3, typename Matrix4, typename CompareFunc>
void partition_schur_pencil_impl(Matrix1& A, Matrix2& B, Matrix3* Q, Matrix4* Z,
                                 CompareFunc compare,
                                 typename mat_traits<Matrix1>::value_type NumTol) {
  typedef typename mat_traits<Matrix1>::value_type ValueType;
  typedef typename mat_traits<Matrix1>::size_type SizeType;
  using std::fabs;
  SizeType N = A.get_row_count();
  
  // This algorithm is basically an insertion sort based on the compare functor and 
  //  a mix of 1-1 and 2-2 blocks to be swapped along the diagonal to sort the eigen-values.
  SizeType q = 0;
  while(q < N-1) {
    bool is_next_block_by2 = false;
    if((++q < N-1) && (fabs(A(q+1,q)) > NumTol(fabs(A(q,q)) + fabs(A(q+1,q+1)))))
      is_next_block_by2 = true;
    SizeType p = q;
    while(true) {
      bool is_prev_block_by2 = false;
      if((p > 1) && (fabs(A(p-1,p-2)) > NumTol(fabs(A(p-2,p-2)) + fabs(A(p-1,p-1)))))
        is_prev_block_by2 = true;
      bool swap_needed = false;
      if(is_next_block_by2 && is_prev_block_by2) {
        swap_needed = (-1 == compare(sub(A)(range(p-2,p-1),range(p-2,p-1)), sub(B)(range(p-2,p-1),range(p-2,p-1)),
                                     sub(A)(range(p,p+1),range(p,p+1)), sub(B)(range(p,p+1),range(p,p+1))));
        if(swap_needed) {
          mat_sub_block<Matrix1> subA(A, p+2, N-p+2, 0, p-2);
          mat_sub_block<Matrix2> subB(B, p+2, N-p+2, 0, p-2);
          if(Q) {
            mat_sub_block<Matrix3> subQ(*Q, Q->get_row_count(), p+2, 0, 0);
            if(Z) {
              mat_sub_block<Matrix4> subZ(*Z, Z->get_row_count(), N-p+2, 0, p-2);
              swap_schur_blocks22_impl(subA,subB,&subQ,&subZ,0,p-2,NumTol);
            } else {
              swap_schur_blocks22_impl(subA,subB,&subQ,static_cast<Matrix4*>(NULL),0,p-2,NumTol);
            };
          } else {
            if(Z) {
              mat_sub_block<Matrix4> subZ(*Z, Z->get_row_count(), N-p+2, 0, p-2);
              swap_schur_blocks22_impl(subA,subB,static_cast<Matrix3*>(NULL),&subZ,0,p-2,NumTol);
            } else {
              swap_schur_blocks22_impl(subA,subB,static_cast<Matrix3*>(NULL),static_cast<Matrix4*>(NULL),0,p-2,NumTol);
            };
          };
          p -= 2;
        };
      } else if(is_next_block_by2 && !is_prev_block_by2) {
        swap_needed = (-1 == compare(sub(A)(range(p-1,p-1),range(p-1,p-1)), sub(B)(range(p-1,p-1),range(p-1,p-1)),
                                     sub(A)(range(p,p+1),range(p,p+1)), sub(B)(range(p,p+1),range(p,p+1))));
        if(swap_needed) {
          mat_sub_block<Matrix1> subA(A, p+2, N-p+1, 0, p-1);
          mat_sub_block<Matrix2> subB(B, p+2, N-p+1, 0, p-1);
          if(Q) {
            mat_sub_block<Matrix3> subQ(*Q, Q->get_row_count(), p+2, 0, 0);
            if(Z) {
              mat_sub_block<Matrix4> subZ(*Z, Z->get_row_count(), N-p+1, 0, p-1);
              swap_schur_blocks22_impl(subA,subB,&subQ,&subZ,0,p-1);
            } else {
              swap_schur_blocks22_impl(subA,subB,&subQ,static_cast<Matrix4*>(NULL),0,p-1);
            };
          } else {
            if(Z) {
              mat_sub_block<Matrix4> subZ(*Z, Z->get_row_count(), N-p+1, 0, p-1);
              swap_schur_blocks12_impl(subA,subB,static_cast<Matrix3*>(NULL),&subZ,0,p-1);
            } else {
              swap_schur_blocks12_impl(subA,subB,static_cast<Matrix3*>(NULL),static_cast<Matrix4*>(NULL),0,p-1);
            };
          };
          --p;
        };
      } else if(!is_next_block_by2 && is_prev_block_by2) {
        swap_needed = (-1 == compare(sub(A)(range(p-2,p-1),range(p-2,p-1)), sub(B)(range(p-2,p-1),range(p-2,p-1)),
                                     sub(A)(range(p,p),range(p,p)), sub(B)(range(p,p),range(p,p))));
        if(swap_needed) {
          mat_sub_block<Matrix1> subA(A, p+1, N-p+2, 0, p-2);
          mat_sub_block<Matrix2> subB(B, p+1, N-p+2, 0, p-2);
          if(Q) {
            mat_sub_block<Matrix3> subQ(*Q, Q->get_row_count(), p+1, 0, 0);
            if(Z) {
              mat_sub_block<Matrix4> subZ(*Z, Z->get_row_count(), N-p+2, 0, p-2);
              swap_schur_blocks21_impl(subA,subB,&subQ,&subZ,0,p-2);
            } else {
              swap_schur_blocks21_impl(subA,subB,&subQ,static_cast<Matrix4*>(NULL),0,p-2);
            };
          } else {
            if(Z) {
              mat_sub_block<Matrix4> subZ(*Z, Z->get_row_count(), N-p+2, 0, p-2);
              swap_schur_blocks21_impl(subA,subB,static_cast<Matrix3*>(NULL),&subZ,0,p-2);
            } else {
              swap_schur_blocks21_impl(subA,subB,static_cast<Matrix3*>(NULL),static_cast<Matrix4*>(NULL),0,p-2);
            };
          };
          p -= 2;
        };
      } else if(!is_next_block_by2 && !is_prev_block_by2) {
        swap_needed = (-1 == compare(sub(A)(range(p-1,p-1),range(p-1,p-1)), sub(B)(range(p-1,p-1),range(p-1,p-1)),
                                     sub(A)(range(p,p),range(p,p)), sub(B)(range(p,p),range(p,p))));
        if(swap_needed) {
          mat_sub_block<Matrix1> subA(A, p+1, N-p+1, 0, p-1);
          mat_sub_block<Matrix2> subB(B, p+1, N-p+1, 0, p-1);
          if(Q) {
            mat_sub_block<Matrix3> subQ(*Q, Q->get_row_count(), p+1, 0, 0);
            if(Z) {
              mat_sub_block<Matrix4> subZ(*Z, Z->get_row_count(), N-p+1, 0, p-1);
              swap_schur_blocks11_impl(subA,subB,&subQ,&subZ,0,p-1,NumTol);
            } else {
              swap_schur_blocks11_impl(subA,subB,&subQ,static_cast<Matrix4*>(NULL),0,p-1);
            };
          } else {
            if(Z) {
              mat_sub_block<Matrix4> subZ(*Z, Z->get_row_count(), N-p+1, 0, p-1);
              swap_schur_blocks11_impl(subA,subB,static_cast<Matrix3*>(NULL),&subZ,0,p-1);
            } else {
              swap_schur_blocks11_impl(subA,subB,static_cast<Matrix3*>(NULL),static_cast<Matrix4*>(NULL),0,p-1);
            };
          };
          --p;
        };
      };
      
      if(!swap_needed || p == 0)
        break;
    };
    if(is_next_block_by2)
      ++q;
  };
};




}; //detail





/**
 * Solves the Continuous-time Algebraic Riccati Equation (for infinite horizon LQR).
 * This implementation uses the QZ-algorithm approach as described in Van Dooren (1981).
 * This method first reduces the augmented (2n+m x 2n+m) pencil to a (2n x 2n) pencil 
 * using a QR decomposition on the last (mxm) block-column (which has infinite eigenvalues).
 * Then, it performs a generalized real Schur decomposition of the pencil. Finally, it 
 * reorders the eigenvalues in the pencil such that stable (within unit-circle) eigenvalues
 * percolate to the upper (nxn) pencil, which allows the extraction of the eigenvectors 
 * spanning the stable subspace, which are, in turn, used to compute the unique solution P.
 * \n
 * $Q + A^T P + P A - P B R^{-1} B^T P = 0$
 * \n
 * The initial pencil is: lambda * (I 0 0; 0 I 0; 0 0 0) - (A 0 B; -Q -A^T 0; 0 B^T R)
 * \n
 *
 * \tparam Matrix1 A readable matrix type.
 * \tparam Matrix2 A readable matrix type.
 * \tparam Matrix3 A readable matrix type.
 * \tparam Matrix4 A readable matrix type.
 * \tparam Matrix5 A fully-writable matrix type.
 * \param A square (n x n) matrix which represents state-to-state-derivative linear map.
 * \param B rectangular (n x m) matrix which represents input-to-state-derivative linear map.
 * \param Q square (n x n) positive-definite matrix which represents quadratic state-error penalty.
 * \param R square (m x m) positive-semi-definite matrix which represents quadratic input penalty.
 * \param P holds as output, the nonnegative definite solution to Q + A^T P + P A - P B R^-1 B^T P = 0.
 * \param NumTol tolerance for considering a value to be zero in avoiding divisions
 *               by zero and singularities.
 *
 * \throws std::range_error if the matrix dimensions are not consistent.
 *
 * \author Mikael Persson
 */
template <typename Matrix1, typename Matrix2, typename Matrix3, typename Matrix4, typename Matrix5>
typename boost::enable_if_c< is_readable_matrix<Matrix1>::value && 
                             is_readable_matrix<Matrix2>::value && 
                             is_readable_matrix<Matrix3>::value && 
                             is_readable_matrix<Matrix4>::value && 
                             is_fully_writable_matrix<Matrix5>::value, 
void >::type solve_care_problem(const Matrix1& A, const Matrix2& B, 
                                const Matrix3& Q, const Matrix4& R, 
                                Matrix5& P, typename mat_traits<Matrix1>::value_type NumTol = 1E-8) {
  if((A.get_row_count() != A.get_col_count()) || 
     (B.get_row_count() != A.get_row_count()) || 
     (Q.get_row_count() != Q.get_col_count()) || 
     (R.get_row_count() != R.get_col_count()) || 
     (B.get_col_count() != R.get_col_count()))
    throw std::range_error("The dimensions of the CARE system matrices do not match! Should be A(n x n), B(n x m), Q(n x n), and R(m x m).");
  
  typedef typename mat_traits<Matrix1>::value_type ValueType;
  typedef typename mat_traits<Matrix1>::size_type SizeType;
  SizeType N = A.get_row_count();
  SizeType M = R.get_row_count();
  
  mat<ValueType, mat_structure::rectangular> R_tmp(N * 2 + M, M);
  sub(R_tmp)(range(0,M-1), range(0,M-1)) = R;
  sub(R_tmp)(range(M,M + N - 1), range(0,M-1)) = B;
  mat<ValueType, mat_structure::square> Q_tmp = mat<ValueType, mat_structure::square>(N * 2 + M);
  sub(Q_tmp)(range(0, 2 * N - 1), range(M, M + 2 * N - 1)) = mat<ValueType, mat_structure::identity>(2 * N);
  sub(Q_tmp)(range(2 * N, 2 * N + M - 1), range(0, M - 1)) = mat<ValueType, mat_structure::identity>(M);
  
  detail::decompose_QR_impl(R_tmp, &Q_tmp, NumTol);
  Q_tmp = transpose(Q_tmp);
  
  mat<ValueType, mat_structure::rectangular> A_aug(2 * N, 2 * N);
  A_aug = sub(Q_tmp)(range(M,M + 2*N - 1), range(0,2*N - 1));
  
  mat<ValueType, mat_structure::rectangular> B_aug(2 * N, 2 * N);
  sub(B_aug)(range(0,2 * N - 1),range(0, N - 1)) = 
      sub(Q_tmp)(range(M,M + 2*N-1),range(0,N-1)) * A
    - sub(Q_tmp)(range(M,M + 2*N-1),range(N,2*N-1)) * Q;
  sub(B_aug)(range(0,2 * N - 1),range(N, 2*N - 1)) = 
      sub(Q_tmp)(range(M,M + 2*N-1),range(2*N,2*N+M-1)) * transpose_view(B)
    - sub(Q_tmp)(range(M,M + 2*N-1),range(N,2*N-1)) * transpose_view(A);
  
  mat<ValueType, mat_structure::square> Q_aug = mat<ValueType, mat_structure::square>(mat<ValueType, mat_structure::identity>(2*N));
  mat<ValueType, mat_structure::square> Z_aug = mat<ValueType, mat_structure::square>(mat<ValueType, mat_structure::identity>(2*N));
  
  detail::gen_schur_decomp_impl(A_aug,B_aug,&Q_aug,&Z_aug,NumTol);
  
  detail::partition_schur_pencil_impl(A_aug,B_aug,&Q_aug,&Z_aug,detail::stable_eigen_first(),NumTol);
  
  P.set_row_count(N);
  P.set_col_count(N);
  linlsq_QR(transpose_view(sub(Z)(range(0,N-1),range(0,N-1))), P, transpose_view(sub(Z)(range(N,2*N-1),range(0,N-1))), NumTol);
  P = transpose(P);
};








};


#endif



