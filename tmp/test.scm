(define ( foo ) (define y (box 100)) (define withdraw (closure withdraw#0 y)) withdraw )
(define ( withdraw#0 _obj z ) (setbox! (getfield 1 _obj) (- (unbox (getfield 1 _obj)) z)) )
1
