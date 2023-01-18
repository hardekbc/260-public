#include "ir/ir.h"

#include <gtest/gtest.h>

#include <filesystem>

#include "ir/ir_tostring_visitor.h"
#include "ir/irvisitor.h"

namespace {

using namespace ir;

// Returns whether 'str' ends in 'suffix'.
bool EndsWith(const string& str, const string& suffix) {
  return str.size() >= suffix.size() &&
         str.compare(str.size() - suffix.size(), string::npos, suffix) == 0;
}

class IrTest : public ::testing::Test {
 protected:
  BasicBlock MakeBasicBlock(const string& label, const vector<string> codes) {
    vector<Instruction> insts;
    for (const auto& code : codes) {
      if (code == "arith") {
        insts.push_back(arith_inst_);
      } else if (code == "cmp") {
        insts.push_back(cmp_inst_);
      } else if (code == "phi") {
        insts.push_back(phi_inst_);
      } else if (code == "copy") {
        insts.push_back(copy_inst_);
      } else if (code == "alloc") {
        insts.push_back(alloc_inst_);
      } else if (code == "addrof") {
        insts.push_back(addrof_inst_);
      } else if (code == "load") {
        insts.push_back(load_inst_);
      } else if (code == "store") {
        insts.push_back(store_inst_);
      } else if (code == "gep") {
        insts.push_back(gep_inst_);
      } else if (code == "select") {
        insts.push_back(select_inst_);
      } else if (code == "call") {
        insts.push_back(call_inst_);
      } else if (code == "icall") {
        insts.push_back(icall_inst_);
      } else if (code == "ret") {
        insts.push_back(ret_inst_);
      } else if (code == "jump") {
        insts.push_back(jump_inst_);
      } else if (code == "branch") {
        insts.push_back(branch_inst_);
      } else {
        LOG(FATAL) << "unknown inst";
      }
    }

    return BasicBlock(label, insts);
  }

  Function MakeFunction(const string& name, const vector<BasicBlock>& blocks) {
    return Function(name, Type::Int(), {}, blocks);
  }

  Program MakeProgram(const map<string, map<string, Type>>& struct_types,
                      const vector<Function>& functions) {
    return Program(struct_types, functions);
  }

