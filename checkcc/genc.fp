#define genLineNumbers 0
#define genCoverage 1
#define genLineProfile 1


function ASTImport_genc(import as ASTImport, level as Integer)
{
    str_tr_ip(import.importFile, '.', '_', 0)
    print("\n#include \"%s.h\"\n", import.importFile)
    if import.hasAlias)
        print("#define %s %s\n", import.importFile + import.aliasOffset,
            import.importFile)
    str_tr_ip(import.importFile, '_', '.', 0)
end


function ASTImport_undefc(import as ASTImport)
{
    if import.hasAlias)
        print("#undef %s\n", import.importFile + import.aliasOffset)
end


function ASTTypeSpec_genc(typeSpec as ASTTypeSpec, level as Integer, isconst as YesOrNo)
{
    if isconst) print("const ")
    # TODO: actually this depends on the collectionType. In general
    # Array is the default, but in other cases it may be SArray, Array64,
    # whatever
    if typeSpec.dims) {
        if typeSpec.dims > 1)
            # TODO: this should be TensorND, without typ params?
            # well actually there isn't a TensorND, since its not always
            # double thats in a tensor but can be Complex, Range,
            # Reciprocal, Rational, whatever
            # -- sure, but double (and float) should be enough since
            # the other types are rarely needed in a tensor form
            print("SArray%dD(", typeSpec.dims)
        else
            print("SArray(")
    end

    switch (typeSpec.typeType) {
    case .object:
        # objects are always T* const, if meant to be r/o they are
        # const T* const. Later we may have a byval flag to embed structs
        # or pass around by value.
        # leaving it as is for now
        print("%s", typeSpec.typ.name)
        break
    case .unresolved:
        unreachable("unresolved: '%s' at %d:%d", typeSpec.name, typeSpec.line,
            typeSpec.col)
        print("%s", typeSpec.name)
        break
    default:
        print("%s", TypeType_name(typeSpec.typeType))
        break
    end

    #     if isconst ) print(" const") # only if a ptr typ
    if typeSpec.dims /*or typeSpec.typeType == .object*/) print("%s", ")")
    #        if status == TSDimensionedNumber) {
    #            generate(units, level)
    #        end
end

function generate(
    self as ASTExpr, level as Integer, spacing as YesOrNo, inFuncArgs as YesOrNo, escStrings as YesOrNo)


function generate(var as ASTVar, level as Integer, isconst as YesOrNo)
{
    # for C the variables go at the top of the block, without init
    print("%.*s", level, spaces)
    if var.typeSpec) ASTTypeSpec_genc(var.typeSpec, level + STEP, isconst)
    print(" %s", var.name)
end


# Functions like Array_any_filter, Array_count_filter etc.
# are macros and don't out = a value but may set one. For these
# and other such funcs, the call must be moved to before the
# containing statement, and in place of the original call you
# should place a temporary holding the value that would have been
# "returned".
function YesOrNo mustPromote(const char* name)
{
    # TODO: at some point these should go into a dict or trie or MPH
    # whatever
    if not strcmp(name, "Array_any_filter")) out = true
    if not strcmp(name, "Array_all_filter")) out = true
    if not strcmp(name, "Array_count_filter")) out = true
    if not strcmp(name, "Array_write_filter")) out = true
    if not strcmp(name, "Strs_print_filter")) out = true
    out = no
end


function unmarkVisited(expr as ASTExpr)
{
    switch (expr.kind) {
    case .identifierResolved:
    case .varAssign:
        expr.var.visited = no
        break
    case .functionCallResolved:
    case .functionCall: # shouldnt happen
    case .subscriptResolved:
    case .subscript:
    case .keyword_if:
    case .keyword_for:
    case .keyword_else:
    case .keyword_while:
        unmarkVisited(expr.left)
        break
    default:
        if expr.prec) {
            if not expr.unary) unmarkVisited(expr.left)
            unmarkVisited(expr.right)
        end
    end
end


# given an expr, generate code to print all the resolved vars in it (only
# scalars). for example in f(x + 4) + m + y[5:6], the following should be
# generated
# print("x = %?\n", x)
# print("m = %?\n", m)
# checks will print the vars involved in the check expr, if the check
# fails. This routine will be used there.
function genPrintVars(expr as ASTExpr, level as Integer)
{
    assert(expr)
    # what about func args?
    switch (expr.kind) {
    case .identifierResolved:
    case .varAssign:
        if expr.var.visited) break
        print("%.*sprintf(\"    %s = %s\\n\", %s)\n", level, spaces,
            expr.var.name, TypeType_format(expr.typeType, true),
            expr.var.name)
        expr.var.visited = true
        break

    case .period:
        #        {
        #            ASTExpr* e = expr.right
        #            while (e.kind==.period) e=e.right
        ##            if e.var.visited) break
        #            print("%.*sprintf(\"    %s = %s\\n\", %s)\n", level,
        #            spaces,
        #                   expr.var.name, TypeType_format(e.typeType,
        #                   true), expr.var.name)
        #        end
        break

    case .functionCallResolved:
    case .functionCall: # shouldnt happen
    case .subscriptResolved:
    case .subscript:
    case .keyword_if:
    case .keyword_else:
    case .keyword_for:
    case .keyword_while:
        genPrintVars(expr.left, level)
        break

    default:
        if expr.prec) {
            if not expr.unary) genPrintVars(expr.left, level)
            genPrintVars(expr.right, level)
        end
    end
end


# Promotion scan & promotion happens AFTER resolving functions!
function promotionCandidate(expr as ASTExpr) result (out as ASTExpr)
{
    assert(expr)
    var ret as ASTExpr

    # what about func args?
    match expr.kind
    case .functionCallResolved:
        # promote innermost first, so check args
        if expr.left and (ret = promotionCandidate(expr.left)))
            out = ret
        else if mustPromote(expr.func.selector))
            out = expr
        break

    case .subscriptResolved:
        # TODO: here see if the subscript itself needs to be promoted up
        out = promotionCandidate(expr.left)

    case .subscript:
        out = promotionCandidate(expr.left)

    case .keyword_if:
    case .keyword_for:
    case .keyword_else:
    case .keyword_elif:
    case .keyword_while:
        if expr.left) out = promotionCandidate(expr.left)
        # body will be handled by parent scope

    case .varAssign:
        if (ret = promotionCandidate(expr.var.init))) out = ret
        break

    case .functionCall: # unresolved
        # assert(0)
        unreachable("unresolved call %s\n", expr.string)
        if (ret = promotionCandidate(expr.left))) out = ret
        break

    else:
        if expr.prec) {
            if (ret = promotionCandidate(expr.right))) out = ret
            if not expr.unary)
                if (ret = promotionCandidate(expr.left))) out = ret
        end
    end
    out = missing
