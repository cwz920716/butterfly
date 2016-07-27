(define (foo x) 
        (define (bar y) 
               (define (baz z) y) 
               (foobar 1 2) 
               baz ) 
        (define (foobar a b) (+ a b)) 
        bar 
        (foobar 1 2) 
        (set! foobar 2) )
