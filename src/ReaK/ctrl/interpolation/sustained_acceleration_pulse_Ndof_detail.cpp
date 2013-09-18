
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

#include "sustained_acceleration_pulse_Ndof_detail.hpp"

#include <cmath>


#include "root_finders/bisection_method.hpp"
#include "root_finders/secant_method.hpp"


namespace ReaK {

namespace pp {
  
  
namespace detail {

  
  
  
  static void sap_Ndof_compute_interpolated_values_closedform(
    double start_position, double end_position,
    double start_velocity, double end_velocity,
    double peak_velocity, double max_velocity, double max_acceleration,
    double dt, double dt_total,
    double& result_pos, double& result_vel, 
    double& result_acc, double& result_desc_jerk) {
    
    using std::fabs;
    using std::sqrt;
    
    double dv1 = peak_velocity - start_velocity;
    double dv2 = end_velocity - peak_velocity;
    result_pos = start_position;
    result_vel = start_velocity;
    result_acc = 0.0;
    result_desc_jerk = 0.0;
    
    double dt_vp1_1st = fabs(dv1);
    double sgn_vp1 = 1.0;
    if( dv1 < 0.0 ) sgn_vp1 = -1.0;
    // we know that dt_vp_2nd = dt_vp_1st + dt_amax
    double dt_vp1 = dt_vp1_1st - max_acceleration;
    double dt_ap1 = max_acceleration;
    if( dt_vp1 < 0.0 ) {
      //means that we don't have time to reach the maximum acceleration:
      dt_vp1 = 0.0;
      dt_ap1 = sqrt(max_acceleration * dt_vp1_1st);
    };
    
    double dt_vp2_1st = fabs(dv2);
    double sgn_vp2 = 1.0;
    if( dv2 < 0.0 ) sgn_vp2 = -1.0;
    // we know that dt_vp_2nd = dt_vp_1st + dt_amax
    double dt_vp2 = dt_vp2_1st - max_acceleration;
    double dt_ap2 = max_acceleration;
    if( dt_vp2 < 0.0 ) {
      //means that we don't have time to reach the maximum acceleration:
      dt_vp2 = 0.0;
      dt_ap2 = sqrt(max_acceleration * dt_vp2_1st);
    };
    
    
    dt_total -= dt_vp2 + 2.0 * dt_ap2 + dt_vp1 + 2.0 * dt_ap1;
    
    
    if( dt < (2.0 * dt_ap1 + dt_vp1) ) {
      
      if( dt < dt_ap1 ) {
        
        //Segment 1: in the jerk-up phase of velocity ramp-up.
        // assume result_acc == 0
        result_pos += ( result_vel + ( dt * sgn_vp1 / 6.0 ) * dt / max_acceleration ) * dt / max_velocity;
        result_vel += ( 0.5 * dt * sgn_vp1 ) * dt / max_acceleration;
        result_acc  = dt * sgn_vp1;
        result_desc_jerk = sgn_vp1;
        
      } else if( dt < dt_ap1 + dt_vp1 ) {
        //Segment 2: in the constant accel phase of velocity ramp-up.
        
        dt -= dt_ap1;
        
        result_pos += ( ( result_vel + 0.5 * dt * sgn_vp1) * ( max_acceleration + dt ) 
                      + max_acceleration * max_acceleration * sgn_vp1 / 6.0 ) / max_velocity;
        result_vel += (dt + 0.5 * max_acceleration) * sgn_vp1;
        result_acc  = max_acceleration * sgn_vp1;
        result_desc_jerk = 0.0;
        
      } else {
        //Segment 3: in the jerk-down phase of velocity ramp-up.
        
        dt -= dt_ap1 + dt_vp1;
        
        result_pos += ( result_vel * ( dt_ap1 + dt_vp1 + dt )
          + ( dt_ap1 * dt * ( 0.5 * dt + dt_vp1 + 0.5 * dt_ap1 )
            + 0.5 * dt_ap1 * ( dt_ap1 * dt_ap1 / 3.0 + dt_vp1 * dt_ap1 + dt_vp1 * dt_vp1 ) 
            - dt * dt * dt / 6.0 ) * sgn_vp1 / max_acceleration ) / max_velocity;
        result_vel += ( dt_ap1 * ( dt + dt_vp1 + 0.5 * dt_ap1 )
                      - 0.5 * dt * dt ) * sgn_vp1 / max_acceleration;
        result_acc  = ( dt_ap1 - dt ) * sgn_vp1;
        result_desc_jerk = -sgn_vp1;
        
        
        // // alternative calculation (from the end of segment, backwards)
        // double mdt -= 2.0 * dt_ap1 + dt_vp1 - dt;
        
        // result_pos += ( peak_velocity * ( 2.0 * dt_ap1 + dt_vp1 - mdt )
        //   + ( mdt * mdt * mdt / 6.0 - dt_ap1 * ( dt_ap1 + dt_vp1 ) * ( dt_ap1 + 0.5 * dt_vp1 ) ) * sgn_vp1 / max_acceleration ) / max_velocity;
        // // peak_velocity == result_vel + ( dt_ap1 * dt_vp1 + dt_ap1 * dt_ap1 ) * sgn_vp1 / max_acceleration
        // result_vel  = peak_velocity - ( 0.5 * mdt * sgn_vp1 ) * mdt / max_acceleration;
        // result_acc  = mdt * sgn_vp1;
        // result_desc_jerk = -sgn_vp1;
        
      };
      
      return;
      
    };
    
    result_pos += ( peak_velocity * ( 2.0 * dt_ap1 + dt_vp1 )
        - dt_ap1 * ( dt_ap1 + dt_vp1 ) * ( dt_ap1 + 0.5 * dt_vp1 ) * sgn_vp1 / max_acceleration ) / max_velocity;
    result_vel  = peak_velocity;
    result_acc = 0.0;
    result_desc_jerk = 0.0;
    
    if( dt < (2.0 * dt_ap1 + dt_vp1 + dt_total) ) {
      //Segment 4: in the cruise phase.
      
      dt -= 2.0 * dt_ap1 + dt_vp1;
      
      result_pos += dt * peak_velocity / max_velocity;
      
      return;
    };
    
    result_pos += dt_total * peak_velocity / max_velocity;
    
    
    if( dt < (2.0 * dt_ap1 + dt_vp1 + dt_total + 2.0 * dt_ap2 + dt_vp2) ) {
      
      if( dt < (2.0 * dt_ap1 + dt_vp1 + dt_total + dt_ap2) ) {
        
        //Segment 5: in the jerk-up phase of velocity ramp-down.
        
        dt -= 2.0 * dt_ap1 + dt_vp1 + dt_total;
        
        result_pos += ( result_vel + ( dt * sgn_vp2 / 6.0 ) * dt / max_acceleration ) * dt / max_velocity;
        result_vel += ( 0.5 * dt * sgn_vp2 ) * dt / max_acceleration;
        result_acc  = dt * sgn_vp2;
        result_desc_jerk = sgn_vp2;
        
      } else if( dt < (2.0 * dt_ap1 + dt_vp1 + dt_total + dt_ap2 + dt_vp2) ) {
        
        //Segment 6: in the constant accel phase of velocity ramp-down.
        
        dt -= 2.0 * dt_ap1 + dt_vp1 + dt_total + dt_ap2;
        
        result_pos += ( result_vel * ( max_acceleration + dt )
          + ( max_acceleration * max_acceleration / 6.0 + 0.5 * max_acceleration * dt + 0.5 * dt * dt ) * sgn_vp2 ) / max_velocity;
        result_vel += ( dt + 0.5 * max_acceleration ) * sgn_vp2;
        result_acc  = max_acceleration * sgn_vp2;
        result_desc_jerk = 0.0;
        
      } else {
        
        //Segment 7: in the jerk-down phase of velocity ramp-down.
        
        dt -= 2.0 * dt_ap1 + dt_vp1 + dt_total + dt_ap2 + dt_vp2;
        
        result_pos += ( result_vel * ( dt_ap2 + dt_vp2 + dt )
          + ( dt_ap2 * dt_ap2 * dt_ap2 / 6.0 
            + 0.5 * dt_vp2 * dt_ap2 * dt_ap2 
            + 0.5 * dt_vp2 * dt_vp2 * dt_ap2 
            + dt * dt_vp2 * dt_ap2 
            + 0.5 * dt * dt_ap2 * dt_ap2 
            + 0.5 * dt * dt * dt_ap2 
            - dt * dt * dt / 6.0 ) * sgn_vp2 / max_acceleration ) / max_velocity;
        result_vel += ( dt_ap2 * sgn_vp2 - 0.5 * dt * sgn_vp2 ) * dt / max_acceleration + ( dt_vp2 + 0.5 * dt_ap2 ) * dt_ap2 * sgn_vp2 / max_acceleration;
        result_acc  = ( dt_ap2 - dt ) * sgn_vp2;
        result_desc_jerk = -sgn_vp2;
        
        // // alternative calculation (from the end of segment, backwards)
        // double mdt -= 2.0 * dt_ap1 + dt_vp1 + dt_total + 2.0 * dt_ap1 + dt_vp1 - dt;
        
        // result_pos += ( end_velocity * ( 2.0 * dt_ap2 + dt_vp2 - mdt )
        //   + ( mdt * mdt * mdt / 6.0 - dt_ap2 * ( dt_ap2 + dt_vp2 ) * ( dt_ap2 + 0.5 * dt_vp2 ) ) * sgn_vp2 / max_acceleration ) / max_velocity;
        // // end_velocity == result_vel + ( dt_ap2 * dt_vp2 + dt_ap2 * dt_ap2 ) * sgn_vp2 / max_acceleration
        // result_vel  = end_velocity - ( 0.5 * mdt * sgn_vp2 ) * mdt / max_acceleration;
        // result_acc  = mdt * sgn_vp2;
        // result_desc_jerk = -sgn_vp2;
        
      };
      
      return;
    };
    
    result_acc  = 0.0;
    result_desc_jerk = 0.0;
    
    // Expected end-conditions:
    
//     result_pos = end_position;
//     result_vel = end_velocity;
    
    // Resulting position / velocity formulae:
    
//     result_pos += ( result_vel + 0.5 * ( dt_vp2 + dt_ap2 ) * dt_ap2 * sgn_vp2 / max_acceleration ) * ( 2.0 * dt_ap2 + dt_vp2 ) / max_velocity;
    result_vel += ( dt_ap2 + dt_vp2 ) * dt_ap2 * sgn_vp2 / max_acceleration;

    
//     result_pos += ( 
//         peak_velocity * ( dt_total + 2.0 * dt_ap1 + dt_vp1 + 2.0 * dt_ap2 + dt_vp2 )
//       - 0.5 * ( dt_ap1 + dt_vp1 ) * ( 2.0 * dt_ap1 + dt_vp1 ) * dt_ap1 * sgn_vp1 / max_acceleration
//       + 0.5 * ( dt_vp2 + dt_ap2 ) * ( 2.0 * dt_ap2 + dt_vp2 ) * dt_ap2 * sgn_vp2 / max_acceleration ) / max_velocity;
    
    result_pos = start_position + ( peak_velocity * dt_total 
      + 0.5 * ( start_velocity + peak_velocity ) * ( 2.0 * dt_ap1 + dt_vp1 ) 
      + 0.5 * ( peak_velocity  + end_velocity  ) * ( 2.0 * dt_ap2 + dt_vp2 ) ) / max_velocity;
    
      
  };
  
  
  
