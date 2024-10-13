
# Lr Parser for Arithmetic Expressions and Relocations
An arithmetic expression consists of Values, Brackets and Operators.  
There are multiple types of Values:
- Numbers (Integers)
- Constants
- Named Symbols
- `.` Symbol  

The following Operators are implemented:  
`+` `-` `*` `/` `%`  
More Operators will be implemented later  

## Numerical Expressions

Numerical expressions are used to express numbers.  
The values of numerical expressions are limited to numbers and constants.  
No Symbols or dot-symbols are allowed.  
A Numerical Expression is evaluated to one Number.  

### Examples
```
1 + 1
( a * b ) / ( a + 10 * 2 - 0x22)
```

## Symbol Expressions

Symbol expressions are used to express relocations.  
The values of symbol expressions can be symbols and dot-symbols as well as numbers and constants.  
However there are some restrictions for symbols and dot-symbols.  
- Only `+` and `-` operators can be applied to symbols and dot-symbols.  
- Putting symbols and dot-symbols in brackets is not implemented at this point.  
- There must be exactly one symbol in the entire expression and it has to have a positive sign.
- Optionally there can be one dot-symbol with a negative sign (for pcrel relocations)

# Syntax

# Technical

