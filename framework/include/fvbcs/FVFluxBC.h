//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#pragma once

#include "FVBoundaryCondition.h"
#include "NeighborCoupleableMooseVariableDependencyIntermediateInterface.h"
#include "TwoMaterialPropertyInterface.h"
#include "FVUtils.h"

// Provides an interface for computing residual contributions from finite
// volume numerical fluxes computed on faces to neighboring elements.
class FVFluxBC : public FVBoundaryCondition,
                 public NeighborCoupleableMooseVariableDependencyIntermediateInterface,
                 public TwoMaterialPropertyInterface
{
public:
  FVFluxBC(const InputParameters & parameters);

  static InputParameters validParams();

  virtual void computeResidual(const FaceInfo & fi);
  virtual void computeJacobian(const FaceInfo & fi);

protected:
  virtual ADReal computeQpResidual() = 0;

  const ADRealVectorValue & normal() const { return _normal; }

  const unsigned int _qp = 0;
  const ADVariableValue & _u;
  const ADVariableValue & _u_neighbor;
  // TODO: add gradients once we have reconstruction.
  ADRealVectorValue _normal;
  const FaceInfo * _face_info = nullptr;

  /**
   * @return the value of u at the cell centroid on the subdomain on which u is defined. E.g. u is
   * defined on either the FaceInfo elem or FacInfo neighbor subdomain
   */
  const ADReal & uOnUSub() const;

  /**
   * @return the value of u at the ghost cell centroid
   */
  const ADReal & uOnGhost() const;

  /**
   * @return the value of \p makeSidedFace with \p fi_elem = true
   */
  std::tuple<const libMesh::Elem *, const FaceInfo *, SubdomainID> elemFromFace() const;

  /**
   * @return the value of \p makeSidedFace with \p fi_elem = false
   */
  std::tuple<const libMesh::Elem *, const FaceInfo *, SubdomainID> neighborFromFace() const;

private:
  /**
   * This creates a tuple of an element, \p FaceInfo, and subdomain ID. The element returned will
   * correspond to the method argument, e.g. if \p fi_elem is true, then this will return the \p
   * FaceInfo element, else it will return the \p FaceInfo neighbor. The \p FaceInfo part of the
   * tuple will simply correspond to the current \p _face_info. The subdomain ID part of the tuple
   * will correspond to the subdomain ID that this object is defined on because flux boundary
   * conditions do indeed have sidedness. If a variable is only defined on the element side of the
   * current face, then the subdomain ID will be equivalent to \p _face_info->elem().subdomain_id().
   * If the variable is only defined on the neighbor side of the face, then the subdomain ID will be
   * equivalent to \p _face_info->neighborPtr()->subdomain_id(). We currently error in flux bcs if
   * the variable is defined on both sides of the face
   */
  std::tuple<const libMesh::Elem *, const FaceInfo *, SubdomainID>
  makeSidedFace(bool fi_elem) const;

  /// Computes the Jacobian contribution for every coupled variable.
  ///
  /// @param type Either ElementElement, ElementNeighbor, NeighborElement, or NeighborNeighbor. As an
  /// example ElementNeighbor means the derivatives of the elemental residual with respect to the
  /// neighbor degrees of freedom
  ///
  /// @param residual The already computed residual (probably done with \p computeQpResidual) that
  /// also holds derivative information for filling in the Jacobians
  void computeJacobian(Moose::DGJacobianType type, const ADReal & residual);
};
