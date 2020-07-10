fp_mpi_send
fp_mpi_recv
fp_mpi_isend
fp_mpi_irecv
fp_mpi_gather
fp_mpi_scatter
fp_mpi_allToAll
fp_mpi_broadcast
fp_mpi_barrier

function scatter(arr[:,:] as Number, source as Number) result (out as Number)
 
function scatter(arr, source) result (out)
    let arr[:,:] as ~kg/s
    let source[:,:,:] in 0:1
    var out[:] in 0:1



function scatter(arr as Number[:,:], source as Number) as Number[]

fp_mpi_Array2D_scatter(Array2D arr, )

var str as String = firstName(5)