/*
    Copyright 2011-2016, 2018-2020 Frederic Vincent, Thibaut Paumard

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

// GYOTO HEADERS
#include "GyotoUtils.h"
#include "GyotoAstrobj.h"
#include "GyotoMetric.h"
#include "GyotoPhoton.h"
#include "GyotoRegister.h"
#include "GyotoFactoryMessenger.h"
#include "GyotoProperty.h"

// SYSTEM HEADERS
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <cfloat>
#include <cmath>
#include <sstream>
#include <limits>

// NAMESPACES
using namespace std;
using namespace Gyoto;
using namespace Gyoto::Astrobj;

Register::Entry* Gyoto::Astrobj::Register_ = NULL;

GYOTO_PROPERTY_START(Gyoto::Astrobj::Generic,
   "Whatever emits or absorbs light.")
GYOTO_PROPERTY_METRIC(Generic, Metric, metric,
   "The geometry of space-time at this end of the Universe.")
GYOTO_PROPERTY_DOUBLE_UNIT(Generic, RMax, rMax,
   "Maximum distance from the centre of mass (geometrical units).")
GYOTO_PROPERTY_DOUBLE_UNIT(Generic, DeltaMaxInsideRMax, deltaMaxInsideRMax,
   "Maximum step for Photon integration inside RMax (geometrical units).")
GYOTO_PROPERTY_BOOL(Generic, Redshift, NoRedshift, redshift,
    "Whether to take redshift into account.")
GYOTO_PROPERTY_BOOL(Generic, ShowShadow, NoShowShadow, showshadow,
    "Whether to highlight the shadow region on the image.")
GYOTO_PROPERTY_BOOL(Generic, OpticallyThin, OpticallyThick, opticallyThin,
    "Whether the object should be considered optically thin or thick.")
GYOTO_PROPERTY_END(Generic, Object::properties)

Generic::Generic(string kin) :
  SmartPointee(), Object(kin),
  __defaultfeatures(0),
  gg_(NULL), rmax_(DBL_MAX), deltamaxinsidermax_(1.), flag_radtransf_(0),
  noredshift_(0), shadow_(0)
{
#if GYOTO_DEBUG_ENABLED
  GYOTO_DEBUG << endl;
#endif
}

Generic::Generic() :
  SmartPointee(), Object("Default"),
  __defaultfeatures(0),
  gg_(NULL), rmax_(DBL_MAX), deltamaxinsidermax_(1.), flag_radtransf_(0),
  noredshift_(0), shadow_(0)
{
#if GYOTO_DEBUG_ENABLED
  GYOTO_DEBUG << endl;
#endif
}

Generic::Generic(double radmax) :
  SmartPointee(), Object("Default"),
  __defaultfeatures(0),
  gg_(NULL), rmax_(radmax), deltamaxinsidermax_(1.), flag_radtransf_(0),
  noredshift_(0), shadow_(0)
{
#if GYOTO_DEBUG_ENABLED
  GYOTO_DEBUG << endl;
#endif
}

Generic::Generic(const Generic& orig) :
  SmartPointee(orig), Object(orig),
  __defaultfeatures(orig.__defaultfeatures),
  gg_(NULL),
  rmax_(orig.rmax_),
  deltamaxinsidermax_(orig.deltamaxinsidermax_),
  flag_radtransf_(orig.flag_radtransf_),
  noredshift_(orig.noredshift_), shadow_(orig.shadow_)
{
#if GYOTO_DEBUG_ENABLED
  GYOTO_DEBUG << endl;
#endif
    if (orig.gg_()) {
#if GYOTO_DEBUG_ENABLED
      GYOTO_DEBUG << "orig had a metric, cloning"<< endl;
#endif
      gg_=orig.gg_->clone();
    }
#if GYOTO_DEBUG_ENABLED
    GYOTO_DEBUG << "done" << endl;
#endif
}

Generic::~Generic() {
#if GYOTO_DEBUG_ENABLED
  GYOTO_DEBUG << endl;
#endif
}

SmartPointer<Metric::Generic> Generic::metric() const { return gg_; }
void Generic::metric(SmartPointer<Metric::Generic> gg) {gg_=gg;}

const string Generic::kind() const { return kind_; }

double Generic::rMax() { return rmax_; }
double Generic::rMax() const { return rmax_; }
double Generic::rMax(string const &unit) {
  return Units::FromGeometrical(rMax(), unit, gg_); }
double Generic::rMax(string const &unit) const {
  return Units::FromGeometrical(rMax(), unit, gg_); }
void Generic::rMax(double val) { rmax_=val; }
void Generic::rMax(double val, string const &unit) {
  rMax(Units::ToGeometrical(val, unit, gg_)); }

double Generic::deltaMaxInsideRMax() const { return deltamaxinsidermax_; }
double Generic::deltaMaxInsideRMax(string const &unit) const {
  return Units::FromGeometrical(deltaMaxInsideRMax(), unit, gg_); }
void Generic::deltaMaxInsideRMax(double val) { deltamaxinsidermax_=val; }
void Generic::deltaMaxInsideRMax(double val, string const &unit) {
  deltaMaxInsideRMax(Units::ToGeometrical(val, unit, gg_)); }

#ifdef GYOTO_USE_XERCES
void Generic::setParameters(FactoryMessenger *fmp) {
  if (fmp) metric(fmp->metric());
  Object::setParameters(fmp);
}
#endif


void Generic::opticallyThin(bool flag) {flag_radtransf_=flag;}
bool Generic::opticallyThin() const {return flag_radtransf_;}

void Generic::showshadow(bool flag) {shadow_=flag;}
bool Generic::showshadow() const {return shadow_;}

void Generic::redshift(bool flag) {noredshift_=!flag;}
bool Generic::redshift() const {return !noredshift_;}

void Generic::processHitQuantities(Photon * ph, state_t const &coord_ph_hit,
				     double const * coord_obj_hit, double dt,
				     Properties* data) const {
#if GYOTO_DEBUG_ENABLED
  GYOTO_DEBUG << endl;
#endif
  /*
      NB: freqObs is the observer's frequency chosen in
      Screen::getRayCoord for the actual computation of the geodesic ;
      the physical value of nuobs will be used in spectrum
      computations by resorting to the xml specifications of the user
      (see below) ; this freqObs is used to transform the null
      worldline parameter dlambda (see below)
  */
  double freqObs=ph->freqObs(); // this is a useless quantity, always 1
  SmartPointer<Spectrometer::Generic> spr = ph -> spectrometer();
  size_t nbnuobs = spr() ? spr -> nSamples() : 0 ;
  double const * const nuobs = nbnuobs ? spr -> getMidpoints() : NULL;
  double dlambda = dt/coord_ph_hit[4]; //dlambda = dt/tdot
  //  cout << "freqObs="<<freqObs<<endl;
  double ggredm1 = -gg_->ScalarProd(&coord_ph_hit[0],coord_obj_hit+4,
				    &coord_ph_hit[4]);// / 1.;
                                       //this is nu_em/nu_obs
                                       // for nuobs=1. Hz
  if (noredshift_) ggredm1=1.;
  double ggred = 1./ggredm1;           //this is nu_obs/nu_em
  double dsem = dlambda*ggredm1; // * 1Hz ?
  double inc =0.;

  if (data) {
#if GYOTO_DEBUG_ENABLED
  GYOTO_DEBUG << "data requested. "
	      << "freqObs=" << freqObs << ", ggredm1=" << ggredm1
	      << ", ggred=" << ggred
	      << endl;
#endif

    if (data->redshift) {
      *data->redshift=ggred;
#if GYOTO_DEBUG_ENABLED
      GYOTO_DEBUG_EXPR(*data->redshift);
#endif
    }
    if (data->time) {
      *data->time=coord_ph_hit[0];
#if GYOTO_DEBUG_ENABLED
      GYOTO_DEBUG_EXPR(*data->time);
#endif
    }
    if (data->impactcoords && data->impactcoords[0]==DBL_MAX) {
      if (coord_ph_hit.size() > 8) GYOTO_ERROR("ImpactCoords is incompatible with parallel transport");
      memcpy(data->impactcoords, coord_obj_hit, 8 * sizeof(double));
      memcpy(data->impactcoords+8, &coord_ph_hit[0], 8 * sizeof(double));
    }
#if GYOTO_DEBUG_ENABLED
    GYOTO_DEBUG << "dlambda = (dt="<< dt << ")/(tdot="<< coord_ph_hit[4]
		<< ") = " << dlambda << ", dsem=" << dsem << endl;
#endif
    if (data->intensity) {
      /*
	Comment on intensity integration:
	Eq of radiative transfer used:
	dI_nu_em/ds_em = j_nu_em (assumes no absorption for
	simplicity) where : nu_em is measured by the emitter, ds_em
	is the proper length measured by the emitter corresponding
	to an increase dlambda of the parameter of the null
	worldline of the photon ; We have (see manual for demo) :
	ds_em = dlambda*nu_em

	BUT: dlambda is depending on the choice of freqObs (defined
	above) ; indeed we have: dlambda(freqObs) * freqObs = cst
	(i.e. independent of the choice of freqObs). Thus, for
	spectra computations where the observed frequency varies, we
	have to use the dlambda corresponding to the physical
	frequency chosen by the user, dlambda(nuobs), instead of
	dlambda(freqObs).  The conversion is easy : dlambda(nuobs) =
	dlambda(freqObs)*freqObs/nuobs Thus: ds_em =
	dlambda(nuobs)*nu_em = dlambda(freqObs)*freqObs*ggredm1

	Then, j_nu_em is computed by the emission() function of the
	Astrobj [NB: with rad. transfer emission() computes
	j_nu*dsem, without rad. transfer it computes I_nu, thus
	the result is always homogenous to intensity] Finally:
	I_nu_obs = I_nu_em*(nu_obs/nu_em)^3
      */

      //Intensity increment :
      GYOTO_DEBUG_EXPR(freqObs);
      GYOTO_DEBUG_EXPR(freqObs*ggredm1);
	inc = (emission(freqObs*ggredm1, dsem, coord_ph_hit, coord_obj_hit))
	  * (ph -> getTransmission(size_t(-1)))
	  * ggred*ggred*ggred; // I_nu/nu^3 invariant
#     ifdef HAVE_UDUNITS
      if (data -> intensity_converter_)
	inc = (*data -> intensity_converter_)(inc);
#     endif
      *data->intensity += inc;

#     if GYOTO_DEBUG_ENABLED
	GYOTO_DEBUG
	  << "intensity +=" << *data->intensity
	  << "= emission((dsem=" << dsem << "))="
	  << (emission(freqObs*ggredm1,dsem,coord_ph_hit, coord_obj_hit))
	  << ")*(ggred=" << ggred << ")^3*(transmission="
	  << (ph -> getTransmission(size_t(-1))) << ")"
	  << endl;
#     endif

    }
    if (data->binspectrum) {
      size_t nbounds = spr-> getNBoundaries();
      double const * const channels = spr -> getChannelBoundaries();
      size_t const * const chaninds = spr -> getChannelIndices();
      double * I  = new double[nbnuobs];
      double * boundaries = new double[nbounds];

      for (size_t ii=0; ii<nbounds; ++ii)
	boundaries[ii]=channels[ii]*ggredm1;
      integrateEmission(I, boundaries, chaninds, nbnuobs,
			dsem, coord_ph_hit, coord_obj_hit);
      for (size_t ii=0; ii<nbnuobs; ++ii) {
	inc = I[ii] * ph -> getTransmission(ii) * ggred*ggred*ggred*ggred;
#       ifdef HAVE_UDUNITS
	if (data -> binspectrum_converter_)
	  inc = (*data -> binspectrum_converter_)(inc);
#       endif
	data->binspectrum[ii*data->offset] += inc ;
#       if GYOTO_DEBUG_ENABLED
	GYOTO_DEBUG
	       << "nuobs[" << ii << "]="<< channels[ii]
	       << ", nuem=" << boundaries[ii]
	       << ", binspectrum[" << ii+data->offset << "]="
	       << data->binspectrum[ii*data->offset]<< endl;
#       endif

	if (!data->spectrum) // else it will be done in spectrum
	  ph -> transmit(ii,transmission(nuobs[ii]*ggredm1,dsem,coord_ph_hit, coord_obj_hit));
      }
      delete [] I;
      delete [] boundaries;
    }
    if (data->spectrum||data->stokesQ||data->stokesU||data->stokesV) {
      if (ph -> parallelTransport()) { // Compute polarization
	double * Inu          = new double[nbnuobs];
	double * Qnu          = new double[nbnuobs];
	double * Unu          = new double[nbnuobs];
	double * Vnu          = new double[nbnuobs];
	double * alphaInu     = new double[nbnuobs];
	double * alphaQnu     = new double[nbnuobs];
	double * alphaUnu     = new double[nbnuobs];
	double * alphaVnu     = new double[nbnuobs];
	double * rQnu         = new double[nbnuobs];
	double * rUnu         = new double[nbnuobs];
	double * rVnu         = new double[nbnuobs];
	double * nuem         = new double[nbnuobs];

	for (size_t ii=0; ii<nbnuobs; ++ii) {
	  nuem[ii]=nuobs[ii]*ggredm1;
	}
	GYOTO_DEBUG_ARRAY(nuobs, nbnuobs);
	GYOTO_DEBUG_ARRAY(nuem, nbnuobs);
	radiativeQ(Inu, Qnu, Unu, Vnu,
		   alphaInu, alphaQnu, alphaUnu, alphaVnu,
		   rQnu, rUnu, rVnu,
		   nuem, nbnuobs, dsem,
		   coord_ph_hit, coord_obj_hit);
	ph -> transfer (Inu, Qnu, Unu, Vnu,
			alphaInu, alphaQnu, alphaUnu, alphaVnu,
			rQnu, rUnu, rVnu);
	double ggred3 = ggred*ggred*ggred;
	for (size_t ii=0; ii<nbnuobs; ++ii) {
	  if (data-> spectrum) {
	    inc = Inu[ii] * ggred3;
#           ifdef HAVE_UDUNITS
	    if (data -> spectrum_converter_)
	      inc = (*data -> spectrum_converter_)(inc);
#           endif
	    data->spectrum[ii*data->offset] += inc;
	  }
	  if (data-> stokesQ) {
	    inc = Qnu[ii] * ggred3;
#           ifdef HAVE_UDUNITS
	    if (data -> spectrum_converter_)
	      inc = (*data -> spectrum_converter_)(inc);
#           endif
	    data->stokesQ [ii*data->offset] += inc;
	  }
	  if (data-> stokesU) {
	    inc = Unu[ii] * ggred3;
#           ifdef HAVE_UDUNITS
	    if (data -> spectrum_converter_)
	      inc = (*data -> spectrum_converter_)(inc);
#           endif
	    data->stokesU [ii*data->offset] += inc;
	  }
	  if (data-> stokesV) {
	    inc = Vnu[ii] * ggred3;
#           ifdef HAVE_UDUNITS
	    if (data -> spectrum_converter_)
	      inc = (*data -> spectrum_converter_)(inc);
#           endif
	    data->stokesV [ii*data->offset] += inc;
	  }

#         if GYOTO_DEBUG_ENABLED
	  {
	    double t=ph -> getTransmission(ii);
	    double o = t>0?-log(t):(-std::numeric_limits<double>::infinity());
	  GYOTO_DEBUG
	    << "DEBUG: Generic::processHitQuantities(): "
	    << "nuobs[" << ii << "]="<< nuobs[ii]
	    << ", nuem=" << nuem[ii]
	    << ", dsem=" << dsem
	    << ", Inu * GM/c2="
	    << Inu[ii]
	    << ", spectrum[" << ii*data->offset << "]="
	    << data->spectrum[ii*data->offset]
	    << ", transmission=" << t
	    << ", optical depth=" << o
	    << ", redshift=" << ggred << ")\n" << endl;
	  }
#         endif
	}
	delete [] Inu;
	delete [] Qnu;
	delete [] Unu;
	delete [] Vnu;
	delete [] alphaInu;
	delete [] alphaQnu;
	delete [] alphaUnu;
	delete [] alphaVnu;
	delete [] rQnu;
	delete [] rUnu;
	delete [] rVnu;
	delete [] nuem;
      } else { // No polarization
	double * Inu          = new double[nbnuobs];
	double * Taunu        = new double[nbnuobs];
	double * nuem         = new double[nbnuobs];

	for (size_t ii=0; ii<nbnuobs; ++ii) {
	  nuem[ii]=nuobs[ii]*ggredm1;
	}
	GYOTO_DEBUG_ARRAY(nuobs, nbnuobs);
	GYOTO_DEBUG_ARRAY(nuem, nbnuobs);
	radiativeQ(Inu, Taunu, nuem, nbnuobs, dsem,
		   coord_ph_hit, coord_obj_hit);
	for (size_t ii=0; ii<nbnuobs; ++ii) {
	  inc = Inu[ii] * ph -> getTransmission(ii) * ggred*ggred*ggred;
#         ifdef HAVE_UDUNITS
	  if (data -> spectrum_converter_)
	    inc = (*data -> spectrum_converter_)(inc);
#         endif
	  data->spectrum[ii*data->offset] += inc;
	  ph -> transmit(ii,Taunu[ii]);

#         if GYOTO_DEBUG_ENABLED
	  {
	    double t = ph -> getTransmission(ii);
	    double o = t>0?-log(t):(-std::numeric_limits<double>::infinity());
	  GYOTO_DEBUG
	    << "DEBUG: Generic::processHitQuantities(): "
	    << "nuobs[" << ii << "]="<< nuobs[ii]
	    << ", nuem=" << nuem[ii]
	    << ", dsem=" << dsem
	    << ", Inu * GM/c2="
	    << Inu[ii]
	    << ", spectrum[" << ii*data->offset << "]="
	    << data->spectrum[ii*data->offset]
	    << ", transmission=" << t
	    << ", optical depth=" << o
	    << ", redshift=" << ggred << ")\n" << endl;
	  }
#         endif
	}
	delete [] Inu;
	delete [] Taunu;
	delete [] nuem;
      }
    }
    /* update photon's transmission */
    ph -> transmit(size_t(-1),
		   transmission(freqObs*ggredm1, dsem,coord_ph_hit, coord_obj_hit));
  } else {
#   if GYOTO_DEBUG_ENABLED
    GYOTO_DEBUG << "NO data requested!" << endl;
#   endif
  }
}

