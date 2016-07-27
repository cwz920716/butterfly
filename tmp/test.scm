(define ( foo x ) (define y (box x)) (define withdraw (new-closure withdraw#0 y)) withdraw )
(define ( withdraw#0 _obj z ) (- (unbox (getfield 1 _obj)) z) )
