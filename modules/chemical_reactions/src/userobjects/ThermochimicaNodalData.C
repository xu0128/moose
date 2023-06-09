//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "ThermochimicaNodalData.h"
#include "ThermochimicaUtils.h"
#include "libmesh/int_range.h"

#ifdef THERMOCHIMICA_ENABLED
#include "Thermochimica-cxx.h"
#include "checkUnits.h"
#endif

registerMooseObject("ChemicalReactionsApp", ThermochimicaNodalData);

InputParameters
ThermochimicaNodalData::validParams()
{
  InputParameters params = NodalUserObject::validParams();
  ThermochimicaUtils::addClassDescription(
      params, "Provides access to Thermochimica-calculated data at nodes.");

  params.addRequiredCoupledVar("elements", "Amounts of elements");
  params.addCoupledVar("element_potentials", "Chemical potentials of elements");

  params.addParam<Real>("pressure", 1.0, "Pressure");
  params.addRequiredCoupledVar("temperature", "Coupled temperature");

  params.addParam<bool>("reinit_requested", true, "Should Thermochimica use re-initialization?");
  params.addCoupledVar("output_phases", "Amounts of phases to be output");
  params.addCoupledVar("output_species", "Amounts of species to be output");
  return params;
}

ThermochimicaNodalData::ThermochimicaNodalData(const InputParameters & parameters)
  : NodalUserObject(parameters),
    _pressure(getParam<Real>("pressure")),
    _temperature(coupledValue("temperature")),
    _reinit_requested(getParam<bool>("reinit_requested")),
    _n_phases(coupledComponents("output_phases")),
    _n_species(coupledComponents("output_species")),
    _n_elements(coupledComponents("elements")),
    _el(_n_elements),
    _el_name(_n_elements),
    _ph_name(_n_phases),
    _sp_phase_name(_n_species),
    _sp_species_name(_n_species),
    _output_element_potential(isCoupled("element_potentials"))
{
  ThermochimicaUtils::checkLibraryAvailability(*this);

  if (n_threads() > 1)
    mooseError("Thermochimica does not support multi-threaded runs.");
  if (coupledComponents("element_potentials") != _n_elements)
    paramError("element_potentials", "Specify one element potential for each entry in `elements`");

  for (const auto i : make_range(_n_elements))
  {
    _el[i] = &coupledValue("elements", i);
    _el_name[i] = getVar("elements", i)->name();
  }

  for (const auto i : make_range(_n_phases))
    _ph_name[i] = getVar("output_phases", i)->name();

  for (const auto i : make_range(_n_species))
  {
    auto species_var_name = getVar("output_species", i)->name();
    auto semicolon = species_var_name.find("_");
    _sp_phase_name[i] = species_var_name.substr(0, semicolon);
    _sp_species_name[i] = species_var_name.substr(semicolon + 1);
  }

  if (_output_element_potential)
  {
    _element_potentials.resize(_n_elements);
    for (const auto i : make_range(_n_elements))
    {
      auto element_var_name = getVar("element_potentials", i)->name();
      auto semicolon = element_var_name.find("_");
      _element_potentials[i] = element_var_name.substr(semicolon + 1);
    }
  }
}

void
ThermochimicaNodalData::initialize()
{
}

