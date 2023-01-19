#include "ir/ir.h"

#include "ir_tostring_visitor.h"
#include "util/tokenizer.h"

namespace ir {

namespace {  // Helpers for *::FromString().

Type ReadType(util::Tokenizer& tk) {
  vector<Type> types;
  string type_str = tk.ConsumeToken();

  Type type = (type_str == "int") ? Type::Int() : Type::Struct(type_str);
  while (tk.QueryConsume("*")) type = type.PtrTo();

  if (tk.QueryConsume("[")) {
    types.push_back(type);

    while (!tk.QueryConsume("]")) {
      types.push_back(ReadType(tk));
      if (!tk.QueryNoConsume("]")) tk.Consume(",");
    }

    type = Type::Function(types);
    while (tk.QueryConsume("*")) type = type.PtrTo();
  }

  return type;
}

class FromStringHelper {
 public:
  FromStringHelper(const string& str)
      : tk_(str, whitespace_, delimiters_, reserved_) {}

  Instruction ReadInstruction() {
    // Convert string into ArithInst::Aop.
    static const map<string, ArithInst::Aop> str_to_aop{
        {"add", ArithInst::kAdd},
        {"sub", ArithInst::kSubtract},
        {"mul", ArithInst::kMultiply},
        {"div", ArithInst::kDivide},
    };

    // Convert string into CmpInst::Rop.
    static const map<string, CmpInst::Rop> str_to_rop{
        {"eq", CmpInst::kEqual},          {"neq", CmpInst::kNotEqual},
        {"lt", CmpInst::kLessThan},       {"gt", CmpInst::kGreaterThan},
        {"lte", CmpInst::kLessThanEqual}, {"gte", CmpInst::kGreaterThanEqual},
    };

    // Read and return a variable.
    auto read_var = [&]() {
      string name = tk_.ConsumeToken();
      tk_.Consume(":");
      Type type = ReadType(tk_);

      if (name == "@nullptr") {
        if (!null_vars_.count(type)) {
          null_vars_[type] = make_shared<const Variable>(name, type);
        }
        return null_vars_.at(type);
      } else if (name[0] == '@') {
        if (!func_vars_.count(name)) {
          func_vars_[name] = make_shared<const Variable>(name, type);
        } else {
          CHECK_EQ(func_vars_.at(name)->type(), type)
              << "Global function pointers with same name but different types: "
              << name << " with types " << func_vars_.at(name)->type()
              << " and " << type;
        }
        return func_vars_.at(name);
      } else if (!vars_.count(name)) {
        vars_[name] = make_shared<const Variable>(name, type);
      } else {
        CHECK_EQ(vars_.at(name)->type(), type)
            << "Local variables with same name but different types: " << name
            << " with types " << vars_.at(name)->type() << " and " << type;
      }
      return vars_.at(name);
    };

    // Read and return an operand (a variable or an integer constant).
    auto read_op = [&]() {
      string token = tk_.ConsumeToken();
      if (token.find_first_not_of("-0123456789") == string::npos) {
        // Token is an integer constant.
        return Operand(stoi(token));
      } else {
        // Token is the name of a variable.
        tk_.Put(token);
        return Operand(read_var());
      }
    };

    // Read a set of comma-delimited operands inside parentheses.
    auto read_args = [&]() {
      vector<Operand> ops;
      tk_.Consume("(");
      while (!tk_.QueryConsume(")")) {
        ops.push_back(read_op());
        if (!tk_.QueryNoConsume(")")) tk_.Consume(",");
      }
      return ops;
    };

    // Figure out what kind of instruction this is.
    if (tk_.QueryConsume("$store")) {
      return StoreInst(read_var(), read_op());
    } else if (tk_.QueryConsume("$jump")) {
      return JumpInst(tk_.ConsumeToken());
    } else if (tk_.QueryConsume("$branch")) {
      return BranchInst(read_op(), tk_.ConsumeToken(), tk_.ConsumeToken());
    } else if (tk_.QueryConsume("$ret")) {
      return RetInst(read_op());
    } else {
      // Must be an instruction with an assignment.
      auto lhs = read_var();
      tk_.Consume("=");

      if (tk_.QueryConsume("$arith")) {
        string op = tk_.ConsumeToken();
        CHECK(str_to_aop.count(op)) << "unknown arithmetic operation";
        auto aop = str_to_aop.at(op);
        return ArithInst(lhs, read_op(), read_op(), aop);
      } else if (tk_.QueryConsume("$cmp")) {
        string op = tk_.ConsumeToken();
        CHECK(str_to_rop.count(op)) << "unknown comparison operation";
        auto rop = str_to_rop.at(op);
        return CmpInst(lhs, read_op(), read_op(), rop);
      } else if (tk_.QueryConsume("$phi")) {
        return PhiInst(lhs, read_args());
      } else if (tk_.QueryConsume("$copy")) {
        return CopyInst(lhs, read_op());
      } else if (tk_.QueryConsume("$alloc")) {
        return AllocInst(lhs);
      } else if (tk_.QueryConsume("$addrof")) {
        return AddrOfInst(lhs, read_var());
      } else if (tk_.QueryConsume("$load")) {
        return LoadInst(lhs, read_var());
      } else if (tk_.QueryConsume("$gep")) {
        auto var = read_var();
        auto op = read_op();
        string field;
        // There may or may not be a field name.
        if (!tk_.EndOfInput() && !tk_.IsNextReserved() && tk_.Peek(1) != ":") {
          field = tk_.ConsumeToken();
        }
        return GepInst(lhs, var, op, field);
      } else if (tk_.QueryConsume("$select")) {
        return SelectInst(lhs, read_op(), read_op(), read_op());
      } else if (tk_.QueryConsume("$call")) {
        return CallInst(lhs, tk_.ConsumeToken(), read_args());
      } else if (tk_.QueryConsume("$icall")) {
        return ICallInst(lhs, read_var(), read_args());
      }
    }

    LOG(FATAL) << "Unknown opcode: " << tk_.ConsumeToken();
  }

