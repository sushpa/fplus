function unsetPrinted(expr @ASTExpr)
    match expr.kind
    case .identResolved, .varAssign
        expr.var.flags.printed = no
    case .funcCallResolved, .funcCall,
         .subscriptResolved, .subscript,
         .keywordIf, .keywordFor,
         .keywordElse, .keywordWhile
        unsetPrintedVarsFlag(expr.left)
    case else
        if expr.prec
            if not expr.unary then unsetPrintedVarsFlag(expr.left)
            unsetPrintedVarsFlag(expr.right)
        end if
    end match
end function

function genPrintVars(expr[@String] @ASTExpr, level[:,:] @Number)
    let spc @String = repeat("    ", times = level)
    match expr.kind
    case .identResolved, .varAssign
        if expr.var.flags.printed then return
        let name = expr.var.name
        let fmt @String = format(expr.var.init.typeType, quoted = yes)
        print("{{spc}}printf('    {{name}} = {{fmt}}\\n', {{name}});")
        expr.var.flags.printed = yes
    case .funcCallResolved, .funcCall,
         .subscriptResolved, .subscript,
         .keywordIf, .keywordElse,
         .keywordFor, .keywordWhile
        genPrintVars(expr.left, level = level)
    case else
        if expr.prec
            if not expr.unary then genPrintVars(expr.left, level = level)
            genPrintVars(expr.right, level = level)
        end if
    end match
end function

function promotionCandidate(expr @ASTExpr) returns (ret @ASTExpr)
    match expr.kind
    case .funcCallResolved
        try
            ret = promotionCandidate(expr.left)
        catch Error.notFound
            if mustPromote(expr.func.selector) then ret = expr
        end try
    case .subscriptResolved, .subscript,
         .keywordIf, .keywordElse, .keywordFor, .keywordWhile
        ret = promotionCandidate(expr.left) or throw
    case .varAssign
        ret = promotionCandidate(expr.left) or throw
    case else
        if expr.prec
            try
                ret = promotionCandidate(expr.right)
            catch Error.notFound
                if not expr.unary
                    ret = promotionCandidate(expr.right) or throw
                end if
            end try
        end if
    end match
    throw Error.notFound
end function

let prom = promotionCandidate(stmt) or skip/break/return
prom.left = ... # this should work since there is a skip above

function promotionCandidate(expr @ASTExpr) returns (ret @ASTExpr)
    match expr.kind
    case .funcCallResolved
        ret = promotionCandidate(expr.left)
        if ret == nil and mustPromote(expr.func.selector) then ret = expr
    case .subscriptResolved,
         .subscript,
         .keywordIf,
         .keywordElse,
         .keywordFor,
         .keywordWhile,
         .varAssign
        ret = promotionCandidate(expr.left)
    case else
        if expr.prec
            ret = promotionCandidate(expr.right)
            if ret == nil and not expr.unary
                ret = promotionCandidate(expr.right)
            end if
        end if
    end match
end function