  void sap_Ndof_compute_interpolated_values(
    double start_position, double end_position,
    double start_velocity, double end_velocity,
    double peak_velocity, double max_velocity, double max_acceleration,
    double dt, double dt_total,
    double& result_pos, double& result_vel, 
    double& result_acc, double& result_desc_jerk) {
    
    sap_Ndof_compute_interpolated_values_closedform(
      start_position, end_position, start_velocity, end_velocity,
      peak_velocity, max_velocity, max_acceleration, dt, dt_total,
      result_pos, result_vel, result_acc, result_desc_jerk);
    
  };
  
  
  
  
  
  inline void sap_Ndof_compute_ramp_dist_and_time(
    double v1, double v2, double vmax, double amax, 
    double& d_pos, double& dt) {
    
    using std::fabs;
    using std::sqrt;
    
    if( fabs(v2 - v1) >= amax ) {
      dt = fabs(v2 - v1) + amax;
      d_pos = 0.5 * dt * (v1 + v2) / vmax;
    } else {
      dt = 2.0 * sqrt(amax * fabs(v2 - v1));
      d_pos = 0.5 * dt * (v1 + v2) / vmax;
    };
    
  };
  
  struct sap_Ndof_no_cruise_calculator {
    double dp, v1, v2, vmax, amax;
    