  BasicBlock ReadBasicBlock() {
    // Parse basic block.
    string label = tk_.ConsumeToken();
    tk_.Consume(":");

    vector<Instruction> bb_body;

    // Keep reading instructions until we reach a terminator instruction.
    while (true) {
      bb_body.push_back(ReadInstruction());
      if (bb_body.back().GetOpcode() == Instruction::kRet ||
          bb_body.back().GetOpcode() == Instruction::kJump ||
          bb_body.back().GetOpcode() == Instruction::kBranch) {
        break;
      }
    }

    return BasicBlock(label, bb_body);
  }

  Function ReadFunction() {
    // Forget local variables we've seen in other functions.
    vars_.clear();

    tk_.Consume("function");
    string fun_name = tk_.ConsumeToken();

    // Function parameters.
    vector<VarPtr_t> params;

    // Parse function parameters.
    tk_.Consume("(");
    while (!tk_.QueryConsume(")")) {
      string param_name = tk_.ConsumeToken();
      tk_.Consume(":");
      auto param = make_shared<const Variable>(param_name, ReadType(tk_));
      params.push_back(param);
      vars_[param_name] = param;
      if (!tk_.QueryNoConsume(")")) tk_.Consume(",");
    }

    tk_.Consume("->");
    Type fun_rettype = ReadType(tk_);

    // Function basic blocks.
    vector<BasicBlock> fun_body;

    // Parse function body.
    tk_.Consume("{");
    while (!tk_.QueryConsume("}")) {
      BasicBlock bb = ReadBasicBlock();
      fun_body.emplace_back(bb);
    }

    return Function(fun_name, fun_rettype, params, fun_body);
  }

  Program ReadProgram() {
    // Program struct types, keyed by name.
    map<string, map<string, Type>> struct_types;

    // Parse struct type definitions.
    while (tk_.QueryConsume("struct")) {
      string name = tk_.ConsumeToken();

      CHECK_EQ(struct_types.count(name), 0)
          << "Two structs with same name: " << name;

      tk_.Consume("{");
      while (!tk_.QueryConsume("}")) {
        string field = tk_.ConsumeToken();

        CHECK(!struct_types.count(name) || !struct_types.at(name).count(field))
            << "Two fields of same struct with same name: " << field;

        tk_.Consume(":");
        struct_types[name][field] = ReadType(tk_);
      }
    }

    // Program functions.
    vector<Function> functions;

    // Parse function definitions.
    while (!tk_.EndOfInput()) {
      Function fun = ReadFunction();
      functions.emplace_back(fun);
    }

    return Program(struct_types, functions);
  }

