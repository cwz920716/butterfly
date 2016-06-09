# Syntax By Example

Example 1 (Function Definition):
---
function fib(x)
  if x > 3 then
    1 
  else
    fib(x-1)+fib(x-2)
  fi
end

fib(40)
--- 

Example 2 (Assignment):
---
a = 10
b = a + 20
a = 15
---
