(define (fac x)
        (if (> x 2) 
            (g (fac (- x 1)) x)
            1))

(define (g x y) (* x y))

(define (even n)
        (if (= n 0) 
            1
            (odd (- n 1))))

(define (odd n)
        (if (= n 0) 
            0
            (even (- n 1)))) 

(fac 0)

(fac 4)

(even 0)

(even 120)

(even 121)

(odd 121)
