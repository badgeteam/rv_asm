
# Data Directives

## `.ascii` `.string`

The `.ascii` and the `.string` directive write ascii characters to a `.data` or `.rodata` section.  
`.string` zero terminates the char sequence, `.ascii` does not.  
The directives take a single char sequence or a comma seperated list of char sequences.  

### Syntax

```
.ascii "test"
.ascii "test1", "test2", "test3"  
.string "test"
.string "test1", "test2", "test3"  
```

### Special characters  

The following special characters are implemented  
`\0` `\a` `\b` `\t` `\n` `\v` `\f` `\r` `\\` `\"`  

## `.zero`

The `.zero` directive writes a number of zeros into a `.data` section.
### Syntax
```
.zero <arithmetic expression>
```

## `.byte`

The `.byte` directive writes one or a comma seperated list of bytes represented as number or as char into a `.data` section.  

### Syntax

```
.byte 0x24, 255-128, 'A', '\\', '\n'
```

## `.2byte` `.half`
The `.2byte` and the `.half` directive writes a comma seperated list of 2 bytes into a `data` section.  
`.half` aligns the buffer to 2 bytes, `.2byte` does not.  

### Syntax

```
.2byte 0x1337
.half 0x1234, 0x1200 + 0x0034
```

## `.4byte` `.word`

The `.4byte` and the `.word` directive writes a list of 4 bytes or relocations into a `.data` section.  
`.word` aligns the buffer to 2 bytes, `.4byte` does not.

### Syntax

```
.4byte 0x12345678, 1+1
.4byte symbol + 8
.4byte symbol - . + 8
.word %32(symbol + 8)
.word %pcrel_32(symbol + 8)
```

## `.incbin`

The `.incbin` directive writes the binary content of a file into a `.data` section.  

### Syntax

```
.incbin "filename"
```

