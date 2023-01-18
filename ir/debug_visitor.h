#pragma once

#include "ir/ir.h"
#include "ir/irvisitor.h"

namespace ir {

// A wrapper around some visitor that outputs helpful debug messages.
class DebugVisitor : public IrVisitor {
 public:
  DebugVisitor(IrVisitor* visitor, std::ostream& out)
      : visitor_(visitor), out_(out) {}

  void VisitProgram(const Program& program) override {
    out_ << "entering VisitProgram" << std::endl;
    visitor_->VisitProgram(program);
    out_ << "exiting VisitProgram" << std::endl;
  }

  void VisitProgramPost(const Program& program) override {
    out_ << "entering VisitProgramPost" << std::endl;
    visitor_->VisitProgramPost(program);
    out_ << "exiting VisitProgramPost" << std::endl;
  }

  void VisitStructType(const string& name,
                       const map<string, Type>& elements) override {
    out_ << "entering VisitStructType" << std::endl;
    visitor_->VisitStructType(name, elements);
    out_ << "exiting VisitStructType" << std::endl;
  }

  void VisitFunction(const Function& function) override {
    out_ << "entering VisitFunction" << std::endl;
    visitor_->VisitFunction(function);
    out_ << "exiting VisitFunction" << std::endl;
  }

  void VisitFunctionPost(const Function& function) override {
    out_ << "entering VisitFunctionPost" << std::endl;
    visitor_->VisitFunctionPost(function);
    out_ << "exiting VisitFunctionPost" << std::endl;
  }

  void VisitBasicBlock(const BasicBlock& basic_block) override {
    out_ << "entering VisitBasicBlock" << std::endl;
    visitor_->VisitBasicBlock(basic_block);
    out_ << "exiting VisitBasicBlock" << std::endl;
  }

  void VisitBasicBlockPost(const BasicBlock& basic_block) override {
    out_ << "entering VisitBasicBlockPost" << std::endl;
    visitor_->VisitBasicBlockPost(basic_block);
    out_ << "exiting VisitBasicBlockPost" << std::endl;
  }

  void VisitInst(const ArithInst& inst) override {
    out_ << "entering VisitInst(Arith)" << std::endl;
    visitor_->VisitInst(inst);
    out_ << "exiting VisitInst(Arith)" << std::endl;
  }

  void VisitInst(const CmpInst& inst) override {
    out_ << "entering VisitInst(Cmp)" << std::endl;
    visitor_->VisitInst(inst);
    out_ << "exiting VisitInst(Cmp)" << std::endl;
  }

  void VisitInst(const PhiInst& inst) override {
    out_ << "entering VisitInst(Phi)" << std::endl;
    visitor_->VisitInst(inst);
    out_ << "exiting VisitInst(Phi)" << std::endl;
  }

  void VisitInst(const CopyInst& inst) override {
    out_ << "entering VisitInst(Copy)" << std::endl;
    visitor_->VisitInst(inst);
    out_ << "exiting VisitInst(Copy)" << std::endl;
  }

  void VisitInst(const AllocInst& inst) override {
    out_ << "entering VisitInst(Alloc)" << std::endl;
    visitor_->VisitInst(inst);
    out_ << "exiting VisitInst(Alloc)" << std::endl;
  }

  void VisitInst(const AddrOfInst& inst) override {
    out_ << "entering VisitInst(AddrOf)" << std::endl;
    visitor_->VisitInst(inst);
    out_ << "exiting VisitInst(AddrOf)" << std::endl;
  }

  void VisitInst(const LoadInst& inst) override {
    out_ << "entering VisitInst(Load)" << std::endl;
    visitor_->VisitInst(inst);
    out_ << "exiting VisitInst(Load)" << std::endl;
  }

  void VisitInst(const StoreInst& inst) override {
    out_ << "entering VisitInst(Store)" << std::endl;
    visitor_->VisitInst(inst);
    out_ << "exiting VisitInst(Store)" << std::endl;
  }

  void VisitInst(const GepInst& inst) override {
    out_ << "entering VisitInst(Gep)" << std::endl;
    visitor_->VisitInst(inst);
    out_ << "exiting VisitInst(Gep)" << std::endl;
  }

  void VisitInst(const SelectInst& inst) override {
    out_ << "entering VisitInst(Select)" << std::endl;
    visitor_->VisitInst(inst);
    out_ << "exiting VisitInst(Select)" << std::endl;
  }

  void VisitInst(const CallInst& inst) override {
    out_ << "entering VisitInst(Call)" << std::endl;
    visitor_->VisitInst(inst);
    out_ << "exiting VisitInst(Call)" << std::endl;
  }

  void VisitInst(const ICallInst& inst) override {
    out_ << "entering VisitInst(ICall)" << std::endl;
    visitor_->VisitInst(inst);
    out_ << "exiting VisitInst(ICall)" << std::endl;
  }

  void VisitInst(const RetInst& inst) override {
    out_ << "entering VisitInst(Ret)" << std::endl;
    visitor_->VisitInst(inst);
    out_ << "exiting VisitInst(Ret)" << std::endl;
  }

  void VisitInst(const JumpInst& inst) override {
    out_ << "entering VisitInst(Jump)" << std::endl;
    visitor_->VisitInst(inst);
    out_ << "exiting VisitInst(Jump)" << std::endl;
  }

  void VisitInst(const BranchInst& inst) override {
    out_ << "entering VisitInst(Branch)" << std::endl;
    visitor_->VisitInst(inst);
    out_ << "exiting VisitInst(Branch)" << std::endl;
  }

 private:
  // The visitor being debugged.
  IrVisitor* visitor_;

  // The output stream to print things to.
  std::ostream& out_;
};

}  // namespace ir
