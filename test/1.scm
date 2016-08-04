(define (make) 
        (define y 100) 
        (define (withdraw z) (set! y (- y z)) y ) 
        withdraw )

(define (entry)
        (define w (make))
        (w 10)
        (w 20)
        (w 30)
)

(entry)
