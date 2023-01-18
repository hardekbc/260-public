#pragma once

#include "ir/irvisitor.h"
#include "util/standard_includes.h"

namespace ir {

// A type can be an int, struct, function, or pointer to one of these. Struct
// types are defined by name, and the overall program should contain a map from
// struct type name to the element types of that struct type (this indirection
// is necessary to handle recursive types). We represent a type as (1) a level
// of pointer indirection (0 for no indirection) and (2) the base type (i.e.,
// int, struct, or function).
class Type {
 public:
  // It's important that these are listed in the same order as the Base enum.
  using TypeVariant = variant<std::monostate, string, vector<Type>>;
  enum Base { kInt, kStruct, kFunc };

  // The default type is integer.
  Type() : indirection_(0) {}

  // Returns the level of pointer indirection.
  int indirection() const { return indirection_; }

  // Returns the variant containing the base type.
  const TypeVariant& base_type() const { return base_type_; }

  // Returns whether this type is the integer type.
  bool IsInt() const {
    return indirection_ == 0 &&
           std::holds_alternative<std::monostate>(base_type_);
  }

  // Returns whether this type contains any pointer indirection.
  bool IsPtr() const { return indirection_ > 0; }

  // Returns whether this type is a struct.
  bool IsStruct() const { return indirection_ == 0 && BaseKind() == kStruct; }

  // Returns whether this type is a pointer to a struct.
  bool IsStructPtr() const {
    return indirection_ == 1 && BaseKind() == kStruct;
  }

  // Returns whether this type is a function pointer.
  bool IsFunctionPtr() const {
    return indirection_ == 1 && BaseKind() == kFunc;
  }

  // Returns whether the base type (i.e., ignoring any pointer indirection) is
  // an integer (kInt), struct (kStruct), or function (kFunc) type.
  Base BaseKind() const { return static_cast<Base>(base_type_.index()); }

  // If the base type is a struct, returns the name of the struct. FATALs if the
  // base type is not a struct.
  const string& GetStructName() const { return std::get<string>(base_type_); }

  // If the base type is a function, returns a vector containing the return type
  // followed by the parameter types. FATALs if the base type is not a function.
  const vector<Type>& GetFuncTypes() const {
    return std::get<vector<Type>>(base_type_);
  }

  // Return the type that is a pointer to this type.
  Type PtrTo() const { return Type(indirection_ + 1, base_type_); }

  // Return the type of a dereference of this type. FATALs if this type is not a
  // pointer.
  Type Deref() const {
    CHECK_GT(indirection_, 0) << "Cannot dereference a non-pointer";
    return Type(indirection_ - 1, base_type_);
  }

  string ToString() const;

  // Return a type corresponding to the given string, formatted the same as the
  // output of ToString().
  static Type FromString(const string& type);

  // Get an integer type.
  static Type Int() { return Type(); }

  // Get a struct type given its name.
  static Type Struct(const string& name) {
    CHECK_NE(name, "") << "Struct type name must be non-empty";
    return Type(0, name);
  }

  // Get a function type given its return type and parameter types.
  static Type Function(const vector<Type>& types) { return Type(0, types); }

  friend inline bool operator==(const Type& type1, const Type& type2) {
    return type1.indirection_ == type2.indirection_ &&
           type1.base_type_ == type2.base_type_;
  }

  friend inline bool operator!=(const Type& type1, const Type& type2) {
    return !(type1 == type2);
  }

  friend inline std::ostream& operator<<(std::ostream& os, const Type& type) {
    os << type.ToString();
    return os;
  }

 private:
  Type(int indirection, const TypeVariant& base_type)
      : indirection_(indirection), base_type_(base_type) {
    CHECK_GE(indirection_, 0) << "indirection must be non-negative";
  }

  // The level of pointer indirection (0 for none).
  int indirection_;

  // std::monostate means Integer type, string means Struct type (the string is
  // the name of the struct), and a vector means a function type (the vector
  // contains the function return type followed by the parameter types).
  TypeVariant base_type_;
};

// A program variable and its type.
class Variable {
 public:
  Variable(const string& name, const Type& type) : name_(name), type_(type) {
    CHECK_NE(name, "") << "name must be non-empty";
  }