 private:
  inline static set<char> whitespace_{' ', '\n'};
  inline static set<string> delimiters_{":", ",", "=", "->", "*", "[",
                                        "]", "{", "}", "(",  ")"};
  inline static set<string> reserved_{
      "$arith", "$cmp",    "$phi",  "$alloc", "$addrof", "$load", "$store",
      "$gep",   "$select", "$call", "$icall", "$ret",    "$jump", "$branch"};

  util::Tokenizer tk_;

  // Variables that are local to a function, indexed by name.
  unordered_map<string, VarPtr_t> vars_;

  // Variables that refer to global function pointers, indexed by name.
  unordered_map<string, VarPtr_t> func_vars_;

  // Variables that refer to global null pointers, indexed by type.
  unordered_map<Type, VarPtr_t> null_vars_;
};

}  // namespace

////////////////////////////////////////////////////////////////////////////////

string Type::ToString() const {
  switch (BaseKind()) {
    case kInt:
      return "int"s + string(indirection_, '*');

    case kStruct:
      return GetStructName() + string(indirection_, '*');

    case kFunc: {
      auto& types = GetFuncTypes();
      vector<string> type_strs;
      for (const auto& type : types) type_strs.push_back(type.ToString());
      std::ostringstream out;
      out << type_strs[0] << "[";
      if (type_strs.size() > 1) {
        std::copy(std::next(type_strs.begin()), std::prev(type_strs.end()),
                  std::ostream_iterator<string>(out, ","));
        out << *type_strs.rbegin();
      }
      out << "]";
      return out.str() + string(indirection_, '*');
    }
  }
}

Type Type::FromString(const string& type) {
  util::Tokenizer tk(type, {}, {"[", "]", ",", "*"}, {});
  return ReadType(tk);
}

////////////////////////////////////////////////////////////////////////////////

int Instruction::GetIndex() const {
  if (parent_ == nullptr) return -1;
  return std::distance(&parent_->body()[0], this);
}

void Instruction::Visit(IrVisitor* visitor) const {
  visitor->VisitInst(*this);

  switch (GetOpcode()) {
    case kArith:
      visitor->VisitInst(AsArith());
      break;

    case kCmp:
      visitor->VisitInst(AsCmp());
      break;

    case kPhi:
      visitor->VisitInst(AsPhi());
      break;

    case kCopy:
      visitor->VisitInst(AsCopy());
      break;

    case kAlloc:
      visitor->VisitInst(AsAlloc());
      break;

    case kAddrof:
      visitor->VisitInst(AsAddrOf());
      break;

    case kLoad:
      visitor->VisitInst(AsLoad());
      break;

    case kStore:
      visitor->VisitInst(AsStore());
      break;

    case kGep:
      visitor->VisitInst(AsGep());
      break;

    case kSelect:
      visitor->VisitInst(AsSelect());
      break;

    case kCall:
      visitor->VisitInst(AsCall());
      break;

    case kICall:
      visitor->VisitInst(AsICall());
      break;

    case kRet:
      visitor->VisitInst(AsRet());
      break;

    case kJump:
      visitor->VisitInst(AsJump());
      break;

    case kBranch:
      visitor->VisitInst(AsBranch());
      break;

    default:
      LOG(FATAL) << "Unknown opcode";
  }

  visitor->VisitInstPost(*this);
}

string Instruction::ToString() const {
  ToStringVisitor visitor;
  this->Visit(&visitor);
  return visitor.GetString();
}

Instruction Instruction::FromString(const string& instruction) {
  return FromStringHelper(instruction).ReadInstruction();
}

////////////////////////////////////////////////////////////////////////////////

BasicBlock::BasicBlock(const string& label, const vector<Instruction>& body,
                       const Function* parent)
    : label_(label), parent_(parent) {
  CHECK_NE(label_, "") << "label must be non-empty";
  CHECK(!body.empty()) << "body must be non-empty";

  for (const auto& inst : body) {
    body_.emplace_back(inst, this);
  }
}

// The copy constructor needs to update the parent pointers of the instructions.
BasicBlock::BasicBlock(const BasicBlock& bb)
    : label_(bb.label_), parent_(bb.parent_) {
  for (const auto& inst : bb.body_) {
    body_.emplace_back(inst, this);
  }
}

const Instruction& BasicBlock::operator[](int index) const {
  CHECK(index >= 0 && index < body_.size()) << "index out of bounds";
  return body_[index];
}

void BasicBlock::Visit(IrVisitor* visitor) const {
  visitor->VisitBasicBlock(*this);

  for (const auto& inst : body_) {
    inst.Visit(visitor);
  }

  visitor->VisitBasicBlockPost(*this);
}

string BasicBlock::ToString() const {
  ToStringVisitor visitor;
  this->Visit(&visitor);
  return visitor.GetString();
}

BasicBlock BasicBlock::FromString(const string& basic_block) {
  return FromStringHelper(basic_block).ReadBasicBlock();
}

////////////////////////////////////////////////////////////////////////////////

Function::Function(const string& name, const Type& return_type,
                   const vector<VarPtr_t>& parameters,
                   const vector<BasicBlock>& body)
    : name_(name), return_type_(return_type), parameters_(parameters) {
  CHECK_NE(name_, "") << "name must be non-empty";
  CHECK(!body.empty()) << "body must be non-empty";

  for (const auto& block : body) {
    CHECK(!body_.count(block.label()))
        << "cannot have duplicate basic block labels";
    body_[block.label()] =
        make_shared<BasicBlock>(block.label(), block.body(), this);
  }
}

// The copy constructor needs to update the parent pointers of the basic blocks.
Function::Function(const Function& func)
    : name_(func.name_),
      return_type_(func.return_type_),
      parameters_(func.parameters_) {
  for (const auto& [label, bb] : func.body()) {
    body_[label] = make_shared<BasicBlock>(label, bb->body(), this);
  }
}

const BasicBlock& Function::operator[](const string& label) const {
  CHECK(body_.count(label)) << "unknown basic block label";
  return *body_.at(label);
}

void Function::Visit(IrVisitor* visitor) const {
  visitor->VisitFunction(*this);

  for (const auto& [label, bb] : body_) {
    bb->Visit(visitor);
  }

  visitor->VisitFunctionPost(*this);
}

string Function::ToString() const {
  ToStringVisitor visitor;
  this->Visit(&visitor);
  return visitor.GetString();
}

Function Function::FromString(const string& function) {
  return FromStringHelper(function).ReadFunction();
}

////////////////////////////////////////////////////////////////////////////////

Program::Program(const map<string, map<string, Type>>& struct_types,
                 const vector<Function>& functions)
    : struct_types_(struct_types) {
  for (const auto& func : functions) {
    CHECK(!functions_.count(func.name()))
        << "cannot have duplicate function names";
    functions_[func.name()] = make_shared<Function>(func);
  }

  string errs = VerifyIr();
  CHECK_EQ(errs, "") << "Malformed program: " << std::endl << errs;
}

const Function& Program::operator[](const string& name) const {
  CHECK(functions_.count(name)) << "unknown function name";
  return *functions_.at(name);
}

void Program::Visit(IrVisitor* visitor) const {
  visitor->VisitProgram(*this);

  for (const auto& [name, info] : struct_types_) {
    visitor->VisitStructType(name, info);
  }

  for (const auto& [name, func] : functions_) {
    func->Visit(visitor);
  }

  visitor->VisitProgramPost(*this);
}

string Program::ToString() const {
  ToStringVisitor visitor;
  this->Visit(&visitor);
  return visitor.GetString();
}

Program Program::FromString(const string& program) {
  return FromStringHelper(program).ReadProgram();
}

namespace {  // Helper visitor class for Program::Verify.

class VerifyVisitor : public IrVisitor {
 public:
  // Resets the visitor to its initial state.
  void Reset() {
    err_.clear();
    program_ = nullptr;
    curr_function_ = nullptr;
    bb_id_ = "";
    nonexistent_structs_.clear();
  }

