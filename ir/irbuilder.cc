#include "ir/irbuilder.h"

namespace ir {

Builder::Builder() : curr_function_rettype_(Type::Int()) {}

Builder& Builder::AddStructType(const string& type_name,
                                const map<string, Type>& fields) {
  CHECK(struct_types_.count(type_name) == 0)
      << "Struct type name already exists: " << type_name;
  CHECK(!fields.empty()) << "Structs must have at least one field";
  struct_types_[type_name] = fields;
  return *this;
}

Builder& Builder::StartFunction(const string& name, const Type& return_type) {
  CHECK_NE(name, "") << "Function name must be non-empty";
  if (curr_bb_label_ != "") {
    FinalizeCurrentBasicBlock();
    FinalizeCurrentFunction();
  }
  curr_function_name_ = name;
  curr_function_rettype_ = return_type;
  return *this;
}

Builder& Builder::AddParameter(VarPtr_t param) {
  CHECK_NE(curr_function_name_, "")
      << "Cannot add a parameter outside of a function: " << param->ToString();
  curr_function_parameters_.push_back(param);
  return *this;
}

Builder& Builder::StartBasicBlock(const string& label) {
  CHECK_NE(label, "") << "Basic block label must be non-empty";
  CHECK_NE(curr_function_name_, "")
      << "Can't start a basic block outside of a function: " << label;
  if (curr_bb_label_ != "") FinalizeCurrentBasicBlock();
  curr_bb_label_ = label;
  return *this;
}

Builder& Builder::AddInstruction(const Instruction& inst) {
  CHECK_NE(curr_bb_label_, "")
      << "Cannot add an instruction outside a basic block: " << inst.ToString();
  curr_bb_body_.push_back(inst);
  return *this;
}

Program Builder::FinalizeProgram() {
  FinalizeCurrentBasicBlock();
  FinalizeCurrentFunction();
  return Program(struct_types_, functions_);
}

void Builder::FinalizeCurrentFunction() {
  CHECK_NE(curr_function_name_, "") << "Cannot finalize a nonexistent function";
  functions_.emplace_back(curr_function_name_, curr_function_rettype_,
                          curr_function_parameters_, curr_function_body_);
  curr_function_name_ = "";
  curr_function_rettype_ = Type::Int();
  curr_function_parameters_.clear();
  curr_function_body_.clear();
}

void Builder::FinalizeCurrentBasicBlock() {
  CHECK_NE(curr_bb_label_, "") << "Cannot finalize a nonexistent basic block";
  curr_function_body_.emplace_back(curr_bb_label_, curr_bb_body_);
  curr_bb_label_ = "";
  curr_bb_body_.clear();
}

}  // namespace ir