/*
  User code is free to provide any or none of the various versions of
  emission(), transmission() and radiativeQ(). The default
  implementations call one another to try and find user-provided
  code. In order to avoid infinite recursion as well as for
  efficiency, several of those methose set a flag in __defaultfeatures
  if they are called to inform the other methods. This is what each
  method will try:
    - polarized radiativeQ:
      + unpolarized radiativeQ;
    - unpolarized radiativeQ:
      + polarized radiativeQ;
      + emission(double*, ...) and transmission;
    - emission(double*, ...):
      + unpolarized radiativeQ;
      + polarized radiativeQ;
      + emission(double, ...);
    - emission(double, ...):
      + emission(double*, ...);
      + unpolarized radiativeQ;
      + fall back to uniform, unit emission;
    - transmission:
      + unpolarized radiativeQ;
      + fall-back to uniform, unit opacity.
 */

#define __default_radiativeQ_polar 1
#define __default_radiativeQ       2
#define __default_emission_vector  4
double Generic::transmission(double nuem, double dsem, state_t const &coord_ph, double const coord_obj[8]) const {
# if GYOTO_DEBUG_ENABLED
  GYOTO_DEBUG_EXPR(flag_radtransf_);
  GYOTO_DEBUG_EXPR(__defaultfeatures);
# endif
  if ((!(__defaultfeatures & __default_radiativeQ)) ||
      (!(__defaultfeatures & __default_radiativeQ_polar))) {
    // We don't know (yet?) whether both radiativeQ are the default
    // implementations. Let's call the unpolarized version, which will
    // call the polarized version if the former is not reimplemented.
    double Inu, Taunu;
    radiativeQ(&Inu, &Taunu, &nuem, 1, dsem, coord_ph, coord_obj);
    return Taunu;
    // If radiativeQ is also the default implementation, it will set
    // __defaultfeatures and recurse back here
  }
  return double(flag_radtransf_);
}

