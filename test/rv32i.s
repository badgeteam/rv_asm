
  .text

symbol0:

 # lui
  lui t0, 4
  lui a0, %hi(symbol0)
  lui s0, %hi(symbol0 + 4)
  lui s0, symbol0

  #auipc
  auipc t1, 4
  auipc a1, %pcrel_hi(symbol0)
  auipc s1, %pcrel_hi(symbol0 + 4)
  auipc s1, symbol0


