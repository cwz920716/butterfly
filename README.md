# butterfly

Note: This project is temporarily halted after I have a semi-working prototype (without GC support). It is halted since I do not have the time to mantain multiple side projects simultaneously.

High Level Objectives:

1. Butterfly is an experiment for High-Performance Scripting Language, in which I'll explore language design space for future high performance application, like deep learning

2. There is no specification for this language except it *should* be a scripting (or interpreted, managed) language like Python, Julia, etc.

3. This language (compiler/runtime) should be much more easier to extend, e.g., adding new language constructs or compiler optimization

4. I get frustrated by most current scripting language implementations: they are stable and powerful but unfriendly to newbies like me; This project may end up with a failure, but the goal is to offer a playground for inexperienced programmer to explore compiler/runtime research

Design Decisions:

1. we start with a high perfromance implementation of Scheme using LLVM + JIT

2. metaprogramming, reflection and introspection (like LISP or Julia) *must* be supported

0. More features will be added when necessary.