double Generic::emission(double nuem, double dsem, state_t const &cph, double const *co) const
{
# if GYOTO_DEBUG_ENABLED
  GYOTO_DEBUG_EXPR(__defaultfeatures);
# endif

  if (!(__defaultfeatures & __default_emission_vector)) {
    // We don't know (yet?) whether emission(double* ,...) is the
    // default implementation, let's call it
    double Inu;
    emission(&Inu, &nuem , 1, dsem, cph, co);
    return Inu;
  } else if ((!(__defaultfeatures & __default_radiativeQ)) ||
	     (!(__defaultfeatures & __default_radiativeQ_polar))) {
    // emission(double*, ...) is the default implementation, but
    // we don't know (yet?) about radiativeQ(); let's call it
    double Inu, Taunu;
    radiativeQ(&Inu, &Taunu, &nuem, 1, dsem, cph, co);
    return Inu;
  }

  // emission(double*, ...) and radiativeQ are the default
  // implementations: we are on our own. Fall-back to uniform
  // emission.

# if GYOTO_DEBUG_ENABLED
  GYOTO_DEBUG_EXPR(flag_radtransf_);
# endif
  if (flag_radtransf_) return dsem;
  return 1.;
}

void Generic::emission(double * Inu, double const * nuem , size_t nbnu,
			 double dsem, state_t const &cph, double const *co) const
{
# if GYOTO_DEBUG_ENABLED
  GYOTO_DEBUG_EXPR(__defaultfeatures);
# endif

  // Inform emission(double, ...)  that emission(double *, ...)  is
  // the default implementation and that it should not recurse back
  const_cast<Generic*>(this)->__defaultfeatures |= __default_emission_vector;

  if (!(__defaultfeatures & __default_radiativeQ)) {
    // We don't know (yet?) whether unpolarized radiativeQ is the
    // default implementation, let's call it
    double * Taunu = new double[nbnu];
    radiativeQ(Inu, Taunu, nuem, nbnu, dsem, cph, co);
    delete [] Taunu;
    // If radiativeQ is the default implementation, it will recurse
    // back here and we are going to skip to the next case below
    return;
  } else if (!(__defaultfeatures & __default_radiativeQ_polar)) {
    // We don't know (yet?) whether polarized radiativeQ is the
    // default implementation, let's call it
    double * Taunu = new double[nbnu];
    double * Qnu = new double[nbnu];
    double * Unu = new double[nbnu];
    double * Vnu = new double[nbnu];
    double * alphaInu = new double[nbnu];
    double * alphaQnu = new double[nbnu];
    double * alphaUnu = new double[nbnu];
    double * alphaVnu = new double[nbnu];
    double * rQnu = new double[nbnu];
    double * rUnu = new double[nbnu];
    double * rVnu = new double[nbnu];
    radiativeQ(Inu, Qnu, Unu, Vnu,
	       alphaInu, alphaQnu, alphaUnu, alphaVnu,
	       rQnu, rUnu, rVnu,
	       nuem , nbnu, dsem,
	       cph, co);
    // in all cases, clean up
    delete [] Qnu;
    delete [] Unu;
    delete [] Vnu;
    delete [] alphaInu;
    delete [] alphaQnu;
    delete [] alphaUnu;
    delete [] alphaVnu;
    delete [] rQnu;
    delete [] rUnu;
    delete [] rVnu;
    delete [] Taunu;
    return;
  }

  // If both radiativeQ() versions are the default implementations,
  // fall back to emission(double, ...)
  for (size_t i=0; i< nbnu; ++i) Inu[i]=emission(nuem[i], dsem, cph, co);
}

