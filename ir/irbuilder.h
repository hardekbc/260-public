#pragma once

#include "ir/ir.h"
#include "util/standard_includes.h"

namespace ir {

class Builder {
 public:
  Builder();

  // Adds a new struct type to the entire program.
  Builder& AddStructType(const string& type_name,
                         const map<string, Type>& fields);

  // Signals the start of a new function in the program. Automatically means the
  // end of any currently ongoing basic block and function.
  Builder& StartFunction(const string& name, const Type& return_type);

  // Adds a new parameter to the currently ongoing function.
  Builder& AddParameter(VarPtr_t param);

  // Signals the start of a new basic block in the currently ongoing function.
  // Automatically means the end of any currently ongoing basic block.
  Builder& StartBasicBlock(const string& label);

  // Adds a new instruction to the currently ongoing basic block.
  Builder& AddInstruction(const Instruction& inst);

  // Takes all of the information given so far and uses it to build and return a
  // Program.
  Program FinalizeProgram();

 private:
  // Finalize the current function/basic block being constructed.
  void FinalizeCurrentFunction();
  void FinalizeCurrentBasicBlock();

  // Struct type name ==> (field name ==> type).
  map<string, map<string, Type>> struct_types_;

  // Function name ==> function.
  vector<Function> functions_;

  // Information about the current function being constructed.
  string curr_function_name_;
  Type curr_function_rettype_;
  vector<VarPtr_t> curr_function_parameters_;
  vector<BasicBlock> curr_function_body_;

  // Information about the current basic block being constructed.
  string curr_bb_label_;
  vector<Instruction> curr_bb_body_;
};

}  // namespace ir
