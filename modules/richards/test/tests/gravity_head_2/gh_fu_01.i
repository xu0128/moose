# unsaturated = true
# gravity = false
# supg = false
# transient = false

[Mesh]
  type = GeneratedMesh
  dim = 1
  nx = 20
  xmin = 0
  xmax = 1
[]


[GlobalParams]
  richardsVarNames_UO = PPNames
  density_UO = 'DensityWater DensityGas'
  relperm_UO = 'RelPermWater RelPermGas'
  SUPG_UO = 'SUPGwater SUPGgas'
  sat_UO = 'SatWater SatGas'
  seff_UO = 'SeffWater SeffGas'
[]

[UserObjects]
  [./PPNames]
    type = RichardsVarNames
    richards_vars = 'pwater pgas'
  [../]
  [./DensityWater]
    type = RichardsDensityConstBulk
    dens0 = 1
    bulk_mod = 1.0E2
  [../]
  [./DensityGas]
    type = RichardsDensityConstBulk
    dens0 = 0.5
    bulk_mod = 0.5E2
  [../]
  [./SeffWater]
    type = RichardsSeff2waterVG
    m = 0.8
    al = 1
  [../]
  [./SeffGas]
    type = RichardsSeff2gasVG
    m = 0.8
    al = 1
  [../]
  [./RelPermWater]
    type = RichardsRelPermPower
    simm = 0.0
    n = 2
  [../]
  [./RelPermGas]
    type = RichardsRelPermPower
    simm = 0.0
    n = 3
  [../]
  [./SatWater]
    type = RichardsSat
    s_res = 0.1
    sum_s_res = 0.15
  [../]
  [./SatGas]
    type = RichardsSat
    s_res = 0.05
    sum_s_res = 0.15
  [../]
  [./SUPGwater]
    type = RichardsSUPGnone
  [../]
  [./SUPGgas]
    type = RichardsSUPGnone
  [../]
[]

[Variables]
  [./pwater]
    order = FIRST
    family = LAGRANGE
  [../]
  [./pgas]
    order = FIRST
    family = LAGRANGE
  [../]
[]

[ICs]
  [./water_ic]
    type = RandomIC
    min = 0.4
    max = 0.6
    variable = pwater
  [../]
  [./gas_ic]
    type = RandomIC
    min = 1.4
    max = 1.6
    variable = pgas
  [../]
[]


[Kernels]
  active = 'richardsfwater richardsfgas'
  [./richardstwater]
    type = RichardsMassChange
    variable = pwater
  [../]
  [./richardsfwater]
    type = RichardsFullyUpwindFlux
    variable = pwater
  [../]
  [./richardstgas]
    type = RichardsMassChange
    variable = pgas
  [../]
  [./richardsfgas]
    type = RichardsFullyUpwindFlux
    variable = pgas
  [../]
[]


[AuxVariables]
  [./seffgas]
  [../]
  [./seffwater]
  [../]
[]

[AuxKernels]
  [./seffgas_kernel]
    type = RichardsSeffAux
    pressure_vars = 'pwater pgas'
    seff_UO = SeffGas
    variable = seffgas
  [../]
  [./seffwater_kernel]
    type = RichardsSeffAux
    pressure_vars = 'pwater pgas'
    seff_UO = SeffWater
    variable = seffwater
  [../]
[]

[Postprocessors]
  [./mwater_init]
    type = RichardsMass
    variable = pwater
    execute_on = timestep_begin
    outputs = none
  [../]
  [./mgas_init]
    type = RichardsMass
    variable = pgas
    execute_on = timestep_begin
    outputs = none
  [../]
  [./mwater_fin]
    type = RichardsMass
    variable = pwater
    execute_on = timestep_end
    outputs = none
  [../]
  [./mgas_fin]
    type = RichardsMass
    variable = pgas
    execute_on = timestep_end
    outputs = none
  [../]

  [./mass_error_water]
    type = FunctionValuePostprocessor
    function = fcn_mass_error_w
    outputs = none # no reason why mass should be conserved
  [../]
  [./mass_error_gas]
    type = FunctionValuePostprocessor
    function = fcn_mass_error_g
    outputs = none # no reason why mass should be conserved
  [../]

  [./pw_left]
    type = PointValue
    point = '0 0 0'
    variable = pwater
    outputs = none
  [../]
  [./pw_right]
    type = PointValue
    point = '1 0 0'
    variable = pwater
    outputs = none
  [../]
  [./error_water]
    type = FunctionValuePostprocessor
    function = fcn_error_water
  [../]

  [./pg_left]
    type = PointValue
    point = '0 0 0'
    variable = pgas
    outputs = none
  [../]
  [./pg_right]
    type = PointValue
    point = '1 0 0'
    variable = pgas
    outputs = none
  [../]
  [./error_gas]
    type = FunctionValuePostprocessor
    function = fcn_error_gas
  [../]
[]

[Functions]
  [./fcn_mass_error_w]
    type = ParsedFunction
    expression = 'abs(0.5*(mi-mf)/(mi+mf))'
    symbol_names = 'mi mf'
    symbol_values = 'mwater_init mwater_fin'
  [../]
  [./fcn_mass_error_g]
    type = ParsedFunction
    expression = 'abs(0.5*(mi-mf)/(mi+mf))'
    symbol_names = 'mi mf'
    symbol_values = 'mgas_init mgas_fin'
  [../]
  [./fcn_error_water]
    type = ParsedFunction
    expression = 'abs((p0-p1)/p1)'
    symbol_names = 'b gdens0 p0 xval p1'
    symbol_values = '1E2 -1 pw_left 1 pw_right'
  [../]
  [./fcn_error_gas]
    type = ParsedFunction
    expression = 'abs((p0-p1)/p1)'
    symbol_names = 'b gdens0 p0 xval p1'
    symbol_values = '0.5E2 -0.5 pg_left 1 pg_right'
  [../]
[]

[Materials]
  [./rock]
    type = RichardsMaterial
    block = 0
    mat_porosity = 0.1
    mat_permeability = '1E-5 0 0  0 1E-5 0  0 0 1E-5'
    viscosity = '1E-3 0.5E-3'
    gravity = '0 0 0'
    linear_shape_fcns = true
  [../]
[]



[Preconditioning]
  [./andy]
    type = SMP
    full = true
    petsc_options_iname = '-pc_factor_shift_type'
    petsc_options_value = 'nonzero'
  [../]
[]

[Executioner]
  type = Steady
  solve_type = Newton
  nl_rel_tol = 1.e-10
  nl_max_its = 10
[]

[Outputs]
  execute_on = 'timestep_end'
  file_base = gh_fu_01
  csv = true
[]