void Generic::radiativeQ(double * Inu, double * Taunu,
			 double const * nuem , size_t nbnu,
			 double dsem, state_t const &cph, double const *co) const
{
# if GYOTO_DEBUG_ENABLED
  GYOTO_DEBUG_EXPR(flag_radtransf_);
# endif
  // Inform emission() and transmission() that radiativeQ is the
  // default implementation and that they should not recurse back here
  const_cast<Generic*>(this)->__defaultfeatures |= __default_radiativeQ;

  if (!(__defaultfeatures & __default_radiativeQ_polar)) {
    // We don't know (yet?) whether polarized radiativeQ is the
    // default implementation, let's call it
    double * Qnu = new double[nbnu];
    double * Unu = new double[nbnu];
    double * Vnu = new double[nbnu];
    double * alphaInu = new double[nbnu];
    double * alphaQnu = new double[nbnu];
    double * alphaUnu = new double[nbnu];
    double * alphaVnu = new double[nbnu];
    double * rQnu = new double[nbnu];
    double * rUnu = new double[nbnu];
    double * rVnu = new double[nbnu];
    radiativeQ(Inu, Qnu, Unu, Vnu,
	       alphaInu, alphaQnu, alphaUnu, alphaVnu,
	       rQnu, rUnu, rVnu,
	       nuem , nbnu, dsem,
	       cph, co);
    if (!(__defaultfeatures & __default_radiativeQ_polar)) {
      for (size_t i=0; i<nbnu; ++i) {
	Taunu[i] = exp(-alphaInu[i]);
      }
    } else {
      for (size_t i=0; i<nbnu; ++i) {
	Taunu[i]=transmission(nuem[i], dsem, cph, co);
      }
    }
    // in all cases, clean up
    delete [] Qnu;
    delete [] Unu;
    delete [] Vnu;
    delete [] alphaInu;
    delete [] alphaQnu;
    delete [] alphaUnu;
    delete [] alphaVnu;
    delete [] rQnu;
    delete [] rUnu;
    delete [] rVnu;
    return;
    // If polarized radiativeQ is not implemented, the default
    // implementation will recurse here.
  }

  // Polarized radiativeQ for sure not implemented, fall-back to
  // emission() and transmission().

  // Call emission()
  emission(Inu, nuem, nbnu, dsem, cph, co);

  // Call transmission()
  for (size_t i=0; i< nbnu; ++i)
    Taunu[i]=transmission(nuem[i], dsem, cph, co);
}