void
ThermochimicaNodalData::execute()
{
#ifdef THERMOCHIMICA_ENABLED
  auto temperature = _temperature[_qp];
  auto pressure = _pressure;

  // Set temperature and pressure for thermochemistry solver
  Thermochimica::setTemperaturePressure(temperature, pressure);

  Thermochimica::setElementMass(0, 0.0); // Reset all element masses to 0

  // Set element masses
  for (const auto i : make_range(_n_elements))
    Thermochimica::setElementMass(Thermochimica::atomicNumber(_el_name[i]), (*_el[i])[_qp]);

  // Optionally ask for a re-initialization (if reinit_requested == true)
  reinitDataMooseToTc();

  // Calculate thermochemical equilibrium
  Thermochimica::thermochimica();

  // fetch data for the current node
  auto & d = _data[_current_node->id()];

  // Check for error status
  auto idbg = Thermochimica::checkInfoThermo();
  if (idbg != 0)
  {
    // Thermochimica::printState();
    mooseError("Thermochimica error ", idbg);
  }
  else
  {
    // Get requested phase indices if phase concentration output was requested
    // i.e. if output_phases is coupled
    d._phase_indices.resize(_n_phases);
    for (const auto i : make_range(_n_phases))
    {
      auto [index, idbg] = Thermochimica::getPhaseIndex(_ph_name[i]);
      if (idbg != 0)
        mooseError("Failed to get index of phase '", _ph_name[i], "'");
      // Convert from 1-based (fortran) to 0-based (c++) indexing
      d._phase_indices[i] = index - 1;
    }

    // Save data for future reinits
    reinitDataMooseFromTc();

    d._species_fractions.resize(_n_species);
    for (const auto i : make_range(_n_species))
    {
      auto [fraction, idbg] =
          Thermochimica::getOutputMolSpeciesPhase(_sp_phase_name[i], _sp_species_name[i]);

      if (idbg == 0)
        d._species_fractions[i] = fraction;
      else if (idbg == 1)
        d._species_fractions[i] = 0;
      else
        mooseError("Failed to get phase speciation for phase '",
                   _sp_phase_name[i],
                   "' and species '",
                   _sp_species_name[i],
                   "'. Thermiochimica returned ",
                   idbg);
    }

    if (_output_element_potential)
    {
      d._element_potential_for_output.resize(_element_potentials.size());
      for (const auto i : index_range(_element_potentials))
      {
        auto [potential, idbg] = Thermochimica::getOutputChemPot(_element_potentials[i]);

        if (idbg == 0)
          d._element_potential_for_output[i] = potential;
        else if (idbg == 1)
          // element not present, just leave this at 0 for now
          d._element_potential_for_output[i] = 0.0;
        else if (idbg == -1)
          Moose::out << "getoutputchempot " << idbg << "\n";
      }
    }
  }
#endif
}

void
ThermochimicaNodalData::reinitDataMooseFromTc()
{
#ifdef THERMOCHIMICA_ENABLED
  auto & d = _data[_current_node->id()];

  if (_reinit_requested)
  {
    Thermochimica::saveReinitData();
    auto data = Thermochimica::getReinitData();
    d._assemblage = std::move(data.assemblage);
    d._moles_phase = std::move(data.molesPhase);
    d._element_potential = std::move(data.elementPotential);
    d._chemical_potential = std::move(data.chemicalPotential);
    d._mol_fraction = std::move(data.moleFraction);
    d._elements_used = std::move(data.elementsUsed);
    d._reinit_available = data.reinitAvailable;
  }
  else
  {
    // If phase concentration data output has been requested, _moles_phase is required even if other
    // re-initialization data is not
    if (_n_phases > 0)
      d._moles_phase = Thermochimica::getMolesPhase();

    // If element chemical potential data output has been requested, _element_potential is required
    // even if other re-initialization data is not
    if (_output_element_potential)
      d._element_potential = Thermochimica::getAllElementPotential();
  }
#endif
}

void
ThermochimicaNodalData::reinitDataMooseToTc()
{
#ifdef THERMOCHIMICA_ENABLED
  // Tell Thermochimica whether a re-initialization is requested for this calculation
  Thermochimica::setReinitRequested(_reinit_requested);

  // If we have re-initialization data and want a re-initialization, then
  // load data into Thermochimica
  auto it = _data.find(_current_node->id());
  if (it != _data.end() && _reinit_requested)
  {
    auto & d = it->second;
    if (d._reinit_available)
    {
      Thermochimica::resetReinit();
      Thermochimica::reinitData data;
      data.assemblage = d._assemblage;
      data.molesPhase = d._moles_phase;
      data.elementPotential = d._element_potential;
      data.chemicalPotential = d._chemical_potential;
      data.moleFraction = d._mol_fraction;
      data.elementsUsed = d._elements_used;
      Thermochimica::setReinitData(data);
    }
  }
#endif
}

const ThermochimicaNodalData::Data &
ThermochimicaNodalData::getNodalData(dof_id_type node_id) const
{
  const auto it = _data.find(node_id);
  if (it == _data.end())
    mooseError("Unable to look up data for node ", node_id);
  return it->second;
}
