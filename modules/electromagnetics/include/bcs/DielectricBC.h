#ifndef DIELECTRICBC_H
#define DIELECTRICBC_H

#include "DiffusionFluxBC.h"

class DielectricBC;

template <>
InputParameters validParams<DielectricBC>();

/**
 *
 */
class DielectricBC : public DiffusionFluxBC
{
public:
  DielectricBC(const InputParameters & parameters);

protected:
  virtual Real computeQpResidual();
  virtual Real computeQpJacobian();

  Real _eps_one;
  Real _eps_two;
};

#endif // DIELECTRICBC_H