void Generic::radiativeQ(double *Inu, double *Qnu, double *Unu, double *Vnu,
                         double *alphaInu, double *alphaQnu,
			 double *alphaUnu, double *alphaVnu,
                         double *rQnu, double *rUnu, double *rVnu,
			 double const *nuem , size_t nbnu, double dsem,
                         state_t const &cph,
			 double const *co) const
{
  // cph has 16 elements, 4 elements for each one of
  // X, Xdot, Ephi, Etheta
  //
  // If this method is not reimplemented, the emission is not polarized.

  // Inform non-polarized radiativeQ() that this method is the default
  // implementation and that they should not recurse back here
  const_cast<Generic*>(this)->__defaultfeatures |= __default_radiativeQ_polar;

  // Compute the output from the non-polarized radiativeQ().
  double * Taunu = new double[nbnu];
  radiativeQ(Inu, Taunu, nuem, nbnu, dsem, cph, co);
  for (size_t i=0; i<nbnu; ++i) {
    // Inu[i] = Inu[i];
    Qnu[i] = 0.;
    Unu[i] = 0.;
    Vnu[i] = 0.;
    GYOTO_DEBUG_EXPR(Taunu[i]);
    if (Taunu[i]<0.1) alphaInu[i] = -std::numeric_limits<double>::infinity();
    else alphaInu[i] = -log(Taunu[i]); // should we divide by dsem?
    GYOTO_DEBUG_EXPR(alphaInu[i]);
    alphaQnu[i] = 0.; // Is everything else 0.?
    alphaUnu[i] = 0.;
    alphaVnu[i] = 0.;
    rQnu[i] = 0.;
    rUnu[i] = 0.;
    rVnu[i] = 0.;
  }
  delete [] Taunu;
}

