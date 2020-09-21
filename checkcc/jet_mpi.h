jet_mpi_send jet_mpi_recv jet_mpi_isend jet_mpi_irecv jet_mpi_gather
    jet_mpi_scatter jet_mpi_allToAll jet_mpi_broadcast jet_mpi_barrier

        function scatter(arr[:, :] as Number, source as Number)
            result(out as Number)

                function scatter(arr, source)
                    result(out) let arr[:, :] as ~kg / s let source
                                       [:, :, :] in 0 : 1 var out
                                       [:] in 0 : 1

    function scatter(arr as Number[:, :], source as Number) as Number[]

    jet_mpi_Array2D_scatter(Array2D arr, )

        var str as String = firstName(5)