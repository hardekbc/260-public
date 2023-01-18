#include "ir/irbuilder.h"

#include <gtest/gtest.h>

#include "ir/ir_tostring_visitor.h"

namespace {

using namespace ir;

TEST(BuilderTest, DoesItWork) {
  Builder builder;
  Program prog = builder.AddStructType("foo", {{"field", Type::Int()}})
                     .StartFunction("main", Type::Int())
                     .StartBasicBlock("entry")
                     .AddInstruction(JumpInst("foo"))
                     .StartBasicBlock("foo")
                     .AddInstruction(RetInst(42))
                     .StartFunction("foo", Type::Int())
                     .AddParameter(make_shared<Variable>("foo", Type::Int()))
                     .StartBasicBlock("entry")
                     .AddInstruction(RetInst(42))
                     .FinalizeProgram();

  EXPECT_EQ(prog.ToString(), R"""(struct foo {
  field: int
}

function foo(foo:int) -> int {
entry:
  $ret 42
}

function main() -> int {
entry:
  $jump foo

foo:
  $ret 42
}

)""");
}

}  // namespace

int main(int argc, char** argv) {
  ::google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