end

function char* newTmpVarName(num as Integer, char c)
{
    char buf[8]
    var l as Integer = snprintf(buf, 8, "_%c%d", c, num)
    out = pstrndup(buf, l)
end


function  isCtrlExpr(expr as ASTExpr) as YesOrNo
{
    out = expr.kind == .keyword_if #
        or expr.kind == .keyword_for #
        or expr.kind == .keyword_while #
        or expr.kind == .keyword_else
end

function  isLiteralExpr(expr as ASTExpr) as YesOrNo { out = no
end
function  isComparatorExpr(expr as ASTExpr)  as YesOrNo { out = no
end


function lowerElementalOps(scope as ASTScope)
{
    for stmt = scope.stmts # fp_foreach(ASTExpr*, stmt, scope.stmts)

        if isCtrlExpr(stmt) and stmt.body) then lowerElementalOps(stmt.body)

        if not stmt.elemental) then skip

        # wrap it in an empty block (or use if true)
        var ifblk = ASTExpr() with {
            .kind = .keyword_if
            .left = ASTExpr() with {
                .kind = .number
            }
            .string = "1"
        }

        # look top-down for subscripts. if you encounter a node with
        # elemental=no, don't process it further even if it may have
        # ranges inside. e.g.
        # vec[7:9] = arr2[6:8] + sin(arr2[-6:-1:-4]) + test[[8,6,5]]
        # + 3 + count(vec[vec < 5]) + M ** x[-8:-1:-4]
        # the matmul above is not
        # elemental, but the range inside it is.
        # the Array_count_filter will be promoted and isnt elemental
        # (unless you plan to set elemental op on boolean subscripts.)
        # Even so, count is a reduce op and will unset elemental.
        # --
        # as you find each subscript, add 2 local vars to the ifblk body
        # so then you might have for the above example :
        # T* vec_p1 = vec.start + 7
        # # ^ this func could be membptr(a,i) . i<0 ? a.end-i :
        # a.start+i #define vec_1 *vec_p1 # these could be ASTVars with
        # an isCMacro flag T2* arr2_p1 = membptr(arr2, 6) #define arr2_1
        # *arr2_p1 T3* arr2_p2 = membptr(arr2, -6) #define arr2_2 *arr2_p2
        # ...
        # # now add vars for each slice end and delta
        # const T* const vec_e1 = vec.start + 9 # use membptr
        # const T2* const arr2_e1 = arr2.start + 8 # membptr
        # const T3* const arr2_e2 = membptr(arr2, -4)
        # what about test[[8,6,5]]?
        # const T* const vec_d1 = 1
        # const T2* const arr2_d1 = 1
        # const T3* const arr2_d2 = -1
        # ...
        # # the ends (and starts) could be used for BC.
        # ...
        # # now add a check / separate checks for count match and bounds
        # check_span1deq(vec_e1,vec_p1,arr2_e1,arr2_p1,col1,col2,"vec[7:9]","arr2[6:8]",__FILE__,__LINE__)
        # check_span1deq(arr2_e1,arr2_p1,arr2_e2,arr2_p2,col1,col2,"arr2[6:8]","arr2[-6:-4]",__FILE__,__LINE__)
        # check_inbounds1d(vec, vec_p1, vec_e1,col1,
        # "vec[7:9]",__FILE__,__LINE__) check_inbounds1d(arr2, arr2_p1,
        # arr2_e1,col1, "arr2[6:8]",__FILE__,__LINE__) now change the
        # subscripts in the stmt to unresolved idents, and change the ident
        # by appending _1, _2 etc. based on their position. so when they
        # are generated they will refer to the current item of that array.
        # then wrap the stmt in a for expr 'forblk'. put the for expr in
        # ifblk. the active scope is now the for's body. generate the stmt.
        # it should come out in Number form if all went well.. add
        # increments for each ptr. vec_p1 += vec_d1 arr2_p1 += arr2_d1
        # arr2_p2 += arr2_d2
        # ...
        # all done, at the end put the ifblk at the spot of stmt in the
        # original scope. stmt is already inside ifblk inside forblk.
    end
end


function promoteCandidates(scope as ASTScope)
{
    var tmpCount as Integer = 0
    var pc as ASTExpr = missing
    var prev[] as ASTExpr
    # List(ASTExpr)* prev = missing
    for stmt = scope.stmts #fp_foreachn(ASTExpr*, stmt, stmts, scope.stmts)
    # {
        # TODO:
        # if not stmt.promote) {prev=stmtscontinueend

        if isCtrlExpr(stmt) and stmt.body) then promoteCandidates(stmt.body)

    startloop:
        pc = promotionCandidate(stmt)
        if missing(pc)  # most likely
            prev = stmts
            skip
        end
        if pc == stmt
            # possible, less likely: stmt already at toplevel.
            # TODO: in this case, you still have to add the extra arg.
            prev = stmts
            skip
        end

        var pcClone as ASTExpr = clone(pc)

        # 1. add a temp var to the scope
        tmpCount += 1
        var tmpvar as ASTVar = ASTVar() with {
            .name = "_1p$tmpCount" #newTmpVarName(tmpCount, 'p')
            .typeSpec = ASTTypeSpec()
        }
        #        tmpvar.typeSpec.typeType = .real64 # FIXME
        # TODO: setup tmpvar.typeSpec
        #push(&scope.locals, item = tmpvar)
        scope.locals[end+1] = tmpvar

        # 2. change the original to an ident
        pc.kind = .identifierResolved
        pc.prec = 0
        pc.var = tmpvar

        # 3. insert the tmp var as an additional argument into the call

        if not pcClone.left
            pcClone.left = pc
        else if pcClone.left.kind != .opComma
            # single arg
            pcClone.left = ASTExpr() with {
            # TODO: really should have an astexpr ctor
                .kind = TokenKinds.opComma
                .prec = precedence(TokenKinds.opComma)
                .left = pcClone.left
                .right = pc
            }
            # pcClone.left = com
        else
            var argn as ASTExpr = pcClone.left
            while argn.kind == .opComma and argn.right.kind == .opComma
                argn = argn.right
            end while
            argn.right = ASTExpr() with {
            # TODO: really should have an astexpr ctor
                .kind = TokenKinds.opComma
                .prec = precedence(TokenKinds.opComma)
                .left = argn.right
                .right = pc
            }
#            argn.right = com
        end

        # 4. insert the promoted expr BEFORE the current stmt
        #        PtrList_append(prev ? &prev : &self.stmts, pcClone)
        #        PtrList* tmp = prev.next
        # THIS SHOULD BE in PtrList as insertAfter method
        if not prev
            scope.stmts = fp_PtrList_with(pcClone)
            scope.stmts.next = stmts
            prev = scope.stmts
        else
            prev.next = fp_PtrList_with(pcClone)
            prev.next.next = stmts
            prev = prev.next
        end # List(ASTExpr)* insertionPos = prev ? prev.next : self.stmts
          #  insertionPos
        #  = insertionPos
        goto startloop # it will continue there if no more promotions are
                        # needed

        prev = stmts
    end
end


function generate(scope as ASTScope, level as Integer)
{
    for local = scope.locals #fp_foreach(ASTVar*, local, scope.locals)
    if local.used
        generate(local, level = level, static = no)
        print()
    end if # these will be declared at top and defined within the expr list

    for stmt in scope.stmts #fp_foreach(ASTExpr*, stmt, scope.stmts)
    #{
        if stmt.kind == .lineComment then skip

        if genLineNumbers then print("#line %d\n", stmt.line)
        if genCoverage then print("_cov_[%d]++\n", stmt.line - 1)
        if genLineProfile
            print("_lprof_tmp_ = getticks()\n")
            print("_lprof_[%d] += (_lprof_tmp_-_lprof_last_)/100\n",
                stmt.line - 1)
            print("_lprof_last_ = _lprof_tmp_\n")
        end
        generate(stmt, level, true, no, no)
        if not isCtrlExpr(stmt) and stmt.kind != .keyword_return)
            print("")
        else
            print("")
        # convert this into a flag which is set in the resolution pass
        if throws(stmt)
            print("    if _err_ == ERROR_TRACE) goto backtrace")
        end if
    end
end


function genJson(typ as ASTType)
{
    print("function %s_json_(const %s self, Integer nspc) {\n", typ.name,
        typ.name)

    print("    print(\"{\\n\")\n")
    # print("    print(\"\\\"_type_\\\": \\\"%s\\\"\")\n", typ.name)
    # if typ.body.locals) print("    print(\",\\n\")\n")

    # TODO: move this part into its own func so that subclasses can ask the
    # superclass to add in their fields inline
    fp_foreachn(ASTVar*, var, vars, typ.body.locals)
    {
        if not var then skip
        print("    print(\"%%.*s\\\"%s\\\": \", nspc+4, _spaces_)\n",
            var.name)
        var valueType as Text = typeName(var.init)
        print("    %s_json_(self.%s, nspc+4)\n    print(\"", valueType,
            var.name)
        if vars.next) print(",")
        print("\\n\")\n")
    end
    print("    print(\"%%.*send\", nspc, _spaces_)\n")
    print("end\nMAKE_json_wrap_(%s)\n#MAKE_json_file(%s)\n", typ.name,
        typ.name)
    # print("#define %s_json(x) { print(\"\\\"%%s\\\": \",#x) "
    #        "%s_json_wrap_(x)
    # end\n\n",
    #     typ.name, typ.name)
end


function genJsonReader(typ as ASTType)
 end function

let functionEntryStuff_UNESCAPED = '
    #ifndef NOSTACKCHECK
        STACKDEPTH_UP
        // print("%8lu %8lu\n",_scUsage_, _scSize_)
        if _scUsage_ >= _scSize_) {
    #ifdef DEBUG
        _scPrintAbove_ = _scDepth_ - _btLimit_;
        print("\e[31mfatal: stack overflow at call depth %lu.\n    in %s\e[0m\n", _scDepth_, sig_);
        print("\e[90mBacktrace (innermost first):\n");
        if _scDepth_ > 2*_btLimit_)
            print("    limited to %d outer and %d inner entries.\n", _btLimit_, _btLimit_);
            print("[%lu] \e[36m%s\n", _scDepth_, callsite_);
    #else
            print("\e[31mfatal: stack  overflow at call depth %lu.\e[0m\n", _scDepth_);
    #endif
            DONE
        }
    #endif
    '

let functionExitStuff_UNESCAPED = '
    out = DEFAULT_VALUE;
    assert(0);
    error:
    #ifdef DEBUG
        eprintf("error: %s\n",_err_);
    #endif
    backtrace:
    #ifdef DEBUG
        if _scDepth_ <= _btLimit_ || _scDepth_ > _scPrintAbove_)
            print("\e[90m[%lu] \e[36m%s\n", _scDepth_, callsite_);
        else if _scDepth_ == _scPrintAbove_)
            print("\e[90m... truncated ...\e[0m\n");
    #endif
    done:
    #ifndef NOSTACKCHECK
        STACKDEPTH_DOWN
    #endif
        out = DEFAULT_VALUE;
    '

