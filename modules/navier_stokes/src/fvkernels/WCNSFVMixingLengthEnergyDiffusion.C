//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "WCNSFVMixingLengthEnergyDiffusion.h"
#include "NS.h"
#include "MathFVUtils.h"

registerMooseObject("NavierStokesApp", WCNSFVMixingLengthEnergyDiffusion);

InputParameters
WCNSFVMixingLengthEnergyDiffusion::validParams()
{
  InputParameters params = FVFluxKernel::validParams();
  params.addClassDescription("Computes the turbulent diffusive flux that appears in "
                             "Reynolds-averaged fluid energy conservation equations.");
  params.addRequiredParam<MooseFunctorName>("u", "The velocity in the x direction.");
  params.addParam<MooseFunctorName>("v", "The velocity in the y direction.");
  params.addParam<MooseFunctorName>("w", "The velocity in the z direction.");
  params.addRequiredParam<MooseFunctorName>("mixing_length", "The turbulent mixing length.");
  params.addRequiredParam<Real>(
      "schmidt_number",
      "The turbulent Schmidt number (or turbulent Prandtl number if the passive scalar is energy) "
      "that relates the turbulent scalar diffusivity to the turbulent momentum diffusivity.");
  params.addRequiredParam<MooseFunctorName>(NS::density, "Density");
  params.addRequiredParam<MooseFunctorName>(NS::cp, "Specific heat capacity");

  params.set<unsigned short>("ghost_layers") = 2;
  return params;
}

WCNSFVMixingLengthEnergyDiffusion::WCNSFVMixingLengthEnergyDiffusion(const InputParameters & params)
  : FVFluxKernel(params),
    _dim(_subproblem.mesh().dimension()),
    _u(getFunctor<ADReal>("u")),
    _v(isParamValid("v") ? &getFunctor<ADReal>("v") : nullptr),
    _w(isParamValid("w") ? &getFunctor<ADReal>("w") : nullptr),
    _rho(getFunctor<ADReal>(NS::density)),
    _cp(getFunctor<ADReal>(NS::cp)),
    _mixing_len(getFunctor<ADReal>("mixing_length")),
    _schmidt_number(getParam<Real>("schmidt_number"))
{
#ifndef MOOSE_GLOBAL_AD_INDEXING
  mooseError("INSFV is not supported by local AD indexing. In order to use INSFV, please run the "
             "configure script in the root MOOSE directory with the configure option "
             "'--with-ad-indexing-type=global'");
#endif

  if (_dim >= 2 && !_v)
    mooseError(
        "In two or more dimensions, the v velocity must be supplied using the 'v' parameter");
  if (_dim >= 3 && !_w)
    mooseError("In threedimensions, the w velocity must be supplied using the 'w' parameter");
}

ADReal
WCNSFVMixingLengthEnergyDiffusion::computeQpResidual()
{
#ifdef MOOSE_GLOBAL_AD_INDEXING
  constexpr Real offset = 1e-15; // prevents explosion of sqrt(x) derivative to infinity

  const auto face = makeCDFace(*_face_info);

  const auto grad_u = _u.gradient(face);
  ADReal symmetric_strain_tensor_norm = 2.0 * Utility::pow<2>(grad_u(0));
  if (_dim >= 2)
  {
    const auto grad_v = _v->gradient(face);
    symmetric_strain_tensor_norm +=
        2.0 * Utility::pow<2>(grad_v(1)) + Utility::pow<2>(grad_v(0) + grad_u(1));
    if (_dim >= 3)
    {
      const auto grad_w = _w->gradient(face);
      symmetric_strain_tensor_norm += 2.0 * Utility::pow<2>(grad_w(2)) +
                                      Utility::pow<2>(grad_u(2) + grad_w(0)) +
                                      Utility::pow<2>(grad_v(2) + grad_w(1));
    }
  }

  symmetric_strain_tensor_norm = std::sqrt(symmetric_strain_tensor_norm + offset);

  // Interpolate the mixing length to the face
  ADReal mixing_len = _mixing_len(face);

  // Compute the eddy diffusivity for momentum
  ADReal eddy_diff = symmetric_strain_tensor_norm * mixing_len * mixing_len;

  // Use the turbulent Schmidt/Prandtl number to get the eddy diffusivity for
  // the scalar variable
  eddy_diff /= _schmidt_number;

  const auto dTdn = gradUDotNormal();

  ADReal rho_cp_face;
  if (onBoundary(*_face_info))
  {
    const auto ssf = singleSidedFaceArg();
    rho_cp_face = _rho(ssf) * _cp(ssf);
  }
  else
  {
    // Interpolate the heat capacity
    const auto face_elem = elemArg();
    const auto face_neighbor = neighborArg();
    interpolate(Moose::FV::InterpMethod::Average,
                rho_cp_face,
                _rho(face_elem) * _cp(face_elem),
                _rho(face_neighbor) * _cp(face_neighbor),
                *_face_info,
                true);
  }

  return -1 * eddy_diff * rho_cp_face * dTdn;

#else
  return 0;

#endif
}