  const string& name() const { return name_; }
  const Type& type() const { return type_; }

  string ToString() const { return name_ + ":" + type_.ToString(); }

 private:
  string name_;
  Type type_;
};

// A convenient type alias. We use VarPtr_t to reference variables so that we
// don't have a bunch of copies of exactly the same information, as well as to
// distinguish between different variables with the same name.
using VarPtr_t = shared_ptr<const Variable>;

// An instruction operand can be a variable or a constant value.
class Operand {
 public:
  Operand(VarPtr_t var) : op_(CHECK_NOTNULL(var)) {}
  Operand(int value) : op_(value) {}

  bool IsVariable() const { return std::holds_alternative<VarPtr_t>(op_); }

  bool IsConstInt() const { return std::holds_alternative<int>(op_); }

  const Type& GetType() const {
    if (IsConstInt()) return int_type_;
    return GetVar()->type();
  }

  VarPtr_t GetVar() const {
    auto varp = std::get_if<VarPtr_t>(&op_);
    CHECK(varp != nullptr) << "Operand is not a variable";
    return *varp;
  }

  int GetInt() const {
    auto intp = std::get_if<int>(&op_);
    CHECK(intp != nullptr) << "Operand is not an integer";
    return *intp;
  }

  bool operator==(const Operand& other) const { return op_ == other.op_; }

  // If the operand is a variable returns the result of calling 'func_var' on
  // it, otherwise the operand is an integer and it returns the result of
  // calling 'func_int' in it.
  template <class ReturnTy>
  ReturnTy Map(const std::function<ReturnTy(VarPtr_t)>& func_var,
               const std::function<ReturnTy(int)>& func_int) const {
    if (IsConstInt()) return func_int(GetInt());
    return func_var(GetVar());
  }

  string ToString() const {
    if (IsVariable()) return GetVar()->ToString();
    return std::to_string(GetInt());
  }

 private:
  // The type to return when holding an integer constant (so that we don't have
  // to keep creating it whenever GetType() is called).
  inline static Type int_type_ = Type::Int();

  variant<VarPtr_t, int> op_;
};

// Arithmetic: "lhs = op1 'operation' op2".
class ArithInst {
 public:
  // Arithmetic operations.
  enum Aop { kAdd, kSubtract, kMultiply, kDivide };

  ArithInst(VarPtr_t lhs, const Operand& op1, const Operand& op2, Aop operation)
      : lhs_(CHECK_NOTNULL(lhs)), op1_(op1), op2_(op2), operation_(operation) {}

  VarPtr_t lhs() const { return lhs_; }
  const Operand& op1() const { return op1_; }
  const Operand& op2() const { return op2_; }
  Aop operation() const { return operation_; }

 private:
  VarPtr_t lhs_;
  Operand op1_, op2_;
  Aop operation_;
};

// Comparison: "lhs = (op1 'operation' op2)". 'lhs' is 1 for true, 0 for false.
class CmpInst {
 public:
  // Relational operations.
  enum Rop {
    kEqual,
    kNotEqual,
    kLessThan,
    kGreaterThan,
    kLessThanEqual,
    kGreaterThanEqual
  };

  CmpInst(VarPtr_t lhs, const Operand& op1, const Operand& op2, Rop operation)
      : lhs_(CHECK_NOTNULL(lhs)), op1_(op1), op2_(op2), operation_(operation) {}

  VarPtr_t lhs() const { return lhs_; }
  const Operand& op1() const { return op1_; }
  const Operand& op2() const { return op2_; }
  Rop operation() const { return operation_; }

 private:
  VarPtr_t lhs_;
  Operand op1_, op2_;
  Rop operation_;
};

// Phi: 'lhs' is a copy of one of the operands depending on which predecessor
// block the execution came from.
class PhiInst {
 public:
  PhiInst(VarPtr_t lhs, const vector<Operand>& ops)
      : lhs_(CHECK_NOTNULL(lhs)), ops_(ops) {}

