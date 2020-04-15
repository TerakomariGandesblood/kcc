//
// Created by kaiser on 2019/11/8.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>

#include "ast.h"
#include "scope.h"
#include "type.h"

namespace kcc {

template <typename T, std::size_t block_size = 4096>
class MemoryPool {
 public:
  using ValueType = T;
  using Pointer = T*;
  using ConstPointer = const T*;
  using SizeType = std::size_t;

  MemoryPool() noexcept = default;
  ~MemoryPool() noexcept;

  MemoryPool(const MemoryPool&) = delete;
  MemoryPool(MemoryPool&&) = delete;
  MemoryPool& operator=(const MemoryPool&) = delete;
  MemoryPool& operator=(MemoryPool&&) = delete;

  Pointer Allocate(SizeType n = 1, ConstPointer hint = 0);

 private:
  union Slot {
    ValueType element;
    Slot* next;
  };

  using DataPointer = char*;
  using SlotType = Slot;
  using SlotPointer = Slot*;

  SlotPointer current_block_{};
  SlotPointer current_slot_{};
  SlotPointer last_slot_{};
  SlotPointer free_slots_{};

  void AllocateBlock();
  SizeType PadPointer(DataPointer p, SizeType align) const noexcept;

  static_assert(block_size >= 2 * sizeof(SlotType), "BlockSize too small");
};

template <typename T, size_t block_size>
MemoryPool<T, block_size>::~MemoryPool() noexcept {
  auto curr{current_block_};
  while (curr != nullptr) {
    auto prev{curr->next};
    operator delete(reinterpret_cast<void*>(curr));
    curr = prev;
  }
}

template <typename T, size_t block_size>
inline typename MemoryPool<T, block_size>::Pointer
MemoryPool<T, block_size>::Allocate(SizeType, ConstPointer) {
  if (free_slots_ != nullptr) {
    auto result{reinterpret_cast<Pointer>(free_slots_)};
    free_slots_ = free_slots_->next;
    return result;
  } else {
    if (current_slot_ >= last_slot_) {
      AllocateBlock();
    }
    return reinterpret_cast<Pointer>(current_slot_++);
  }
}

template <typename T, size_t block_size>
void MemoryPool<T, block_size>::AllocateBlock() {
  auto new_block{reinterpret_cast<DataPointer>(operator new(block_size))};
  reinterpret_cast<SlotPointer>(new_block)->next = current_block_;
  current_block_ = reinterpret_cast<SlotPointer>(new_block);

  auto body{new_block + sizeof(SlotPointer)};
  auto body_padding{PadPointer(body, alignof(SlotType))};
  current_slot_ = reinterpret_cast<SlotPointer>(body + body_padding);
  last_slot_ = reinterpret_cast<SlotPointer>(new_block + block_size -
                                             sizeof(SlotType) + 1);
}

template <typename T, size_t block_size>
inline typename MemoryPool<T, block_size>::SizeType
MemoryPool<T, block_size>::PadPointer(DataPointer p,
                                      SizeType align) const noexcept {
  auto result{reinterpret_cast<std::uintptr_t>(p)};
  return ((align - result) % align);
}

inline MemoryPool<UnaryOpExpr> UnaryOpExprPool;
inline MemoryPool<TypeCastExpr> TypeCastExprPool;
inline MemoryPool<BinaryOpExpr> BinaryOpExprPool;
inline MemoryPool<ConditionOpExpr> ConditionOpExprPool;
inline MemoryPool<FuncCallExpr> FuncCallExprPool;
inline MemoryPool<ConstantExpr> ConstantExprPool;
inline MemoryPool<StringLiteralExpr> StringLiteralExprPool;
inline MemoryPool<IdentifierExpr> IdentifierExprPool;
inline MemoryPool<EnumeratorExpr> EnumeratorExprPool;
inline MemoryPool<ObjectExpr> ObjectExprPool;
inline MemoryPool<StmtExpr> StmtExprPool;

inline MemoryPool<LabelStmt> LabelStmtPool;
inline MemoryPool<CaseStmt> CaseStmtPool;
inline MemoryPool<DefaultStmt> DefaultStmtPool;
inline MemoryPool<CompoundStmt> CompoundStmtPool;
inline MemoryPool<ExprStmt> ExprStmtPool;
inline MemoryPool<IfStmt> IfStmtPool;
inline MemoryPool<SwitchStmt> SwitchStmtPool;
inline MemoryPool<WhileStmt> WhileStmtPool;
inline MemoryPool<DoWhileStmt> DoWhileStmtPool;
inline MemoryPool<ForStmt> ForStmtPool;
inline MemoryPool<GotoStmt> GotoStmtPool;
inline MemoryPool<ContinueStmt> ContinueStmtPool;
inline MemoryPool<BreakStmt> BreakStmtPool;
inline MemoryPool<ReturnStmt> ReturnStmtPool;

inline MemoryPool<TranslationUnit> TranslationUnitPool;
inline MemoryPool<Declaration> DeclarationPool;
inline MemoryPool<FuncDef> FuncDefPool;

inline MemoryPool<VoidType> VoidTypePool;
inline MemoryPool<ArithmeticType> ArithmeticTypePool;
inline MemoryPool<PointerType> PointerTypePool;
inline MemoryPool<ArrayType> ArrayTypePool;
inline MemoryPool<StructType> StructTypePool;
inline MemoryPool<FunctionType> FunctionTypePool;

inline MemoryPool<Scope> ScopePool;

}  // namespace kcc
