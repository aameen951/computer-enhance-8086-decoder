#include <stdio.h>
#include "my_std.h"
#include "file_reader.h"

char *decode_REG(u8 W, u8 REG){
  switch(REG) {
    case 0b000: return W ? "AX" : "AL";
    case 0b001: return W ? "CX" : "CL";
    case 0b010: return W ? "DX" : "DL";
    case 0b011: return W ? "BX" : "BL";
    case 0b100: return W ? "SP" : "AH";
    case 0b101: return W ? "BP" : "CH";
    case 0b110: return W ? "SI" : "DH";
    case 0b111: return W ? "DI" : "BH";
  }
  return "<ERR>";
}
char *decode_SR(u8 SR){
  switch(SR) {
    case 0b00: return "ES";
    case 0b01: return "CS";
    case 0b10: return "SS";
    case 0b11: return "DS";
  }
  return "<ERR>";
}
char *decode_rm_reg_displacement(u8 RM){
  switch(RM) {
    case 0b000: return "BX + SI";
    case 0b001: return "BX + DI";
    case 0b010: return "BP + SI";
    case 0b011: return "BP + DI";
    case 0b100: return "SI";
    case 0b101: return "DI";
    case 0b110: return "BP";
    case 0b111: return "BX";
  }
  return "<ERR>";
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

bool decode_MOD(u8 **ptr_p, u8 *end, u8 W, u8 MOD, u8 RM, char *decoded_mod){
  auto ptr = *ptr_p;
  auto w_str = W == 0 ? "BYTE " : W == 1 ? "WORD " : "";

  if(MOD == 0b00) {
    if(RM == 0b110) {
      if(!has_remaining(ptr, end, 2))return false;
      u16 DIRECT_ADDRESS = read_u16(&ptr);
      sprintf(decoded_mod, "%s[0x%04X]", w_str, DIRECT_ADDRESS); 
    } else {
      sprintf(decoded_mod, "%s[%s]", w_str, decode_rm_reg_displacement(RM));
    }
  } else if(MOD == 0b01) {
    if(!has_remaining(ptr, end, 1))return false;
    auto DISPLACEMENT = read_s8(&ptr);
    sprintf(decoded_mod, "%s[%s + %d]", w_str, decode_rm_reg_displacement(RM), DISPLACEMENT);
  } else if(MOD == 0b10) {
    if(!has_remaining(ptr, end, 2))return false;
    auto DISPLACEMENT = read_s16(&ptr);
    sprintf(decoded_mod, "%s[%s + %d]", w_str, decode_rm_reg_displacement(RM), DISPLACEMENT);
  } else if(MOD == 0b11) {
    sprintf(decoded_mod, "%s", decode_REG(W, RM));
  }
  *ptr_p = ptr;
  return true;
}

bool masked_equal(u8 byte, u8 mask, u8 value){
  return (byte & mask) == value;
}

bool decode_2__ii__rg__rm(const char *inst, u8 **ptr_p, u8 *end, u8 D, u8 W) {
  auto ptr = *ptr_p;

  if(!has_remaining(ptr, end, 1))return false;

  auto MOD = (*ptr >> 6) & 0b11;
  auto REG = (*ptr >> 3) & 0b111;
  auto RM = (*ptr >> 0) & 0b111;
  read_u8(&ptr);

  auto w_str = W ? "WORD" : "BYTE";

  auto reg_name = decode_REG(W, REG);

  char decoded_mod[256] = "";
  if(!decode_MOD(&ptr, end, W, MOD, RM, decoded_mod))return false;

  auto src = !D ? reg_name : decoded_mod;
  auto dest = !D ? decoded_mod : reg_name;

  printf("%s %s, %s\n", inst, dest, src);

  *ptr_p = ptr;
  return true;
}
bool decode_2__ii__im__rm(char *inst, u8 **ptr_p, u8 *end, u8 S, u8 W) {
  auto ptr = *ptr_p;

  if(!has_remaining(ptr, end, 1))return false;

  auto MOD = (*ptr >> 6) & 0b11;
  auto RM = (*ptr >> 0) & 0b111;
  read_u8(&ptr);

  char decoded_mod[256] = "";
  if(!decode_MOD(&ptr, end, W, MOD, RM, decoded_mod))return false;

  auto dest = decoded_mod;

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

  auto dest = decode_REG(1, REG);

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

  char decoded_mod[256] = "";
  if(!decode_MOD(&ptr, end, W, MOD, RM, decoded_mod))return false;

  printf("%s %s\n", inst, decoded_mod);

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

    if((*ptr & 0b11111100) == 0b10001000) {
      if(!decode_2__dw__rg__rm("MOV", &ptr, end))return false;
    } else if((*ptr & 0b11111110) == 0b11000110) {
      if(!decode_2___w__im__rm("MOV", &ptr, end))return false;
    } else if((*ptr & 0b11110000) == 0b10110000) {
      auto W = (*ptr >> 3) & 0b1;
      auto REG = (*ptr >> 0) & 0b111;
      read_u8(&ptr);

      auto dest = decode_REG(W, REG);

      if(!has_remaining(ptr, end, W ? 2 : 1))return false;
      u16 src = W ? read_u16(&ptr) : read_u8(&ptr);

      printf("MOV %s, 0x%0*X\n", dest, W ? 4 : 2, src);
    } else if((*ptr & 0b11111100) == 0b10100000) {
      auto D = (*ptr >> 1) & 0b1;
      auto W = (*ptr >> 0) & 0b1;
      read_u8(&ptr);

      if(!has_remaining(ptr, end, 2))return false;
      u16 address = read_u16(&ptr);

      auto reg = W ? "AX" : "AL";

      char mem[256] = "";

      auto w_str = W ? "WORD":"BYTE";
      sprintf(mem, "%s [0x%04X]", w_str, address);

      auto src = D ? reg : mem;
      auto dest = D ? mem : reg;
      
      printf("MOV %s, %s\n", dest, src);
    } else if((*ptr & 0b11111101) == 0b10001100) {
      auto D = (*ptr >> 1) & 0b1;
      read_u8(&ptr);
      
      if(!has_remaining(ptr, end, 1))return false;
      
      auto MOD = (*ptr >> 6) & 0b11;
      auto SR = (*ptr >> 3) & 0b11;
      auto RM = (*ptr >> 0) & 0b111;
      read_u8(&ptr);

      auto seg_reg = decode_SR(SR);

      char decoded_mod[256] = "";
      if(!decode_MOD(&ptr, end, 1, MOD, RM, decoded_mod))return false;

      auto src = D ? decoded_mod : seg_reg;
      auto dest = D ? seg_reg : decoded_mod;

      printf("MOV %s, %s\n", dest, src);
    } else if((*ptr & 0b11111111) == 0b11111111) {
      read_u8(&ptr);

      if(!has_remaining(ptr, end, 1))return false;

      if((*ptr & 0b00111000) == 0b00110000) {
        if(!decode_1__i__rm("PUSH", &ptr, end, 1))return false;
      } else return false;

    } else if((*ptr & 0b11111000) == 0b01010000) {
      if(!decode_1__rg("PUSH", &ptr, end))return false;
    } else if((*ptr & 0b11100111) == 0b00000110) {
      auto SEG_REG = (*ptr >> 3) & 0b11;
      read_u8(&ptr);

      auto seg_reg = decode_SR(SEG_REG);

      printf("PUSH %s\n", seg_reg);
    } else if((*ptr & 0b11111000) == 0b01011000) {
      auto REG = (*ptr >> 0) & 0b111;
      read_u8(&ptr);

      auto dest = decode_REG(1, REG);

      printf("POP %s\n", dest);
    } else if((*ptr & 0b11100111) == 0b00000111) {
      auto SEG_REG = (*ptr >> 3) & 0b11;
      read_u8(&ptr);

      auto seg_reg = decode_SR(SEG_REG);

      printf("POP %s\n", seg_reg);
    } else if((*ptr & 0b11111111) == 0b10001111) {
      read_u8(&ptr);

      if(!has_remaining(ptr, end, 1))return false;

      if((*ptr & 0b00111000) == 0b00000000) {
        if(!decode_1__i__rm("POP", &ptr, end, 1))return false;
      } else return false;

    } else if((*ptr & 0b11111110) == 0b10000110) {
      if(!decode_2___w__rg__rm("XCHG", &ptr, end))return false;
    } else if((*ptr & 0b11111000) == 0b10010000) {
      auto REG = (*ptr >> 0) & 0b111;
      read_u8(&ptr);

      auto b = decode_REG(1, REG);

      printf("XCHG AX, %s\n", b);

    } else if((*ptr & 0b11111100) == 0b00000000) {
      if(!decode_2__dw__rg__rm("ADD", &ptr, end))return false;
    } else if((*ptr & 0b11111100) == 0b00010000) {
      if(!decode_2__dw__rg__rm("ADC", &ptr, end))return false;
    } else if((*ptr & 0b11111110) == 0b00000100) {
      if(!decode_2___w__im__ac("ADD", &ptr, end))return false;
    } else if((*ptr & 0b11111110) == 0b00010100) {
      if(!decode_2___w__im__ac("ADC", &ptr, end))return false;
    } else if((*ptr & 0b11111100) == 0b10000000) {
      if(!has_remaining(ptr, end, 2))return false;

      if(masked_equal(ptr[1], 0b00111000, 0b00000000)) {
        if(!decode_2__sw__im__rm("ADD", &ptr, end))return false;
      } else if(masked_equal(ptr[1], 0b00111000, 0b00010000)) {
        if(!decode_2__sw__im__rm("ADC", &ptr, end))return false;
      } else return false;

    } else if((*ptr & 0b11111100) == 0b11100100) {
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
    } else if((*ptr & 0b11111100) == 0b11101100) {
      auto W = (*ptr >> 0) & 0b1;
      auto D = (*ptr >> 1) & 0b1;
      read_u8(&ptr);

      auto inst = D ? "OUT" : "IN";

      auto data_reg = W ? "AX":"AL";

      auto a = !D ? data_reg : "DX";
      auto b = !D ? "DX" : data_reg;

      printf("%s %s, %s\n", inst, a, b);
    } else if((*ptr & 0b11111111) == 0b11010111) {
      read_u8(&ptr);
      printf("XLAT\n");
    } else if((*ptr & 0b11111111) == 0b10001101) {
      read_u8(&ptr);
      if(!decode_2__ii__rg__rm("LEA", &ptr, end, 1, 1))return false;
    } else if((*ptr & 0b11111110) == 0b11000100) {
      auto inst = (*ptr >> 0) & 0b1 ? "LDS" : "LES";
      read_u8(&ptr);
      if(!decode_2__ii__rg__rm(inst, &ptr, end, 1, 2))return false;
    } else if((*ptr & 0b11111110) == 0b10011110) {
      auto inst = (*ptr >> 0) & 0b1 ? "LAHF" : "SAHF";
      read_u8(&ptr);
      printf("%s\n", inst);
    } else if((*ptr & 0b11111110) == 0b10011100) {
      auto inst = (*ptr >> 0) & 0b1 ? "POPF" : "PUSHF";
      read_u8(&ptr);
      printf("%s\n", inst);
    } else if(masked_equal(*ptr, 0b11111111, 0b00110111)) {
      read_u8(&ptr);
      printf("AAA\n");
    } else if(masked_equal(*ptr, 0b11111111, 0b00100111)) {
      read_u8(&ptr);
      printf("DAA\n");
    } else if(masked_equal(*ptr, 0b11111111, 0b00111111)) {
      read_u8(&ptr);
      printf("DAA\n");
    } else if(masked_equal(*ptr, 0b11111111, 0b00101111)) {
      read_u8(&ptr);
      printf("DAS\n");
    } else if(masked_equal(*ptr, 0b11111111, 0b10011000)) {
      read_u8(&ptr);
      printf("CBW\n");
    } else if(masked_equal(*ptr, 0b11111111, 0b10011001)) {
      read_u8(&ptr);
      printf("CWD\n");
    }
    else {
      fprintf(stderr, "Error: Unknown byte %02x\n", *ptr);
      return false;
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