    sap_Ndof_no_cruise_calculator(
      double a_dp, double a_v1, double a_v2, double a_vmax, double a_amax) : 
      dp(a_dp), v1(a_v1), v2(a_v2), vmax(a_vmax), amax(a_amax) { };
    
    double operator()(double vp) {
      double dp1, dt1, dp2, dt2;
      sap_Ndof_compute_ramp_dist_and_time(v1, vp, vmax, amax, dp1, dt1);
      sap_Ndof_compute_ramp_dist_and_time(vp, v2, vmax, amax, dp2, dt2);
      
      if( dp < 0.0 )
        return dp1 + dp2 - dp;
      else 
        return dp - dp1 - dp2;
    };
    
  };
  
  
  static double sap_Ndof_compute_min_delta_time_numsolve(
    double start_position, double end_position,
    double start_velocity, double end_velocity,
    double& peak_velocity, double max_velocity, double max_acceleration) {
    using std::fabs;
    using std::sqrt;
    
    if( ( fabs(end_position - start_position) < 1e-6 * max_velocity ) &&
        ( fabs(end_velocity - start_velocity) < 1e-6 * max_acceleration ) ) {
      peak_velocity = start_velocity;
      return 0.0;
    };
    
    if( ( fabs(start_velocity) > max_velocity ) || ( fabs(end_velocity) > max_velocity ) ) {
      peak_velocity = 0.0;
      return std::numeric_limits<double>::infinity();
    };
    
    double sign_p1_p0 = 1.0;
    if(start_position > end_position)
      sign_p1_p0 = -1.0;
    
    double dt_ramp1 = 0.0, dp_ramp1 = 0.0, dt_ramp2 = 0.0, dp_ramp2 = 0.0;
    
    sap_Ndof_no_cruise_calculator nc_calc(end_position - start_position, start_velocity, end_velocity, max_velocity, max_acceleration);
    double peak_vel_low = -sign_p1_p0 * max_velocity;
    double peak_vel_hi  =  sign_p1_p0 * max_velocity;
    
    double delta_first_order = nc_calc(peak_vel_hi);
    if( delta_first_order > 0.0 ) {
      peak_velocity = peak_vel_hi;
      sap_Ndof_compute_ramp_dist_and_time(start_velocity, peak_velocity, max_velocity, max_acceleration, dp_ramp1, dt_ramp1);
      sap_Ndof_compute_ramp_dist_and_time(peak_velocity, end_velocity, max_velocity, max_acceleration, dp_ramp2, dt_ramp2);
      return delta_first_order + dt_ramp1 + dt_ramp2;
    };
    
    delta_first_order = nc_calc(peak_vel_low);
    if( delta_first_order < 0.0 ) {
      peak_velocity = peak_vel_low;
      sap_Ndof_compute_ramp_dist_and_time(start_velocity, peak_velocity, max_velocity, max_acceleration, dp_ramp1, dt_ramp1);
      sap_Ndof_compute_ramp_dist_and_time(peak_velocity, end_velocity, max_velocity, max_acceleration, dp_ramp2, dt_ramp2);
      return -delta_first_order + dt_ramp1 + dt_ramp2;
    };
    
//     ford3_method(peak_vel_low, peak_vel_hi, nc_calc, 1e-8);
    bisection_method(peak_vel_low, peak_vel_hi, nc_calc, 1e-6 * max_velocity);
    
    peak_velocity = peak_vel_hi;
    sap_Ndof_compute_ramp_dist_and_time(start_velocity, peak_velocity, max_velocity, max_acceleration, dp_ramp1, dt_ramp1);
    sap_Ndof_compute_ramp_dist_and_time(peak_velocity, end_velocity, max_velocity, max_acceleration, dp_ramp2, dt_ramp2);
    return fabs(end_position - start_position - dp_ramp1 - dp_ramp2) + dt_ramp1 + dt_ramp2;
  };
  
  
  
