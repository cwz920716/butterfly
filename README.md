# butterfly

High Level Objectives:
1. Butterfly is an experiment for High-Performance Scripting Language, in which I'll explore language design space for future high performance application, like deep learning
2. There is no specification for this language except it *should* be a scripting (or interpreted, managed) language like Python, Julia, etc.
3. This language (compiler/runtime) should be much more easier to extend, e.g., adding new language constructs or compiler optimization
4. I get frustrated by most current scripting language implementations: they are stable and powerful but unfriendly to newbies like me; This project may end up with a failure, but the goal is to offer a playground for inexperienced programmer to explore compiler/runtime research

Design Decisions:
1. Butterfly is for High Performance Computing, so the first design is to cover a reduced set of C language feature, i.e.,  "Scripted C"
0. More features will be added when necessary.
