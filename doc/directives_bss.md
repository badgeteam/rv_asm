
# BSS Directives

## `.space` Directive

The `.space` directive reserves a number of bytes in a `.bss` section.  
It is usually used in combination with an `.align` directive and a `symbol:` declaration.

### Syntax

```
.space <arithmetic expression>
```

### Example

```
.bss  
.align 10
symbol:
.space 1 << 10
```
