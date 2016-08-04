(define (foo) 
        (define y 100) 
        (define (withdraw z) (set! y (- y z)) ) 
        withdraw )
