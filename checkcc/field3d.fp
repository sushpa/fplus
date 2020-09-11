
type Field3D
    var values as Number[:]
    var mesh as Mesh3D
end type

type VectorField3D
    var values as Number[:,:]
    var mesh as Mesh3D
end type

function mean(field as Field3D) as Number
    # volume weighted average of the scalar field's values on the mesh
    mean = sum(field.mesh.volumes * field.values) ...
         / sum(field.mesh.volumes)
end function

function map(source as Field3D, dest as Mesh3D) as Field3D
    map = Field3D(mesh=dest)
    map.values[:] = interp(field=source, atPoints=map.mesh.points[:])
end function

map(source as Field3D, dest as Mesh3D) :=
    Field3D(values=interp(field=source, atPoints=ans.mesh.points), mesh=dest)