  VarPtr_t lhs() const { return lhs_; }
  const vector<Operand>& ops() const { return ops_; }

 private:
  VarPtr_t lhs_;
  vector<Operand> ops_;
};

// Copy: "lhs = rhs".
class CopyInst {
 public:
  CopyInst(VarPtr_t lhs, const Operand& rhs)
      : lhs_(CHECK_NOTNULL(lhs)), rhs_(rhs) {}

  VarPtr_t lhs() const { return lhs_; }
  const Operand& rhs() const { return rhs_; }

 private:
  VarPtr_t lhs_;
  Operand rhs_;
};

// Memory allocation: "lhs = allocate_memory()". The type of the left-hand side
// variable determines what is being allocated; the number of things being
// allocated is left unspecified (i.e., it could be an array of things).
class AllocInst {
 public:
  explicit AllocInst(VarPtr_t lhs) : lhs_(CHECK_NOTNULL(lhs)) {}

  VarPtr_t lhs() const { return lhs_; }

 private:
  VarPtr_t lhs_;
};

// Get address of a local variable: "lhs = &rhs".
class AddrOfInst {
 public:
  AddrOfInst(VarPtr_t lhs, VarPtr_t rhs)
      : lhs_(CHECK_NOTNULL(lhs)), rhs_(CHECK_NOTNULL(rhs)) {}

  VarPtr_t lhs() const { return lhs_; }
  VarPtr_t rhs() const { return rhs_; }

 private:
  VarPtr_t lhs_, rhs_;
};

// Load: "lhs = *src".
class LoadInst {
 public:
  LoadInst(VarPtr_t lhs, VarPtr_t src)
      : lhs_(CHECK_NOTNULL(lhs)), src_(CHECK_NOTNULL(src)) {}

  VarPtr_t lhs() const { return lhs_; }
  VarPtr_t src() const { return src_; }

 private:
  VarPtr_t lhs_, src_;
};

// Store: "*dst = value".
class StoreInst {
 public:
  StoreInst(VarPtr_t dst, const Operand& value)
      : dst_(CHECK_NOTNULL(dst)), value_(value) {}

  VarPtr_t dst() const { return dst_; }
  const Operand& value() const { return value_; }

 private:
  VarPtr_t dst_;
  Operand value_;
};

// GetElementPtr: take the value of 'src_ptr', advance it by 'index' elements
// (of size determined by the type of src_ptr), then (if non-empty and the
// element type is a struct) further advance it to the field specified by
// 'field_name'.
//
// Thus, if src_ptr points to an array then 'index' moves the pointer within the
// array, and if the type of src_ptr is to a struct then 'field_name' moves the
// pointer within a single struct.
class GepInst {
 public:
  GepInst(VarPtr_t lhs, VarPtr_t src_ptr, const Operand& index,
          const string& field_name)
      : lhs_(CHECK_NOTNULL(lhs)),
        src_ptr_(CHECK_NOTNULL(src_ptr)),
        index_(index),
        field_name_(field_name) {}

  VarPtr_t lhs() const { return lhs_; }
  VarPtr_t src_ptr() const { return src_ptr_; }
  const Operand& index() const { return index_; }
  const string& field_name() const { return field_name_; }

 private:
  VarPtr_t lhs_, src_ptr_;
  Operand index_;
  string field_name_;
};

// Ternary operator: "lhs = (condition ? true_op : false_op)".
class SelectInst {
 public:
  SelectInst(VarPtr_t lhs, const Operand& condition, const Operand& true_op,
             const Operand& false_op)
      : lhs_(CHECK_NOTNULL(lhs)),
        condition_(condition),
        true_op_(true_op),
        false_op_(false_op) {}

  VarPtr_t lhs() const { return lhs_; }
  const Operand& condition() const { return condition_; }
  const Operand& true_op() const { return true_op_; }
  const Operand& false_op() const { return false_op_; }

 private:
  VarPtr_t lhs_;
  Operand condition_, true_op_, false_op_;
};

// Direct function call: "lhs = func_name(args)".
class CallInst {
 public:
  CallInst(VarPtr_t lhs, const string& callee, const vector<Operand>& args)
      : lhs_(CHECK_NOTNULL(lhs)), callee_(callee), args_(args) {}

