
#define OPC(bits) "B:" #bits
#define D "D"
#define W "W"
#define S "S"
#define REG "REG"
#define MOD "MOD"
#define SR "SR"
#define RM "R/M"
#define ADDR_HI "ADDR_HI"
#define ADDR_LO "ADDR_LO"
#define W_DATA "W_DATA"
#define DECODE_RG "DECODE_RG"
#define DECODE_RM "DECODE_RM"
#define DECODE_IM "DECODE_IM"

char *instructions[][16] = {

  // DATA TRANSFER
  {"MOV", OPC(100010), D, W, MOD, REG, RM},
  {"MOV", OPC(1100011), W, MOD, OPC(000), RM, W_DATA},
  {"MOV", OPC(1011), W, REG, W_DATA},
  {"MOV", OPC(1010000), W, ADDR_LO, ADDR_HI},
  {"MOV", OPC(1010001), W, ADDR_LO, ADDR_HI},
  {"MOV", OPC(10001110), MOD, OPC(0), SR, RM},
  {"MOV", OPC(10001100), MOD, OPC(0), SR, RM},

  {"PUSH", OPC(11111111), MOD, OPC(110), RM},
  {"PUSH", OPC(01010), REG},
  {"PUSH", OPC(000), REG, OPC(110)},

  {"POP", OPC(10001111), MOD, OPC(000), RM},
  {"POP", OPC(01011), REG},
  {"POP", OPC(000), REG, OPC(111)},

  {"XCHG", OPC(1000011), W, MOD, REG, RM},
  {"XCHG", OPC(10010), REG},

  // {"IN", OPC(1110010), W, DATA},
  {"IN", OPC(1110110), W},

  // {"OUT", OPC(1110011), W, DATA},
  {"OUT", OPC(1110111), W},

  {"XLAT", OPC(11010111)},
  {"LEA", OPC(10001101), MOD, REG, RM},
  {"LDS", OPC(11000101), MOD, REG, RM},
  {"LES", OPC(11000100), MOD, REG, RM},

  {"LAHF", OPC(10011111)},
  {"SAHF", OPC(10011110)},
  {"PUSHF", OPC(10011100)},
  {"POPF", OPC(10011101)},

  // ARITHMETIC
  {"ADD", OPC(000000), D, W, MOD, REG, RM},
  {"ADD", OPC(100000), S, W, MOD, OPC(000), RM, W_DATA},
  {"ADD", OPC(0000010), W, W_DATA},

};

#undef OPC
#undef D
#undef W
#undef S
#undef REG
#undef MOD
#undef SR
#undef RM
#undef ADDR_HI
#undef ADDR_LO
#undef W_DATA
