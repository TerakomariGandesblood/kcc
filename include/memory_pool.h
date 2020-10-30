//
// Created by kaiser on 2019/11/8.
//

#pragma once

#include <boost/pool/object_pool.hpp>

#include "ast.h"
#include "scope.h"
#include "type.h"

namespace kcc {

inline boost::object_pool<UnaryOpExpr> UnaryOpExprPool;
inline boost::object_pool<TypeCastExpr> TypeCastExprPool;
inline boost::object_pool<BinaryOpExpr> BinaryOpExprPool;
inline boost::object_pool<ConditionOpExpr> ConditionOpExprPool;
inline boost::object_pool<FuncCallExpr> FuncCallExprPool;
inline boost::object_pool<ConstantExpr> ConstantExprPool;
inline boost::object_pool<StringLiteralExpr> StringLiteralExprPool;
inline boost::object_pool<IdentifierExpr> IdentifierExprPool;
inline boost::object_pool<EnumeratorExpr> EnumeratorExprPool;
inline boost::object_pool<ObjectExpr> ObjectExprPool;
inline boost::object_pool<StmtExpr> StmtExprPool;

inline boost::object_pool<LabelStmt> LabelStmtPool;
inline boost::object_pool<CaseStmt> CaseStmtPool;
inline boost::object_pool<DefaultStmt> DefaultStmtPool;
inline boost::object_pool<CompoundStmt> CompoundStmtPool;
inline boost::object_pool<ExprStmt> ExprStmtPool;
inline boost::object_pool<IfStmt> IfStmtPool;
inline boost::object_pool<SwitchStmt> SwitchStmtPool;
inline boost::object_pool<WhileStmt> WhileStmtPool;
inline boost::object_pool<DoWhileStmt> DoWhileStmtPool;
inline boost::object_pool<ForStmt> ForStmtPool;
inline boost::object_pool<GotoStmt> GotoStmtPool;
inline boost::object_pool<ContinueStmt> ContinueStmtPool;
inline boost::object_pool<BreakStmt> BreakStmtPool;
inline boost::object_pool<ReturnStmt> ReturnStmtPool;

inline boost::object_pool<TranslationUnit> TranslationUnitPool;
inline boost::object_pool<Declaration> DeclarationPool;
inline boost::object_pool<FuncDef> FuncDefPool;

inline boost::object_pool<VoidType> VoidTypePool;
inline boost::object_pool<ArithmeticType> ArithmeticTypePool;
inline boost::object_pool<PointerType> PointerTypePool;
inline boost::object_pool<ArrayType> ArrayTypePool;
inline boost::object_pool<StructType> StructTypePool;
inline boost::object_pool<FunctionType> FunctionTypePool;

inline boost::object_pool<Scope> ScopePool;

}  // namespace kcc
