# rvasm Documentation

# Directives

## Sections
### .text
### .data and .rodata
### .bss

## Misc
### .align

## Symbols

## Data

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
lui rd, %hi(symbol)
lui rd, %hi(symbol + 4)
lui rd, symbol
```
##### Auipc - Add Upper Immediate to Program Counter
```
Auipc rd, <number>
Auipc rd, pcrel_hi( <symbol> )
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