  // Returns the list of generated errors.
  string GetErrors() { return err_.str(); }

  // Returns the set of global function pointers contained in the program (may
  // not include global pointers to all functions, just the ones mentioned in
  // the code).
  map<string, VarPtr_t> GetGlobalFuncPtrs() { return func_ptrs_; }

  void VisitProgram(const Program& program) override {
    program_ = &program;

    if (program.functions().count("main") == 0) {
      err_ << "Program does not have a main function." << std::endl;
    }

    for (const auto& [name, fun] : program.functions()) {
      if (name != fun->name()) {
        err_ << "Mismatched function names: " << name << " mapped to "
             << fun->name() << std::endl;
      }
    }
  }

  void VisitStructType(const string& name,
                       const map<string, Type>& elements) override {
    if (name.find('.') != string::npos) {
      err_ << "struct type name can't contain '.': " << name << std::endl;
    }
    if (elements.size() == 0) {
      err_ << "Struct type can't have empty fields: " << name << std::endl;
    }
    for (const auto& [fieldname, type] : elements) {
      if (fieldname == "") {
        err_ << "Struct field names must be non-empty: " << name << "."
             << fieldname << std::endl;
      }
      if (fieldname.find('.') != string::npos) {
        err_ << "struct field name can't contain '.': " << name << "."
             << fieldname << std::endl;
      }
      ReportIfNonexistentStruct(type);
    }
  }

