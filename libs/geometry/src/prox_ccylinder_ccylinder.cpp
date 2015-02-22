
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

#include <ReaK/geometry/proximity/prox_ccylinder_ccylinder.hpp>

#include <cmath>

/** Main namespace for ReaK */
namespace ReaK {

/** Main namespace for ReaK.Geometry */
namespace geom {


void prox_ccylinder_ccylinder::computeProximity(const shape_3D_precompute_pack& aPack1, 
                                                const shape_3D_precompute_pack& aPack2) {
  if((!mCCylinder1) || (!mCCylinder2)) {
    mLastResult.mDistance = std::numeric_limits<double>::infinity();
    mLastResult.mPoint1 = vect<double,3>(0.0,0.0,0.0);
    mLastResult.mPoint2 = vect<double,3>(0.0,0.0,0.0);
    return;
  };
  using std::fabs; using std::sqrt; using ReaK::unit; using ReaK::norm_2;
  
  const pose_3D<double>& c1_pose = (aPack1.parent == mCCylinder1 ? 
                                    aPack1.global_pose : aPack2.global_pose);
  const pose_3D<double>& c2_pose = (aPack1.parent == mCCylinder1 ? 
                                    aPack2.global_pose : aPack1.global_pose);
  
  const vect<double,3> cy2_c = c2_pose.Position;
  const vect<double,3> cy2_t = c2_pose.rotateToGlobal(vect<double,3>(0.0,0.0,1.0));
  
  const vect<double,3> cy2_c_rel = c1_pose.transformFromGlobal(cy2_c);
  const vect<double,3> cy2_t_rel = c1_pose.rotateFromGlobal(cy2_t);
  
  const double cy1_len = mCCylinder1->getLength();
  const double cy1_rad = mCCylinder1->getRadius();
  const double cy2_len = mCCylinder2->getLength();
  const double cy2_rad = mCCylinder2->getRadius();
  
  if(sqrt(cy2_t_rel[0] * cy2_t_rel[0] + cy2_t_rel[1] * cy2_t_rel[1]) < 1e-5) {
    // The capped-cylinders are parallel.
    if((cy2_c_rel[2] + 0.5 * cy2_len > -0.5 * cy1_len) || 
       (cy2_c_rel[2] - 0.5 * cy2_len <  0.5 * cy1_len)) {
      // there is an overlap between the capped-cylinder sides.
      const double max_z_rel = ((cy2_c_rel[2] + 0.5 * cy2_len <  0.5 * cy1_len) ? (cy2_c_rel[2] + 0.5 * cy2_len) : ( 0.5 * cy1_len));
      const double min_z_rel = ((cy2_c_rel[2] - 0.5 * cy2_len > -0.5 * cy1_len) ? (cy2_c_rel[2] - 0.5 * cy2_len) : (-0.5 * cy1_len));
      const double avg_z_rel = (max_z_rel + min_z_rel) * 0.5;
      const vect<double,3> cy2_r_rel = unit(vect<double,3>(cy2_c_rel[0],cy2_c_rel[1],0.0));
      mLastResult.mPoint1 = c1_pose.transformToGlobal(vect<double,3>(cy1_rad * cy2_r_rel[0], cy1_rad * cy2_r_rel[1], avg_z_rel));
      mLastResult.mPoint2 = c1_pose.transformToGlobal(vect<double,3>(cy2_c_rel[0] - cy2_rad * cy2_r_rel[0], cy2_c_rel[1] - cy2_rad * cy2_r_rel[1], avg_z_rel));
      mLastResult.mDistance = sqrt(cy2_c_rel[0] * cy2_c_rel[0] + cy2_c_rel[1] * cy2_c_rel[1]) - cy1_rad - cy2_rad;
      return;
    };
    // there is no overlap, and thus, this boils down to a sphere-sphere problem.
    vect<double,3> cy1_spc_rel(0.0,0.0,0.0);
    vect<double,3> cy2_spc_rel = cy2_c_rel;
    if(cy2_c_rel[2] < 0.0) {
      cy1_spc_rel[2] -= 0.5 * cy1_len;
      cy2_spc_rel[2] += 0.5 * cy2_len;
    } else {
      cy1_spc_rel[2] += 0.5 * cy1_len;
      cy2_spc_rel[2] -= 0.5 * cy2_len;
    };
    const vect<double,3> diff_v_rel = cy2_spc_rel - cy1_spc_rel;
    const double dist_v_rel = norm_2(diff_v_rel);
    mLastResult.mPoint1 = c1_pose.transformToGlobal(cy1_spc_rel + (cy1_rad / dist_v_rel) * diff_v_rel);
    mLastResult.mPoint2 = c1_pose.transformToGlobal(cy2_spc_rel - (cy2_rad / dist_v_rel) * diff_v_rel);
    mLastResult.mDistance = dist_v_rel - cy1_rad - cy2_rad;
    return;
  };
  
  // Line-Line solution:
  const double d = cy2_t_rel * cy2_c_rel;
  const double denom = 1.0 - cy2_t_rel[2] * cy2_t_rel[2];
  double s_c = (cy2_t_rel[2] * cy2_c_rel[2] - d) / denom;
  double t_c = (cy2_c_rel[2] - cy2_t_rel[2] * d) / denom;
  
  // Segment-Segment solution:
  if(s_c < -0.5 * cy2_len) {
    s_c = -0.5 * cy2_len;
    t_c = cy2_c_rel[2] - 0.5 * cy2_len * cy2_t_rel[2];
  } else if(s_c > 0.5 * cy2_len) {
    s_c = 0.5 * cy2_len;
    t_c = cy2_c_rel[2] + 0.5 * cy2_len * cy2_t_rel[2];
  };
  
  if(t_c < -0.5 * cy1_len) {
    t_c = -0.5 * cy1_len;
    s_c = -0.5 * cy1_len * cy2_t_rel[2] - d;
  } else if(t_c > 0.5 * cy1_len) {
    t_c = 0.5 * cy1_len;
    s_c = 0.5 * cy1_len * cy2_t_rel[2] - d;
  };
  
  if(s_c < -0.5 * cy2_len)
    s_c = -0.5 * cy2_len;
  else if(s_c > 0.5 * cy2_len)
    s_c = 0.5 * cy2_len;
  
  // we have parameters s and t for the min-dist points on the center segments.
  
  // just apply a sphere-sweep on the line-segments.
  const vect<double,3> cy1_ptc(0.0,0.0,t_c);
  const vect<double,3> cy2_ptc = cy2_c_rel + s_c * cy2_t_rel;
  
  const vect<double,3> diff_v_rel = cy2_ptc - cy1_ptc;
  const double dist_v_rel = norm_2(diff_v_rel);
  mLastResult.mPoint1 = c1_pose.transformToGlobal(cy1_ptc + (cy1_rad / dist_v_rel) * diff_v_rel);
  mLastResult.mPoint2 = c1_pose.transformToGlobal(cy2_ptc - (cy2_rad / dist_v_rel) * diff_v_rel);
  mLastResult.mDistance = dist_v_rel - cy1_rad - cy2_rad;
};


prox_ccylinder_ccylinder::prox_ccylinder_ccylinder(const capped_cylinder* aCCylinder1,
                                                   const capped_cylinder* aCCylinder2) :
                                                   proximity_finder_3D(),
                                                   mCCylinder1(aCCylinder1),
                                                   mCCylinder2(aCCylinder2) { };

};

};

