(define (foo x) 
        (define y x) 
        (define (withdraw z) (- y z) ) 
        withdraw )