  void VisitFunction(const Function& function) override {
    curr_function_ = &function;

    if (function.body().count("entry") == 0) {
      err_ << "Function must have a basic block named 'entry': "
           << function.name() << std::endl;
    }

    for (const auto& [label, bb] : function.body()) {
      if (label != bb->label()) {
        err_ << "Mismatched basic block labels: " << label << " mapped to "
             << bb->label() << std::endl;
      }
    }

    set<VarPtr_t> params;

    for (const auto& param : function.parameters()) {
      if (params.count(param)) {
        err_ << "Duplicate parameter variables for function " << function.name()
             << std::endl;
      }
      params.insert(param);

      if (param->name()[0] == '@') {
        err_ << "Cannot use global variable as parameter in function "
             << function.name() << std::endl;
      }

      ReportIfNonexistentStruct(param->type());
      ReportIfNotToplevelType(param->type());
    }

    ReportIfNotToplevelType(function.return_type());
  }

  void VisitBasicBlock(const BasicBlock& basic_block) override {
    curr_bb_ = &basic_block;
    bb_id_ = curr_function_->name() + "::" + basic_block.label();

    auto opcode = basic_block.body().back().GetOpcode();
    if (opcode != Instruction::kRet && opcode != Instruction::kJump &&
        opcode != Instruction::kBranch) {
      err_ << "Basic block does not end in a terminator instruction: " << bb_id_
           << std::endl;
    }

    for (auto iter = basic_block.body().begin();
         iter != std::prev(basic_block.body().end()); ++iter) {
      opcode = iter->GetOpcode();
      if (opcode == Instruction::kRet || opcode == Instruction::kJump ||
          opcode == Instruction::kBranch) {
        err_ << "Basic block contains a terminator instruction before its "
                "end: "
             << bb_id_ << std::endl;
        break;
      }
    }

    if (basic_block.parent() == nullptr) {
      err_ << "Basic block's parent pointer isn't set: " << bb_id_ << std::endl;
    } else if (basic_block.parent() != curr_function_) {
      err_ << "Basic block's parent doesn't match containing function: "
           << bb_id_ << std::endl;
    }
  }

  void VisitInst(const ArithInst& inst) override {
    ReportIfNonexistentStruct(inst.lhs()->type());
    ReportIfNonexistentStruct(inst.op1().GetType());
    ReportIfNonexistentStruct(inst.op2().GetType());

    CheckIfGlobal(inst.lhs());
    CheckIfGlobal(inst.op1());
    CheckIfGlobal(inst.op2());

    if (!inst.lhs()->type().IsInt()) {
      err_ << "Type error: result of arithmetic must be an int: "
           << Instruction(inst).ToString() << std::endl;
    }

    if (!inst.op1().GetType().IsInt()) {
      err_ << "Type error: operand of arithmetic must be an int: "
           << Instruction(inst).ToString() << std::endl;
    }

    if (!inst.op2().GetType().IsInt()) {
      err_ << "Type error: operand of arithmetic must be an int: "
           << Instruction(inst).ToString() << std::endl;
    }
  }

  void VisitInst(const CmpInst& inst) override {
    ReportIfNonexistentStruct(inst.lhs()->type());
    ReportIfNonexistentStruct(inst.op1().GetType());
    ReportIfNonexistentStruct(inst.op2().GetType());

    CheckIfGlobal(inst.lhs());
    CheckIfGlobal(inst.op1());
    CheckIfGlobal(inst.op2());

    if (!inst.lhs()->type().IsInt()) {
      err_ << "Type error: result of comparison must be an int: "
           << Instruction(inst).ToString() << std::endl;
    }

    if (!inst.op1().GetType().IsInt() && !inst.op1().GetType().IsPtr()) {
      err_ << "Type error: operand of comparison must be an int or pointer: "
           << Instruction(inst).ToString() << std::endl;
    }

    if (!inst.op2().GetType().IsInt() && !inst.op2().GetType().IsPtr()) {
      err_ << "Type error: operand of comparison must be an int or pointer: "
           << Instruction(inst).ToString() << std::endl;
    }
  }