  VarPtr_t lhs() const { return lhs_; }
  const string& callee() const { return callee_; }
  const vector<Operand>& args() const { return args_; }

 private:
  VarPtr_t lhs_;
  string callee_;
  vector<Operand> args_;
};

// Indirect function call: "lhs = (*func_ptr)(args)".
class ICallInst {
 public:
  ICallInst(VarPtr_t lhs, VarPtr_t func_ptr, const vector<Operand>& args)
      : lhs_(CHECK_NOTNULL(lhs)),
        func_ptr_(CHECK_NOTNULL(func_ptr)),
        args_(args) {}

  VarPtr_t lhs() const { return lhs_; }
  VarPtr_t func_ptr() const { return func_ptr_; }
  const vector<Operand>& args() const { return args_; }

 private:
  VarPtr_t lhs_, func_ptr_;
  vector<Operand> args_;
};

// Return from function.
class RetInst {
 public:
  explicit RetInst(const Operand& retval) : retval_(retval) {}

  const Operand& retval() const { return retval_; }

 private:
  Operand retval_;
};

// Jump to basic block.
class JumpInst {
 public:
  explicit JumpInst(const string& label) : label_(label) {}

  const string& label() const { return label_; }

 private:
  string label_;
};

// Branch to one of two basic blocks depending on condition.
class BranchInst {
 public:
  BranchInst(const Operand& condition, const string& label_true,
             const string& label_false)
      : condition_(condition),
        label_true_(label_true),
        label_false_(label_false) {}

  const Operand& condition() const { return condition_; }
  const string& label_true() const { return label_true_; }
  const string& label_false() const { return label_false_; }

 private:
  Operand condition_;
  string label_true_, label_false_;
};

// A forward reference because instructions have a pointer to their enclosing
// basic block.
class BasicBlock;

// A program instruction (i.e., one of the above instruction classes).
class Instruction {
 public:
  enum Opcode {
    kArith = 0,
    kCmp,
    kPhi,
    kCopy,
    kAlloc,
    kAddrof,
    kLoad,
    kStore,
    kGep,
    kSelect,
    kCall,
    kICall,
    kRet,
    kJump,
    kBranch,
  };

  Instruction(const ArithInst& inst, const BasicBlock* parent = nullptr)
      : inst_(inst), parent_(parent) {}
  Instruction(const CmpInst& inst, const BasicBlock* parent = nullptr)
      : inst_(inst), parent_(parent) {}
  Instruction(const PhiInst& inst, const BasicBlock* parent = nullptr)
      : inst_(inst), parent_(parent) {}
  Instruction(const CopyInst& inst, const BasicBlock* parent = nullptr)
      : inst_(inst), parent_(parent) {}
  Instruction(const AllocInst& inst, const BasicBlock* parent = nullptr)
      : inst_(inst), parent_(parent) {}
  Instruction(const AddrOfInst& inst, const BasicBlock* parent = nullptr)
      : inst_(inst), parent_(parent) {}
  Instruction(const LoadInst& inst, const BasicBlock* parent = nullptr)
      : inst_(inst), parent_(parent) {}
  Instruction(const StoreInst& inst, const BasicBlock* parent = nullptr)
      : inst_(inst), parent_(parent) {}
  Instruction(const GepInst& inst, const BasicBlock* parent = nullptr)
      : inst_(inst), parent_(parent) {}
  Instruction(const SelectInst& inst, const BasicBlock* parent = nullptr)
      : inst_(inst), parent_(parent) {}
  Instruction(const CallInst& inst, const BasicBlock* parent = nullptr)
      : inst_(inst), parent_(parent) {}
  Instruction(const ICallInst& inst, const BasicBlock* parent = nullptr)
      : inst_(inst), parent_(parent) {}
  Instruction(const RetInst& inst, const BasicBlock* parent = nullptr)
      : inst_(inst), parent_(parent) {}
  Instruction(const JumpInst& inst, const BasicBlock* parent = nullptr)
      : inst_(inst), parent_(parent) {}
  Instruction(const BranchInst& inst, const BasicBlock* parent = nullptr)
      : inst_(inst), parent_(parent) {}
  Instruction(const Instruction& inst, const BasicBlock* parent)
      : inst_(inst.inst_), parent_(parent) {}

