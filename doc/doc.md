
# Table of Contents
1. [Sections](#Sections)
2. [Symbols](#Symbols)


# Sections
## Generic Section Directives
The following Directives create or select a section named like the Directive with default Attributes.

| Directive | Description           | Type           | Flags                       |
| --------- | --------------------- | -------------- | --------------------------- |
| `.text`   | Code                  | `SHT_PROGBITS` | `SHF_ALLOC` `SHF_EXECINSTR` |
| `.data`   | Initialized Data      | `SHT_PROGBITS` | `SHF_ALLOC` `SHF_WRITE`     |
| `.rodata` | Initialized Data      | `SHT_PROGBITS` | `SHF_ALLOC`                 |
| `.bss`    | Zero Initialized Data | `SHT_NOBITS`   | `SHF_ALLOC` `SHF_WRITE`     |

## .section
The `.section` Directive creates or selects a Section with specified Name, Type and Flags.  
If the Name starts with a generic Section name (for example `.text.mysectionname`),  
the Type and Flags can be omitted and the Default will be selected.  
### Valid Syntax
```
.section .text.mysectionname
.section .text.mysectionname, "<flags>"
.section .text.mysectionname, "<flags>", <type>
.section .mysectionname, "<flags>", <type>
```
#### Field Descriptions
- `"<flags>"` : Any Combination of `a` `w` `x` in Parenthesis. For Example: `"awx"`  
- `<type>` : `@progbits` or `@nobits`



# Symbols

There are several directives that create or modify Symbols.  

## Label Directive

Syntax: `<symbolname>:`  
This is the only one that sets the value and the section index.  
Those values are then locked into place.  
There will be an error if another Label Directive tries to change the value or the shndx.  
  
The Default Attributes are:
- Size: `0`
- Type: `STT_NOTYPE`
- Bind: `STB_LOCAL`
- Visibility: `STV_DEFAULT`

## Other Symbol Directives

All the other Symbol Directives only modify the relevant Attributes or create Placeholder Symbols

| Directive                 | Modified Field | Type         | Bind         | Visibility      | Size         |
| ------------------------- | -------------- | ------------ | ------------ | --------------- | ------------ |
| `.extern <name>`          | None           | Notype       | Local        | Default         | 0            |
| `.type <name>, @function` | Type           | `STT_FUNC`   | Local        | Default         | 0            |
| `.type <name>, @object`   | Type           | `STT_OBJECT` | Local        | Default         | 0            |
| `.type <name>, @notype`   | Type           | `STT_NOTYPE` | Local        | Default         | 0            |
| `.global <name>`          | Bind           | Notype       | `STB_GLOBAL` | Default         | 0            |
| `.globl <name>`           | Bind           | Notype       | `STB_GLOBAL` | Default         | 0            |
| `.local <name>`           | Bind           | Notype       | `STB_LOCAL`  | Default         | 0            |
| `.weak <name>`            | Bind           | Notype       | `STB_WEAK`   | Default         | 0            |
| `.hidden_name <name>`     | Visibility     | Notype       | Local        | `STV_HIDDEN`    | 0            |
| `.internal <name>`        | Visibility     | Notype       | Local        | `STV_INTERNAL`  | 0            |
| `.protected <name>`       | Visibility     | Notype       | Local        | `STV_PROTECTED` | 0            |
| `.size <name>, <size>`    | Size           | Notype       | Local        | Default         | **`<size>`** |

# Misc Directives
## .align
## .set
## .equ

# Data
## .string
## .ascii
## .zero
## .byte
## .half
## .word
## .2byte
## .4byte
## .incbin

# Bss
## .space


# RiscV
### Registers
#### Integer Registers
#### Float Registers
#### CSR Regasters
### Relocations
### Instructions
#### RiscV32I

##### Lui - Load Upper Immediate
```
lui rd, number
lui rd, symbol
lui rd, %hi(symbol)
```
##### Auipc - Add Upper Immediate to Program Counter
```
auipc rd, number
auipc rd, symbol
auipc rd, %pcrel_hi(symbol)
auipc rd, %got_pcrel_hi(symbol)
```


##### Load Store

```
lw rd, (rs1)
lw rd, offset(rs1)
lw rd, %lo(symbol)(rs1)
lw rd, %pcrel_lo(symbol)(rs1)
```

```
sw rs2, (rs1)
sw rs2, offset(rs1)
sw rs2, %lo(symbol)(rs1)
sw rs2, %pcrel_lo(symbol)(rs1)
```

##### Jalr
```
jalr rd, (rs1)
jalr rd, offset(rs1)
jalr rd, %lo(symbol)(rs1)
jalr rd, %pcrel_lo(symbol)(rs1)
```

##### Jal - Jump And Link
```
jal rd, symbol
```

##### Branch
```
beq rs1, rs2, symbol
bne rs1, rs2, symbol
blt rs1, rs2, symbol
bge rs1, rs2, symbol
bltu rs1, rs2, symbol
bgeu rs1, rs2, symbol
```

