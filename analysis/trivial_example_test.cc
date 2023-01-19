#include "analysis/trivial_example.h"

#include <gtest/gtest.h>

namespace {

using namespace analysis;
using namespace trivial_example;

// Helper function: translate the analysis solution into a map of strings for
// easy comparison. An instruction pointer will become a string "<basic block
// label>.<index of instruction>", and a VarPtr_t will become the name of the
// variable.
unordered_map<string, set<string>> SolnToString(
    const InstToVars::Solution& soln) {
  unordered_map<string, set<string>> result;

  for (const auto& [inst, vars] : soln) {
    string inst_name =
        inst->parent()->label() + "." + std::to_string(inst->GetIndex());

    for (const auto& var : vars) {
      result[inst_name].insert(var->name());
    }
  }

  return result;
}

TEST(TrivialExampleTest, FirstTest) {
  string code = R"""(
    function main() -> int {
      entry:
        x:int = $copy 6
        y:int = $arith div x:int 2
        $jump while_head

      while_head:
        comp:int = $cmp gt y:int 0
        $branch comp:int while_true exit

      while_true:
        comp2:int = $cmp lt y:int x:int
        $branch comp2:int if_true if_false

      if_true:
        x:int = $arith div x:int y:int
        y:int = $arith sub y:int 1
        $jump if_end

      if_false:
        $jump if_end

      if_end:
        x:int = $arith sub x:int 1
        $jump while_head

      exit:
        $ret x:int
    }
  )""";

  auto program = ir::Program::FromString(code);

  InstToVars analysis(program);
  auto soln = analysis.Analyze("main");

  unordered_map<string, set<string>> expected{
      {"entry.1", {"x"}},          {"while_head.0", {"y"}},
      {"while_head.1", {"comp"}},  {"while_true.0", {"x", "y"}},
      {"while_true.1", {"comp2"}}, {"if_true.0", {"x", "y"}},
      {"if_true.1", {"y"}},        {"if_end.0", {"x"}},
      {"exit.0", {"x"}},
  };

  EXPECT_EQ(SolnToString(soln), expected);
}

TEST(TrivialExampleTest, SecondTest) {
  string code = R"""(
    function main() -> int {
      entry:
        v1:int* = $call foo()
        _x:int = $call sink1(v1:int*)
        v2:int = $load v1:int*
        v3:int* = $addrof v2:int
        _y:int = $call sink2(v3:int*)
        $ret 0
    }

    function foo() -> int* {
      entry:
        p:int* = $alloc
        $ret p:int*
    }
  )""";

  auto program = ir::Program::FromString(code);
  InstToVars analysis(program);

  auto soln_main = analysis.Analyze("main");

  unordered_map<string, set<string>> expected_main{
      {"entry.1", {"v1"}},
      {"entry.2", {"v1"}},
      {"entry.3", {"v2"}},
      {"entry.4", {"v3"}},
  };

  EXPECT_EQ(SolnToString(soln_main), expected_main);

  auto soln_foo = analysis.Analyze("foo");

  unordered_map<string, set<string>> expected_foo{
      {"entry.1", {"p"}},
  };

  EXPECT_EQ(SolnToString(soln_foo), expected_foo);
}

}  // namespace

int main(int argc, char** argv) {
  ::google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