  VarPtr_t var_ = make_shared<Variable>("foo", Type::Int());
  VarPtr_t varp_ = make_shared<Variable>("foop", Type::Int().PtrTo());
  VarPtr_t fun_ =
      make_shared<Variable>("fun", Type::Function({Type::Int()}).PtrTo());
  ArithInst arith_inst_{var_, 42, 42, ArithInst::kAdd};
  CmpInst cmp_inst_{var_, 42, 42, CmpInst::kEqual};
  PhiInst phi_inst_{var_, {42, 42}};
  CopyInst copy_inst_{var_, 42};
  AllocInst alloc_inst_{varp_};
  AddrOfInst addrof_inst_{varp_, var_};
  LoadInst load_inst_{var_, varp_};
  StoreInst store_inst_{varp_, 42};
  GepInst gep_inst_{varp_,
                    make_shared<Variable>("bar", Type::Struct("foo").PtrTo()),
                    0, "field"};
  SelectInst select_inst_{var_, 42, 42, 42};
  CallInst call_inst_{var_, "foo", {}};
  ICallInst icall_inst_{var_, fun_, {}};
  RetInst ret_inst_{42};
  JumpInst jump_inst_{"foo"};
  BranchInst branch_inst_{42, "foo", "bar"};
};

using IrDeathTest = IrTest;

// Testing that we can retrieve the specific type of instruction from a generic
// instruction as expected.
TEST_F(IrTest, InstructionGetters) {
  Instruction arith(arith_inst_);
  EXPECT_EQ(arith.AsArith().operation(), ArithInst::kAdd);

  Instruction cmp(cmp_inst_);
  EXPECT_EQ(cmp.AsCmp().operation(), CmpInst::kEqual);

  Instruction phi(phi_inst_);
  EXPECT_EQ(phi.AsPhi().ops().begin()->ToString(), "42");

  Instruction copy(copy_inst_);
  EXPECT_EQ(copy.AsCopy().rhs().ToString(), "42");

  Instruction alloc(alloc_inst_);
  EXPECT_EQ(alloc.AsAlloc().lhs()->name(), "foop");

  Instruction addrof(addrof_inst_);
  EXPECT_EQ(addrof.AsAddrOf().lhs()->name(), "foop");

  Instruction load(load_inst_);
  EXPECT_EQ(load.AsLoad().lhs()->name(), "foo");

  Instruction store(store_inst_);
  EXPECT_EQ(store.AsStore().value().ToString(), "42");

  Instruction gep(gep_inst_);
  EXPECT_EQ(gep.AsGep().field_name(), "field");

  Instruction select(select_inst_);
  EXPECT_EQ(select.AsSelect().condition().ToString(), "42");

  Instruction call(call_inst_);
  EXPECT_EQ(call.AsCall().callee(), "foo");

  Instruction icall(icall_inst_);
  EXPECT_EQ(icall.AsICall().lhs()->name(), "foo");

  Instruction ret(ret_inst_);
  EXPECT_EQ(ret.AsRet().retval().ToString(), "42");

  Instruction jump(jump_inst_);
  EXPECT_EQ(jump.AsJump().label(), "foo");

  Instruction branch(branch_inst_);
  EXPECT_EQ(branch.AsBranch().label_true(), "foo");
}

TEST_F(IrTest, VisitorTest) {
  class TestVisitor : public IrVisitor {
   public:
    vector<int> GetOrder() { return order_; }

    void VisitProgram(const Program& program) override { order_.push_back(1); }
    void VisitProgramPost(const Program& program) override {
      order_.push_back(2);
    }
    void VisitStructType(const string& name,
                         const map<string, Type>& elements) override {
      order_.push_back(3);
    }
    void VisitFunction(const Function& function) override {
      order_.push_back(4);
    }
    void VisitFunctionPost(const Function& function) override {
      order_.push_back(5);
    }
    void VisitBasicBlock(const BasicBlock& basic_block) override {
      order_.push_back(6);
    }
    void VisitBasicBlockPost(const BasicBlock& basic_block) override {
      order_.push_back(7);
    }
    void VisitInst(const Instruction& inst) override { order_.push_back(9); }
    void VisitInst(const RetInst& inst) override { order_.push_back(8); }

   private:
    vector<int> order_;
  };

  map<string, map<string, Type>> struct_types;
  struct_types["foo"]["field"] = Type::Int();

  auto bb1 = MakeBasicBlock("entry", {"ret"});
  auto fun = MakeFunction("main", {bb1});
  auto prog = MakeProgram(struct_types, {fun});

  TestVisitor visitor;
  prog.Visit(&visitor);

  vector<int> expected{1, 3, 4, 6, 8, 9, 7, 5, 2};
  EXPECT_EQ(visitor.GetOrder(), expected);
}

TEST_F(IrTest, ToStringTest) {
  map<string, map<string, Type>> struct_types;
  struct_types["foo"]["field"] = Type::Int();
  struct_types["foo"]["field2"] = Type::Int().PtrTo();
  struct_types["bar"]["field"] = Type::Struct("foo").PtrTo();

  auto bb1 = MakeBasicBlock(
      "entry", {"arith", "cmp", "phi", "copy", "alloc", "load", "jump"});
  auto bb2 = MakeBasicBlock(
      "foo", {"addrof", "store", "gep", "select", "call", "icall", "ret"});
  auto fun1 = MakeFunction("foo", {bb1, bb2});

  auto bb3 = MakeBasicBlock(
      "entry", {"arith", "cmp", "phi", "copy", "alloc", "load", "branch"});
  auto bb4 = MakeBasicBlock(
      "foo", {"store", "gep", "select", "call", "icall", "jump"});
  auto bb5 =
      MakeBasicBlock("bar", {"store", "gep", "select", "call", "icall", "ret"});
  auto fun2 = MakeFunction("main", {bb3, bb4, bb5});

  auto prog = MakeProgram(struct_types, {fun1, fun2});

  EXPECT_EQ(prog.ToString(), R"""(struct bar {
  field: foo*
}

struct foo {
  field: int
  field2: int*
}

function foo() -> int {
entry:
  foo:int = $arith add 42 42
  foo:int = $cmp eq 42 42
  foo:int = $phi(42, 42)
  foo:int = $copy 42
  foop:int* = $alloc
  foo:int = $load foop:int*
  $jump foo

foo:
  foop:int* = $addrof foo:int
  $store foop:int* 42
  foop:int* = $gep bar:foo* 0 field
  foo:int = $select 42 42 42
  foo:int = $call foo()
  foo:int = $icall fun:int[]*()
  $ret 42
}

function main() -> int {
bar:
  $store foop:int* 42
  foop:int* = $gep bar:foo* 0 field
  foo:int = $select 42 42 42
  foo:int = $call foo()
  foo:int = $icall fun:int[]*()
  $ret 42

entry:
  foo:int = $arith add 42 42
  foo:int = $cmp eq 42 42
  foo:int = $phi(42, 42)
  foo:int = $copy 42
  foop:int* = $alloc
  foo:int = $load foop:int*
  $branch 42 foo bar

foo:
  $store foop:int* 42
  foop:int* = $gep bar:foo* 0 field
  foo:int = $select 42 42 42
  foo:int = $call foo()
  foo:int = $icall fun:int[]*()
  $jump foo
}

)""");
}

TEST_F(IrTest, FromStringTest) {
  // Iterate over all files in //testdata. For any file "<name>.ir", use
  // FromString() then ToString() and compare against the original.
  for (const auto& dir_entry :
       std::filesystem::directory_iterator("ir/testdata/")) {
    string filename = dir_entry.path();
    if (EndsWith(filename, ".ir")) {
      std::ifstream ir_in(filename);
      ASSERT_TRUE(ir_in) << filename;

      string ir(std::istreambuf_iterator<char>{ir_in}, {});
      EXPECT_EQ(Program::FromString(ir).ToString(), ir) << "FILE: " << filename;
    }
  }
}

TEST_F(IrTest, FromStringGepTest) {
  auto inst1 = Instruction::FromString("x:int* = $gep y:int* z:int foo");
  EXPECT_EQ(inst1.ToString(), "x:int* = $gep y:int* z:int foo\n");

  auto inst2 = Instruction::FromString("x:int* = $gep y:int* 42");
  EXPECT_EQ(inst2.ToString(), "x:int* = $gep y:int* 42\n");

  string bb1_str = R"""(
bb:
  x:int* = $gep y:int* z:int foo
  a:int* = $gep b:int* 42
  p:int = $copy 42
  $ret 0
)""";

  auto bb1 = BasicBlock::FromString(bb1_str);
  EXPECT_EQ(bb1.ToString(), bb1_str);

  string bb2_str = R"""(
bb:
  x:int* = $gep y:int* z:int foo
  a:int* = $gep b:int* 42
  $ret 0
)""";

  auto bb2 = BasicBlock::FromString(bb2_str);
  EXPECT_EQ(bb2.ToString(), bb2_str);
}

TEST_F(IrTest, FromStringGlobalVarsTest) {
  string code = R"""(
    function foo() -> int {
    entry:
      foo_fptr:int[]* = $copy @foo:int[]*
      foo_null:int* = $copy @nullptr:int*
      $ret 42
    }

    function main() -> int {
    entry:
      main_fptr:int[]* = $copy @foo:int[]*
      main_null:int* = $copy @nullptr:int*
      $ret 42
    }
  )""";

  auto program = Program::FromString(code);

  class VarFinder : public IrVisitor {
   public:
    unordered_set<VarPtr_t> vars_;

    void VisitInst(const CopyInst& inst) override {
      vars_.insert(inst.lhs());
      vars_.insert(inst.rhs().GetVar());
    }
  };

  VarFinder finder;
  program.Visit(&finder);

  // The variables @foo:int[]* and @nullptr:int* in main() should be the same
  // VarPtr_t as used in foo().
  EXPECT_EQ(finder.vars_.size(), 6);
}

TEST_F(IrTest, FromStringComplexTypeTest) {
  string code = R"""(function foo(p1:int*, p2:int*) -> int {
entry:
  $ret 42
}

function main() -> int {
entry:
  src:int[int*,int*]* = $copy @foo:int[int*,int*]*
  $ret 0
}

)""";

  auto program = Program::FromString(code);
  EXPECT_EQ(program.ToString(), code);
}

TEST_F(IrTest, TypeFromToStringTest) {
  string type = "foo**[int,int*,bar*[int,int]*]*";
  EXPECT_EQ(Type::FromString(type).ToString(), type);
}

TEST_F(IrTest, InstIndexInBasicBlockTest) {
  auto bb = MakeBasicBlock(
      "entry", {"arith", "cmp", "phi", "copy", "alloc", "load", "jump"});

  auto inst = &bb.body()[2];

  EXPECT_EQ(inst->GetIndex(), 2);
}

TEST_F(IrDeathTest, EmptyVariableName) {
  EXPECT_DEATH(Variable("", Type::Int()), "non-empty");
}

TEST_F(IrDeathTest, EmptyBasicBlockLabel) {
  EXPECT_DEATH(MakeBasicBlock("", {"arith"}), "label must be non-empty");
}

TEST_F(IrDeathTest, EmptyBasicBlock) {
  EXPECT_DEATH(MakeBasicBlock("foo", {}), "body must be non-empty");
}

TEST_F(IrDeathTest, EmptyFunctionName) {
  EXPECT_DEATH(MakeFunction("", {}), "name must be non-empty");
}

TEST_F(IrDeathTest, EmptyFunction) {
  EXPECT_DEATH(MakeFunction("foo", {}), "body must be non-empty");
}

TEST_F(IrDeathTest, MalformedProgram) {
  map<string, map<string, Type>> struct_types;
  struct_types["blah"] = {};
  auto bb = MakeBasicBlock("bar", {"jump", "gep"});
  auto fun = MakeFunction("fun", {bb});
  EXPECT_DEATH(MakeProgram(struct_types, {fun}),
               R"""(Struct type can't have empty fields: blah
Function must have a basic block named 'entry': fun
Basic block does not end in a terminator instruction: fun::bar
Basic block contains a terminator instruction before its end: fun::bar
Basic block 'fun::bar' jumps to nonexistent basic block 'foo'
Type uses nonexistent struct: foo)""");
}

// FIXME: need death tests for typechecking

}  // namespace

int main(int argc, char** argv) {
  ::google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
