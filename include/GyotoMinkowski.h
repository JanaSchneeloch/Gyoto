/**
 * \file GyotoMinkowski.h
 * \brief The Minkowski flat-space metric
 * 
 * Use </Cartesian> or </Spherical> to select the coordinate system
 * kind.
 */

/*
    Copyright 2014 Thibaut Paumard

    This file is part of Gyoto.

    Gyoto is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Gyoto is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Gyoto.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __GyotoMinkowski_H_
#define __GyotoMinkowski_H_

#include <iostream>
#include <fstream>

namespace Gyoto {
  namespace Metric { class Minkowski; }
}

#include <GyotoMetric.h>
#include <GyotoWorldline.h>
#include <GyotoSmartPointer.h>

#ifdef GYOTO_USE_XERCES
#include <GyotoRegister.h>
#endif

class Gyoto::Metric::Minkowski
: public Gyoto::Metric::Generic
{
  friend class Gyoto::SmartPointer<Gyoto::Metric::Minkowski>;

 private:
  
 public:
  // This is the bare minimum of what a Metric class must implement:
  Minkowski();
  virtual Minkowski* clone() const ;
  void gmunu(double g[4][4], const double * x) const ;
  void christoffel(double dst[4][4][4], const double * x) const ;
  virtual void setParameter(std::string, std::string, std::string);
#ifdef GYOTO_USE_XERCES
  virtual void fillElement(FactoryMessenger *fmp); ///< called from Factory
#endif

  // Those two are implemented as examples.
  double gmunu(const double * x, int mu, int nu) const ;
  double christoffel(const double coord[8], const int alpha, const int mu, 
		     const int nu) const ;


};

#endif