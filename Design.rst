*********
IR Design
*********

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

An operand can be one of the followings:

* a value, which can be either a variable or slot,
* a literal constatnt, however, this constant could be a label ID or a real number depends on instruction semantics.

Usually an operand's type is fixed at certain location.

An operator is what we should do with the instruction, common operators are ADD, SUB, GOTO etc.
More details can be found in next section.

Instruction Semantics
---------------------

