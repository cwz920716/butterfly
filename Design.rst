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
------

we will show some basic concepts through examples. More explanation will be introduced later gradually. The example will be shown in LISP first and then translate to IR.

Example 1: (+ 1 2)

           %1 = Int 1

           %2 = Int 2

           %3 = Add %1 %2

Example 2: (* (+ 1 1) (1.0 / 2))

           %1 = Int 1

           %2 = Int 1

           %3 = Add %1 %2

           %4 = Float 1.0

           %5 = Int 2

           %6 = Div %4 %5

           %7 = Mult %3 %6

Example 3: (define a 10) (if (< a 0) -a a)

           Vdefine %a

           %1 = Int 10

           Vstore %a %1

           %2 = Vload %a

           %3 = Int 0

           %4 = Lt %2 %3

           If %4 then L1 else L2

           Lable L1

           %5 = Vload %a

           %6 = Neg %a

           Goto L3

           Lable L2

           %7 = Vload %a

           Goto L3

           Label L3

           %8 = Phi %6 from L1 or %7 from L2
           
Example 4: (define (square x) (* x x))

           function @square (%x):

             Label L0

             %1 = Vload %x

             %2 = Vload %x

             %3 = Mult %1 %2
 
             Ret %3

           end

           
Example 5: (define (make-withdraw balance) (lambda (amount) (set! balance (- balance amount)) balance))

           closure @lambda-gensym1 (%amount):

             Label L0

             %1 = Vload.c %balance %%self

             %2 = Vload %amount

             %3 = Sub %1 %2

             VStore.c %balance %%self %3

             %4 = Vload.c %balance %%self

             Ret %4

           end

           function @make-withdraw (%balance):

             Label L0

             %1 = Closure @lambda-gensym1 %%self

             Ret %1

           end

Example 6: ex = :(a + b); eval(ex)

           %1 = Quote Vload a

           %2 = Quote Vload b

           %3 = Quote Add %1 %2

           %4 = Eval %3

Example 7: (define arguments '(10 50 100)) (apply + arguments)

           %1 = Int 10

           %2 = Int 50

           %3 = Int 100

           %4 = Call @list %1 %2 %3 

           %5 = Vload @+

           %6 = Apply %5 %4

           Note: this example used two feature that is not-yet-designed, "variable arguments" and "operator as function", the intuition of this example is to introduce apply instruction.

Example 8: macro time(ex) local t0 = time() local val = $ex local t1 = time() [t1-t0] val end end

           macro @time(%ex)

             Label L0

             %1 = Quote Vdefine %t0

             %2 = Quote Call @time

             %3 = Quote Vstore %t0 %1

             %4 = Quote Vdefine %val

             %5 = Vload %ex

             %6 = Quote Vstore %val %5

             %7 = Quote Vdefine %t1

             %8 = Quote Call @time

             %9 = Quote Vstore %t1 %4

             ...

             %a = Quote Vload %t0

             %b = Quote Vload %t1

             %c = Quote Sub %a %b

             ...

             %x = Quote Vload %val

             %y = Quote Begin %1 %2 %3 ... %a ... %x

             Ret %y

           end

           

Definitions
-----------

By design, this IR is to resemble LLVM IR as much as possible. However, there are certain differences that you have already noticed, like there is no types, closures and macros, etc. So here, we give an overview of how this IR is defined.

In high level, this is a modular system just like LLVM. A IR module can be mapped to a file in a non-modular scripting language or a module in a modular language. For now, we assume there is only one module since the details of IR module is not-designed-yet.

There are four primitives can live in a module, a global variable, a function, a closure and a macro. We will describe all of them later. Just like LLVM, all global names are named after @.

Fornow, we focus on functions. As in LLVM, functions are constructed from a graph of basic blocks. A basic block started with a Label instruction (unlike LLVM), and ends with either a) Ret instruction, b) Goto instruction, and c) If instruction. Within basic blocks there are a few compute/read/write instructions. Let's look at them in order.

Label instruction accepts a integer starting at 0 as its label, with 0 indicating entry label. In our example, we use L0, L1, ... to represent a label. Usually label won't be mapped to any ISA instruction, but it may help to have a IR instruction for label since we may need *jump threading*.

As you have notice, we have a lot of familiar instructions such as Add/Sub/Mult, etc. We also have LLVM-like Single Static Assignment Values returned by instructions. However, there are a few differences:

1. all our varaibles are polymorphic/dynamic typed, no type checking is required by almost all instructions

2. Unlike LLVM, Arithmetic-like instructions, (i.e., Add, Sub, And, even If), never directly operate on named varible (like %x, %y) nor literal constants. In LLVM, you may have instructions such as Add %a %b, where both a and b are local variables in C/C++. However, it is impossible in our IR. You must using Vload first to load the local variable and then add the returned value from Vload instructions. Read Example 3.

Before we introduce why wehave such restrictions, we first differentiate two types local variable names in our IR. The first type has a symbolic name such as 'a', 't0', 't1', andthey can be one-to-one mapped to the local variable in original languages (LISP). The second is numbered name such as %1 %2, etc., they are always returned by some instruction and strict SSA, you never assign to any numbered name. We refer to first type of name as 'variable', since they reflect scripting developers view of variable. The second type is called 'slot', meaning they are just temporaily hold variables and stored somewhere, butdoes not have a name. 

The key difference of slot and variable is that slot is not scripting developer visible and strict SSA. A slot may reflect a temporal value of a sub expression in a scripting language statements. Also, any variable must be convert to slot (through Vload) and then can be used in arithmetic instructions.

But why we must convert a variableto a slot and then operate on that slot in our IR? The reason is because our IR supports eval, hence new variables can be introduced or modified at runtime. Hence a variable access is more than a simple stack read in static languages. As a result, explicit variable access may enable more IR level optimization oppurtunities compared with implicit variable access.

Hence, in our IR, a variable is mimic to a memory location in LLVM so we have instructions such as Vload, Vstore and Vdefine. These three instructions will modify the evaluation environment hence may incur complicated operations in the interpreter runtime.

