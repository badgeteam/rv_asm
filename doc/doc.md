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
Lui rd, <number>
Lui rd, %hi( <symbol> )
```
##### Auipc - Add Upper Immediate to Program Counter
```
Auipc rd, <number>
Auipc rd, pcrel_hi( <symbol> )
```
##### Jal - Jump And Link
```
Jal ra, <symbol>
```
##### Jalr - Jump And Link Register
```
Jalr ra, sp 
Jalr ra, sp, <number>
Jalr ra, sp, lo( <symbol> )
Jalr ra, sp, pcrel_lo( <symbol> )
```
##### Beq - Branch if EQual
```
Beq t0, t1, <symbol>
```
##### Bne - Branch if Not Equal
```
Bne t0, t1, <symbol>
```
##### Blt - Branch if Less Than
```
Blt t0, t1, <symbol>
```
##### Bge - Branch if Greater or Equal
```
Bge t0, t1, <symbol>
```
##### Bltu - Branch if Less Than Unsigned
```
Bltu t0, t1, <symbol>
```
##### Bgeu - Branch if Greater or Equal Unsigned
```
Bgeu t0, t1, <symbol>
```

### Load
Load instructions load values from memory into registers.  
They are encoded into the I Encoding, so They

| Syntax | Offset | Value Extension |
| - | - | - |
| `{lb, lh, lw} rd, rs` | $0$ |
| `{lb, lh, lw} rd, rs, <imm12>`| imm12 | sign
| `{lb, lh, lw} rd, rs, lo(<symbol>)` | Relocation Low 12 Bytes of Symbol | signed |
| `{lb, lh, lw} rd, rs, pcrel_lo(<symbol>)` | Relocation Low 12 bytes pc relative to symbol | signed |


##### Lb - Load Byte
Loads a byte from memory into rd sign extending it to 32 bits.  
The immediate offset is optional and can be expressed as a %lo or a %pcrel_lo relocation.
```
Lb rd, rs
Lb rd, rs, <offset>
Lb rd, rs, lo( <symbol> )
Lb rd, rs, pcrel_lo( <symbol> )
```
##### Lh - Load Halfword
Loads a halfword from memory into rd sign extending it to 32 bits
```
Lh rd, rs
Lh rd, rs, <offset>
Lh rd, rs, lo( <symbol> )
Lh rd, rs, pcrel_lo( <symbol> )
```
##### Lw - Load Word
Loads a word from memory into rd
```
Lw rd, rs
Lw rd, rs, <offset>
Lw rd, rs, lo( <symbol> )
Lw rd, rs, pcrel_lo( <symbol> )
```
##### Lbu - Load Byte Unsigned
Loads a byte from memory into rd zero extending it to 32 bits
```
Lbu rd, rs
Lbu rd, rs, <offset>
Lbu rd, rs, lo( <symbol> )
Lbu rd, rs, pcrel_lo( <symbol> )
```
##### Lhu - Load Halfword Unsigned
Loads a halfword from memory into rd zero extending it to 32 bits
```
Lhu rd, rs
Lhu rd, rs, <offset>
Lhu rd, rs, lo( <symbol> )
Lhu rd, rs, pcrel_lo( <symbol> )
```
##### Sb
##### Sh
##### Sw
##### Addi
##### Slti
##### Sltiu
##### Xori
##### Ori
##### Andi
##### Slli
##### Srli
##### Srai
##### Add
##### Sub
##### Sll
##### Slt
##### Sltu
##### Xor
##### Srl
##### Sra
##### Or
##### And
##### Fence
##### Fence.Tso
##### Pause
##### Ecall
##### Ebreak
#### RiscV32M
##### Mul
##### Mulh
##### Mulhsu
##### Mulhu
##### Div
##### Divu
##### Rem
##### Remu
#### RiscV32A
#### RiscV32F
#### RiscV32C
#### RiscV32Zicsr
#### RiscV32Zifencei


