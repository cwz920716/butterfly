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

           closure @lambda-gensym1 (%amount):

             Label L0

             _1 = Vload.c %balance %%self

             _2 = Vload %amount

             _3 = Sub _1 _2

             VStore.c %balance %%self _3

             _4 = Vload.c %balance %%self

             Ret _4

           end

           function @make-withdraw (%balance):

             Label L0

             _1 = Closure @lambda-gensym1 %%self

             Ret _1

           end

Example 6: ex = :(a + b); eval(ex)

           _1 = Quote Vload a

           _2 = Quote Vload b

           _3 = Quote Add _1 _2

           _4 = Eval _3

Example 7: (define arguments '(10 50 100)) (apply + arguments)

           _1 = Int 10

           _2 = Int 50

           _3 = Int 100

           _4 = Call @list _1 _2 _3 

           _5 = Vload @@plus

           Apply _5 _4

           Note: this example used two feature that is not-yet-designed, "variable arguments" and "operator as function", the intuition of this example is to introduce apply instruction.

Example 8: macro time(ex) local t0 = time() local val = $ex local t1 = time() [t1-t0] val end end

           macro @time(%ex)

             Label L0

             _1 = Quote Vdefine %t0

             _2 = Quote Call @time

             _3 = Quote Vstore %t0 _1

             _4 = Quote Vdefine %val

             _5 = Vload %ex

             _6 = Quote Vstore %val _5

             _7 = Quote Vdefine %t1

             _8 = Quote Call @time

             _9 = Quote Vstore %t1 _4

             ...

             _a = Quote Vload %t0

             _b = Quote Vload %t1

             _c = Quote Sub _a _b

             ...

             _x = Quote Vload %val

             _y = Quote Begin _1 _2 _3 ... _a ... _x

             Ret _y

           end

           

Definitions
-----------

By design, this IR is to resemble LLVM IR as much as possible. However, there are certain differences that you have already noticed, like there is no types, closures and macros, etc. So here, we give an overview of how this IR is defined.

In high level, this is a modular system just like LLVM. A IR module can be mapped to a file in a non-modular scripting language or a module in a modular language. For now, we assume there is only one module since the details of IR module is not-designed-yet.

There are four primitives can live in a module, a global variable, a function, a closure and a macro. We will describe all of them later. Just like LLVM, all global names are named after @.


