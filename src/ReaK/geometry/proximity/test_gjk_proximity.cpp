
/*
 *    Copyright 2011 Sven Mikael Persson
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

#include <ReaK/core/optimization/nl_interior_points_methods.hpp>

#include <ReaK/geometry/proximity/prox_fundamentals_3D.hpp>

#include <iostream>


using namespace ReaK;


pose_3D<double> a1 = pose_3D<double>(shared_ptr< pose_3D<double> >(), vect<double,3>(0.0,0.0,0.0), quaternion<double>(vect<double,4>(0.8,0.0,0.6,0.0)));
pose_3D<double> a2 = pose_3D<double>(shared_ptr< pose_3D<double> >(), vect<double,3>(0.0,3.0,5.0), quaternion<double>(vect<double,4>(0.8,-0.6,0.0,0.0)));
pose_3D<double> a3 = pose_3D<double>(shared_ptr< pose_3D<double> >(), vect<double,3>(10.0,-3.0,-2.0), quaternion<double>(vect<double,4>(1.0,0.0,0.0,0.0)));
pose_3D<double> a4 = pose_3D<double>(shared_ptr< pose_3D<double> >(), vect<double,3>(-3.0,-3.0,6.0), quaternion<double>(vect<double,4>(sqrt(3.0)/3.0,0.0,-sqrt(3.0)/3.0,sqrt(3.0)/3.0)));

pose_3D<double> a5 = pose_3D<double>(shared_ptr< pose_3D<double> >(), vect<double,3>(0.0,0.0,0.0), quaternion<double>(vect<double,4>(1.0,0.0,0.0,0.0)));
pose_3D<double> a6 = pose_3D<double>(shared_ptr< pose_3D<double> >(), vect<double,3>(0.5,0.0,4.0), quaternion<double>(vect<double,4>(1.0,0.0,0.0,0.0)));

shared_ptr< geom::cylinder > cy1 = shared_ptr< geom::cylinder >(new geom::cylinder("cy1", shared_ptr< pose_3D<double> >(), a1, 5.0, 0.5));
shared_ptr< geom::cylinder > cy2 = shared_ptr< geom::cylinder >(new geom::cylinder("cy2", shared_ptr< pose_3D<double> >(), a1, 10.0, 0.25));
shared_ptr< geom::cylinder > cy3 = shared_ptr< geom::cylinder >(new geom::cylinder("cy3", shared_ptr< pose_3D<double> >(), a1, 1.0, 2.0));
shared_ptr< geom::cylinder > cy4 = shared_ptr< geom::cylinder >(new geom::cylinder("cy4", shared_ptr< pose_3D<double> >(), a2, 5.0, 0.5));
shared_ptr< geom::cylinder > cy5 = shared_ptr< geom::cylinder >(new geom::cylinder("cy5", shared_ptr< pose_3D<double> >(), a3, 5.0, 0.5));
shared_ptr< geom::cylinder > cy6 = shared_ptr< geom::cylinder >(new geom::cylinder("cy6", shared_ptr< pose_3D<double> >(), a4, 5.0, 0.5));

shared_ptr< geom::cylinder > cy7 = shared_ptr< geom::cylinder >(new geom::cylinder("cy7", shared_ptr< pose_3D<double> >(), a6, 5.0, 0.5));

shared_ptr< geom::box > bx1 = shared_ptr< geom::box >(new geom::box("bx1", shared_ptr< pose_3D<double> >(), a1, vect<double,3>(1.0,2.0,1.0)));
shared_ptr< geom::box > bx2 = shared_ptr< geom::box >(new geom::box("bx2", shared_ptr< pose_3D<double> >(), a1, vect<double,3>(4.0,1.0,10.0)));
shared_ptr< geom::box > bx3 = shared_ptr< geom::box >(new geom::box("bx3", shared_ptr< pose_3D<double> >(), a1, vect<double,3>(4.0,4.0,1.0)));
shared_ptr< geom::box > bx4 = shared_ptr< geom::box >(new geom::box("bx4", shared_ptr< pose_3D<double> >(), a2, vect<double,3>(4.0,2.0,2.0)));
shared_ptr< geom::box > bx5 = shared_ptr< geom::box >(new geom::box("bx5", shared_ptr< pose_3D<double> >(), a3, vect<double,3>(4.0,2.0,2.0)));
shared_ptr< geom::box > bx6 = shared_ptr< geom::box >(new geom::box("bx6", shared_ptr< pose_3D<double> >(), a4, vect<double,3>(4.0,2.0,2.0)));

shared_ptr< geom::box > bx7 = shared_ptr< geom::box >(new geom::box("bx7", shared_ptr< pose_3D<double> >(), a5, vect<double,3>(4.0,2.0,4.0)));


template <typename BF1, typename BF2>
double get_brute_force_dist(BF1& bf1, BF2& bf2) {
  double min_dist_brute = std::numeric_limits<double>::infinity();
  for(double i = 0; i < 2.0 * M_PI; i += M_PI / 8.0) {
    for(double j = 0; j < M_PI; j += M_PI / 16.0) {
      vect<double,3> u(cos(i)*sin(j), sin(i)*sin(j), cos(j));
      vect<double,3> pt1 = bf1(u);
      for(double k = 0; k < 2.0 * M_PI; k += M_PI / 8.0) {
        for(double l = 0; l < M_PI; l += M_PI / 16.0) {
          vect<double,3> v(cos(k)*sin(l), sin(k)*sin(l), cos(l));
          vect<double,3> pt2 = bf2(v);
          double d = norm_2(pt2 - pt1);
          if( d < min_dist_brute )
            min_dist_brute = d;
        };
      };
    };
  };
  return min_dist_brute;
};        


struct proximity_solver {
  shared_ptr< geom::shape_3D > mShape1;
  shared_ptr< geom::shape_3D > mShape2;
  
  proximity_solver(const shared_ptr< geom::shape_3D >& aShape1, 
                   const shared_ptr< geom::shape_3D >& aShape2) : 
                   mShape1(aShape1), mShape2(aShape2) {};
  
  void operator()() {
    geom::proximity_record_3D result;
    vect<double,3> v1, v2;
    using std::sin; using std::cos;
    
    if(mShape1->getObjectType() == geom::box::getStaticObjectType()) {
      shared_ptr< geom::box > bx1 = rtti::rk_dynamic_ptr_cast< geom::box >(mShape1);
      if(mShape2->getObjectType() == geom::box::getStaticObjectType()) {
        // box-box case.
        shared_ptr< geom::box > bx2 = rtti::rk_dynamic_ptr_cast< geom::box >(mShape2);
        
        std::cout << "Checking proximity between Box '" << bx1->getName() 
                  << "' and Box '" << bx2->getName() << "'..." << std::endl;
        
        result = geom::findProximityByGJKEPA(geom::box_support_func(bx1), 
                                             geom::box_support_func(bx2));
        
        v1 = mShape1->getPose().rotateToGlobal(mShape1->getPose().transformFromGlobal(result.mPoint1));
        v2 = mShape2->getPose().rotateToGlobal(mShape2->getPose().transformFromGlobal(result.mPoint2));
        
        geom::box_boundary_func bf1(bx1);
        geom::box_boundary_func bf2(bx2);
        v1 = bf1(v1);
        v2 = bf2(v2);
        
        std::cout << " which has brute-force approximate min-dist of: " 
                  << get_brute_force_dist(bf1, bf2) << std::endl;
        
      } else {
        // box-cylinder case.
        shared_ptr< geom::cylinder > cy2 = rtti::rk_dynamic_ptr_cast< geom::cylinder >(mShape2);
        
        std::cout << "Checking proximity between Box '" << bx1->getName() 
                  << "' and Cylinder '" << cy2->getName() << "'..." << std::endl;
        
        result = geom::findProximityByGJKEPA(geom::box_support_func(bx1),
                                             geom::cylinder_support_func(cy2));
        
        v1 = mShape1->getPose().rotateToGlobal(mShape1->getPose().transformFromGlobal(result.mPoint1));
        v2 = mShape2->getPose().rotateToGlobal(mShape2->getPose().transformFromGlobal(result.mPoint2));
        
        geom::box_boundary_func bf1(bx1);
        geom::cylinder_boundary_func bf2(cy2);
        v1 = bf1(v1);
        v2 = bf2(v2);
        
        std::cout << " which has brute-force approximate min-dist of: " 
                  << get_brute_force_dist(bf1, bf2) << std::endl;
        
      };
    } else {
      shared_ptr< geom::cylinder > cy1 = rtti::rk_dynamic_ptr_cast< geom::cylinder >(mShape1);
      if(mShape2->getObjectType() == geom::box::getStaticObjectType()) {
        // cylinder-box case.
        shared_ptr< geom::box > bx2 = rtti::rk_dynamic_ptr_cast< geom::box >(mShape2);
        
        std::cout << "Checking proximity between Cylinder '" << cy1->getName() 
                  << "' and Box '" << bx2->getName() << "'..." << std::endl;
        
        result = geom::findProximityByGJKEPA(geom::cylinder_support_func(cy1),
                                             geom::box_support_func(bx2));
        
        v1 = mShape1->getPose().rotateToGlobal(mShape1->getPose().transformFromGlobal(result.mPoint1));
        v2 = mShape2->getPose().rotateToGlobal(mShape2->getPose().transformFromGlobal(result.mPoint2));
        
        geom::cylinder_boundary_func bf1(cy1);
        geom::box_boundary_func bf2(bx2);
        v1 = bf1(v1);
        v2 = bf2(v2);
        
        std::cout << " which has brute-force approximate min-dist of: " 
                  << get_brute_force_dist(bf1, bf2) << std::endl;
        
      } else {
        // cylinder-cylinder case.
        shared_ptr< geom::cylinder > cy2 = rtti::rk_dynamic_ptr_cast< geom::cylinder >(mShape2);
        
        std::cout << "Checking proximity between Cylinder '" << cy1->getName() 
                  << "' and Cylinder '" << cy2->getName() << "'..." << std::endl;
        
        result = geom::findProximityByGJKEPA(geom::cylinder_support_func(cy1), 
                                             geom::cylinder_support_func(cy2));
        
        v1 = mShape1->getPose().rotateToGlobal(mShape1->getPose().transformFromGlobal(result.mPoint1));
        v2 = mShape2->getPose().rotateToGlobal(mShape2->getPose().transformFromGlobal(result.mPoint2));
        
        geom::cylinder_boundary_func bf1(cy1);
        geom::cylinder_boundary_func bf2(cy2);
        v1 = bf1(v1);
        v2 = bf2(v2);
        
        std::cout << " which has brute-force approximate min-dist of: " 
                  << get_brute_force_dist(bf1, bf2) << std::endl;
        
      };
    };
    
    std::cout << "  -- Solution distance is: " << result.mDistance << std::endl;
    
    std::cout << "  -- Solution point-1 is: " << result.mPoint1 << std::endl;
    std::cout << "  -- Boundary at point-1 is: " << v1 << std::endl;
    
    std::cout << "  -- Solution point-2 is: " << result.mPoint2 << std::endl;
    std::cout << "  -- Boundary at point-2 is: " << v2 << std::endl;
    
    std::cout << "  -- Distance between pt-1 and pt-2 is: " 
              << norm_2(result.mPoint1 - result.mPoint2) << std::endl;
    
  };
  
};


int main() {
  
  std::vector< proximity_solver > prox_tasks;
  prox_tasks.push_back(proximity_solver(cy1,cy4));
  prox_tasks.push_back(proximity_solver(cy1,cy5));
  prox_tasks.push_back(proximity_solver(cy1,cy6));
  prox_tasks.push_back(proximity_solver(cy2,cy4));
  prox_tasks.push_back(proximity_solver(cy3,cy4));
  
  prox_tasks.push_back(proximity_solver(bx1,bx4));
  prox_tasks.push_back(proximity_solver(bx1,bx5));
  prox_tasks.push_back(proximity_solver(bx1,bx6));
  prox_tasks.push_back(proximity_solver(bx2,bx4));
  prox_tasks.push_back(proximity_solver(bx3,bx4));
  
  prox_tasks.push_back(proximity_solver(bx1,cy4));
  prox_tasks.push_back(proximity_solver(bx2,cy4));
  prox_tasks.push_back(proximity_solver(bx3,cy4));
  
  prox_tasks.push_back(proximity_solver(cy1,bx4));
  prox_tasks.push_back(proximity_solver(cy2,bx4));
  prox_tasks.push_back(proximity_solver(cy3,bx4));
  
  prox_tasks.push_back(proximity_solver(cy7,bx7));
  
  for(std::size_t i = 0; i < prox_tasks.size(); ++i) {
    
    prox_tasks[i]();
    
  };
  
  return 0;
};







