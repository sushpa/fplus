
static void setStmtFuncTypeInfo(Parser* self, ASTFunc* func)
{
    // this assumes that setExprTypeInfo has been called on the func body
    const ASTExpr* stmt = func->body->stmts->item;
    if (not func->returnSpec->typeType)
        func->returnSpec->typeType = stmt->typeType;
    else if (func->returnSpec->typeType != stmt->typeType)
        Parser_errorTypeMismatchBinOp(self, stmt);
}
