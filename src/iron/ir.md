note: internal instructions (usually prefixed with `FE__`, `FeInst__`, etc.) are not documented here.

## `param` - get function parameter

`%result: ty = param n`

get the value of parameter `n`. may only be used in the entry block of a function, and may only be the first instruction or preceded by other `param` instructions.

this instruction is automatically generated upon function creation. generating it in client code is a bad idea and will probably break shit.

## `proj` - tuple projection

`%result: ty = proj %src, n`

get a specific value at index `n` from a tuple provided by intruction `%src`. `%src` must have type `FE_TY_TUPLE` and must provide at least `n`+1 values.

## `const` - scalar constant

`%result: ty = const n`

create an integer or floating point constant of type `ty` with value `n`.

- `ty` must be a scalar type (single integer or floating point value)

## `sym-addr` - address of symbol

`%result: ty = sym-addr "symbol"`

get the address of a symbol `"symbol"` as a constant value of type `ty`, where `ty` is the pointer-sized integer of the target architecture.

## `iadd` - integer addition

`%result: ty = iadd %lhs, %rhs`

return the sum of two integers of type `ty`. overflow/underflow is defined to wrap according to two's complement.

## `isub` - integer subtraction

`%result: ty = isub %lhs, %rhs`

return the difference of two integers of type `ty`. overflow/underflow is defined to wrap according to two's complement.

## `imul` - integer multiplication

`%result: ty = imul %lhs, %rhs`

return the product of two integers of type `ty`. overflow/underflow is defined to wrap according to two's complement.

## `idiv` - signed integer division

`%result: ty = idiv %lhs, %rhs`

return the quotient from the division of type `ty`. if `%rhs` is `const 0`, this instruction cannot be evaluated to a constant during optimization.

## `udiv` - unsigned integer division

`%result: ty = udiv %lhs, %rhs`

return the quotient from the division of two unsigned integers of type `ty`. if `%rhs` is `const 0`, this instruction cannot be evaluated to a constant during optimization.

## `irem` - signed integer remainder

`%result: ty = irem %lhs, %rhs`

return the remainder from the division of two signed integers of type `ty`. if `%rhs` is `const 0`, this instruction cannot be evaluated to a constant during optimization.

note that this is the *remainder*, where the result is zero or has the same sign as `%rhs`, not the *modulo*.

## `urem` - unsigned integer remainder

`%result: ty = urem %lhs, %rhs`

return the remainder from the division of two unsigned integers of type `ty`. if `%rhs` is `const 0`, this instruction cannot be evaluated to a constant during optimization.

## `and` - bitwise and

`%result: ty = and %lhs, %rhs`

return the bitwise logical 'and' of two integers of type `ty`

## `or` - bitwise and

`%result: ty = or %lhs, %rhs`

return the bitwise logical 'or' of two integers of type `ty`

## `xor` - bitwise and

`%result: ty = xor %lhs, %rhs`

return the bitwise logical 'xor' of two integers of type `ty`

## `shl` - bitwise shift left

`%result: ty = shl %lhs, %rhs`

return the integer `%lhs` shifted left by (unsigned) `%rhs` bits. if (unsigned) `%rhs` is greater than the bit size of `ty`, the result is equivalent to `const 0`.

## `usr` - unsigned bitwise shift right

`%result: ty = usr %lhs, %rhs`

return the integer `%lhs` shifted right by (unsigned) `%rhs` bits, where the most-significant bits are *zeroed*. if (unsigned) `%rhs` is greater than the bit size of `ty`, the result is equivalent to `const 0`.

## `isr` - signed bitwise shift right

`%result: ty = isr %lhs, %rhs`

return the integer `%lhs` shifted right by (unsigned) `%rhs` bits, where the most-significant bits are *taken from the most significant bit of `%lhs`*. if (unsigned) `%rhs` is greater than the bit size of `ty`, the result is equivalent to `const 0`.
