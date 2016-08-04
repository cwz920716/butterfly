(define ( entry ) (define w (make )) (w 10 ) (w 20 ) (w 30 ) (w 40) (w 50) )
(define ( make ) (define y (box 100)) (define withdraw (closure withdraw#0 y)) withdraw )
(define ( withdraw#0 _obj z ) (setbox! (getfield 1 _obj) (- (unbox (getfield 1 _obj)) z)) (unbox (getfield 1 _obj)) )
(entry)
