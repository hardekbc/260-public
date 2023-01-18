#pragma once

#include "util/standard_includes.h"

namespace ir {

// Need to forward declare all of the IR classes.
class Type;
class Program;
class Function;
class BasicBlock;
class Instruction;
class ArithInst;
class CmpInst;
class PhiInst;
class CopyInst;
class AllocInst;
class AddrOfInst;
class LoadInst;
class StoreInst;
class GepInst;
class SelectInst;
class CallInst;
class ICallInst;
class RetInst;
class JumpInst;
class BranchInst;

// Will visit all components of a program from the most general to the most
// specific: Program -> StructType -> Function -> BasicBlock -> Instruction ->
// specific kind of instruction. The *Post methods will be called after each
// relevant component's sub-components have all been visited (i.e.,
// VisitProgramPost will be called after all its struct types and functions are
// visited, VisitFunctionPost will be called after all its basic blocks are
// visited, and VisitBasicBlockPost will be called after all its instructions
// are visited).
class IrVisitor {
 public:
  virtual ~IrVisitor() {}

  virtual void VisitProgram(const Program& program) {}
  virtual void VisitProgramPost(const Program& program) {}
  virtual void VisitStructType(const string& name,
                               const map<string, Type>& elements) {}
  virtual void VisitFunction(const Function& function) {}
  virtual void VisitFunctionPost(const Function& function) {}
  virtual void VisitBasicBlock(const BasicBlock& basic_block) {}
  virtual void VisitBasicBlockPost(const BasicBlock& basic_block) {}
  virtual void VisitInst(const Instruction& inst) {}
  virtual void VisitInst(const ArithInst& inst) {}
  virtual void VisitInst(const CmpInst& inst) {}
  virtual void VisitInst(const PhiInst& inst) {}
  virtual void VisitInst(const CopyInst& inst) {}
  virtual void VisitInst(const AllocInst& inst) {}
  virtual void VisitInst(const AddrOfInst& inst) {}
  virtual void VisitInst(const LoadInst& inst) {}
  virtual void VisitInst(const StoreInst& inst) {}
  virtual void VisitInst(const GepInst& inst) {}
  virtual void VisitInst(const SelectInst& inst) {}
  virtual void VisitInst(const CallInst& inst) {}
  virtual void VisitInst(const ICallInst& inst) {}
  virtual void VisitInst(const RetInst& inst) {}
  virtual void VisitInst(const JumpInst& inst) {}
  virtual void VisitInst(const BranchInst& inst) {}
};

}  // namespace ir