function printStackUsageDef(stackUsage as Integer)
    print('
        #ifdef DEBUG
        #define MYSTACKUSAGE ($stackUsage + 6*sizeof(void*) + sizeof(char*))
        #else
        #define MYSTACKUSAGE ($stackUsage + 6*sizeof(void*))
        #endif
        ')
end function


function generate(typ as ASTType, level as Integer)
{
    if not typ.body or not typ.analysed then return
    var name as Text = typ.name
    print('#define FIELDS_$name')
    for local = typ.body.locals # fp_foreach(ASTVar*, var, typ.body.locals)
    {
        if not local then skip
        generate(local, level = level + STEP, no)
        print()
    end
    print('

    struct $name {
    ')
    if typ.super
        print("    FIELDS_")
        generate(typ.super, level=level, other=no)
        print()
    end

    print('
        FIELDS_$name
        }

        static const char* $name_name_ = "$name"
        static $name $name_alloc_() {
            out = _Pool_alloc_(&gPool_, sizeof(struct $name));
        }
        static $name $name_init_($name self) {')

    # fp_foreach(ASTVar*, var, typ.body.locals)
    #     print("#define %s self.%s\n", var.name, var.name)
    for local in typ.body.locals
        print('#define $local.name self.$local.name')
    end for

    fp_foreach(ASTExpr*, stmt, typ.body.stmts)
    {
        if not stmt or stmt.kind != .varAssign or not stmt.var.init)
            continue
        print("%.*s%s = ", level + STEP, spaces, stmt.var.name)
        generate(stmt.var.init, 0, true, no, no)
        print("")
        if throws(stmt.var.init))
            print("    if _err_ == ERROR_TRACE) out = missing")
    end
    fp_foreach(ASTVar*, var, typ.body.locals)
        print("#undef %s \n", var.name)

    print("    out = self\nend\n")
    print(
        "%s %s_new_(\n#ifdef DEBUG\n    const char* callsite_\n#endif\n) {\n",
        name, name)
    print("#define DEFAULT_VALUE missing\n#ifdef DEBUG\n    function const char* "
           "sig_ = \"%s()\"\n#endif\n",
        name)
    printStackUsageDef(48)
    print(functionEntryStuff_UNESCAPED)
    print("    %s ret = %s_alloc_() %s_init_(ret) if _err_==ERROR_TRACE) "
           "goto backtrace"
           "_err_ = missing \n#ifndef NOSTACKCHECK\n    "
           "STACKDEPTH_DOWN\n#endif\n     out = ret",
        name, name, name)
    print(functionExitStuff_UNESCAPED)
    print("#undef DEFAULT_VALUE\n#undef MYSTACKUSAGE\nend")
    print("#define %s_print_(p) %s_print__(p, STR(p))\n", name, name)
    print("void %s_print__(%s self, const char* name) {\n    print(\"<%s "
           "'%%s' at %%p\\n>\",name,self)\nend\n",
        name, name, name)
    print("")

    genJson(typ)
    genJsonReader(typ)
end


function generateHeader(typ as ASTType, level as Integer)
{
    if not typ.body or not typ.analysed) out =
    const char* const name = typ.name
    print("typedef struct %s* %s\nstruct %s\n", name, name, name)
    print("function %s %s_alloc_() \n", name, name)
    print("function %s %s_init_(%s self)\n", name, name, name)
    print(
        "%s %s_new_(\n#ifdef DEBUG\n    const char* callsite_\n#endif\n) \n",
        name, name)
    print("\nDECL_json_wrap_(%s)\n#DECL_json_file(%s)\n", name, name)
    print("#define %s_json(x) { print(\"\\\"%%s\\\": \",#x) "
           "%s_json_wrap_(x)
           end\n\n",
        name, name)
    print("function %s_json_(const %s self, Integer nspc)\n", name, name)
end


function generate(func as ASTFunc, level as Integer)
{
    if not func.body or not func.analysed) out = # declares, default ctors

    # actual stack usage is higher due to stack protection, frame bookkeeping
    # ...
    Integer stackUsage = calcSizeUsage(func)
    printStackUsageDef(stackUsage)

    print(
        "#define DEFAULT_VALUE %s\n", getDefaultValueForType(func.returnSpec))
    if not func.isExported) print("function ")
    if func.returnSpec) {
        ASTTypeSpec_genc(func.returnSpec, level, no)
    end else {
        print("void")
    end
    print(" %s(", func.selector)
    fp_foreachn(ASTVar*, arg, args, func.args)
    {
        generate(arg, level, true)
        print(args.next ? ", " : "")
    end

    print("\n#ifdef DEBUG\n"
           "    %c const char* callsite_ "
           "\n#endif\n",
        ((func.args and func.args.item ? ',' : ' ')))

    # TODO: if flags.throws) print("const char** _err_")
    print(") {")
    print("#ifdef DEBUG\n"
           "    function const char* sig_ = \"")
    print("%s%s(", func.isStmt ? "" : "function ", func.name)

    fp_foreachn(ASTVar*, arg, args, func.args)
    {
        gen(arg, level)
        print(args.next ? ", " : ")")
    end
    if func.returnSpec) {
        print(" returns ")
        ASTTypeSpec_gen(func.returnSpec, level)
    end
    print("\"\n#endif")

    print(functionEntryStuff_UNESCAPED)

    generate(func.body, level + STEP)

    print(functionExitStuff_UNESCAPED)
    print("end\n#undef DEFAULT_VALUE")
    print("#undef MYSTACKUSAGE")
end


function generateHeader(func as ASTFunc, level as Integer)
{
    if not func.body or not func.analysed) out =
    if not func.isExported) print("function ")
    if func.returnSpec) {
        ASTTypeSpec_genc(func.returnSpec, level, no)
    end else {
        print("void")
    end
    print(" %s(", func.selector)
    fp_foreachn(ASTVar*, arg, args, func.args)
    {
        generate(arg, level, true)
        print(args.next ? ", " : "")
    end
    print("\n#ifdef DEBUG\n    %c const char* callsite_ "
           "\n#endif\n",
        ((func.args and func.args.item) ? ',' : ' '))
    print(")\n")
end

##########################

function ASTTest_genc(test as ASTTest)
{
    # TODO: should tests not out = BOOL?
    if not test.body) out =
    print("\nstatic void test_%s() {\n", test.name)
    generate(test.body, STEP)
    print("end")
end


function generate(
    expr as ASTExpr, level as Integer, spacing as YesOrNo, inFuncArgs as YesOrNo, escStrings as YesOrNo)
{
    # generally an expr is not split over several lines (but maybe in
    # rare cases). so level is not passed on to recursive calls.

    print("%.*s", level, spaces)
    switch (expr.kind) {
    case .number: {
        ls as Integer = strlen(expr.string)
        if expr.string[ls - 1] == 'i') {
            print("_Complex_I*")
            expr.string[ls - 1] = 0
        end
        print("%s", expr.string)
    end break
    case .multiDotNumber:
    case .identifier:
        print("%s", expr.string)
        break

    case .string:
        # TODO: parse vars inside, escape stuff, etc.
        print(escStrings ? "\\%s\\\"" : "%s\"", expr.string)
        break

    case .identifierResolved:
        print("%s", expr.var.name)
        break

    case .regex:
        # 'raw strings' or 'regexes'
        print("\"%s\"", expr.string + 1)
        break

    case .inline:
        # inline C code?
        print("%s", expr.string + 1)
        break

    case .lineComment:
        # TODO: skip  comments in generated code
        print("# %s", expr.string)
        break

    case .functionCall:
        unreachable("unresolved call to %s\n", expr.string)
        assert(0)
        break

    case .functionCallResolved: {
        var tmp as Text = expr.func.selector

        var arg1 as ASTExpr = expr.left
        const char* tmpc = ""
        if arg1) {
            if arg1.kind == .opComma) arg1 = arg1.left
            tmpc = CollectionType_nativeName(arg1.collectionType)
        end
        print("%s%s", tmpc, tmp)
        if *tmp >= 'A' and *tmp <= 'Z' and not strchr(tmp, '_'))
            print("_new_")
        print("(")

        if expr.left) generate(expr.left, 0, no, true, escStrings)

        if not expr.func.isDeclare) {
            print("\n#ifdef DEBUG\n"
                   "      %c \"./\" THISFILE \":%d:%d:\\e[0m ",
                expr.left ? ',' : ' ', expr.line, expr.col)
            gen(expr, 0, no, true)
            print("\"\n"
                   "#endif\n        ")
        end
        print(")")
    end break

    case .subscript:
        assert(0)
        break

    case .subscriptResolved: {
        char* name = expr.var.name
        index as ASTExpr = expr.left
        assert(index)
        switch (index.kind) {
        case .number:
            # indexing with a single number
            # can be a -ve number
            print("Array_get_%s(%s, %s)",
                ASTTypeSpec_cname(expr.var.typeSpec), name, index.string)
            break

        case .string:
        case .regex:
            # indexing with single string or regex
            print("Dict_get_CString_%s(%s, %s)",
                ASTTypeSpec_cname(expr.var.typeSpec), name, index.string)
            break

        case .opComma:
            # higher dimensions. validation etc. has been done by this
            # stage.

            # this is for cases like arr[2, 3, 4].
            print("Tensor%dD_get_%s(%s, {", expr.var.typeSpec.dims,
                ASTTypeSpec_cname(expr.var.typeSpec), name)
            generate(index, 0, no, inFuncArgs, escStrings)
            print("end)")

            # TODO: cases like arr[2:3, 4:5, 1:end]
            # basically the idea is to generate getijk/getIJK/getIJk etc.
            # where a caps means range and lowercase means single number.
            # so arr[2:3, 4:5, 1:end] should generate `getIJK`,
            # arr[2:3, 4, 2:end] should generate `getIjK` and so on.
            # Those are then macros in the "runtime" that have for loops
            # for the ranges and nothing special for the single indices.
            # but they should be put into a tmpvar to avoid repeated eval.

            break

        case .opColon:
            # a single range.
            print("Array_getSlice_%s(%s, ",
                ASTTypeSpec_name(expr.var.typeSpec), name)
            generate(index, 0, no, inFuncArgs, escStrings)
            print(")")
            break
            # what about mixed cases, e.g. arr[2:3, 5, 3:end]
            # make this portion a recursive function then, or promote
            # all indexes to ranges first and then let opcomma handle it

        case .opEQ:
        case .opLE:
        case .opGE:
        case .opGT:
        case .opLT:
        case .opNE:
        case .keyword_and:
        case .keyword_or:
        case .keyword_not:
            # indexing by a Boolean expression (filter)
            # by default this implies a copy, but certain funcs e.g. print
            # min max sum count etc. can be done in-place without a copy
            # since they are not mutating the array. That requires either
            # the user to call print(arr, filter = arr < 5) instead of
            # print(arr[arr < 5]), or the compiler to transform the second
            # into the first transparently.
            # Probably the .functionCall should check if its argument is
            # a .subscript with a Boolean index, and then tip the user
            # to call the optimised function instead (or just generate it).
            # For now, and in the absence of more context, this is a copy.
            # Array_copy_filter is implemented as a C macro for loop, as
            # are most other filtering-enabled functions on arrays.
            # TODO: be careful with the "template" style call here xx()()
            # TODO: actually I think arr[arr < 5] etc. should just be
            # promoted
            #    and then the generation will follow the modified AST.
            #    Don't handle this as a special case at the code generation
            #    stage.
            print("Array_copy_filter_%s(%s, ",
                ASTTypeSpec_name(expr.var.typeSpec), name)
            generate(index, 0, no, inFuncArgs, escStrings)
            print(")")
            break

        default:
            assert(0)
            break
        end
        break
    end

    case .opAssign:
    case .plusEq:
    case .minusEq:
    case .timesEq:
    case .slashEq:
    case .powerEq:
    case .opModEq:

        switch (expr.left.kind) {
        case .subscriptResolved:
            switch (expr.left.left.kind) {
            case .number:
            case .string:
            case .regex:
                # TODO: astexpr_typename should out = Array_Scalar or
                # Tensor2D_Scalar or Dict_String_Scalar etc.
                print("%s_set(%s, %s,%s, ", typeName(expr.left),
                    expr.left.var.name, expr.left.left.string,
                    repr(expr.kind, spacing))
                generate(expr.right, 0, spacing, inFuncArgs, escStrings)
                print(")")
                break

            case .opColon:
                print("%s_setSlice(%s, ", typeName(expr.left),
                    expr.left.var.name)
                generate(
                    expr.left.left, 0, spacing, inFuncArgs, escStrings)
                print(",%s, ", repr(expr.kind, spacing))
                generate(expr.right, 0, spacing, inFuncArgs, escStrings)
                print(")")
                break

            case .opEQ:
            case .opGE:
            case .opNE:
            case .opGT:
            case .opLE:
            case .opLT:
            case .keyword_and:
            case .keyword_or:
            case .keyword_not:
                print("%s_setFiltered(%s, ", typeName(expr.left),
                    expr.left.var.name)
                generate(
                    expr.left.left, 0, spacing, inFuncArgs, escStrings)
                print(",%s, ", repr(expr.kind, spacing))
                generate(expr.right, 0, spacing, inFuncArgs, escStrings)
                print(")")
                break

            case .opComma:
                # figure out the typ of each element
                # there should be a RangeND just like TensorND and SliceND
                # then you can just pass that to _setSlice
                break
            case .identifierResolved:
                # lookup the var typ. note that it need not be Number,
                # string, range etc. it could be an arbitrary object in
                # case you are indexing a Dict with keys of that typ.
                break
            case .subscriptResolved:
                # arr[arr2[4]] etc.
                break
            case .functionCallResolved:
                # arr[func(x)]
                break
            default:
                unreachable("%s\n", str[expr.left.kind])
                assert(0)
            end
            break
        case .identifierResolved:
        case .period:
            generate(expr.left, 0, spacing, inFuncArgs, escStrings)
            print(repr(expr.kind, spacing=spacing))
            generate(expr.right, 0, spacing, inFuncArgs, escStrings)
            break
        case .identifier:
            check inFuncArgs
            generate(expr.right, 0, spacing, inFuncArgs, escStrings)
            # function call arg label, do not generate .left
            break
        default:
            # error: not a valid lvalue
            # TODO: you should at some point e,g, during resolution check
            # for assignments to invalid lvalues and raise an error
            assert(0)
        end
        # if not inFuncArgs) {
        #     generate(self.left, 0, spacing, inFuncArgs,
        #     escStrings) print("%s", repr(.opAssign,
        #     spacing))
        # end
        # generate(self.right, 0, spacing, inFuncArgs, escStrings)
        # check various types of lhs  here, eg arr[9:87] = 0,
        # map["uuyt"]="hello" etc.
        break

    case .arrayOpen:
        # TODO: send parent ASTExpr* as an arg to this function. Then
        # here do various things based on whether parent is a =,
        # funcCall, etc.
        print("mkarr(((%s[]) {", "double") # FIXME
        # TODO: MKARR should be different based on the CollectionType
        # of the var or arg in question, eg stack cArray, heap
        # allocated Array, etc.
        generate(expr.right, 0, spacing, inFuncArgs, escStrings)
        print("end)")
        print(", %d)", countCommaList(expr.right))
        break

    case .opColon: # convert 3:4:5 to range(...)
                    # must do bounds check first!
        print(
            "%s(", expr.left.kind != .opColon ? "range_to" : "range_to_by")
        if expr.left.kind == .opColon) {
            expr.left.kind = .opComma
            generate(expr.left, 0, no, inFuncArgs, escStrings)
            expr.left.kind = .opColon
        end else
            generate(expr.left, 0, no, inFuncArgs, escStrings)
        print(", ")
        generate(expr.right, 0, no, inFuncArgs, escStrings)
        print(")")
        break

    case .varAssign: # basically a .opAssign corresponding to a local
                      # var
        # var x as XYZ = abc... . becomes an ASTVar and an
        # ASTExpr (to keep location). Send it to ASTVar::gen.
        if expr.var.init != nil and expr.var.used
            print("$expr.var.name = ")
            generate(expr.var.init, 0, true, inFuncArgs, escStrings)
        end
        break

    case .keyword_else:
        print("else {")
        if expr.body) generate(expr.body, level + STEP)
        print("$indent end", level, spaces)
        break

    case .keyword_elif:
        print("else if ")
        generate(expr.left, 0, spacing, inFuncArgs, escStrings)
        print(") {")
        if expr.body) generate(expr.body, level + STEP)
        print("%.*send", level, spaces)
        break

    case .keyword_for:
    case .keyword_if:
        #    case .keyword_elif:
        #    case .keyword_else:
    case .keyword_while:
        if expr.kind == .keyword_for)
            print("FOR(")
        else
            print("%s (", repr(expr.kind, true))
        if expr.kind == .keyword_for) expr.left.kind = .opComma
        if expr.left)
            generate(expr.left, 0, spacing, inFuncArgs, escStrings)
        if expr.kind == .keyword_for) expr.left.kind = .opAssign
        print(") {")
        if expr.body) generate(expr.body, level + STEP)
        print("%.*send", level, spaces)
        break

    case .power:
        print("pow(")
        generate(expr.left, 0, no, inFuncArgs, escStrings)
        print(",")
        generate(expr.right, 0, no, inFuncArgs, escStrings)
        print(")")
        break

    case .keyword_return:
        print("{_err_ = missing \n#ifndef NOSTACKCHECK\n    "
               "STACKDEPTH_DOWN\n#endif\nreturn ")
        generate(expr.right, 0, spacing, inFuncArgs, escStrings)
        print("end\n")
        break
    case .keyword_check: {
        # TODO: need llhs and lrhs in case all 3 in 3way are exprs
        # e.g. check a+b < c+d < e+f
        checkExpr as ASTExpr = expr.right # now use checkExpr below
        lhsExpr as ASTExpr = checkExpr.left
        rhsExpr as ASTExpr = checkExpr.right
        print("{\n")
        if not checkExpr.unary) {
            print("%.*s%s _lhs = ", level, spaces, typeName(lhsExpr))
            generate(lhsExpr, 0, spacing, no, no)
            print("\n")
        end
        print("%.*s%s _rhs = ", level, spaces, typeName(rhsExpr))
        generate(rhsExpr, 0, spacing, no, no)
        print("\n")
        print("%.*sif not(", level, spaces)
        generate(checkExpr, 0, spacing, no, no)
        print(")) {\n")
        print("%.*sprintf(\"\\n\\n\e[31mruntime error:\e[0m check "
               "failed at "
               "\e[36m./%%s:%d:%d:\e[0m\\n    "
               "%%s\\n\\n\", THISFILE, \"",
            level + STEP, spaces, expr.line, expr.col + 6)
        gen(checkExpr, 0, spacing, true)
        print("\")\n")
        print("#ifdef DEBUG\n%.*sprintf(\"\e[90mHere's some "
               "help:\e[0m\\n\")\n",
            level + STEP, spaces)

        genPrintVars(checkExpr, level + STEP)
        # the `printed` flag on all vars of the expr will be set
        # (genPrintVars uses this to avoid printing the same var
        # twice). This should be unset after every toplevel call to
        # genPrintVars.
        if not checkExpr.unary) {
            # dont print literals or arrays
            if lhsExpr.collectionType == C.none #
                and lhsExpr.kind != .string #
                and lhsExpr.kind != .number #
                and lhsExpr.kind != .regex #
                and lhsExpr.kind != .opLE #
                and lhsExpr.kind != .opLT) {

                if lhsExpr.kind != .identifierResolved
                    or not lhsExpr.var.visited) {
                    print(
                        "%.*s%s", level + STEP, spaces, "print(\"    %s = ")
                    print("%s", TypeType_format(lhsExpr.typeType, true))
                    print("%s", "\\n\", \"")
                    gen(lhsExpr, 0, spacing, true)
                    print("%s", "\", _lhs)\n")
                end
                # checks can't have .varAssign inside them
                # if )
                #     lhsExpr.var.visited = true
            end
        end
        if rhsExpr.collectionType == C.none #
            and rhsExpr.kind != .string #
            and rhsExpr.kind != .number #
            and rhsExpr.kind != .regex) {
            if rhsExpr.kind != .identifierResolved
                or not rhsExpr.var.visited) {
                print("%.*s%s", level + STEP, spaces, "print(\"    %s = ")
                print("%s", TypeType_format(rhsExpr.typeType, true))
                print("%s", "\\n\", \"")
                gen(rhsExpr, 0, spacing, true)
                print("%s", "\", _rhs)\n")
            end
        end

        unmarkVisited(checkExpr)

        print("%.*sprintf(\"\\n\")\n", level + STEP, spaces)
        print("%s",
            /*"#ifdef DEBUG\n"*/
            "        print(\"\\e[90mBacktrace (innermost "
            "first):\\n\")\n"
            "        if _scDepth_ > 2*_btLimit_)\n        "
            "print(\"    limited to %d outer and %d inner "
            "entries.\\n\", "
            "_btLimit_, _btLimit_)\n"
            "        BACKTRACE\n    \n"
            "#else\n"
            "        eputs(\"(run in debug mode to get more info)\\n\") "
            "exit(1)\n"
            "#endif\n")
        print("\n%.*send\n", level, spaces)
        print("%.*send", level, spaces)
    end break

    case .period:
        generate(expr.left, 0, spacing, inFuncArgs, escStrings)
        print(".") # may be . if right is embedded and not a reference
        generate(expr.right, 0, spacing, inFuncArgs, escStrings)
        break

    case .opEQ:
    case .opNE:
    case .opGE:
    case .opLE:
    case .opGT:
    case .opLT:
        if (expr.kind == .opLE or expr.kind == .opLT)
            and (expr.left.kind == .opLE or expr.left.kind == .opLT)) {
            print("%s_cmp3way_%s_%s(", typeName(expr.left.right),
                ascrepr(expr.kind, no),
                ascrepr(expr.left.kind, no))
            generate(expr.left.left, 0, spacing, inFuncArgs, escStrings)
            print(", ")
            generate(expr.left.right, 0, spacing, inFuncArgs, escStrings)
            print(", ")
            generate(expr.right, 0, spacing, inFuncArgs, escStrings)
            print(")")
            break
        end else if expr.right.typeType == .string) {
            print("str_cmp(%s, ", repr[expr.kind])
            generate(expr.left, 0, spacing, inFuncArgs, escStrings)
            print(", ")
            generate(expr.right, 0, spacing, inFuncArgs, escStrings)
            print(")")
            break
        end
        fallthrough default : if not expr.prec) break
        # not an operator, but this should be error if you reach here
        var leftBr as YesOrNo
            = expr.left
            and expr.left.prec
            and expr.left.prec < expr.prec

        var rightBr as YesOrNo =
            expr.right
            and expr.right.prec
            and expr.right.kind != .keyword_return
            and expr.right.prec < expr.prec
        # found in 'or out ='

        char lpo = '('
        char lpc = ')'
        if leftBr) putc(lpo, stdout)
        if expr.left)
            generate(expr.left, 0,
                spacing and not leftBr and expr.kind != .opColon, inFuncArgs,
                escStrings)
        if leftBr) putc(lpc, stdout)

        if expr.kind == .arrayOpen)
            putc('{', stdout)
        else
            print("%s", repr(expr.kind, spacing))

        char rpo = '('
        char rpc = ')'
        if rightBr) putc(rpo, stdout)
        if expr.right)
            generate(expr.right, 0,
                spacing and not rightBr and expr.kind != .opColon, inFuncArgs,
                escStrings)
        if rightBr) putc(rpc, stdout)

        if expr.kind == .arrayOpen) putc('end', stdout)
    end
