#ifndef SPECIFICTOTALENTHALPYAUX_H
#define SPECIFICTOTALENTHALPYAUX_H

#include "AuxKernel.h"

// Forward Declarations
class SpecificTotalEnthalpyAux;

template <>
InputParameters validParams<SpecificTotalEnthalpyAux>();

/**
 * Nodal auxiliary variable for specific total enthalpy
 *
 * This computes the specific total enthalpy:
 * \f[
 *   H = E + \frac{p}{\rho}
 * \f]
 */
class SpecificTotalEnthalpyAux : public AuxKernel
{
public:
  SpecificTotalEnthalpyAux(const InputParameters & parameters);
  virtual ~SpecificTotalEnthalpyAux();

protected:
  virtual Real computeValue();

  const VariableValue & _rhoA;
  const VariableValue & _rhoEA;
  const VariableValue & _pressure;
  const VariableValue & _area;
  const VariableValue & _alpha;
};

#endif // SPECIFICTOTALENTHALPYAUX_H