  double sap_Ndof_compute_min_delta_time(double start_position, double end_position,
                                         double start_velocity, double end_velocity,
                                         double& peak_velocity, 
                                         double max_velocity, double max_acceleration) {
    
    double min_delta_time = sap_Ndof_compute_min_delta_time_numsolve(
      start_position, end_position, start_velocity, end_velocity,
      peak_velocity, max_velocity, max_acceleration);
    
    return min_delta_time;
  };
  
  
  
  
  
  
  
  struct sap_Ndof_pos_diff_calculator {
    double dp, v1, v2, vmax, amax, dt;
    
    sap_Ndof_pos_diff_calculator(
      double a_dp, double a_v1, double a_v2, double a_vmax, double a_amax, double a_dt) : 
      dp(a_dp), v1(a_v1), v2(a_v2), vmax(a_vmax), amax(a_amax), dt(a_dt) { };
    
    double operator()(double vp) {
      using std::fabs;
      
      double dp1, dt1, dp2, dt2;
      sap_Ndof_compute_ramp_dist_and_time(v1, vp, vmax, amax, dp1, dt1);
      sap_Ndof_compute_ramp_dist_and_time(vp, v2, vmax, amax, dp2, dt2);
      return dp - dp1 - dp2 - vp / vmax * (dt - dt1 - dt2);
    };
    
