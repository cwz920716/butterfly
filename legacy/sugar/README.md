# Sugar

Sugar is a source-to-source compiler, for now it is LISP-to-LISP compiler which unfolds complex grammar to simple grammar.
Its main objectives are:

1. compile closure to object creation

(define (inner_f a b ...) ... ) -> (define inner_f (new-clocure inner_f#0 a b ...))
                                   Global: (define (inner_f#0 _obj a b) ...)

Escaped: (define a ...) -> (define a (box ...))
         a -> (unbox a)

Enclosed: a -> (unbox (getfield ? _obj))

2. compile lambda expr to function def

(lambda (formal_args) expr) -> (define (gensymbol1 formal_args) expr) replace its appearance with gensym1

3. handle let keyword:

(let ((a 1) (b 2) ...) expr...) -> (begin (define a##0 1) (define b##1 2) ... )

3. handle recursion

4. etc.