void Generic::integrateEmission(double * I, double const * boundaries,
				size_t const * chaninds, size_t nbnu,
				double dsem, state_t const &cph, double const *co) const
{
  for (size_t i=0; i<nbnu; ++i)
    I[i] = integrateEmission(boundaries[chaninds[2*i]],
			     boundaries[chaninds[2*i+1]],
			     dsem, cph, co);
}

double Generic::integrateEmission (double nu1, double nu2, double dsem,
				   state_t const &coord_ph, double const coord_obj[8])
  const {
  double nu;
  if(nu1>nu2) {nu=nu1; nu1=nu2; nu2=nu;}
  double Inu1 = emission(nu1, dsem, coord_ph, coord_obj);
  double Inu2 = emission(nu2, dsem, coord_ph, coord_obj);
  double dnux2 = ((nu2-nu1)*2.);
  double Icur = (Inu2+Inu1)*dnux2*0.25;
  double Iprev;

# if GYOTO_DEBUG_ENABLED
  GYOTO_DEBUG_EXPR(Icur);
# endif

  do {
    Iprev = Icur;
    dnux2 *= 0.5;
    for (nu = nu1 + 0.5*dnux2; nu < nu2; nu += dnux2) {
      Icur += emission(nu, dsem, coord_ph, coord_obj) * dnux2;
    }
    Icur *= 0.5;
#   if GYOTO_DEBUG_ENABLED
    GYOTO_DEBUG_EXPR(Icur);
#   endif
  } while( fabs(Icur-Iprev) > (1e-2 * Icur) );

# if GYOTO_DEBUG_ENABLED
  GYOTO_DEBUG << "dnu=" << dnux2*0.5
	      << "=(nu2-nu1)/" << (nu2-nu1)/(dnux2*0.5) << endl;
# endif

  return Icur;
}

