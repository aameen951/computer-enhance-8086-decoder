#include <stdio.h>
#include "my_std.h"
#include "file_reader.h"

char *JCC_INST[] = {
  "JO", "JNO", "JB", "JAE", "JE", "JNE", "JBE", "JA",
  "JS", "JNS", "JP", "JNP", "JL", "JGE", "JLE", "JG",
};
char *LOGIC_OP_2OPR[] = {
  "ADD", "OR", "ADC", "SBB", "AND", "SUB", "XOR", "CMP"
};
char *BYTE_WORD_REG[2][8] = {
  {"AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH"},
  {"AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI"},
};
char *SR_REG[] = {
  "ES", "CS", "SS", "DS"
};
char *RM_DISPLACEMENT[] = {
  "BX + SI", "BX + DI", "BP + SI", "BP + DI", "SI", "DI", "BP", "BX"
};
char *W_TO_OPR_SIZE[] = {"BYTE", "WORD"};

inline bool masked_equal(u8 byte, u8 mask, u8 value){
  return (byte & mask) == value;
}
inline u8 shift_and(u8 byte, u8 shift, u8 and){
  return (byte >> shift) & and;
}

bool has_remaining(u8 *ptr, u8 *end, um size){
  return ptr + size <= end;
}
u8 read_u8(u8 **ptr_p){
  u8 result = *(u8 *)*ptr_p;
  *ptr_p += 1;
  return result;
}
u16 read_u16(u8 **ptr_p){
  u16 result = *(u16 *)*ptr_p;
  *ptr_p += 2;
  return result;
}
s8 read_s8(u8 **ptr_p){ return (s8)read_u8(ptr_p); }
s16 read_s16(u8 **ptr_p){ return (s16)read_u16(ptr_p); }

struct Inst2ndByte {
  u8 MOD;
  union {
    u8 REG;
    u8 OP;
    u8 SR;
    u8 YYY;
  };
  u8 RM;
};
Inst2ndByte decode_2nd_byte(u8 byte){
  Inst2ndByte result = {};
  result.MOD = (byte >> 6) & 0b11;
  result.REG = (byte >> 3) & 0b111;
  result.RM = (byte >> 0) & 0b111;
  return result;
}

bool decode_rm(u8 **ptr_p, u8 *end, u8 W, u8 MOD, u8 RM, char *decoded_rm, bool skip_size_override = false) {
  auto ptr = *ptr_p;
  auto w_str = !skip_size_override ? (!W ? "BYTE " : "WORD ") : "";

  switch(MOD) {

    case 0b00: {
      if(RM == 0b110) {
        if(!has_remaining(ptr, end, 2))return false;
        u16 DIRECT_ADDRESS = read_u16(&ptr);
        sprintf(decoded_rm, "%s[0x%04X]", w_str, DIRECT_ADDRESS); 
      } else {
        sprintf(decoded_rm, "%s[%s]", w_str, RM_DISPLACEMENT[RM]);
      }
    } break;

    case 0b01: {
      if(!has_remaining(ptr, end, 1))return false;
      auto DISPLACEMENT = read_s8(&ptr);
      sprintf(decoded_rm, "%s[%s + %d]", w_str, RM_DISPLACEMENT[RM], DISPLACEMENT);
    } break;

    case 0b10: {
      if(!has_remaining(ptr, end, 2))return false;
      auto DISPLACEMENT = read_s16(&ptr);
      sprintf(decoded_rm, "%s[%s + %d]", w_str, RM_DISPLACEMENT[RM], DISPLACEMENT);
    } break;

    case 0b11: {
      sprintf(decoded_rm, "%s", BYTE_WORD_REG[!!W][RM]);
    } break;

    default: return false;
  }

  *ptr_p = ptr;
  return true;
}