  void VisitInst(const PhiInst& inst) override {
    ReportIfNonexistentStruct(inst.lhs()->type());
    CheckIfGlobal(inst.lhs());

    for (const auto& op : inst.ops()) {
      ReportIfNonexistentStruct(op.GetType());
      CheckIfGlobal(op);

      if (op.GetType() != inst.lhs()->type()) {
        err_ << "Type error: operand type doesn't match left-hand side: "
             << Instruction(inst).ToString() << std::endl;
      }
    }

    ReportIfNotToplevelType(inst.lhs()->type());
    ReportIfUnassignable(inst.lhs());
  }

  void VisitInst(const CopyInst& inst) override {
    ReportIfNonexistentStruct(inst.lhs()->type());
    ReportIfNonexistentStruct(inst.rhs().GetType());

    CheckIfGlobal(inst.lhs());
    CheckIfGlobal(inst.rhs());

    if (inst.rhs().GetType() != inst.lhs()->type()) {
      err_ << "Type error: operand type doesn't match left-hand side: "
           << Instruction(inst).ToString() << std::endl;
    }

    ReportIfNotToplevelType(inst.lhs()->type());
    ReportIfUnassignable(inst.lhs());
  }

  void VisitInst(const AllocInst& inst) override {
    ReportIfNonexistentStruct(inst.lhs()->type());
    CheckIfGlobal(inst.lhs());

    if (!inst.lhs()->type().IsPtr()) {
      err_ << "Type error: result of alloc must be a pointer: "
           << Instruction(inst).ToString() << std::endl;
    }

    ReportIfUnassignable(inst.lhs());
  }

  void VisitInst(const AddrOfInst& inst) override {
    ReportIfNonexistentStruct(inst.lhs()->type());
    ReportIfNonexistentStruct(inst.rhs()->type());

    CheckIfGlobal(inst.lhs());
    CheckIfGlobal(inst.rhs());

    if (!inst.lhs()->type().IsPtr() ||
        inst.lhs()->type().Deref() != inst.rhs()->type()) {
      err_ << "Type error: result of addrof must be a pointer to operand type: "
           << Instruction(inst).ToString() << std::endl;
    }

    ReportIfUnassignable(inst.lhs());
  }

  void VisitInst(const LoadInst& inst) override {
    ReportIfNonexistentStruct(inst.lhs()->type());
    ReportIfNonexistentStruct(inst.src()->type());

    CheckIfGlobal(inst.lhs());
    CheckIfGlobal(inst.src());

    if (inst.lhs()->type().PtrTo() != inst.src()->type()) {
      err_ << "Type error: source of load must be a pointer to type of result: "
           << Instruction(inst).ToString();
    }

    ReportIfNotToplevelType(inst.lhs()->type());
    ReportIfUnassignable(inst.lhs());
  }

  void VisitInst(const StoreInst& inst) override {
    ReportIfNonexistentStruct(inst.dst()->type());
    ReportIfNonexistentStruct(inst.value().GetType());

    CheckIfGlobal(inst.dst());
    CheckIfGlobal(inst.value());

    if (inst.value().GetType().PtrTo() != inst.dst()->type()) {
      err_ << "Type error: destination of store must be a pointer to type of "
              "stored value: "
           << Instruction(inst).ToString();
    }

    ReportIfUnassignable(inst.dst());
  }

  void VisitInst(const GepInst& inst) override {
    ReportIfNonexistentStruct(inst.lhs()->type());
    ReportIfNonexistentStruct(inst.src_ptr()->type());
    ReportIfNonexistentStruct(inst.index().GetType());

    CheckIfGlobal(inst.lhs());
    CheckIfGlobal(inst.src_ptr());
    CheckIfGlobal(inst.index());

    if (!inst.index().GetType().IsInt()) {
      err_ << "Index must be an integer" << std::endl;
    }

    if (inst.field_name() == "") {
      if (!inst.src_ptr()->type().IsPtr()) {
        err_ << "Type error: source must be a pointer: "
             << Instruction(inst).ToString() << std::endl;
      }

      if (inst.lhs()->type() != inst.src_ptr()->type()) {
        err_ << "Type error: type of source pointer must match left-hand side: "
             << Instruction(inst).ToString() << std::endl;
      }

      return;
    }

    if (!inst.src_ptr()->type().IsStructPtr()) {
      err_ << "Type error: source must be a pointer to a struct: "
           << Instruction(inst).ToString() << std::endl;
      return;
    }

    string struct_type = inst.src_ptr()->type().GetStructName();
    if (program_->struct_types().count(struct_type) == 0) return;

    const map<string, Type>& fields = program_->struct_types().at(struct_type);

    if (!fields.count(inst.field_name())) {
      err_ << "Type error: mismatch between struct type and field name: "
           << Instruction(inst).ToString() << std::endl;
    } else if (inst.lhs()->type().Deref() != fields.at(inst.field_name())) {
      err_ << "Type error: Result type must be a pointer to type of field: "
           << Instruction(inst).ToString() << std::endl;
    }

    ReportIfUnassignable(inst.lhs());
  }

