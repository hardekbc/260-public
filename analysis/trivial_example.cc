#include "analysis/trivial_example.h"

#include "ir/irvisitor.h"

namespace analysis::trivial_example {

namespace {

// IrVisitor helper class to iterate through a function and map each instruction
// to the variables used in the instruction.
class GetInstVarsVisitor : public ir::IrVisitor {
 public:
  InstToVars::Solution GetSoln(const ir::Function& function) {
    soln_.clear();
    function.Visit(this);
    return soln_;
  }

  void VisitInst(const ir::Instruction& inst) override { curr_inst_ = &inst; }

  void VisitInst(const ir::ArithInst& inst) override {
    if (inst.op1().IsVariable()) soln_[curr_inst_].insert(inst.op1().GetVar());
    if (inst.op2().IsVariable()) soln_[curr_inst_].insert(inst.op2().GetVar());
  }

  void VisitInst(const ir::CmpInst& inst) override {
    if (inst.op1().IsVariable()) soln_[curr_inst_].insert(inst.op1().GetVar());
    if (inst.op2().IsVariable()) soln_[curr_inst_].insert(inst.op2().GetVar());
  }

  void VisitInst(const ir::PhiInst& inst) override {
    for (const auto& op : inst.ops()) {
      if (op.IsVariable()) soln_[curr_inst_].insert(op.GetVar());
    }
  }

  void VisitInst(const ir::CopyInst& inst) override {
    if (inst.rhs().IsVariable()) soln_[curr_inst_].insert(inst.rhs().GetVar());
  }

  void VisitInst(const ir::AllocInst& inst) override {
    // This instruction has no operands.
  }

  void VisitInst(const ir::AddrOfInst& inst) override {
    // This instruction's operand is necessarily a variable.
    soln_[curr_inst_].insert(inst.rhs());
  }

  void VisitInst(const ir::LoadInst& inst) override {
    // This instruction's operand is necessarily a variable.
    soln_[curr_inst_].insert(inst.src());
  }

  void VisitInst(const ir::StoreInst& inst) override {
    // The dst operand is necessarily a variable; the value can be a variable or
    // an integer.
    soln_[curr_inst_].insert(inst.dst());
    if (inst.value().IsVariable()) {
      soln_[curr_inst_].insert(inst.value().GetVar());
    }
  }

  void VisitInst(const ir::GepInst& inst) override {
    // The src_ptr operand is necessarily a variable; the index can be a
    // variable or an integer.
    soln_[curr_inst_].insert(inst.src_ptr());
    if (inst.index().IsVariable()) {
      soln_[curr_inst_].insert(inst.index().GetVar());
    }
  }

  void VisitInst(const ir::SelectInst& inst) override {
    if (inst.condition().IsVariable()) {
      soln_[curr_inst_].insert(inst.condition().GetVar());
    }
    if (inst.true_op().IsVariable()) {
      soln_[curr_inst_].insert(inst.true_op().GetVar());
    }
    if (inst.false_op().IsVariable()) {
      soln_[curr_inst_].insert(inst.false_op().GetVar());
    }
  }

  void VisitInst(const ir::CallInst& inst) override {
    for (const auto& op : inst.args()) {
      if (op.IsVariable()) soln_[curr_inst_].insert(op.GetVar());
    }
  }

  void VisitInst(const ir::ICallInst& inst) override {
    for (const auto& op : inst.args()) {
      if (op.IsVariable()) soln_[curr_inst_].insert(op.GetVar());
    }
  }

  void VisitInst(const ir::RetInst& inst) override {
    if (inst.retval().IsVariable()) {
      soln_[curr_inst_].insert(inst.retval().GetVar());
    }
  }

  void VisitInst(const ir::JumpInst& inst) override {
    // This instruction has no operands.
  }

  void VisitInst(const ir::BranchInst& inst) override {
    if (inst.condition().IsVariable()) {
      soln_[curr_inst_].insert(inst.condition().GetVar());
    }
  }

 private:
  // The desired map, filled in and returned by GetSoln().
  InstToVars::Solution soln_;

  // The current instruction being visited.
  InstPtr_t curr_inst_ = nullptr;
};

}  // namespace

InstToVars::InstToVars(const ir::Program& program) : program_(program) {}

auto InstToVars::Analyze(const string& function_name) -> Solution {
  // The function being analyzed.
  const auto& function = program_[function_name];

  GetInstVarsVisitor visitor;
  auto soln = visitor.GetSoln(function);

  return soln;
}

}  // namespace analysis::trivial_example