bool decode_2__ii__rg__rm(const char *inst, u8 **ptr_p, u8 *end, u8 D, u8 W) {
  auto ptr = *ptr_p;

  if(!has_remaining(ptr, end, 1))return false;

  auto second = decode_2nd_byte(*ptr);
  read_u8(&ptr);

  auto w_str = W_TO_OPR_SIZE[W];

  auto reg_name = BYTE_WORD_REG[W][second.REG];

  char decoded_rm[256] = "";
  if(!decode_rm(&ptr, end, W, second.MOD, second.RM, decoded_rm))return false;

  auto src = !D ? reg_name : decoded_rm;
  auto dest = !D ? decoded_rm : reg_name;

  printf("%s %s, %s\n", inst, dest, src);

  *ptr_p = ptr;
  return true;
}
bool decode_2__ii__im__rm(char *inst, u8 **ptr_p, u8 *end, u8 S, u8 W) {
  auto ptr = *ptr_p;

  if(!has_remaining(ptr, end, 1))return false;

  auto second = decode_2nd_byte(*ptr);
  read_u8(&ptr);

  char decoded_rm[256] = "";
  if(!decode_rm(&ptr, end, W, second.MOD, second.RM, decoded_rm))return false;

  auto dest = decoded_rm;

  u8 data_size = !S && W ? 2 : 1;

  if(!has_remaining(ptr, end, data_size))return false;

  u16 src = data_size == 2 ? read_u16(&ptr) : S ? ((s16)read_s8(&ptr)) : read_u8(&ptr);

  printf("%s %s, 0x%0*X\n", inst, dest, W ? 4 : 2, src);

  *ptr_p = ptr;
  return true;
}
bool decode_1__rg(char *inst, u8 **ptr_p, u8 *end) {
  auto ptr = *ptr_p;

  auto REG = (*ptr >> 0) & 0b111;
  read_u8(&ptr);

  auto dest = BYTE_WORD_REG[1][REG];

  printf("%s %s\n", inst, dest);

  *ptr_p = ptr;
  return true;
}
bool decode_1__i__rm(char *inst, u8 **ptr_p, u8 *end, u8 W) {
  auto ptr = *ptr_p;

  if(!has_remaining(ptr, end, 1))return false;

  auto MOD = (*ptr >> 6) & 0b11;
  auto RM = (*ptr >> 0) & 0b111;
  read_u8(&ptr);

  char decoded_rm[256] = "";
  if(!decode_rm(&ptr, end, W, MOD, RM, decoded_rm))return false;

  printf("%s %s\n", inst, decoded_rm);

  *ptr_p = ptr;
  return true;
}

bool decode_2__dw__rg__rm(char *inst, u8 **ptr_p, u8 *end) {
  auto D = (**ptr_p >> 1) & 0b1;
  auto W = (**ptr_p >> 0) & 0b1;
  read_u8(ptr_p);

  return decode_2__ii__rg__rm(inst, ptr_p, end, D, W);
}
bool decode_2___w__rg__rm(char *inst, u8 **ptr_p, u8 *end) {
  auto D = (**ptr_p >> 1) & 0b1;
  auto W = (**ptr_p >> 0) & 0b1;
  read_u8(ptr_p);

  return decode_2__ii__rg__rm(inst, ptr_p, end, D, W);
}

bool decode_2___w__im__rm(char *inst, u8 **ptr_p, u8 *end) {
  auto W = (**ptr_p >> 0) & 0b1;
  read_u8(ptr_p);

  return decode_2__ii__im__rm(inst, ptr_p, end, 0, W);
}
bool decode_2__sw__im__rm(char *inst, u8 **ptr_p, u8 *end) {
  auto S = (**ptr_p >> 1) & 0b1;
  auto W = (**ptr_p >> 0) & 0b1;
  read_u8(ptr_p);
  return decode_2__ii__im__rm(inst, ptr_p, end, S, W);
}
bool decode_2___w__im__ac(char *inst, u8 **ptr_p, u8 *end) {
  auto ptr = *ptr_p;

  auto W = (*ptr >> 0) & 0b1;
  read_u8(&ptr);

  if(!has_remaining(ptr, end, 1))return false;

  auto dest = W ? "AX" : "AL";

  u8 data_size = W ? 2 : 1;

  if(!has_remaining(ptr, end, data_size))return false;

  u16 src = data_size == 2 ? read_u16(&ptr) : read_u8(&ptr);

  printf("%s %s, 0x%0*X\n", inst, dest, W ? 4 : 2, src);

  *ptr_p = ptr;
  return true;
}