  void VisitInst(const SelectInst& inst) override {
    ReportIfNonexistentStruct(inst.lhs()->type());
    ReportIfNonexistentStruct(inst.condition().GetType());
    ReportIfNonexistentStruct(inst.true_op().GetType());
    ReportIfNonexistentStruct(inst.false_op().GetType());
    ReportIfUnassignable(inst.lhs());

    CheckIfGlobal(inst.lhs());
    CheckIfGlobal(inst.condition());
    CheckIfGlobal(inst.true_op());
    CheckIfGlobal(inst.false_op());

    if (!inst.condition().GetType().IsInt()) {
      err_ << "Type error: select condition must be an int: "
           << Instruction(inst).ToString() << std::endl;
    }
    if (inst.lhs()->type() != inst.true_op().GetType() ||
        inst.lhs()->type() != inst.false_op().GetType()) {
      err_ << "Type error: type of select operands and left-hand side must "
              "match: "
           << Instruction(inst).ToString() << std::endl;
    }

    ReportIfNotToplevelType(inst.lhs()->type());
  }

  void VisitInst(const CallInst& inst) override {
    // We don't check whether the callee exists in the program because we
    // allow externally defined functions (e.g., 'input', 'output', 'malloc').
    ReportIfNonexistentStruct(inst.lhs()->type());
    ReportIfUnassignable(inst.lhs());

    CheckIfGlobal(inst.lhs());

    if (program_->functions().count(inst.callee()) == 0) return;

    const Function& callee = (*program_)[inst.callee()];

    if (callee.parameters().size() != inst.args().size()) {
      err_ << "Type error: incorrect number of call arguments: "
           << Instruction(inst).ToString() << std::endl;
    }

    for (int i = 0; i < inst.args().size(); i++) {
      CheckIfGlobal(inst.args()[i]);

      if (i >= callee.parameters().size()) break;
      if (inst.args()[i].GetType() != callee.parameters()[i]->type()) {
        err_ << "Type error: type of argument doesn't match type of parameter: "
             << Instruction(inst).ToString() << std::endl;
      }
    }

    if (inst.lhs()->type() != callee.return_type()) {
      err_ << "Type error: function return type doesn't match left-hand side: "
           << Instruction(inst).ToString() << std::endl;
    }
  }

  void VisitInst(const ICallInst& inst) override {
    ReportIfNonexistentStruct(inst.lhs()->type());
    ReportIfNonexistentStruct(inst.func_ptr()->type());
    ReportIfUnassignable(inst.lhs());

    CheckIfGlobal(inst.lhs());
    CheckIfGlobal(inst.func_ptr());

    if (!inst.func_ptr()->type().IsFunctionPtr()) {
      err_ << "Type error: calling a non-function pointer: "
           << Instruction(inst).ToString() << std::endl;
      return;
    }

    const vector<Type>& types = inst.func_ptr()->type().GetFuncTypes();

    if (inst.args().size() != types.size() - 1) {
      err_ << "Type error: incorrect number of call arguments: "
           << Instruction(inst).ToString() << std::endl;
    }

    for (int i = 0; i < inst.args().size(); i++) {
      CheckIfGlobal(inst.args()[i]);

      if (i + 1 >= types.size()) break;
      if (inst.args()[i].GetType() != types[i + 1]) {
        err_ << "Type error: type of argument doesn't match type of parameter: "
             << Instruction(inst).ToString() << std::endl;
      }
    }

    if (inst.lhs()->type() != types[0]) {
      err_ << "Type error: function return type doesn't match left-hand side: "
           << Instruction(inst).ToString() << std::endl;
    }

    ReportIfNotToplevelType(inst.lhs()->type());
  }