end

# WARNING: DO NOT USE THESE STRINGS WITH PRINTF(...) USE PUTS(...).
function const char* coverageFunc[] = { #
    "function fp_coverage_report() { /* unused */ end",
    "function fp_coverage_report() {\n"
    "    Integer count=0,l=NUMLINES\n"
    "    while(--l>0) count+=!!_cov_[l]\n"
    "    print(\"coverage: %d/%d lines = %.2f%%\\n\","
    "        count, NUMLINES, count*100.0/NUMLINES)\n"
    "end"
end
# WARNING: DO NOT USE THESE STRINGS WITH PRINTF(...) USE PUTS(...).
function const char* lineProfileFunc[] = {
    #
    "function fp_lineprofile_report() { /* unused */ end\n"
    "function fp_lineprofile_begin() { /* unused */ end\n",
    "function fp_lineprofile_report() {\n"
    # "    print(\"profiler: %llu cycles\\n\","
    # "        _lprof_[NUMLINES-1]-_lprof_[0])\n"
    "    FILE* fd = fopen(\".\" THISFILE \"r\", \"w\")\n"
    # "    for (Integer i=1 i<NUMLINES i++)\n"
    # "        if 0== _lprof_[i]) _lprof_[i] = _lprof_[i-1]\n"
    # "    for (Integer i=NUMLINES-1 i>0 i--) _lprof_[i] -= _lprof_[i-1]\n"
    "    ticks sum=0 for (Integer i=0 i<NUMLINES i++) sum += _lprof_[i]\n"
    "    for (Integer i=0 i<NUMLINES i++) {\n"
    "        double pct = _lprof_[i] * 100.0 / sum\n"
    "        if pct>1.0)"
    "            fprintf(fd,\" %8.1f%% |\\n\", pct)\n"
    "        else if pct == 0.0)"
    "            fprintf(fd,\"           |\\n\")\n"
    "        else"
    "            fprintf(fd,\"         ~ |\\n\")\n"
    "    end\n"
    "    fclose(fd)\n"
    "    system(\"paste -d ' ' .\" THISFILE \"r \" THISFILE \" > \" "
    "THISFILE "
    "\"r\" )"
    "end\n"
    "function fp_lineprofile_begin() {_lprof_last_=getticks()end\n"
end

# TODO: why do you need to pass level here?
function ASTModule_genc(module as ASTModule, level as Integer)
{
    # print("")
    fp_foreach(ASTImport*, import, module.imports)
        ASTImport_genc(import, level)

    print("")

    for typ as ASTType = module.types do generateHeader(typ, level=level)
    for func as ASTFunc = module.funcs do generateHeader(func, level=level)
    fp_foreach(ASTType*, typ, module.types) generateHeader(typ, level)
    fp_foreach(ASTFunc*, func, module.funcs) generateHeader(func, level)

    fp_foreach(ASTType*, typ, module.types) generate(typ, level)
    fp_foreach(ASTFunc*, func, module.funcs) generate(func, level)
    fp_foreach(ASTImport*, import, module.imports) ASTImport_undefc(import)

    print(coverageFunc[genCoverage])
    print(lineProfileFunc[genLineProfile])
end

function ASTModule_genTests(module as ASTModule, level as Integer)
{
    ASTModule_genc(module, level)
    fp_foreach(ASTTest*, test, module.tests) ASTTest_genc(test)
    # generate a func that main will call
    print("\nvoid tests_run_%s() {\n", module.name)
    fp_foreach(ASTTest*, test, module.tests)
        print("    test_%s()\n", test.name)
    print("end")
end
