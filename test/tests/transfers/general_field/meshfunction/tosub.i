[Mesh]
  type = GeneratedMesh
  dim = 2
  nx = 10
  ny = 10
[]

[Variables]
  [u]
  []
[]

[Kernels]
  [diff]
    type = Diffusion
    variable = u
  []
[]

[BCs]
  [left]
    type = DirichletBC
    variable = u
    boundary = left
    value = 0
  []
  [right]
    type = DirichletBC
    variable = u
    boundary = right
    value = 1
  []
[]

[AuxVariables]
  [from_sub]
  []
  [elemental_from_sub]
    order = CONSTANT
    family = MONOMIAL
  []
[]

[Executioner]
  type = Transient
  num_steps = 1
  dt = 1

  solve_type = 'PJFNK'

  petsc_options_iname = '-pc_type -pc_hypre_type'
  petsc_options_value = 'hypre boomeramg'
[]

[Outputs]
  exodus = true
[]

[MultiApps]
  [sub]
    positions = '.1 .1 0 0.4 0.4 0 0.7 0.1 0'
    type = TransientMultiApp
    app_type = MooseTestApp
    input_files = tosub_sub.i
    execute_on = timestep_end
  []
[]

[Transfers]
  [to_sub]
    source_variable = u
    direction = to_multiapp
    variable = transferred_u
    type = MultiAppGeneralFieldMeshFunctionTransfer
    multi_app = sub
  []

  [elemental_to_sub]
    source_variable = u
    direction = to_multiapp
    variable = elemental_transferred_u
    type = MultiAppGeneralFieldMeshFunctionTransfer
    multi_app = sub
  []

  [from_sub]
    source_variable = sub_u
    direction = from_multiapp
    variable = from_sub
    type = MultiAppGeneralFieldMeshFunctionTransfer
    multi_app = sub
  []

  [elemental_from_sub]
    source_variable = sub_u
    direction = from_multiapp
    variable = elemental_from_sub
    type = MultiAppGeneralFieldMeshFunctionTransfer
    multi_app = sub
  []
[]
