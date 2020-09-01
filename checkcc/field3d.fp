

type Field3D
    var values[:] as Scalar
    var mesh as Mesh3D
end type

type VectorField3D
    var values[:,:] as Scalar
    var mesh as Mesh3D
end type

function mean(field as Field3D) result (ans as Scalar)
    -- volume weighted average of the scalar field's values on the mesh
    ans = sum(field.values * field.mesh.volumes) / field.mesh.volume
end function

function map(source as Field3D, dest as Mesh3D) result (ans as Field3D)
    ans = Field3D(mesh=dest)
    ans.values[:] = interp(field=source, atPoints=ans.mesh.points[:])
end function

map(source as Field3D, dest as Mesh3D) := 
    Field3D(values=interp(field=source, atPoints=ans.mesh.points), mesh=dest)