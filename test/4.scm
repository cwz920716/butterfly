(define (foo x) 
        (define (fib x) 
                (if (> x 0) 
                    1 
                    (+ (fib (+ x 1)) (fib (- x 1)))
                )) 
        fib)
