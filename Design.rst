=========
IR Design
=========

This file summarizes an internal IR deign for Butterfly project. 
This is Low-level IR, which can br interpreted or JIT-ed to machine code or LLVM IR.
It is designed to simplify compiler/runtime code to facilitate runtime interrupt and optimization. 
In high level, this IR is a value-oriented, dynamic-typed, single-opertor-multi-operands bytecode. 
This IR is supposed to be LLVM-like s.t. it can be easily translated to LLVM while enabling high-level type-free optimization like value numbering, etc.
This IR should be viewed as an assembly language for *ANY* scripting languages, although it borrows idea from Scheme LISP and Julia
This IR is scripting-complete, i.e., it can be used to construct any scripting language program.

The code name of this IR is undefined yet, but here is what I am thinking of: *Lightning*, *ColdSteel*, *Azure*, *Trails*, etc.

Basics
~~~~~~

we will show some basic concepts through examples. More explanation will be introduced later gradually. The example will be shown in LISP first and then translate to IR.

Example 1: (+ 1 2)

           _1 = Int 1

           _2 = Int 2

           _3 = Add _1 _2

Example 2: (* (+ 1 1) (1.0 / 2))

           _1 = Int 1

           _2 = Int 1

           _3 = Add _1 _2

           _4 = Float 1.0

           _5 = Int 2

           _6 = Div _4 _5

           _7 = Mult _3 _6

Example 3: (define a 10) (if (< a 0) -a a)

           Vdefine %a

           _1 = Int 10

           Vstore %a _1

           _2 = Vload %a

           _3 = Int 0

           _4 = Lt _2 _3

           If _4 then L1 else L2

           Lable L1

           _5 = Vload %a

           _6 = Neg %a

           Goto L3

           Lable L2

           _7 = Vload %a

           Goto L3

           Label L3

           _8 = Phi _6 from L1 or _7 from L2
           
Example 4: (define (square x) (* x x))

           function @square (%x):

             Label L0

             _1 = Vload %x

             _2 = Vload %x

             _3 = Mult _1 _2
 
             Ret _3

           end

           
Example 5: (define (make-withdraw balance) (lambda (amount) (set! balance (- balance amount)) balance))

           closure ##1:

             @lambda-gensym-##1

             %balance

           end
           
           function @lambda-gensym-##1 (%self, %amount):

             Label L0

             _1 = Vload %self

             _2 = GetField _1 1

             _3 = Unbox _2

             _4 = Vload %amount

             _5 = Sub _3 _4

             Setbox _2 _5

             _6 = Vload %self

             _7 = GetField _6 1

             _8 = Unbox _7

             Ret _8

           end

           function @make-withdraw (%balance):

             Label L0

             _1 = Vload %balance

             _2 = Box _1

             Vstore %balance _2

             _3 = Vload %balance

             _4 = Closure @lambda-gensym-##1 _3

             Ret _4

           end


Definitions
~~~~~~~~~~~

Instructions and Values
-----------------------
In this IR, there will be three main conceptual types: Literal Constant, Instruction, and Value.

Literal Constant is *not* operatable, i.e., most instruction cannot read/write literal constants, they must be converted to value first. This is in order to protect from implementations where some literal constants will be allocated on heap and enable IR level optimizations when we can sttaically determine a value is equivalate to a const.

An instruction contain up to one operator, any number of operands, and returns at most one value.

A value is what returned by an instruction, which you can read and write. A value can be classified into two types: named and nameless. 
A value can also be a global var name, including function name. As LLVM, global var will have '@' and local var will have '%'.

A named value is also called 'variable' in the sense they can are visible to developers in original program, and potentially mutable, and always have an address.
A variable can be stored on stack or heap or either, it is up to backends to decide where to put a variable.
A nameless value is called 'slot' in that it can hold something like 'variable' but it is only for temporary use and (usually) immutable and unaddressable.
Nil is a special value which is a place holder for some instructions like branch, label, etc.

Note: Instruction and Value *must* origin from the same parent cloass in OOP implementation.

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

SetConstfiled Inst: OP_SETFIELD, <Value>, <Const Int> -> <NewSlot>

Tuple Inst: OP_TUPLE, <Value T0>, <Value T1>, ..., -> <NewSlot>

Closure Inst: OP_CLOSURE, <Function Variable>, <Value Arg0>, ..., -> <NewSlot>

Cons Inst: OP_CONS, <Value Arg0>, <Value Arg1> -> <NewSlot>

Operational Instructions
~~~~~~~~~~~~~~~~~~~~~~~~
Phi2 Inst: OP_PHI2, <Value A>:<Label ID A>, <Value B>:<Label ID B>, -> <NewSlot>

Call Inst: OP_CALL, <Value Callable>, <Value Args...>

Return Inst: OP_RET, <Value X>

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

* OP_NEQ

* OP_AND

* OP_OR

* OP_XOR

* OP_NOT

Literal-to-Value Instructions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Int Inst: OP_INT, <Const Int>, -> <NewSlot>

Float Inst: OP_FLOAT, <Const Float>, -> <NewSlot>

Symbol Inst: OP_SYMBOL, <Const Symbol>, -> <NewSlot>

Metalinguistic Instructions
~~~~~~~~~~~~~~~~~~~~~~~~~~~
It is common for scripting language to support stuff like eval, etc.
Hence it is important for our IR to do so.
Below is the design of metalinguistic instructions

Quote instruction (OP_QUOTE) is a quoted instruction which instead of evaluating the instruction and returning evaluated values, it will return evaluatable form of the instruction, i.e., something can be feed to the evaluator like eval() and make effects. Quote instruction can have following forms:

* quote a literal constant will be evaluated to the numerical/mathematical/logical value of that constant
* quote a variable will be evaluated to *that* variable in the eval() environment
* quote an instruction is used to form quoted expression, like :(a + b) will be translated to three instructions: _1 = quote a, _2 = quote b, _3 = quote add, _1, _2, and when you eval _3, it will evaluate to the sum of a and b in the evaluator environment
* quote a slot is kind of tricky, it will be like escaping a variable in Julia, and it works like quoting a literal constant in the eval() because slot is *never* bind to environment, but this literal constant is nt decided at compile time, instead, it is a runtime constant depending on the value of that slot. Say you have a instruction looks like this: 
* Also note a well formed quote instruction should not quote unquoted stuff unless it is quoting literal/variable/slot

Eval Instruction is like this: _3 = eval _2 where _2 is quoted form. It will evaluate _2 according to the current environment.

Environment is a symbol table where key is the variable name and value is the current value of the environment. define/assignment/call/return/eval can modify the environment. The definition of environment is the stack frames of function call trace *AND* global variables.
