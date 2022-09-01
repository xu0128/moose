[Mesh]
  [gen]
    type = GeneratedMeshGenerator
    nx = 10
    ny = 10
    dim = 2
  []
  [block1]
    type = SubdomainBoundingBoxGenerator
    block_id = 1
    bottom_left = '0 0 0'
    top_right = '0.5 1 0'
    input = gen
  []
  [block2]
    type = SubdomainBoundingBoxGenerator
    block_id = 2
    bottom_left = '0.5 0 0'
    top_right = '1 1 0'
    input = block1
  []
  [breakmesh]
    input = block2
    type = BreakMeshByBlockGenerator
    block_pairs = '1 2'
    split_interface = true
    add_interface_on_two_sides = true
  []
[]

[Variables]
  [temperature]
  []
[]

[Kernels]
  [thermal_cond]
    type = HeatConduction
    variable = temperature
  []
[]

[InterfaceKernels]
  [thin_layer]
    type = ThinLayerHeatTransfer
    thermal_conductivity = thermal_conductivity_layer
    thickness = 0.01
    variable = temperature
    neighbor_var = temperature
    boundary = Block1_Block2
  []
[]

[BCs]
  [left_temp]
    type = DirichletBC
    value = 100
    variable = temperature
    boundary = left
  []
  [right_temp]
    type = DirichletBC
    value = 0
    variable = temperature
    boundary = right
  []
[]

[Materials]
  [thermal_cond]
    type = GenericConstantMaterial
    prop_names = 'thermal_conductivity'
    prop_values = '1'
  []
  [thermal_cond_layer]
    type = GenericConstantMaterial
    prop_names = 'thermal_conductivity_layer'
    prop_values = '0.05'
    boundary = Block1_Block2
  []
[]
[Preconditioning]
  [smp]
    type = SMP
    full = true
  []
[]

[Executioner]
  type = Transient
  solve_type = 'PJFNK'

  petsc_options_iname = '-pc_type -pc_factor_mat_solver_package'
  petsc_options_value = 'lu superlu_dist'

  line_search = none

  nl_rel_tol = 1e-10
  nl_abs_tol = 1e-10

  l_tol = 1e-4
  l_max_its = 50

  nl_max_its = 20

  start_time = 0.0

  dtmin = 1e-8
  dt = 0.05
  automatic_scaling = true
  num_steps = 1
[]


[Outputs]
  print_linear_residuals = false
  exodus = true
[]
