/**
 * \file prox_rectangle_rectangle.hpp
 *
 * This library declares a class for proximity queries between two rectangles.
 *
 * \author Mikael Persson, <mikael.s.persson@gmail.com>
 * \date April 2012
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

#ifndef REAK_PROX_RECTANGLE_RECTANGLE_HPP
#define REAK_PROX_RECTANGLE_RECTANGLE_HPP

#include "proximity_finder_2D.hpp"

#include <ReaK/geometry/shapes/rectangle.hpp>

/** Main namespace for ReaK */
namespace ReaK {

/** Main namespace for ReaK.Geometry */
namespace geom {


/**
 * This class is for proximity queries between two rectangles.
 */
class prox_rectangle_rectangle : public proximity_finder_2D {
  protected:
    
    const rectangle* mRectangle1;
    const rectangle* mRectangle2;
    
    static void computeProximityOfPoint(const rectangle&, const pose_2D<double>&, 
                                        const vect<double,2>&, vect<double,2>&, double&);
    
  public:
    
    /** This function performs the proximity query on its associated shapes. */
    virtual void computeProximity(const shape_2D_precompute_pack& aPack1, 
                                  const shape_2D_precompute_pack& aPack2);
    
    /** 
     * Default constructor.
     * \param aRectangle1 The first rectangle involved in the proximity query.
     * \param aRectangle2 The second rectangle involved in the proximity query.
     */
    prox_rectangle_rectangle(const rectangle* aRectangle1 = NULL,
                             const rectangle* aRectangle2 = NULL);
    
    /** Destructor. */
    virtual ~prox_rectangle_rectangle() { };
    
};


};

};

#endif

