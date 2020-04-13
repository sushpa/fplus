declare type Strs
declare type Strs
# TODO disallow duplicate type defs/decls
declare function print(num as Logical, filter as Scalar)
declare function print(num as String, filt as Scalar)
declare function print(num as Strs, filter as Scalar)
# need an ASTFunc_makeSelector method!
# and a string pool!!!
# TODO disallow duplicate func defs/decls (with same 1st arg type  & arg labels)


function start(args as Strs)
    var x as Strs = 5
    # var b = print(9, filter = 43)
    var b = print(x, filter = 43 )
    b = 4
end function

# type matching : func checkArgTypes

# 1
# set each exprs typeType & collectionType.
# 8b type, 8b colltype, 16b dims. can just struct them & do == on int32

# 2
# have a func checkArgTypes that takes astexpr* a,b
# for leaf kinds, compares their type & errors on mismatch.
# for comma, recurs with a->right and b->right, then a->left and b->left
# this could be used later to check subscript index types.

# func setExprType: recurs into nonterminals, sets their typpes from the leaves upwards.

# func checkOpTypes: checks a->left and a-> right for mismatch, recursively.