Quantity_t Generic::getDefaultQuantities() { return GYOTO_QUANTITY_INTENSITY; }

void Astrobj::initRegister() {
  if (Gyoto::Astrobj::Register_) delete Gyoto::Astrobj::Register_;
  Gyoto::Astrobj::Register_ = NULL;
}

double Generic::deltaMax(double coord[8]) {
  double rr=0.,h1max;

  if (!gg_)
    GYOTO_ERROR("Please set metric before calling Astrobj::Generic::deltaMax()");

  switch (gg_ -> coordKind()) {
  case GYOTO_COORDKIND_SPHERICAL:
    rr=coord[1];
    break;
  case GYOTO_COORDKIND_CARTESIAN:
    rr=sqrt(coord[1]*coord[1]+coord[2]*coord[2]+coord[3]*coord[3]);
    break;
  default:
    GYOTO_ERROR("Incompatible coordinate kind in Astrobj.C");
  }

  if (rr<rMax()) h1max=deltamaxinsidermax_; else h1max=rr*0.5;
  return h1max;
}

void Gyoto::Astrobj::Register(std::string name, Subcontractor_t* scp){
  Register::Entry* ne =
    new Register::Entry(name, (SmartPointee::Subcontractor_t*)scp, Gyoto::Astrobj::Register_);
  Gyoto::Astrobj::Register_ = ne;
}

GYOTO_GETSUBCONTRACTOR(Astrobj)

Astrobj::Properties::Properties() :
  intensity(NULL), time(NULL), distance(NULL),
  first_dmin(NULL), first_dmin_found(0),
  redshift(NULL), nbcrosseqplane(NULL),
  spectrum(NULL), stokesQ(NULL), stokesU(NULL), stokesV(NULL),
  binspectrum(NULL), offset(1), impactcoords(NULL),
  user1(NULL), user2(NULL), user3(NULL), user4(NULL), user5(NULL)
# ifdef HAVE_UDUNITS
  , intensity_converter_(NULL), spectrum_converter_(NULL),
  binspectrum_converter_(NULL)
# endif
  , alloc(false)
{}

Astrobj::Properties::Properties( double * I, double * t) :
  intensity(I), time(t), distance(NULL),
  first_dmin(NULL), first_dmin_found(0),
  redshift(NULL), nbcrosseqplane(NULL),
  spectrum(NULL), stokesQ(NULL), stokesU(NULL), stokesV(NULL),
  binspectrum(NULL), offset(1), impactcoords(NULL),
  user1(NULL), user2(NULL), user3(NULL), user4(NULL), user5(NULL)
# ifdef HAVE_UDUNITS
  , intensity_converter_(NULL), spectrum_converter_(NULL),
  binspectrum_converter_(NULL)
# endif
  , alloc(false)
{}

