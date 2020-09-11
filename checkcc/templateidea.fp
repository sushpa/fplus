
gSum(buffer as Number)        := allReduce(buffer, op=.sum)
gSum(buffer[] as Number)      := allReduce(buffer, op=.sum)
gSum(buffer[:,:] as Number)   := allReduce(buffer, op=.sum)
gSum(buffer[:,:,:] as Number) := allReduce(buffer, op=.sum)
gSum(buffer as Int)         := allReduce(buffer, op=.sum)
gSum(buffer[] as Int)       := allReduce(buffer, op=.sum)
gSum(buffer[:,:] as Int)    := allReduce(buffer, op=.sum)
gSum(buffer[:,:,:] as Int)  := allReduce(buffer, op=.sum, team=6)

template (T, S)
function gSum(buffer as T) as (output as T)
    output = allReduce(buffer, op=.sum)
end function

gSum(buffer as <T>) := allReduce(buffer, op=.sum)

function allReduce(buffer as <T>, op as MPIOperations, team = kMPICOMMWORLD)
    if not mpi.enabled then return
    var output as <T>
    ccall('MPI_AllReduce', buffer, output, buffer.size, mpiType(buffer), op, team)
end function

enum MPITypes
end enum

enum MPIOperations
end enum

mpiType(buffer as Number) := MPITypes.real
mpiType(buffer as Number[]) := MPITypes.real
mpiType(buffer as Number[:,:]) := MPITypes.real
mpiType(buffer as Number[:,:,:]) := MPITypes.real
mpiType(buffer as Number or Number[] or Number[:,:] or Number[:,:,:]) := MPITypes.real

# REALLY MUST GET RID OF REAL/INT AND KEEP ONE UNIFIED NUM TYPE

gSum(buffer as Number or Number[] or Number[:,:] or Number[:,:,:]) := MPITypes.real
# throw syntax error if user defines gSum again for one of the types from here
# actually that is what will happen anyway since the generated funcs from here
# will be in conflict!


NOPE gSum(buffer as Number[?]) := MPITypes.real
NOPE gSum(buffer as Number[:,:]) := gSum2D(buffer) # let's say there is such a func
# I know in case of multiple matches the most specific can be taken simply
# but the user will always think WHICH ONE IS GOING TO BE USED????
# SO FOR HEAVENS SAKE LET IT GO
# NO TEMPLATING!!!


# basically this is an elemental func:
NOPE function func(arg as Number[?])
# where [?] means this is a template (not on the type but) on the dimensions.
# it does the same thing as F90 elemental funcs: the same func is valid for
# scalar as well as array args of any dimension, with the caveat that it can
# only operate elementwise (no reductions etc.)
# the syntax suggests I can index arg within the func, which is not true.
# SO DROP IT!!

func(x as Number) := 4 + x
function func(x as Number[]) as Number[]
    resize(&out, like=x) # arrays cannot be null, but empty. they are init'd empty too, not as null
    out = zerosLike(x)
    out.size = x.size
    for i = 1:len(x) do out[i] = func(x[i])
    out[-1] = 3
    out[1:-1] = 51.2
    out[1:] = 5
end function
