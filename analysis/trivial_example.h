#pragma once

#include "ir/ir.h"
#include "util/standard_includes.h"

namespace analysis::trivial_example {

using ir::VarPtr_t;
using InstPtr_t = const ir::Instruction*;
using VarSet = unordered_set<VarPtr_t>;

// A trivial analysis example that, given a function, returns a map from each
// instruction in the function to the set of variables used in that instruction.
class InstToVars {
 public:
  // A solution is a map from instructions to sets of variables.
  using Solution = unordered_map<InstPtr_t, VarSet>;

  // The constructor argument is the program to analyze.
  InstToVars(const ir::Program& program);

  // Analyze the given function and return its solution.
  Solution Analyze(const string& function_name);

 private:
  // The program being analyzed.
  ir::Program program_;
};

}  // namespace analysis::trivial_example
