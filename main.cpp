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
bool decode_MOD(u8 **ptr_p, u8 *end, u8 W, u8 MOD, u8 RM, char *decoded_mod){
  auto ptr = *ptr_p;
  auto w_str = W ? "WORD" : "BYTE";

  if(MOD == 0b00) {
    switch(RM) {
      case 0b000: sprintf(decoded_mod, "%s PTR:[BX + SI]", w_str); break;
      case 0b001: sprintf(decoded_mod, "%s PTR:[BX + DI]", w_str); break;
      case 0b010: sprintf(decoded_mod, "%s PTR:[BP + SI]", w_str); break;
      case 0b011: sprintf(decoded_mod, "%s PTR:[BP + DI]", w_str); break;
      case 0b100: sprintf(decoded_mod, "%s PTR:[SI]", w_str); break;
      case 0b101: sprintf(decoded_mod, "%s PTR:[DI]", w_str); break;
      case 0b110: {
        if(ptr+1 >= end)return false;
        auto DIRECT_ADDRESS = *(u16 *)ptr;
        ptr += 2;
        sprintf(decoded_mod, "%s PTR:[0x%04X]", w_str, DIRECT_ADDRESS); 
      } break;
      case 0b111: sprintf(decoded_mod, "%s PTR:[BX]", w_str); break;
    }
  } else if(MOD == 0b01 || MOD == 0b10) {
    if(ptr + MOD - 1 >= end)return false;
    u16 DISPLACEMENT = MOD == 1 ? *ptr : *(u16 *)ptr;
    ptr += MOD;
    switch(RM) {
      case 0b000: sprintf(decoded_mod, "%s PTR:[BX + SI + 0x%0*X]", w_str, 2*MOD, DISPLACEMENT); break;
      case 0b001: sprintf(decoded_mod, "%s PTR:[BX + DI + 0x%0*X]", w_str, 2*MOD, DISPLACEMENT); break;
      case 0b010: sprintf(decoded_mod, "%s PTR:[BP + SI + 0x%0*X]", w_str, 2*MOD, DISPLACEMENT); break;
      case 0b011: sprintf(decoded_mod, "%s PTR:[BP + DI + 0x%0*X]", w_str, 2*MOD, DISPLACEMENT); break;
      case 0b100: sprintf(decoded_mod, "%s PTR:[SI + 0x%0*X]", w_str, 2*MOD, DISPLACEMENT); break;
      case 0b101: sprintf(decoded_mod, "%s PTR:[DI + 0x%0*X]", w_str, 2*MOD, DISPLACEMENT); break;
      case 0b110: sprintf(decoded_mod, "%s PTR:[BP + 0x%0*X]", w_str, 2*MOD, DISPLACEMENT); break;
      case 0b111: sprintf(decoded_mod, "%s PTR:[BX + 0x%0*X]", w_str, 2*MOD, DISPLACEMENT); break;
    }
  } else if(MOD == 0b11) {
    sprintf(decoded_mod, "%s", decode_REG(W, RM));
  }
  *ptr_p = ptr;
  return true;
}

