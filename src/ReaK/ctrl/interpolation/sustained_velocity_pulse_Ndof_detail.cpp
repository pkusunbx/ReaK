
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

#include "sustained_velocity_pulse_Ndof_detail.hpp"

#include <cmath>


#include "root_finders/bisection_method.hpp"
#include "root_finders/secant_method.hpp"


#include <fstream>


namespace ReaK {

namespace pp {
  
  
namespace detail {

  
  
  
  static void svp_Ndof_compute_interpolated_values_closedform(
    double start_position, double end_position,
    double start_velocity, double end_velocity,
    double peak_velocity, double max_velocity, 
    double dt, double dt_total,
    double& result_pos, double& result_vel, double& result_desc_acc) {
    
    using std::fabs;
    using std::sqrt;
    
    double dv1 = peak_velocity - start_velocity;
    double dv2 = end_velocity - peak_velocity;
    result_pos = start_position;
    result_vel = start_velocity;
    result_desc_acc = 0.0;
    
    double dt_vp1 = fabs(dv1);
    double sgn_vp1 = 1.0;
    if( dv1 < 0.0 ) sgn_vp1 = -1.0;
    
    double dt_vp2 = fabs(dv2);
    double sgn_vp2 = 1.0;
    if( dv2 < 0.0 ) sgn_vp2 = -1.0;
    
    dt_total -= dt_vp2 + dt_vp1;
    
    if( dt < dt_vp1 ) {
      //Segment 1: in the constant accel phase of velocity ramp-up.
      result_pos += ( ( result_vel + 0.5 * dt * sgn_vp1 ) * dt ) / max_velocity;
      result_vel += dt * sgn_vp1;
      result_desc_acc  = sgn_vp1;
      return;
    };
    
    result_pos += ( ( start_velocity + peak_velocity ) * 0.5 * dt_vp1 ) / max_velocity;
    result_vel  = peak_velocity;
    result_desc_acc  = 0.0;
    
    if( dt < (dt_vp1 + dt_total) ) {
      //Segment 2: in the cruise phase.
      dt -= dt_vp1;
      result_pos += dt * peak_velocity / max_velocity;
      return;
    };
    
    result_pos += dt_total * peak_velocity / max_velocity;
    
    if( dt < (dt_vp1 + dt_total + dt_vp2) ) {
      //Segment 3: in the constant accel phase of velocity ramp-down.
      dt -= dt_vp1 + dt_total;
      result_pos += ( ( result_vel + 0.5 * dt * sgn_vp2 ) * dt ) / max_velocity;
      result_vel += dt * sgn_vp2;
      result_desc_acc  = sgn_vp2;
      return;
    };
    
    result_desc_acc  = 0.0;
    
    // Expected end-conditions:
    
//     result_pos = end_position;
//     result_vel = end_velocity;
    
    // Resulting position / velocity formulae:
    
    result_vel += dv2;
    
    result_pos = start_position + ( peak_velocity * dt_total 
      + 0.5 * ( start_velocity + peak_velocity ) * dt_vp1 
      + 0.5 * ( peak_velocity  + end_velocity  ) * dt_vp2 ) / max_velocity;
    
  };
  
  
  
  void svp_Ndof_compute_interpolated_values(
    double start_position, double end_position,
    double start_velocity, double end_velocity,
    double peak_velocity, double max_velocity,
    double dt, double dt_total,
    double& result_pos, double& result_vel, double& result_desc_acc) {
    
    svp_Ndof_compute_interpolated_values_closedform(
      start_position, end_position, start_velocity, end_velocity,
      peak_velocity, max_velocity, dt, dt_total,
      result_pos, result_vel, result_desc_acc);
    
  };
  
  
  
  
  
  inline void svp_Ndof_compute_ramp_dist_and_time(
    double v1, double v2, double vmax, 
    double& d_pos, double& dt) {
    using std::fabs;
    
    dt = fabs(v2 - v1);
    d_pos = 0.5 * dt * (v1 + v2) / vmax;
  };
  
  struct svp_Ndof_no_cruise_calculator {
    double dp, v1, v2, vmax;
    
