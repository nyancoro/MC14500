#pragma once
#include <Arduino.h>

struct tRom{
  unsigned opcode:4;
  unsigned operand:8;
};

struct tRegister{
  byte pc;
  bool rr;
  bool ien;
  bool oen;
};

enum {
  NOP0, LD, LDC, AND, ANDC, OR, ORC, XNOR, STO, STOC, IEN, OEN, JMP, RTN, SKZ, NOPF,
};

class MC14500{
  public:
  MC14500(const tRom *rom, byte startInput, byte endInput, byte startOutput, byte endOutput);
  void reset();
  byte exec();
  void setRegister(const tRegister &reg);       // 全レジスタの取得
  void getRegister(tRegister &reg);             // 全レジスタの設定
  virtual bool bitInput(byte bitNo);            // bitNoに対応した入出力ポートまたはテンポラリRAMの取得
  virtual void bitOutput(byte bitNo, bool bit); // bitNoに対応した入出力ポートまたはテンポラリRAMへbit値を設定
  byte getPC(){ return pc; }                    // PCを取得
  void setPC(byte pc){ this->pc = pc; }         // PCを設定
  byte *getRam(){ return ram; }                 // テンポラリRAMへのポインタ取得
  virtual void nop0(byte operand){ (void)operand; };  // ※未使用変数の警告抑止のためvoidキャストする
  virtual void ret(byte operand){ (void)operand; };
  virtual void nopf(byte operand){ (void)operand; };

  protected:
  byte pc;          // pc
  bool rr;          // リザルトレジスタ
  bool ien;         // true:外部からの入力有効、 false:外部からの入力は'0'とする
  bool oen;         // true:STO,STOC命令で外部出力を行う、false:STO、STOC命令を実行しない。
  const tRom *rom;  //
  byte ram[32];     // テンポラリRAM(256bit)
  byte startInput;  // 入力ポートの開始番号
  byte endInput;    // 入力ポートの終了番号
  byte startOutput; // 出力ポートの開始番号
  byte endOutput;   // 出力ポートの終了番号
};