    double get_delta_time_diff(double vp) {
      double dp1, dt1, dp2, dt2;
      sap_Ndof_compute_ramp_dist_and_time(v1, vp, vmax, amax, dp1, dt1);
      sap_Ndof_compute_ramp_dist_and_time(vp, v2, vmax, amax, dp2, dt2);
      return dt - dt1 - dt2;
    };
    
  };
  
  
  static void sap_Ndof_compute_peak_velocity_numsolve(
    double start_position, double end_position,
    double start_velocity, double end_velocity, double& peak_velocity, 
    double max_velocity, double max_acceleration, double delta_time) {
    
    using std::fabs;
    
    if( ( fabs(end_position - start_position) < 1e-6 * max_velocity ) &&
        ( fabs(end_velocity - start_velocity) < 1e-6 * max_acceleration ) ) {
      peak_velocity = start_velocity;
      return;
    };
    
    if( ( fabs(start_velocity) > max_velocity ) || ( fabs(end_velocity) > max_velocity ) ) {
      peak_velocity = 0.0;
      RK_NOTICE(1," Warning: violation of the velocity bounds was detected on SAP interpolations!");
      return;
    };
    
    double sign_p1_p0 = 1.0;
    if(start_position > end_position)
      sign_p1_p0 = -1.0;
    
    sap_Ndof_pos_diff_calculator pd_calc(
      end_position - start_position, start_velocity, end_velocity, 
      max_velocity, max_acceleration, delta_time);
    
    double prev_vp = 1.03 * sign_p1_p0 * max_velocity;
    double prev_pd = pd_calc(prev_vp);
    for(double cur_vp = prev_vp - 0.02 * sign_p1_p0 * max_velocity; cur_vp * sign_p1_p0 > -1.04 * max_velocity; cur_vp -= 0.02 * sign_p1_p0 * max_velocity) {
      double cur_pd = pd_calc(cur_vp);
      if( cur_pd * prev_pd < 0.0 ) {
        double orig_cur_vp = cur_vp;
//         ford3_method(prev_vp, cur_vp, pd_calc, 1e-7);
        bisection_method(prev_vp, cur_vp, pd_calc, 1e-8 * max_velocity);
        cur_vp = (prev_vp + cur_vp) * 0.5;
        cur_pd = pd_calc(cur_vp);
        if( (pd_calc.get_delta_time_diff(cur_vp) >= -1e-3 * max_velocity) && ( fabs(cur_pd) < 1e-3 * max_velocity ) ) {
          peak_velocity = cur_vp;
          return;
        } else {
//           std::cout << " Could not really finish the root-finding for the peak-velocity:\n"
//                     << "  got a delta-time error of " << pd_calc.get_delta_time_diff(cur_vp) << "\n"
//                     << "  got vp-interval: " << prev_vp << " -- mid: " << cur_vp << " .. diff = " << (cur_vp - prev_vp) << "\n"
//                     << "  corresponding to solutions: " << pd_calc(prev_vp) << " -- mid: " << cur_pd << std::endl;
          cur_vp = orig_cur_vp;
          cur_pd = pd_calc(cur_vp);
        };
      };
      prev_vp = cur_vp;
      prev_pd = cur_pd;
    };
    
    RK_NOTICE(1," Warning: There was no solution to the peak-velocity for the given delta-time!");
    peak_velocity = -sign_p1_p0 * max_velocity;
    return;
  };
  
  
  
