# finding central diff of a 2d array along ONE of its axis
# (it's not a 2D central diff but 1D diff of a 2D array)
# axis has to be specified

function central2d(Q[:,:], by[], axis) returns (dQ[:,:])
    check axis == 1 or axis == 2
    check size(Q, along = axis) == size(by)

    dQ = zeros(shape(Q))
    let nonaxis = 2 + (1-axis)

    for j = 1:Q.shape[nonaxis]
        if axis == 1
            dQ[:,j] = central1d(Q[:,j], by = by)
            # funcs that take arrays generally have element-level deps within
            # the array. Funcs that are elementwise ops on arrays are reducible
            # to a repeated func call in a loop over all elements. For the former
            # disallow complicated slices like by index list or expression, for the
            # latter they are easy to do since its just a for loop calling the
            # function for each element.
        else
            dQ[j,:] = central1d(Q[j,:], by = by)
        end if
    end for

    # return dQ
end function

# Funcs that take an array as arg always receive a Slice. Passing the entire
# array translates to passing a slice with 1:1:size. The actual array structure
# is not passed. OR ELSE
# func is generated to take Array as well as Slice.

# finding the central diff of an array along its axis (there is only one axis)
function central1d(Q[], by[]) returns (dQ[])
# var has a ASTVar* dimsFrom[8or16] field showing if one or more dims are the same
# as another var
# returns are local vars, not args in C
    check Q.size == by.size
    dQ = zeros(Q.size) # this may or may not require an alloc
    # generates to setzeros(Q.size, &dQ); if dQ is NULL does a malloc else uses dQ
    # #define setzeros(ns, target) \
    #     {if (!target) target=malloc(8*ns); memset(target,0,ns);}
    # just shows that vector dQ comes from Q, i.e. it is of the same shape as Q
    # maybe a copy, or zeros, or whatever, and certainly may be Q itself.
    # the caller has sent in either Q if it is not needed later, or a clone.
    # it means the return target IS q in either case!!!
    for i = [2:Q.size-1]
        dQ[i] = (Q[i+1] - Q[i-1]) / (by[i+1] - by[i-1])
        # since you have a dependence on the array that dQ "comes from",
        # dQ REQUIRES an alloc; it cannot be the same as Q
        # in small cases can use stack alloc
    end for

    dQ = boundary(dQ) # no clone dQ here since its not needed later
# implicit reurn: fill q with dq

    # because dq comes from q, q is marked as 'returnval'. means that 'return' actually translates to setting q to the ret val and returning a ptr.

    # in this function, dQ therefore doesn't really escape (ans goes in q) so
    # dQ can be on the stack if it is small enough, or on alt stack. An initial
    # altstack size should be computed during compilation.

    # in this function, dQ will be marked as dependent, so the caller knows
    # that sending Q in as dQ will cause aliasing and therefore it MUST
    # send a clone in dQ. That clone can be on the caller's stack if it doesn't
    # escape the caller, else pool or heap whatever. Note that caller means the
    # func which made the clone, since the ptr can be passed in more than 1 level
    # deep.
end function

function boundary2d(q[:,:], zero as Boolean) returns (qnew[:,:])
    # check size(Q, along = 1) > 1 THESE ARE IMPLIED SINCE YOU USED -2 as index!
    # check size(Q, along = 2) > 1

    # var qnew = q # implies a clone since Q is let
    qnew = copy(q) # whether or not the copy really happens is upto compiler
    # disallow list of exprs and list of ranges for indexing

    # for the callee, the return target is q when it sees copy(q) or
    # zeros(q.shape) or similar, unless the callee's work really needs a
    # copy of the array anyway (ie. there is no inplacing possible because
    # of an index dependency etc.)
    # the caller, unless it doesnt use the var that is the callee's return
    # target, should clone the var before sending it in. when possible, that
    # clone can be on the callers stack frame etc.
    if not zero
        qnew[1, :] = qnew[2, :] # this causes qnew to be same as q since no dep
        qnew[:, 1] = qnew[:, 2]
        qnew[-1, :] = qnew[-2, :]
        qnew[:, -1] = qnew[:, -2]
        qnew[1, :] = q[2, :] # this is a dep on q, so the copy really happens
        qnew[:, 1] = q[:, 2]
        qnew[-1, :] = q[-2, :]
        qnew[:, -1] = q[:, -2]
    else
        qnew[1, :] = 0  # again no dep of Qnew pn Q
        qnew[:, 1] = 0
        qnew[-1, :] = 0
        qnew[:, -1] = 0
    end if

    # return qnew
end function

function boundary1d(Q,zero) result(Qnew)
    real(d) Q(:)
    real(d) Qnew(size(Q))
    Boolean, optional :: zero
    Boolean zero_
    integer n

    zero_=.false.; if (present(zero)) zero_=zero

    Qnew = Q

    n = size(Q)

    if (.not.zero_) then
        Qnew(1)  = Qnew(2)
        Qnew(n)  = Qnew(n-1)
    else
        Qnew(1)  = 0.0d0
        Qnew(n)  = 0.0d0
    end if

end function