void parse_8086(u8 *data, um size){
  auto ptr = data;
  auto end = ptr + size;

  for(um i=0; i<size; i++) {
    printf("%02x ", data[i]);
  }
  printf("\n\n");

  while(ptr < end) {

    if((*ptr & 0b11111100) == 0b10001000) {
      auto D = (*ptr >> 1) & 0b1;
      auto W = (*ptr >> 0) & 0b1;
      ptr++;

      if(ptr >= end)return;

      auto MOD = (*ptr >> 6) & 0b11;
      auto REG = (*ptr >> 3) & 0b111;
      auto RM = (*ptr >> 0) & 0b111;
      ptr++;

      char *dest = "";
      char *src = "";

      auto w_str = W ? "WORD" : "BYTE";

      char *reg_name = decode_REG(W, REG);

      char decoded_mod[256] = "";
      if(!decode_MOD(&ptr, end, W, MOD, RM, decoded_mod))return;

      if(!D) {
        src = reg_name;
        dest = decoded_mod;
      } else {
        src = decoded_mod;
        dest = reg_name;
      }

      printf("  MOV %s, %s\n", dest, src);
    } else if((*ptr & 0b11111110) == 0b11000110) {
      auto W = (*ptr >> 0) & 0b1;
      ptr += 1;

      if(ptr >= end)return;

      auto MOD = (*ptr >> 6) & 0b11;
      auto RM = (*ptr >> 0) & 0b111;
      ptr += 1;

      char *dest = "";
      u16 src = 0;

      char decoded_mod[256] = "";
      if(!decode_MOD(&ptr, end, W, MOD, RM, decoded_mod))return;

      dest = decoded_mod;

      if(W) {
        if(ptr+1 >= end)return;
        src = *(u16 *)ptr;
        ptr += 2;
      } else {
        if(ptr >= end)return;
        src = *(u8 *)ptr;
        ptr += 1;
      }

      printf("  MOV %s, 0x%0*X\n", dest, W ? 4 : 2, src);

    } else if((*ptr & 0b11110000) == 0b10110000) {
      auto W = (*ptr >> 3) & 0b1;
      auto REG = (*ptr >> 0) & 0b111;
      ptr++;

      char *dest = decode_REG(W, REG);

      u16 src = 0;
      if(W) {
        if(ptr+1 >= end)return;
        src = *(u16 *)ptr;
        ptr += 2;
      } else {
        if(ptr >= end)return;
        src = *(u8 *)ptr;
        ptr += 1;
      }

      printf("  MOV %s, 0x%0*X\n", dest, W ? 4 : 2, src);

    } else if((*ptr & 0b11111100) == 0b10100000) {
      auto D = (*ptr >> 1) & 0b1;
      auto W = (*ptr >> 0) & 0b1;
      ptr++;

      if(ptr+1 >= end)return;
      u16 address = *(u16 *)ptr;
      ptr += 2;

      char *reg = W ? "AX" : "AL";

      auto w_str = W ? "WORD":"BYTE";
      char mem[256] = "";
      sprintf(mem, "%s PTR:[0x%04X]", w_str, address);

      char *src = "";
      char *dest = "";

      if(D) {
        src = reg;
        dest = mem;
      } else {
        src = mem;
        dest = reg;
      }
      
      printf("  MOV %s, %s\n", dest, src);

    } else if((*ptr & 0b11111101) == 0b10001100) {
      auto D = (*ptr >> 1) & 0b1;
      ptr++;
      
      if(ptr >= end)return;
      
      auto MOD = (*ptr >> 6) & 0b11;
      auto SR = (*ptr >> 3) & 0b11;
      auto RM = (*ptr >> 0) & 0b111;
      ptr++;

      auto seg_reg = decode_SR(SR);

      char decoded_mod[256] = "";
      if(!decode_MOD(&ptr, end, 1, MOD, RM, decoded_mod))return;

      char *src = "";
      char *dest = "";
      if(D) {
        src = decoded_mod;
        dest = seg_reg;
      } else {
        src = seg_reg;
        dest = decoded_mod;
      }

      printf("  MOV %s, %s\n", dest, src);
    } else {
      printf("Error: Unknown byte %02x\n", *ptr);
      return ;
    }

  }
}

int main(int arg_count, char **args){

  if(arg_count == 2) {

    auto path = args[1];
    auto read_res = file_read_content_to_memory(path);
    if(read_res.ok) {
      parse_8086(read_res.data, read_res.size);
    } else {
      printf("Error: could not open '%s'\n", path);
    }

  } else {
    printf("Error: expecting one argument\n");
    printf("\n");
    printf("  Usage: main.exe <bin-file>\n");
    printf("\n");
    return 1;
  }

  return 0;
}