  void sap_Ndof_compute_peak_velocity(double start_position, double end_position,
                                      double start_velocity, double end_velocity,
                                      double& peak_velocity, double max_velocity, 
                                      double max_acceleration, double delta_time) {
    
    sap_Ndof_compute_peak_velocity_numsolve(
      start_position, end_position, start_velocity, end_velocity,
      peak_velocity, max_velocity, max_acceleration, delta_time);
    
    
    double cf_pos, cf_vel, cf_acc, cf_desc_jerk;
    
    sap_Ndof_compute_interpolated_values_closedform(
      start_position, end_position, start_velocity, end_velocity,
      peak_velocity, max_velocity, max_acceleration,
      delta_time, delta_time,
      cf_pos, cf_vel, cf_acc, cf_desc_jerk);
    
    if( std::fabs(cf_pos - end_position) > 1e-3 ) {
      double dt1, dt2, dp1, dp2;
      
      sap_Ndof_compute_ramp_dist_and_time(start_velocity, peak_velocity, max_velocity, max_acceleration, dp1, dt1);
      sap_Ndof_compute_ramp_dist_and_time(peak_velocity, end_velocity, max_velocity, max_acceleration, dp2, dt2);
      
      sap_Ndof_pos_diff_calculator pd_calc(
        end_position - start_position, start_velocity, end_velocity, 
        max_velocity, max_acceleration, delta_time);
      
      std::cout << "The calculation of the peak velocity yielded a bad interpolated path!\n"
                << " Start position = " << start_position << "\n"
                << " End position   = " << end_position << "\n"
                << " Start velocity = " << start_velocity << "\n"
                << " End velocity   = " << end_velocity << "\n"
                << " Peak velocity  = " << peak_velocity << "\n"
                << " Delta-time     = " << delta_time << "\n"
                << " Delta-time-1   = " << dt1 << "\n"
                << " Delta-time-2   = " << dt2 << "\n"
                << " Delta-pos-1    = " << dp1 << "\n"
                << " Delta-pos-2    = " << dp2 << "\n"
                << " Calculated EDP = " << pd_calc(peak_velocity) << "\n"
                << " Actual EDP     = " << (cf_pos - end_position) << std::endl;
    };
    
    
  };
  
  
  
};


};

};