  Opcode GetOpcode() const { return static_cast<Opcode>(inst_.index()); }

  // Returns the index of this instruction within its containing basic block, if
  // there is one (otherwise returns -1).
  int GetIndex() const;

  // Return the containing basic block, if there is one (otherwise returns
  // nullptr).
  const BasicBlock* parent() const { return parent_; }

  // Getters; these throw an exception if the one called doesn't align with the
  // particular type of instruction being held.
  const ArithInst& AsArith() const { return std::get<kArith>(inst_); }
  const CmpInst& AsCmp() const { return std::get<kCmp>(inst_); }
  const PhiInst& AsPhi() const { return std::get<kPhi>(inst_); }
  const CopyInst& AsCopy() const { return std::get<kCopy>(inst_); }
  const AllocInst& AsAlloc() const { return std::get<kAlloc>(inst_); }
  const AddrOfInst& AsAddrOf() const { return std::get<kAddrof>(inst_); }
  const LoadInst& AsLoad() const { return std::get<kLoad>(inst_); }
  const StoreInst& AsStore() const { return std::get<kStore>(inst_); }
  const GepInst& AsGep() const { return std::get<kGep>(inst_); }
  const SelectInst& AsSelect() const { return std::get<kSelect>(inst_); }
  const CallInst& AsCall() const { return std::get<kCall>(inst_); }
  const ICallInst& AsICall() const { return std::get<kICall>(inst_); }
  const RetInst& AsRet() const { return std::get<kRet>(inst_); }
  const JumpInst& AsJump() const { return std::get<kJump>(inst_); }
  const BranchInst& AsBranch() const { return std::get<kBranch>(inst_); }

  void Visit(IrVisitor* visitor) const;

  string ToString() const;

  // Returns an instruction read from a string in the same format as that output
  // by ToString(). Warning: two different calls to FromString will always
  // return instructions using different VarPtr_t, even if the variable names
  // and types (or even the entire string) are the same.
  static Instruction FromString(const string& instruction);

 private:
  // It is critical that these alternatives are listed in the same order as the
  // Opcode constants in order for the various getters to work correctly.
  variant<ArithInst, CmpInst, PhiInst, CopyInst, AllocInst, AddrOfInst,
          LoadInst, StoreInst, GepInst, SelectInst, CallInst, ICallInst,
          RetInst, JumpInst, BranchInst>
      inst_;

  // If non-null, points to the containing basic block.
  const BasicBlock* parent_;
};

// A basic block: an ordered sequence of instructions ending in a terminator
// instruction (ret, jmp, br). A basic block always has a unique (within the
// containing function) label.
class BasicBlock {
 public:
  BasicBlock(const string& label, const vector<Instruction>& body,
             const Function* parent = nullptr);

  BasicBlock(const BasicBlock& bb);

  const string& label() const { return label_; }
  const vector<Instruction>& body() const { return body_; }

  // If this basic block is not contained within a function then its parent is a
  // nullptr.
  const Function* parent() const { return parent_; }

  // Returns the instruction at the given index within the basic block; FATALs
  // if the index is not within bounds.
  const Instruction& operator[](int index) const;

  void Visit(IrVisitor* visitor) const;

  string ToString() const;

  // Returns a basic block read from a string in the same format as that output
  // by ToString(). Warning: two different calls to FromString will always
  // return instructions using different VarPtr_t, even if the variable names
  // and types (or even the entire string) are the same.
  static BasicBlock FromString(const string& basic_block);

 private:
  string label_;
  vector<Instruction> body_;
  const Function* parent_;
};

// A convenient type alias. We use BbPtr_t to reference basic blocks so that
// their address doesn't change when a function or program is copied.
using BbPtr_t = shared_ptr<const BasicBlock>;

// A function.
class Function {
 public:
  // 'body' should contains a basic block with the label "entry", which is the
  // entry point to the function.
  Function(const string& name, const Type& return_type,
           const vector<VarPtr_t>& parameters, const vector<BasicBlock>& body);