void Astrobj::Properties::init(size_t nbnuobs) {
  if (intensity)      *intensity  = 0.;
  if (time)           *time       = DBL_MAX;
  if (distance)       *distance   = DBL_MAX;
  if (first_dmin){    *first_dmin = DBL_MAX; first_dmin_found=0;}
  if (redshift)       *redshift   = 0.;
  if (nbcrosseqplane) *nbcrosseqplane   = 0.;
  if (spectrum) for (size_t ii=0; ii<nbnuobs; ++ii) spectrum[ii*offset]=0.;
  if (stokesQ)  for (size_t ii=0; ii<nbnuobs; ++ii) stokesQ [ii*offset]=0.;
  if (stokesU)  for (size_t ii=0; ii<nbnuobs; ++ii) stokesU [ii*offset]=0.;
  if (stokesV)  for (size_t ii=0; ii<nbnuobs; ++ii) stokesV [ii*offset]=0.;
  if (binspectrum) for (size_t ii=0; ii<nbnuobs; ++ii)
		     binspectrum[ii*offset]=0.;
  if (impactcoords) for (size_t ii=0; ii<16; ++ii) impactcoords[ii]=DBL_MAX;
  if (user1)          *user1=0.;
  if (user2)          *user2=0.;
  if (user3)          *user3=0.;
  if (user4)          *user4=0.;
  if (user5)          *user5=0.;
}

#ifdef HAVE_UDUNITS
void Astrobj::Properties::intensityConverter(SmartPointer<Units::Converter> conv) {
  intensity_converter_ = conv ;
}

void Astrobj::Properties::intensityConverter(string unit) {
  intensity_converter_ =
    new Units::Converter("J.m-2.s-1.sr-1.Hz-1",
			 unit!=""?unit:"J.m-2.s-1.sr-1.Hz-1");
}

void Astrobj::Properties::spectrumConverter(SmartPointer<Units::Converter> conv) {
  spectrum_converter_ = conv;
}

void Astrobj::Properties::spectrumConverter(string unit) {
  spectrum_converter_ =
    new Units::Converter("J.m-2.s-1.sr-1.Hz-1",
			 unit!=""?unit:"J.m-2.s-1.sr-1.Hz-1");
}

void Astrobj::Properties::binSpectrumConverter(SmartPointer<Units::Converter> conv) {
  binspectrum_converter_ = conv;
}
void Astrobj::Properties::binSpectrumConverter(string unit) {
  binspectrum_converter_ =
    new Units::Converter("J.m-2.s-1.sr-1",
			 unit!=""?unit:"J.m-2.s-1.sr-1");
}

#endif

Astrobj::Properties& Astrobj::Properties::operator+=(ptrdiff_t ofset) {
  if (intensity)      intensity      += ofset;
  if (time)           time           += ofset;
  if (distance)       distance       += ofset;
  if (first_dmin)     first_dmin     += ofset;
  if (redshift)       redshift       += ofset;
  if (nbcrosseqplane) nbcrosseqplane += ofset;
  if (spectrum)       spectrum       += ofset;
  if (stokesQ)        stokesQ        += ofset;
  if (stokesU)        stokesU        += ofset;
  if (stokesV)        stokesV        += ofset;
  if (binspectrum)    binspectrum    += ofset;
  if (impactcoords)   impactcoords   += 16*ofset;
  if (user1)          user1          += ofset;
  if (user2)          user2          += ofset;
  if (user3)          user3          += ofset;
  if (user4)          user4          += ofset;
  if (user5)          user5          += ofset;
  return *this;
}

Astrobj::Properties& Astrobj::Properties::operator++() {
  (*this) += 1;
  return *this;
}

Astrobj::Properties::operator Quantity_t () const {
  Quantity_t res=GYOTO_QUANTITY_NONE;

  if (intensity)      res |= GYOTO_QUANTITY_INTENSITY;
  if (time)           res |= GYOTO_QUANTITY_EMISSIONTIME;
  if (distance)       res |= GYOTO_QUANTITY_MIN_DISTANCE;
  if (first_dmin)     res |= GYOTO_QUANTITY_FIRST_DMIN;
  if (redshift)       res |= GYOTO_QUANTITY_REDSHIFT;
  if (nbcrosseqplane) res |= GYOTO_QUANTITY_NBCROSSEQPLANE;
  if (spectrum)       res |= GYOTO_QUANTITY_SPECTRUM;
  if (stokesQ)        res |= GYOTO_QUANTITY_SPECTRUM_STOKES_Q;
  if (stokesU)        res |= GYOTO_QUANTITY_SPECTRUM_STOKES_U;
  if (stokesV)        res |= GYOTO_QUANTITY_SPECTRUM_STOKES_V;
  if (binspectrum)    res |= GYOTO_QUANTITY_BINSPECTRUM;
  if (impactcoords)   res |= GYOTO_QUANTITY_IMPACTCOORDS;
  if (user1)          res |= GYOTO_QUANTITY_USER1;
  if (user2)          res |= GYOTO_QUANTITY_USER2;
  if (user3)          res |= GYOTO_QUANTITY_USER3;
  if (user4)          res |= GYOTO_QUANTITY_USER4;
  if (user5)          res |= GYOTO_QUANTITY_USER5;

  return res;
}
