
type Field3D
    var values::Number[:]
    var mesh::Mesh3D
end type

type VectorField3D
    var values::Number[:,:]
    var mesh::Mesh3D
end type

function mean(field::Field3D) :: Number
    # volume weighted average of the scalar field's values on the mesh
    mean = sum(field.mesh.volumes * field.values) ...
         / sum(field.mesh.volumes)
end function

function map(source::Field3D, dest::Mesh3D) :: Field3D
    map = Field3D(mesh=dest)
    map.values[:] = interp(source, atPoints=map.mesh.points[:])
end function

map(source::Field3D, dest::Mesh3D) :=
    Field3D(values=interp(source, atPoints=ans.mesh.points), mesh=dest)
