=========
IR Design
=========

This file summarizes an internal IR deign for Butterfly LISP. 
This is Low-level IR, which can br interpreted or JIT-ed to machine code or LLVM IR.
It is designed to simplify compiler/runtime code to facilitate runtime interrupt and optimization. 
In high level, this IR is a value-oriented, dynamic-typed, single-opertor-multi-operands bytecode.

The code name of this IR is undefined yet, but I am considering following options: Lightning, ColdSteel, etc.

Instructions and Values
-----------------------

An instruction contain up to one operator, any number of operands, and return one value.
A value is what returned by an instruction, which you can read and write. A value can be classified into two types: named and nameless. 
A named value is also called 'variable' in the sense they can are visible to developers in original program, and potentially mutable, and always have an address.
A variable can be stored on stack or heap or either, it is up to backends to decide where to put a variable.
A nameless value is called 'slot' in that it can hold something like 'variable' but it is only for temporary use and (usually) immutable and unaddressable.
Nil is a special value which is a place holder for some instructions like branch, label, etc.

Also, a value can be callable is it meets one of the following consitions:

* it is global function name variable (all functions are global, but their access may be restricted)
* it is a closure, i.e., callable object

An operand can be one of the followings:

* a value, which can be either a variable or slot,
* a literal constatnt, however, this constant could be a label ID or a real number depends on instruction semantics.

Usually an operand's type is fixed at certain location.

An operator is what we should do with the instruction, common operators are ADD, SUB, GOTO etc.
More details can be found in next section.

Instruction Semantics
---------------------
This section explains more details on instruction structure and semantics.

Control Instructions
~~~~~~~~~~~~~~~~~~~~
Goto Inst: OP_GOTO, <Label ID>, -> Nil

If Inst: OP_IF, <Value Predicate>, <Lable ID Then>, <Lable ID Else>, -> Nil

Variable Instructions
~~~~~~~~~~~~~~~~~~~
Variable Define Inst: OP_DEFINE, <Variable Name>, <Value Init>, -> Nil

Varaible Write Inst: OP_SET, <Variable Name>, <Value> -> Nil

Heap Object Instructions
~~~~~~~~~~~~~~~~~~~~~~~~
Box Inst: OP_BOX, <Value> -> <NewSlot>

Unbox Inst: OP_UNBOX, <Value> -> <NewSlot>

Setbox Inst: OP_SETBOX, <Value Box>, <Value New>, -> Nil

GetConstfiled Inst: OP_GETFIELD, <Value>, <Const Int> -> <NewSlot>

Closure Inst: OP_CLOSURE, <Function Variable>, <Value Arg0>, ..., -> <NewSlot>

Operational Instructions
~~~~~~~~~~~~~~~~~~~~~~~
Phi2 Inst: OP_PHI2, <Value A>:<Label ID A>, <Value B>:<Label ID B>, -> <NewSlot>

Call Inst: OP_CALL, <Value Callable>

Arithmetic/Logic Inst: OP_ADD, <Value A>, <Value B>, -> <NewSlot>

* OP_ADD

* OP_SUB

* OP_MUL

* OP_DIV

* OP_NEG

* OP_GT

* OP_GTE

* OP_LT

* OP_LTE

* OP_EQ

* OP_AND

* OP_OR

* OP_XOR

* OP_NOT

Int Inst: OP_INT, <Const Int>, -> <NewSlot>

Float Inst: OP_FLOAT, <Const Float>, -> <NewSlot>

Symbol Inst: OP_SYMBOL, <Const Symbol>, -> <NewSlot>