bool parse_8086(u8 *data, um size){
  auto ptr = data;
  auto end = ptr + size;

  while(has_remaining(ptr, end, 1)) {
    auto b78 = *ptr >> 6;

    switch(b78) {
      case 0b00: {

        if(masked_equal(*ptr, 0b000100, 0b000000)) {
          // Arithmetic: register/memory to register
 
          char *inst = LOGIC_OP_2OPR[(*ptr >> 3) & 0b111];
          if(!decode_2__dw__rg__rm(inst, &ptr, end))return false;

        } else if(masked_equal(*ptr, 0b000110, 0b000100)) {
          // Arithmetic: immediate to register/memory

          char *inst = LOGIC_OP_2OPR[(*ptr >> 3) & 0b111];
          if(!decode_2___w__im__ac(inst, &ptr, end))return false;

        } else if(masked_equal(*ptr, 0b100111, 0b000110)) {
          // PUSH: Segment registers

          auto SEG_REG = (*ptr >> 3) & 0b11;
          read_u8(&ptr);
          auto seg_reg = SR_REG[SEG_REG];
          printf("PUSH %s\n", seg_reg);

        } else if(masked_equal(*ptr, 0b100111, 0b000111)) {
          // POP: Segment registers

          auto sr = (*ptr >> 3) & 0b11;
          read_u8(&ptr);
          auto seg_reg = SR_REG[sr];
          printf("POP %s\n", seg_reg);

        } else if(masked_equal(*ptr, 0b100111, 0b100110)) {
          // Segment override
          // TODO:

          auto sr = (*ptr >> 3) & 0b11;
          auto seg_reg = SR_REG[sr];
          return false;

        } else if(masked_equal(*ptr, 0b100111, 0b100111)) {
          char *inst[] = {"DAA", "DAS", "AAA", "AAS"};

          read_u8(&ptr);
          printf("%s\n", inst[(*ptr >> 3) & 0b11]);

        } else return false;

      } break;
      case 0b01: {

        if(masked_equal(*ptr, 0b111000, 0b000000)) {

          if(!decode_1__rg("INC", &ptr, end))return false;

        } else if(masked_equal(*ptr, 0b111000, 0b001000)) {

          if(!decode_1__rg("DEC", &ptr, end))return false;

        } else if(masked_equal(*ptr, 0b111000, 0b010000)) {

          if(!decode_1__rg("PUSH", &ptr, end))return false;

        } else if(masked_equal(*ptr, 0b111000, 0b010000)) {

          if(!decode_1__rg("POP", &ptr, end))return false;

        } else if(masked_equal(*ptr, 0b110000, 0b110000)) {
          auto JCC = (*ptr >> 0) & 0b1111;
          read_u8(&ptr);

          auto jcc = JCC_INST[JCC];

          if(!has_remaining(ptr, end, 1))return false;
          s8 inc = read_s8(&ptr);

          printf("%s %d\n", jcc, inc);

        } else return false;

      } break;
      case 0b10: {

        if(masked_equal(*ptr, 0b111100, 0b000000)) {

          if(!has_remaining(ptr, end, 2))return false;

          char *inst = LOGIC_OP_2OPR[(*ptr >> 3) & 0b111];
          if(!decode_2__sw__im__rm(inst, &ptr, end))return false;

        } else if(masked_equal(*ptr, 0b111110, 0b000100)) {

          if(!decode_2___w__rg__rm("TEST", &ptr, end))return false;

        } else if(masked_equal(*ptr, 0b111110, 0b000110)) {

          if(!decode_2___w__rg__rm("XCHG", &ptr, end))return false;

        } else if(masked_equal(*ptr, 0b111100, 0b001000)) {

          if(!decode_2__dw__rg__rm("MOV", &ptr, end))return false;

        } else if(masked_equal(*ptr, 0b111111, 0b001101)) {

          read_u8(&ptr);
          if(!decode_2__ii__rg__rm("LEA", &ptr, end, 1, 1))return false;

        } else if(masked_equal(*ptr, 0b111101, 0b001100)) {
          auto D = (*ptr >> 1) & 0b1;
          read_u8(&ptr);
          
          if(!has_remaining(ptr, end, 1))return false;

          auto sb = decode_2nd_byte(*ptr);
          
          read_u8(&ptr);

          auto seg_reg = SR_REG[sb.SR];

          char decoded_rm[256] = "";
          if(!decode_rm(&ptr, end, 1, sb.MOD, sb.RM, decoded_rm))return false;

          auto src = D ? decoded_rm : seg_reg;
          auto dest = D ? seg_reg : decoded_rm;

          printf("MOV %s, %s\n", dest, src);

        } else if(masked_equal(*ptr, 0b111111, 0b001111)) {
          if(!has_remaining(ptr, end, 2))return false;

          if((ptr[1] & 0b00111000) == 0b00000000) {
            
            if(!decode_1__i__rm("POP", &ptr, end, 1))return false;

          } else return false;

        } else if(masked_equal(*ptr, 0b111000, 0b010000)) {
          auto REG = *ptr & 0b111;
          read_u8(&ptr);

          auto reg = BYTE_WORD_REG[1][REG];

          printf("XCHG AX, %s\n", reg);

        } else if(masked_equal(*ptr, 0b111111, 0b011000)) {

          read_u8(&ptr);
          printf("CBW\n");

        } else if(masked_equal(*ptr, 0b111111, 0b011001)) {

          read_u8(&ptr);
          printf("CWD\n");

        } else if(masked_equal(*ptr, 0b111111, 0b011010)) {
          read_u8(&ptr);
          
          if(!has_remaining(ptr, end, 4))return false;

          auto IP = read_u16(&ptr);
          auto CS = read_u16(&ptr);

          printf("CALL 0x%x:0x%x\n", CS, IP);

        } else if(masked_equal(*ptr, 0b111111, 0b011011)) {

          read_u8(&ptr);
          printf("WAIT\n");

        } else if(masked_equal(*ptr, 0b111100, 0b011100)) {
          auto idx = *ptr & 0b11;
          read_u8(&ptr);

          char *inst[] = {"PUSHF", "POPF", "SAHF", "LAHF"};
          printf("%s\n", inst[idx]);

        } else if(masked_equal(*ptr, 0b111100, 0b100000)) {

          auto D = (*ptr >> 1) & 0b1;
          auto W = (*ptr >> 0) & 0b1;
          read_u8(&ptr);

          if(!has_remaining(ptr, end, 2))return false;
          u16 address = read_u16(&ptr);

          auto reg = W ? "AX" : "AL";

          char mem[256] = "";

          auto w_str = W_TO_OPR_SIZE[W];
          sprintf(mem, "%s [0x%04X]", w_str, address);

          auto src = D ? reg : mem;
          auto dest = D ? mem : reg;
          
          printf("MOV %s, %s\n", dest, src);

        } else if(masked_equal(*ptr, 0b111100, 0b100100)) {

          auto INST = (*ptr >> 1) & 0b10;
          auto W = (*ptr >> 0) & 0b0;
          read_u8(&ptr);

          auto inst = INST == 0? "MOVS" : "CMPS";
          auto size = W_TO_OPR_SIZE[W];

          printf("%s %s [DI], %s [SI]\n", inst, size, size);

        } else if(masked_equal(*ptr, 0b111110, 0b101000)) {

          auto W = (*ptr >> 0) & 0b1;
          read_u8(&ptr);

          if(!has_remaining(ptr, end, W ? 2 : 1))return false;
          u16 imm = W ? read_u16(&ptr) : read_u8(&ptr);

          char *dest = W ? "AX" : "AL";

          printf("TEST %s, 0x%0*X\n", dest, W ? 4 : 2, imm);

        } else if(masked_equal(*ptr, 0b111110, 0b101010)) {
          auto W = (*ptr >> 0) & 0b0;
          read_u8(&ptr);

          auto size = W_TO_OPR_SIZE[W];

          printf("STOS %s [DI], %s [SI]\n", size, size);
        } else if(masked_equal(*ptr, 0b111110, 0b101100)) {
          auto W = (*ptr >> 0) & 0b0;
          read_u8(&ptr);

          auto size = W_TO_OPR_SIZE[W];

          printf("LODS %s [DI], %s [SI]\n", size, size);
        } else if(masked_equal(*ptr, 0b111110, 0b101110)) {
          auto W = (*ptr >> 0) & 0b0;
          read_u8(&ptr);

          auto size = W_TO_OPR_SIZE[W];

          printf("SCAS %s [DI], %s [SI]\n", size, size);

        } else if(masked_equal(*ptr, 0b110000, 0b110000)) {

          auto W = shift_and(*ptr, 3, 0b1);
          auto REG = shift_and(*ptr, 0, 0b111);
          read_u8(&ptr);

          auto dest = BYTE_WORD_REG[W][REG];

          if(!has_remaining(ptr, end, W ? 2 : 1))return false;
          u16 src = W ? read_u16(&ptr) : read_u8(&ptr);

          printf("MOV %s, 0x%0*X\n", dest, W ? 4 : 2, src);

        } else return false;

      } break;
      case 0b11: {

        if(masked_equal(*ptr, 0b110000, 0b000011)) {

          read_u8(&ptr);
          printf("RET\n");
        
        } else if(masked_equal(*ptr, 0b110000, 0b001011)) {

          read_u8(&ptr);
          printf("RETF\n");

        } else if((*ptr & 0b111110) == 000100) {

          auto inst = (*ptr >> 0) & 0b1 ? "LDS" : "LES";
          read_u8(&ptr);
          if(!decode_2__ii__rg__rm(inst, &ptr, end, 1, 2))return false;

        } else if(masked_equal(*ptr, 0b111110, 0b000110)) {

          if(!decode_2___w__im__rm("MOV", &ptr, end))return false;

        } else if(masked_equal(*ptr, 0b111111, 0b001010)) {

          read_u8(&ptr);
          u16 data = read_u16(&ptr);
          printf("RETF 0x%04X\n", data);

        } else if(masked_equal(*ptr, 0b111111, 0b000010)) {

          read_u8(&ptr);
          u16 data = read_u16(&ptr);
          printf("RET 0x%04X\n", data);
        
        } else if(masked_equal(*ptr, 0b111111, 0b001100)) {
          read_u8(&ptr);
          printf("INT 3\n");
        
        } else if(masked_equal(*ptr, 0b111111, 0b001101)) {

          read_u8(&ptr);
          u8 type = read_u8(&ptr);
          printf("INT %u\n", type);
        
        } else if(masked_equal(*ptr, 0b111111, 0b001110)) {

          read_u8(&ptr);
          printf("INTO\n");
        
        } else if(masked_equal(*ptr, 0b111111, 0b001111)) {

          read_u8(&ptr);
          printf("IRET\n");
        
        } else if(masked_equal(*ptr, 0b111100, 0b010000)) {

          u8 V = (*ptr >> 1) & 0b1;
          u8 W = (*ptr >> 0) & 0b1;
          read_u8(&ptr);

          if(!has_remaining(ptr, end, 1))return false;

          auto sb = decode_2nd_byte(*ptr);
          read_u8(&ptr);

          char decoded_rm[256] = "";
          if(!decode_rm(&ptr, end, W, sb.MOD, sb.RM, decoded_rm))return false;

          char *shift = V ? "CL" : "1";

          char *inst[] = {"ROL", "ROR", "RCL", "RCR", "SHL", "SHR", "", "SAR"};

          printf("%s %s, %s\n", inst[sb.OP], decoded_rm, shift);

        } else if(masked_equal(*ptr, 0b111111, 0b010100)) {
          
          read_u8(&ptr);
          printf("AAM\n");

        } else if(masked_equal(*ptr, 0b111111, 0b010101)) {

          read_u8(&ptr);
          printf("AAD\n");

        } else if(masked_equal(*ptr, 0b111111, 0b010111)) {

          read_u8(&ptr);
          printf("XLAT\n");

        } else if(masked_equal(*ptr, 0b111000, 0b011000)) {

          auto XXX = (*ptr >> 0) & 0b111;
          read_u8(&ptr);

          if(!has_remaining(ptr, end, 1))return false;

          auto second = decode_2nd_byte(*ptr);
          read_u8(&ptr);

          char decoded_rm[256];
          decode_rm(&ptr, end, 0, second.MOD, second.RM, decoded_rm);

          auto opcode = (second.YYY << 3) | XXX;
          printf("ESC 0x%X, %s\n", opcode, decoded_rm);

        } else if(masked_equal(*ptr, 0b111100, 0b100000)) {
          auto INST = (*ptr >> 0) & 0b11;
          read_u8(&ptr);

          if(!has_remaining(ptr, end, 1))return false;

          auto offset = read_s8(&ptr);
          char *inst[] = {"LOOPNZ", "LOOPZ", "LOOP", "JCXZ"};
          printf("%s %d\n", inst[INST], offset);

        } else if(masked_equal(*ptr, 0b111100, 0b100100)) {
          auto W = (*ptr >> 0) & 0b1;
          auto D = (*ptr >> 1) & 0b1;
          read_u8(&ptr);

          if(!has_remaining(ptr, end, 1))return false;
          auto port = read_u8(&ptr);
          char port_str[256];
          sprintf(port_str, "%d", port);

          auto data_reg = W ? "AX":"AL";

          auto inst = D ? "OUT" : "IN";

          auto a = !D ? data_reg : port_str;
          auto b = !D ? port_str : data_reg;

          printf("%s %s, %s\n", inst, a, b);
        } else if(masked_equal(*ptr, 0b111100, 0b101100)) {
          auto W = (*ptr >> 0) & 0b1;
          auto D = (*ptr >> 1) & 0b1;
          read_u8(&ptr);

          auto inst = D ? "OUT" : "IN";

          auto data_reg = W ? "AX":"AL";

          auto a = !D ? data_reg : "DX";
          auto b = !D ? "DX" : data_reg;

          printf("%s %s, %s\n", inst, a, b);

        } else if(masked_equal(*ptr, 0b111111, 0b101000)) {
          read_u8(&ptr);
          if(!has_remaining(ptr, end, 1))return false;

          auto ip_inc = read_s16(&ptr);

          printf("CALL %d\n", ip_inc);

        } else if(masked_equal(*ptr, 0b111111, 0b101000)) {
          read_u8(&ptr);
          if(!has_remaining(ptr, end, 1))return false;

          auto ip_inc = read_s16(&ptr);

          printf("JMP %d\n", ip_inc);

        } else if(masked_equal(*ptr, 0b111111, 0b110000)) {
          read_u8(&ptr);
          printf("LOCK ");

        } else if(masked_equal(*ptr, 0b111110, 0b110010)) {
          auto Z = (*ptr >> 0) & 0b1;
          read_u8(&ptr);

          printf("REP%s ", Z ? "" : "NZ");

        } else if(masked_equal(*ptr, 0b111111, 0b110100)) {
          read_u8(&ptr);
          printf("HLT\n");

        } else if(masked_equal(*ptr, 0b111111, 0b110101)) {
          read_u8(&ptr);
          printf("CMC\n");

        } else if(masked_equal(*ptr, 0b111110, 0b110110)) {
          auto W = (*ptr >> 0) & 0b1;
          read_u8(&ptr);

          if(!has_remaining(ptr, end, 1))return false;

          auto IDX = (*ptr >> 3) & 0b111;

          char *inst[] = {
            "TEST", "", "NOT", "NEG", "MUL", "IMUL", "DIV", "IDIV"
          };

          if(!decode_1__i__rm(inst[IDX], &ptr, end, W))return false;

        } else if(masked_equal(*ptr, 0b111100, 0b111000)) {
          auto IDX = (*ptr >> 0) & 0b11;
          read_u8(&ptr);

          char *inst[] = {"CLC", "STC", "CLI", "STI"};

          printf("%s\n", inst[IDX]);

        } else if(masked_equal(*ptr, 0b111110, 0b111100)) {
          auto IDX = (*ptr >> 0) & 0b1;
          read_u8(&ptr);

          char *inst = IDX == 0 ? "CLD" : "STD";

          printf("%s\n", inst);

        } else if(masked_equal(*ptr, 0b111111, 0b111110)) {
          read_u8(&ptr);

          if(!has_remaining(ptr, end, 1))return false;

          auto IDX = (*ptr >> 3) & 0b111;

          char *inst[] = {
            "INC", "DEC"
          };

          if(IDX == 0b000 || IDX == 0b001) {

            if(!decode_1__i__rm(inst[IDX], &ptr, end, 0))return false;

          } else return false;

        } else if(masked_equal(*ptr, 0b111111, 0b111111)) {
          read_u8(&ptr);

          if(!has_remaining(ptr, end, 1))return false;

          auto IDX = (*ptr >> 3) & 0b111;

          if(IDX == 0b000) {

            if(!decode_1__i__rm("INC", &ptr, end, 1))return false;

          } else if(IDX == 0b001) {

            if(!decode_1__i__rm("DEC", &ptr, end, 1))return false;

          } else if(IDX == 0b010) {

            if(!decode_1__i__rm("CALL", &ptr, end, 1))return false;

          } else if(IDX == 0b011) {

            if(!decode_1__i__rm("CALL", &ptr, end, 1))return false;

          } else if(IDX == 0b100) {

            if(!decode_1__i__rm("JMP", &ptr, end, 1))return false;

          } else if(IDX == 0b101) {

            if(!decode_1__i__rm("JMP", &ptr, end, 1))return false;

          } else if(IDX == 0b110) {

            if(!decode_1__i__rm("PUSH", &ptr, end, 1))return false;

          } else return false;

        } else return false;
      } break;
      
      default: return false;
    }
  }

  return true;
}

int main(int arg_count, char **args){

  if(arg_count == 2) {

    auto path = args[1];
    auto read_res = file_read_content_to_memory(path);
    if(read_res.ok) {
      printf("bits 16\n");
      if(!parse_8086(read_res.data, read_res.size)) {
        fprintf(stderr, "Error: decoding failed\n");
        return 1;
      }
    } else {
      fprintf(stderr, "Error: could not open '%s'\n", path);
      return 1;
    }

  } else {
    fprintf(stderr, "Error: expecting one argument\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  Usage: main.exe <bin-file>\n");
    fprintf(stderr, "\n");
    return 1;
  }

  return 0;
}