  void VisitInst(const RetInst& inst) override {
    ReportIfNonexistentStruct(inst.retval().GetType());
    CheckIfGlobal(inst.retval());

    if (inst.retval().GetType() != curr_function_->return_type()) {
      err_ << "Type error: operand type does not match function return type: "
           << Instruction(inst).ToString() << std::endl;
    }
  }

  void VisitInst(const JumpInst& inst) override {
    if (curr_function_->body().count(inst.label()) == 0) {
      err_ << "Basic block '" << bb_id_
           << "' jumps to nonexistent basic block '" << inst.label() << "'"
           << std::endl;
    }
  }

  void VisitInst(const BranchInst& inst) override {
    ReportIfNonexistentStruct(inst.condition().GetType());
    CheckIfGlobal(inst.condition());

    if (curr_function_->body().count(inst.label_true()) == 0) {
      err_ << "Basic block '" << bb_id_
           << "' branches to nonexistent basic block '" << inst.label_true()
           << "'" << std::endl;
    }
    if (curr_function_->body().count(inst.label_false()) == 0) {
      err_ << "Basic block '" << bb_id_
           << "' branches to nonexistent basic block '" << inst.label_false()
           << "'" << std::endl;
    }
  }

  void VisitInst(const Instruction& inst) override {
    if (inst.parent() == nullptr) {
      err_ << "Instruction's parent pointer isn't set" << std::endl;
    } else if (inst.parent() != curr_bb_) {
      err_ << "Instruction's parent pointer doesn't match containing basic "
              "block: "
           << curr_bb_->label() << std::endl;
    }
  }

 private:
  void ReportIfNonexistentStruct(const Type& type) {
    if (type.BaseKind() == Type::kStruct &&
        program_->struct_types().count(type.GetStructName()) == 0 &&
        nonexistent_structs_.count(type.GetStructName()) == 0) {
      err_ << "Type uses nonexistent struct: " << type << std::endl;
      nonexistent_structs_.insert(type.GetStructName());
    }
  }

  // Top-level values (i.e., stored in a program variable rather than in memory)
  // can only be integers or pointers.
  void ReportIfNotToplevelType(const Type& type) {
    if (!type.IsInt() && !type.IsPtr()) {
      err_ << "Top-level types must be int or pointer: " << type << std::endl;
    }
  }

  // Variables beginning with '@' are assigned their values by the language
  // runtime and never change.
  void ReportIfUnassignable(VarPtr_t var) {
    if (var->name()[0] == '@') {
      err_ << "Variables starting with '@' are special and cannot be assigned "
              "to or stored into"
           << std::endl;
    }
  }

  // Check for global variables (those that start with '@') to make sure they
  // are used properly, and remember any global function pointers.
  void CheckIfGlobal(VarPtr_t var) {
    if (var->name()[0] != '@' || var->name() == "@nullptr") return;
    string fun_name = var->name().substr(1);

    if (program_->functions().count(fun_name) == 0) {
      err_ << "Global function pointer doesn't point to a real function: "
           << var->ToString() << std::endl;
    }

    if (func_ptrs_.count(fun_name)) {
      if (func_ptrs_.at(fun_name) != var) {
        err_ << "Global function pointers for same function but different "
                "VarPtr_t: "
             << var->ToString() << " and "
             << func_ptrs_.at(fun_name)->ToString() << std::endl;
      }
    } else {
      func_ptrs_[fun_name] = var;
    }
  }

  // Pass any VarPtr_t operands to CheckIfGlobal(VarPtr_t).
  void CheckIfGlobal(const Operand& op) {
    if (op.IsVariable()) CheckIfGlobal(op.GetVar());
  }

  // The generated error messages.
  std::ostringstream err_;

  // Program, function, and basic block identifiers.
  const Program* program_ = nullptr;
  const Function* curr_function_ = nullptr;
  const BasicBlock* curr_bb_ = nullptr;
  string bb_id_;

  // Remember nonexistent struct types we've already reported so that we don't
  // report them multiple times.
  set<string> nonexistent_structs_;

  // Remember the global function pointers mentioned in the code, indexed by
  // function name.
  map<string, VarPtr_t> func_ptrs_;
};

}  // namespace

string Program::VerifyIr() {
  VerifyVisitor verifier;
  this->Visit(&verifier);
  func_ptrs_ = verifier.GetGlobalFuncPtrs();
  return verifier.GetErrors();
}

}  // namespace ir