    svp_Ndof_no_cruise_calculator(
      double a_dp, double a_v1, double a_v2, double a_vmax) : 
      dp(a_dp), v1(a_v1), v2(a_v2), vmax(a_vmax) { };
    
    double operator()(double vp) {
      double dp1, dt1, dp2, dt2;
      svp_Ndof_compute_ramp_dist_and_time(v1, vp, vmax, dp1, dt1);
      svp_Ndof_compute_ramp_dist_and_time(vp, v2, vmax, dp2, dt2);
      
      if( dp < 0.0 )
        return dp1 + dp2 - dp;
      else 
        return dp - dp1 - dp2;
    };
    
  };
  
  
  static double svp_Ndof_compute_min_delta_time_numsolve(
    double start_position, double end_position,
    double start_velocity, double end_velocity,
    double& peak_velocity, double max_velocity) {
    using std::fabs;
    using std::sqrt;
    
    if( ( fabs(end_position - start_position) < 1e-6 * max_velocity ) &&
        ( fabs(end_velocity - start_velocity) < 1e-6 * max_velocity ) ) {
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
    
    svp_Ndof_no_cruise_calculator nc_calc(end_position - start_position, start_velocity, end_velocity, max_velocity);
    double peak_vel_low = -sign_p1_p0 * max_velocity;
    double peak_vel_hi  =  sign_p1_p0 * max_velocity;
    
    double delta_first_order = nc_calc(peak_vel_hi);
    if( delta_first_order > 0.0 ) {
      peak_velocity = peak_vel_hi;
      svp_Ndof_compute_ramp_dist_and_time(start_velocity, peak_velocity, max_velocity, dp_ramp1, dt_ramp1);
      svp_Ndof_compute_ramp_dist_and_time(peak_velocity, end_velocity, max_velocity, dp_ramp2, dt_ramp2);
      return delta_first_order + dt_ramp1 + dt_ramp2;
    };
    
    delta_first_order = nc_calc(peak_vel_low);
    if( delta_first_order < 0.0 ) {
      peak_velocity = peak_vel_low;
      svp_Ndof_compute_ramp_dist_and_time(start_velocity, peak_velocity, max_velocity, dp_ramp1, dt_ramp1);
      svp_Ndof_compute_ramp_dist_and_time(peak_velocity, end_velocity, max_velocity, dp_ramp2, dt_ramp2);
      return -delta_first_order + dt_ramp1 + dt_ramp2;
    };
    
//     ford3_method(peak_vel_low, peak_vel_hi, nc_calc, 1e-8);
    bisection_method(peak_vel_low, peak_vel_hi, nc_calc, 1e-6 * max_velocity);
    
    peak_velocity = peak_vel_hi;
    svp_Ndof_compute_ramp_dist_and_time(start_velocity, peak_velocity, max_velocity, dp_ramp1, dt_ramp1);
    svp_Ndof_compute_ramp_dist_and_time(peak_velocity, end_velocity, max_velocity, dp_ramp2, dt_ramp2);
    return fabs(end_position - start_position - dp_ramp1 - dp_ramp2) + dt_ramp1 + dt_ramp2;
  };
  
  
  
  double svp_Ndof_compute_min_delta_time(double start_position, double end_position,
                                         double start_velocity, double end_velocity,
                                         double& peak_velocity, double max_velocity) {
    
    double min_delta_time = svp_Ndof_compute_min_delta_time_numsolve(
      start_position, end_position, start_velocity, end_velocity, peak_velocity, max_velocity);
    
    return min_delta_time;
  };
  
  
  
  
  
  
  
  struct svp_Ndof_pos_diff_calculator {
    double dp, v1, v2, vmax, dt;
    
    svp_Ndof_pos_diff_calculator(
      double a_dp, double a_v1, double a_v2, double a_vmax, double a_dt) : 
      dp(a_dp), v1(a_v1), v2(a_v2), vmax(a_vmax), dt(a_dt) { };
    
    double operator()(double vp) {
      double dp1, dt1, dp2, dt2;
      svp_Ndof_compute_ramp_dist_and_time(v1, vp, vmax, dp1, dt1);
      svp_Ndof_compute_ramp_dist_and_time(vp, v2, vmax, dp2, dt2);
      if( dt > dt1 + dt2 )
        return dp - dp1 - dp2 - vp / vmax * (dt - dt1 - dt2);
      else
        return dp1 + dp2 + vp / vmax * (dt - dt1 - dt2) - dp;
    };
    
    double get_delta_time_diff(double vp) {
      double dp1, dt1, dp2, dt2;
      svp_Ndof_compute_ramp_dist_and_time(v1, vp, vmax, dp1, dt1);
      svp_Ndof_compute_ramp_dist_and_time(vp, v2, vmax, dp2, dt2);
      return dt - dt1 - dt2;
    };
    
  };
  
#if 0
  static void svp_Ndof_compute_peak_velocity_numsolve(
    double start_position, double end_position,
    double start_velocity, double end_velocity, double& peak_velocity, 
    double max_velocity, double delta_time) {
    
    using std::fabs;
    
    if( ( fabs(end_position - start_position) < 1e-6 * max_velocity ) &&
        ( fabs(end_velocity - start_velocity) < 1e-6 * max_velocity ) ) {
      peak_velocity = start_velocity;
      return;
    };
    
    if( ( fabs(start_velocity) > max_velocity ) || ( fabs(end_velocity) > max_velocity ) ) {
      peak_velocity = 0.0;
      RK_NOTICE(1," Warning: violation of the velocity bounds was detected on SVP interpolations!");
      return;
    };
    
    double sign_p1_p0 = 1.0;
    if(start_position > end_position)
      sign_p1_p0 = -1.0;
    
    svp_Ndof_pos_diff_calculator pd_calc(
      end_position - start_position, start_velocity, end_velocity, max_velocity, delta_time);
    
    
    
    static std::ofstream dat_output("svp_pd_calc.txt");
    dat_output << "%----------------------------------------------\n";
    for(double cur_vp = 1.05 * sign_p1_p0 * max_velocity; cur_vp * sign_p1_p0 > -1.055 * max_velocity; cur_vp -= 0.01 * sign_p1_p0 * max_velocity)
      dat_output << cur_vp << " " << pd_calc(cur_vp) << " " << pd_calc.get_delta_time_diff(cur_vp) << "\n";
    dat_output.flush();
    
    
    
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
          std::cout << " Could not really finish the root-finding for the peak-velocity:\n"
                    << "  got a delta-time error of " << pd_calc.get_delta_time_diff(cur_vp) << "\n"
                    << "  got vp-interval: " << prev_vp << " -- mid: " << cur_vp << " .. diff = " << (cur_vp - prev_vp) << "\n"
                    << "  corresponding to solutions: " << pd_calc(prev_vp) << " -- mid: " << cur_pd << std::endl;
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
#endif
  
  
  
  
  void svp_Ndof_compute_peak_velocity_closedform(double start_position, double end_position,
                                      double start_velocity, double end_velocity,
                                      double& peak_velocity, double max_velocity, double delta_time) {
    // NOTE: Assume that delta-time is larger than minimum reachable delta-time 
    //       (avoid checking for that, even if it could mean that vp is higher than maximum)
    
    using std::fabs;
    using std::sqrt;
    
    if( ( fabs(end_position - start_position) < 1e-6 * max_velocity ) &&
        ( fabs(end_velocity - start_velocity) < 1e-6 * max_velocity ) ) {
      peak_velocity = start_velocity;
      return;
    };
    
    if( ( fabs(start_velocity) > max_velocity ) || ( fabs(end_velocity) > max_velocity ) ) {
      peak_velocity = 0.0;
      RK_NOTICE(1," Warning: violation of the velocity bounds was detected on SVP interpolations!");
      return;
    };
    
    svp_Ndof_pos_diff_calculator pd_calc(
      end_position - start_position, start_velocity, end_velocity, max_velocity, delta_time);
    
    double sign_p1_p0 = 1.0;
    if(start_position > end_position)
      sign_p1_p0 = -1.0;
    
    // try to assume that vp more in the direction (p1-p0) than both v1 and v0 (i.e., ramp-up and ramp-down).
    double v0_v1_ts = start_velocity + end_velocity + delta_time * sign_p1_p0;
    double vm_p1_p0 = max_velocity * fabs(end_position - start_position);
    double vsqr_avg = 0.5 * (start_velocity * start_velocity + end_velocity * end_velocity);
    if(v0_v1_ts * v0_v1_ts >= 4.0 * (vm_p1_p0 + vsqr_avg)) {
      // then there is a real root to the quadratic equation.
      double r1 = 0.5 * (v0_v1_ts + sqrt(v0_v1_ts * v0_v1_ts - 4.0 * (vm_p1_p0 + vsqr_avg))) * sign_p1_p0;
      if((fabs(r1) < 1.001 * max_velocity) && (pd_calc.get_delta_time_diff(sign_p1_p0 * r1) >= -1e-3 * max_velocity) && 
         (r1 >= start_velocity * sign_p1_p0) && (r1 >= end_velocity * sign_p1_p0)) {
        peak_velocity = sign_p1_p0 * r1;
        return;
      };
      r1 = 0.5 * (v0_v1_ts - sqrt(v0_v1_ts * v0_v1_ts - 4.0 * (vm_p1_p0 + vsqr_avg))) * sign_p1_p0;
      if((fabs(r1) < 1.001 * max_velocity) && (pd_calc.get_delta_time_diff(sign_p1_p0 * r1) >= -1e-3 * max_velocity) && 
         (r1 >= start_velocity * sign_p1_p0) && (r1 >= end_velocity * sign_p1_p0)) {
        peak_velocity = sign_p1_p0 * r1;
        return;
      };
      // then, there was no suitable root in this region.
    } else if( fabs(v0_v1_ts * v0_v1_ts - 4.0 * (vm_p1_p0 + vsqr_avg)) < 1e-5 * max_velocity ) {
      // then there is a real root to the quadratic equation.
      double r1 = 0.5 * v0_v1_ts * sign_p1_p0;
      if((fabs(r1) < 1.001 * max_velocity) && (pd_calc.get_delta_time_diff(sign_p1_p0 * r1) >= -1e-3 * max_velocity) && 
         (r1 >= start_velocity * sign_p1_p0) && (r1 >= end_velocity * sign_p1_p0)) {
        peak_velocity = sign_p1_p0 * r1;
        return;
      };
    };
    
    // try to assume that vp is somewhere between v1 and v0 (i.e., ramp-up ramp-up or ramp-down ramp-down).
    if(end_velocity > start_velocity) {
      // ramp-up ramp-up
      v0_v1_ts = start_velocity - end_velocity + delta_time;
      vm_p1_p0 = max_velocity * (end_position - start_position);
      vsqr_avg = 0.5 * (start_velocity * start_velocity - end_velocity * end_velocity);
      if(fabs(v0_v1_ts) > 1e-6 * max_velocity) {
        peak_velocity = (vm_p1_p0 + vsqr_avg) / v0_v1_ts;
        if((fabs(peak_velocity) < 1.001 * max_velocity) && (pd_calc.get_delta_time_diff(peak_velocity) >= -1e-3 * max_velocity) && 
           (peak_velocity >= start_velocity) && (peak_velocity <= end_velocity))
          return;
        // then, the solution doesn't fit the assumption.
      };
    } else {
      // ramp-down ramp-down
      v0_v1_ts = end_velocity - start_velocity + delta_time;
      vm_p1_p0 = max_velocity * (end_position - start_position);
      vsqr_avg = 0.5 * (end_velocity * end_velocity - start_velocity * start_velocity);
      if(fabs(v0_v1_ts) > 1e-6 * max_velocity) {
        peak_velocity = (vm_p1_p0 + vsqr_avg) / v0_v1_ts;
        if((fabs(peak_velocity) < 1.001 * max_velocity) && (pd_calc.get_delta_time_diff(peak_velocity) >= -1e-3 * max_velocity) && 
           (peak_velocity >= end_velocity) && (peak_velocity <= start_velocity))
          return;
        // then, the solution doesn't fit the assumption.
      };
    };
    
    // try to assume (the last case) that vp is less in the direction (p1-p0) and both v0 and v1.
    v0_v1_ts = start_velocity + end_velocity - delta_time * sign_p1_p0;
    vm_p1_p0 = max_velocity * fabs(end_position - start_position);
    vsqr_avg = 0.5 * (start_velocity * start_velocity + end_velocity * end_velocity);
    if(v0_v1_ts * v0_v1_ts > 4.0 * (vsqr_avg - vm_p1_p0)) {
      // then there is a real root to the quadratic equation.
      double r1 = 0.5 * (v0_v1_ts + sqrt(v0_v1_ts * v0_v1_ts - 4.0 * (vsqr_avg - vm_p1_p0))) * sign_p1_p0;
      if((fabs(r1) < 1.001 * max_velocity) && (pd_calc.get_delta_time_diff(sign_p1_p0 * r1) >= -1e-3 * max_velocity) && 
         (r1 <= start_velocity * sign_p1_p0) && (r1 <= end_velocity * sign_p1_p0)) {
        peak_velocity = sign_p1_p0 * r1;
        return;
      };
      r1 = 0.5 * (v0_v1_ts - sqrt(v0_v1_ts * v0_v1_ts - 4.0 * (vsqr_avg - vm_p1_p0))) * sign_p1_p0;
      if((fabs(r1) < 1.001 * max_velocity) && (pd_calc.get_delta_time_diff(sign_p1_p0 * r1) >= -1e-3 * max_velocity) && 
         (r1 <= start_velocity * sign_p1_p0) && (r1 <= end_velocity * sign_p1_p0)) {
        peak_velocity = sign_p1_p0 * r1;
        return;
      };
      // then, there was no suitable root in this region.
    } else if( fabs(v0_v1_ts * v0_v1_ts - 4.0 * (vsqr_avg - vm_p1_p0)) < 1e-5 * max_velocity ) {
      // then there is a real root to the quadratic equation.
      double r1 = 0.5 * v0_v1_ts * sign_p1_p0;
      if((fabs(r1) < 1.001 * max_velocity) && (pd_calc.get_delta_time_diff(sign_p1_p0 * r1) >= -1e-3 * max_velocity) && 
         (r1 <= start_velocity * sign_p1_p0) && (r1 <= end_velocity * sign_p1_p0)) {
        peak_velocity = sign_p1_p0 * r1;
        return;
      };
    };
    
    // What the fuck!! This point should never be reached, unless the motion is completely impossible:
    RK_NOTICE(1," Warning: There was no solution to the peak-velocity for the given delta-time!");
    peak_velocity = -sign_p1_p0 * max_velocity;
    return;
  };
  
  
  
  
  
  
  void svp_Ndof_compute_peak_velocity(double start_position, double end_position,
                                      double start_velocity, double end_velocity,
                                      double& peak_velocity, double max_velocity, double delta_time) {
    /*
    svp_Ndof_compute_peak_velocity_numsolve(
      start_position, end_position, start_velocity, end_velocity,
      peak_velocity, max_velocity, delta_time);
    */
    
    svp_Ndof_compute_peak_velocity_closedform(
      start_position, end_position, start_velocity, end_velocity,
      peak_velocity, max_velocity, delta_time);
    
    double cf_pos, cf_vel, cf_desc_acc;
    
    svp_Ndof_compute_interpolated_values_closedform(
      start_position, end_position, start_velocity, end_velocity,
      peak_velocity, max_velocity, delta_time, delta_time,
      cf_pos, cf_vel, cf_desc_acc);
    
    if( std::fabs(cf_pos - end_position) > 1e-3 ) {
      double dt1, dt2, dp1, dp2;
      
      svp_Ndof_compute_ramp_dist_and_time(start_velocity, peak_velocity, max_velocity, dp1, dt1);
      svp_Ndof_compute_ramp_dist_and_time(peak_velocity, end_velocity, max_velocity, dp2, dt2);
      
      svp_Ndof_pos_diff_calculator pd_calc(
        end_position - start_position, start_velocity, end_velocity, 
        max_velocity, delta_time);
      
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






