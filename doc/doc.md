# rvasm Documentation

# Directives

## Sections
### .text
### .data and .rodata
### .bss
### .section
## Misc
### .align
### .equ

## Symbols

### Create Symbol Directives
| Directive             | Type         | Bind         | Visibility    | Description                |
| --------------------- | ------------ | ------------ | ------------- | -------------------------- |
| `label:`              | `STT_NOTYPE` | `STB_LOCAL`  | `STV_DEFAULT` | Create if not existent     |
| `.global symbolname`  | `STT_NOTYPE` | `STB_GLOBAL` | `STV_DEFAULT` | Create or Modify Bind      |
| `.globl symbolname`   | `STT_NOTYPE` | `STB_GLOBAL` | `STV_DEFAULT` | Create or Modify Bind      |
| `.local symbolname`   | `STT_NOTYPE` | `STB_LOCAL`  | `STV_DEFAULT` | Create or Modify Bind      |
| `.weak symbolname`    | `STT_NOTYPE` | `STB_WEAK`   | `STV_DEFAULT` | Create or Modify Bind      |
| `.extern symbolname`  | `STT_NOTYPE` | `STB_GLOBAL` | `STV_DEFAULT` | Create or Error upon Redef |

### Modify Symbol Attributes Directives
| Directive                       | Attribute  | Parameter                                 |
| ------------------------------- | ---------- | ----------------------------------------- |
| `.type symbolname, <type>`      | Type       | `@function` `@object` `@notype`           |
| `.size symbolname, <size>`      | Size       | `<number>`                                |
| `.visibility symbolname, <vis>` | Visibility | `default` `hidden` `internal` `protected` |



## Data
### .string
### .ascii
### .zero
### .byte
### .half
### .word
### .2byte
### .4byte
### .incbin

## Bss
### .space


## RiscV
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

