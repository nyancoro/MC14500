#include "MC14500.h"

MC14500::MC14500(const tRom *rom, byte startInput, byte endInput, byte startOutput, byte endOutput){

  this->rom = rom;
  this->startInput = startInput;
  this->endInput = endInput;
  this->startOutput = startOutput;
  this->endOutput = endOutput;
}

void MC14500::reset(){
  pc = 0;
  rr = false;
  ien = oen = false;
  for(byte i = 0; i < 32; ++i)
    ram[i] = 0;
  for(byte i = startInput; i <= endInput; ++i)
    pinMode(i, INPUT_PULLUP);
  for(byte i = startOutput; i <= endOutput; ++i)
    pinMode(i, OUTPUT);
}

void MC14500::setRegister(const tRegister &reg){
  pc = reg.pc;
  rr = reg.rr;
  oen = reg.oen;
  ien = reg.ien;
}

void MC14500::getRegister(tRegister &reg){
  reg.pc = pc;
  reg.rr = rr;
  reg.oen = oen;
  reg.ien = ien;
}

bool MC14500::bitInput(byte bitNo){
  if(bitNo >= startInput && bitNo <= endInput){
    bool temp = digitalRead(bitNo);
    bitWrite(ram[bitNo >> 3], bitNo & 0b111, temp);
    return temp;
  }else if(bitNo == 0xFF){
    return rr;
  }
  return bitRead(ram[bitNo >> 3], bitNo & 0b111);
}

void MC14500::bitOutput(byte bitNo, bool bit) {
  if(bitNo >= startOutput && bitNo <= endOutput){
    digitalWrite(bitNo, bit);
  }
  bitWrite(ram[bitNo >> 3], bitNo & 0b111, bit);
}

byte MC14500::exec(){
  byte opcode = rom[pc].opcode;
  byte operand = rom[pc].operand;

  ++pc;
  switch(opcode){
    case NOP0:
      nop0(operand); break;
    case LD:
      rr = ien? bitInput(operand) : false;   break;
    case LDC:
      rr = !(ien? bitInput(operand) : false);  break;
    case AND:
      rr &= ien? bitInput(operand) : false; break;
    case ANDC:
      rr &= !(ien? bitInput(operand) : false); break;
    case OR:
      rr |= ien? bitInput(operand) : false;  break;
    case ORC:
      rr |= !(ien? bitInput(operand) : false);  break;
    case XNOR:
      rr = !(rr ^ (ien? bitInput(operand) : false)); break;
    case STO:
      if(oen) bitOutput(operand, rr);  break;
    case STOC:
      if (oen) bitOutput(operand, !rr);  break;
    case IEN:
      ien = bitInput(operand);  break;
    case OEN:
      oen = bitInput(operand);  break;
    case JMP:
      pc = operand;   break;
    case RTN:
      ret(operand);   break;
    case SKZ:
      if(!rr) pc = pc - 1 + operand; break;
    case NOPF:
      nopf(operand); break;
  }
  return pc;
}