  Function(const Function& fun);

  const string& name() const { return name_; }
  const Type& return_type() const { return return_type_; }
  const vector<VarPtr_t>& parameters() const { return parameters_; }
  const map<string, BbPtr_t>& body() const { return body_; }

  // Returns the basic block with the given label; FATALs if no such basic block
  // exists.
  const BasicBlock& operator[](const string& label) const;

  void Visit(IrVisitor* visitor) const;

  string ToString() const;

  // Returns a function read from a string in the same format as that output by
  // ToString(). Warning: two different calls to FromString, even on the same
  // string, will result in different VarPtr_t for globals like function
  // pointers and nullptr.
  static Function FromString(const string& function);

 private:
  string name_;
  Type return_type_;
  vector<VarPtr_t> parameters_;

  // Basic block id ==> basic block.
  map<string, BbPtr_t> body_;
};

// A convenient type alias. We use FuncPtr_t to reference functions so that
// their address doesn't change when a program is copied.
using FuncPtr_t = shared_ptr<const Function>;

// A program.
class Program {
 public:
  // 'struct_types' is a map from struct type name to a map from struct field
  // name to struct field type. 'functions' is a map from function name to
  // function.
  Program(const map<string, map<string, Type>>& struct_types,
          const vector<Function>& functions);

  void Visit(IrVisitor* visitor) const;

  const map<string, map<string, Type>>& struct_types() const {
    return struct_types_;
  }

  const map<string, FuncPtr_t>& functions() const { return functions_; }

  // Returns global function pointers for those functions whose address has been
  // taken (i.e., may not contain pointers to all functions).
  const map<string, VarPtr_t>& func_ptrs() const { return func_ptrs_; }

  // Returns the function with the given name; FATALs if no such function
  // exists.
  const Function& operator[](const string& name) const;

  string ToString() const;

  // Returns a program read from a string in the same format as that output by
  // ToString().
  static Program FromString(const string& program);

 private:
  // Returns an error message if the program is malformed; an empty result means
  // that the program is well-formed. Collects function pointer information
  // along the way.
  string VerifyIr();

  // Struct type name ==> (field name ==> type).
  map<string, map<string, Type>> struct_types_;

  // Function name ==> function. The entry point is a function named 'main'.
  map<string, FuncPtr_t> functions_;

  // Function name ==> global function pointer variable (e.g., function 'foo'
  // would map to variable '@foo' that is a pointer to 'foo'). Only contains
  // entries for those functions whose address has been taken (i.e., the global
  // function pointer is used somewhere in the code).
  map<string, VarPtr_t> func_ptrs_;
};

}  // namespace ir

// Allow hashing of Types and Operands for use in unordered_map and
// unordered_set.
namespace std {

// Forward declaration because Types contain variants which contain Types.
template <>
struct hash<ir::Type>;

template <>
struct hash<variant<monostate, string, vector<ir::Type>>> {
  inline size_t operator()(
      const variant<monostate, string, vector<ir::Type>>& v) const {
    switch (v.index()) {
      case 0:
        return hash<monostate>()(get<monostate>(v));

      case 1:
        return hash<string>()(get<string>(v));

      case 2: {
        std::size_t val = 0;
        for (const ir::Type& type : get<vector<ir::Type>>(v)) {
          hash_combine(val, type);
        }
        return val;
      }

      default:
        return 0;  // unreachable
    }
  }
};

template <>
struct hash<ir::Type> {
  inline size_t operator()(const ir::Type& type) const {
    std::size_t val = 0;
    hash_combine(val, type.indirection(), type.base_type());
    return val;
  }
};

template <>
struct hash<ir::Operand> {
  inline size_t operator()(const ir::Operand& op) const {
    std::size_t val = 0;
    if (op.IsVariable()) {
      hash_combine(val, op.GetVar());
    } else {
      hash_combine(val, op.GetInt());
    }
    return val;
  }
};

}  // namespace std
