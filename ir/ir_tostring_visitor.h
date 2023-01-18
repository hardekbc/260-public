#pragma once

#include "ir/ir.h"
#include "ir/irvisitor.h"

namespace ir {

class ToStringVisitor : public IrVisitor {
 public:
  // Returns the string representation and resets the visitor.
  string GetString() {
    string retval = out_.str();
    out_.clear();
    return retval;
  }

  void VisitProgram(const Program& program) override {
    for (const auto& [name, fields] : program.struct_types()) {
      out_ << "struct " << name << " {" << std::endl;
      for (const auto& [fieldname, fieldtype] : fields) {
        out_ << "  " << fieldname << ": " << fieldtype << std::endl;
      }
      out_ << "}" << std::endl << std::endl;
    }
  }

  void VisitFunction(const Function& function) override {
    out_ << "function " << function.name() << "(";
    if (!function.parameters().empty()) {
      vector<string> params;
      for (const auto& param : function.parameters()) {
        params.push_back(param->ToString());
      }
      std::copy(params.begin(), std::prev(params.end()),
                std::ostream_iterator<string>(out_, ", "));
      out_ << *params.rbegin();
    }
    out_ << ") -> " << function.return_type().ToString() << " {";
  }

  void VisitFunctionPost(const Function& function) override {
    out_ << "}" << std::endl << std::endl;
  }

  void VisitBasicBlock(const BasicBlock& basic_block) override {
    out_ << std::endl << basic_block.label() << ":" << std::endl;
    indent_ = "  ";
  }

  void VisitInst(const ArithInst& inst) override {
    out_ << indent_ << inst.lhs()->ToString() << " = $arith "
         << aop_to_str_.at(inst.operation()) << " " << inst.op1().ToString()
         << " " << inst.op2().ToString() << std::endl;
  }

  void VisitInst(const CmpInst& inst) override {
    out_ << indent_ << inst.lhs()->ToString() << " = $cmp "
         << rop_to_str_.at(inst.operation()) << " " << inst.op1().ToString()
         << " " << inst.op2().ToString() << std::endl;
  }

  void VisitInst(const PhiInst& inst) override {
    out_ << indent_ << inst.lhs()->ToString() << " = $phi(";
    vector<string> ops;
    for (const auto& op : inst.ops()) {
      ops.push_back(op.ToString());
    }
    std::copy(ops.begin(), std::prev(ops.end()),
              std::ostream_iterator<string>(out_, ", "));
    out_ << *ops.rbegin() << ")" << std::endl;
  }

  void VisitInst(const CopyInst& inst) override {
    out_ << indent_ << inst.lhs()->ToString() << " = $copy "
         << inst.rhs().ToString() << std::endl;
  }

  void VisitInst(const AllocInst& inst) override {
    out_ << indent_ << inst.lhs()->ToString() << " = $alloc" << std::endl;
  }

  void VisitInst(const AddrOfInst& inst) override {
    out_ << indent_ << inst.lhs()->ToString() << " = $addrof "
         << inst.rhs()->ToString() << std::endl;
  }

  void VisitInst(const LoadInst& inst) override {
    out_ << indent_ << inst.lhs()->ToString() << " = $load "
         << inst.src()->ToString() << std::endl;
  }

  void VisitInst(const StoreInst& inst) override {
    out_ << indent_ << "$store " << inst.dst()->ToString() << " "
         << inst.value().ToString() << std::endl;
  }

  void VisitInst(const GepInst& inst) override {
    out_ << indent_ << inst.lhs()->ToString() << " = $gep "
         << inst.src_ptr()->ToString() << " " << inst.index().ToString();
    if (inst.field_name() != "") out_ << " " << inst.field_name();
    out_ << std::endl;
  }

  void VisitInst(const SelectInst& inst) override {
    out_ << indent_ << inst.lhs()->ToString() << " = $select "
         << inst.condition().ToString() << " " << inst.true_op().ToString()
         << " " << inst.false_op().ToString() << std::endl;
  }

  void VisitInst(const CallInst& inst) override {
    out_ << indent_ << inst.lhs()->ToString() << " = $call " << inst.callee()
         << "(";
    if (!inst.args().empty()) {
      vector<string> args;
      for (const auto& arg : inst.args()) {
        args.push_back(arg.ToString());
      }
      std::copy(args.begin(), std::prev(args.end()),
                std::ostream_iterator<string>(out_, ", "));
      out_ << *args.rbegin();
    }
    out_ << ")" << std::endl;
  }

  void VisitInst(const ICallInst& inst) override {
    out_ << indent_ << inst.lhs()->ToString() << " = $icall "
         << inst.func_ptr()->ToString() << "(";
    if (!inst.args().empty()) {
      vector<string> args;
      for (const auto& arg : inst.args()) {
        args.push_back(arg.ToString());
      }
      std::copy(args.begin(), std::prev(args.end()),
                std::ostream_iterator<string>(out_, ", "));
      out_ << *args.rbegin();
    }
    out_ << ")" << std::endl;
  }

  void VisitInst(const RetInst& inst) override {
    out_ << indent_ << "$ret " << inst.retval().ToString() << std::endl;
  }

  void VisitInst(const JumpInst& inst) override {
    out_ << indent_ << "$jump " << inst.label() << std::endl;
  }

  void VisitInst(const BranchInst& inst) override {
    out_ << indent_ << "$branch " << inst.condition().ToString() << " "
         << inst.label_true() << " " << inst.label_false() << std::endl;
  }

 private:
  // The string representation.
  std::ostringstream out_;

  // The indentation for instructions. Initially empty, in case we're only
  // visiting instructions and not basic blocks; if we ever visit a basic block
  // it's set to "  ".
  string indent_;

  // Convert ArithInst::Aop into string.
  inline static const map<ArithInst::Aop, string> aop_to_str_{
      {ArithInst::kAdd, "add"},
      {ArithInst::kSubtract, "sub"},
      {ArithInst::kMultiply, "mul"},
      {ArithInst::kDivide, "div"},
  };

  // Convert CmpInst::Rop into string.
  inline static const map<CmpInst::Rop, string> rop_to_str_{
      {CmpInst::kEqual, "eq"},          {CmpInst::kNotEqual, "neq"},
      {CmpInst::kLessThan, "lt"},       {CmpInst::kGreaterThan, "gt"},
      {CmpInst::kLessThanEqual, "lte"}, {CmpInst::kGreaterThanEqual, "gte"},
  };
};

}  // namespace ir
