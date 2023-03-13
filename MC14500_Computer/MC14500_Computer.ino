/*
  MC14500エミュレータ
  赤外線リモコンから、プログラムの入力、ステップ実行などが可能。TK-80などのワンボードマイコンのイメージ

  動作例として、MC14500のデータブックに記載の信号機に似せた物を実装

  // Arduino leonardで動作確認済み
  // UNOなどを使う場合、ホストPCとシリアル通信（プログラム書込みを含む）している時はP0とP1をスイッチオフの開放状態にすること
  
  P2:IN0
  P3:IN1
  P4:IN2
  P5:IN3
  P6:OUT0
  P7:OUT1
  P8:未使用
  P9:未使用
  P10:未使用
  P11:未使用
  P12:赤外線リモコンセンサ入力。赤外線リモコン受信モジュールＧＰ１ＵＸＣ４１ＱＳなどを接続すること
  P13:圧電ブザー出力。無くても良いが、リモコン操作が分かりやすい。
  A0:LCD RS   16桁2行の液晶モジュールなどを接続すること。R/WはGND固定でOK。
  A1:LCD E
  A2:LCD D4
  A3:LCD D5
  A4:LCD D6
  A5:LCD D7

  液晶表示内容
    03 012 NOP0 12 -    アドレス データ 命令 オペランド 状態(-:停止中、*:実行中)
    RIO 00 0000 0000    RIO表示(R:リザルトレジスタ1、I:インプットイネーブル1、アウトプットイネーブル1) テンポラリレジスタの内容

*/

#include "MC14500.h"
#include "Lcd.h"
#include "SharpIr.h"
//#include "JankenROM.h"    // じゃんけんROMの読込
#include "RouletteROM.h"  // ルーレット
#include "Timer.h"

// LCD16x2 PINs R/WピンはGND固定
#define LCD_D7    A5
#define LCD_D6    A4
#define LCD_D5    A3
#define LCD_D4    A2
#define LCD_E     A1
#define LCD_RS    A0
Lcd lcd(LCD_16x2, LCD_D7, LCD_D6, LCD_D5, LCD_D4, LCD_E, LCD_RS);  // D7~D3, RW, E, RS

#define BEEP      13
#define BEEP_GND  12
#define IR_VCC    11
#define IR_GND    10
#define IR_RX     9

tRom rom[256];

class MC14500B : public MC14500{
  public:
  MC14500B(const tRom *rom, byte startInput, byte endInput, byte startOutput, byte endOutput) :
      MC14500(rom, startInput, endInput, startOutput, endOutput){
        t0.init(2000, 2500, 2000);
      }

  void nopf(byte operand) override {  // NOPFをオーバーライド
    switch(operand){
      case 0:
        delay(5); break;
      case 1:
        delay(100); break;
      case 2:
        delay(300); break;
      case 3:
        tone(BEEP, 1000, 50); break;
    }
  }
  bool bitInput(byte bitNo) override {
    bool result = MC14500::bitInput(bitNo);
    switch(bitNo){
      case T0ONA:
        result = t0.isTimerOnA();  break;
      case T0ONB:
        result = t0.isTimerOnB();  break;
    }
    bitWrite(ram[bitNo >> 3], bitNo & 0b111, result);
    return result;
  };
  void bitOutput(byte bitNo, bool bit) override {
    MC14500::bitOutput(bitNo, bit); // 基本クラスのbitOutputを呼出
    // 基本クラスのbitOutputに以下のタイマ機能を追加する
    if(bit){
      switch(bitNo){
        case T0ON:      // 1秒のタイマ起動
          t0.setTimer();  break;
      }
    }
  }
  private:
  Timer t0;
};
MC14500B cpu(rom, ST_INPUT, END_INPUT, ST_OUTPUT, END_OUTPUT);

Sharp0000 ir(IR_RX);

void setup() {
  pinMode(IR_GND, OUTPUT);
  digitalWrite(IR_GND, LOW);
  pinMode(IR_VCC, OUTPUT);
  digitalWrite(IR_VCC, HIGH);  
  pinMode(IR_RX, INPUT_PULLUP);
  pinMode(BEEP_GND, OUTPUT);
  digitalWrite(BEEP_GND, LOW);

  lcd.init();   // 液晶ユニットの初期化
  ir.init();    // 赤外線リモコンの初期化

  Serial.begin(115200);
  if(!Serial) delay(2000);

  for(int i = 0; i < 256; ++i){
    if(i < romSize){
      rom[i].opcode  = pgm_read_byte(romData + (i << 1) + 0);
      rom[i].operand = pgm_read_byte(romData + (i << 1) + 1);
    }else{
      rom[i].opcode = rom[i].operand = 0;
    }
  }
  cpu.reset();
}

void loop() {
  word irCommand = 0;
  static word pc = 0;
  static bool execFlg = false;

  static word dispData = ((word)rom[pc].opcode << 8) | rom[pc].operand;

  if(ir.read()){  // リモコン受信あり
    irCommand = ir.getCommand();  // 受信コマンド
    if(ir.isRepeat() && !(irCommand == IR_CursorUp || irCommand == IR_CursorDown || irCommand == IR_ChanelGuide)){ // 左記のコマンドのみリピート機能有りとする
      irCommand = 0x0000; // リピート無効
    }else{
      tone(BEEP, 2000, 20);
    }
  }

  if(execFlg){  // プログラム実行中
    if(IR_Power == irCommand){
      execFlg = false;
    }else{
      cpu.setPC(pc);
      for(word t = micros(); (word)micros() - t < 2500 && digitalRead(IR_RX) == HIGH; ){  // 最大2.5ms間実行。キャリア検知で抜ける
        cpu.exec(); // 1命令実行
      }
      pc = cpu.getPC();
      dispData = ((word)rom[pc].opcode << 8) | rom[pc].operand;
    }
  }else{      // キー入力処理
    switch(irCommand){
      case IR_VolumeDown:                           // 0  NOP0
        dispData = (dispData << 4) | 0x0; break;
      case IR_VolumeUp:                             // 1  LO
        dispData = (dispData << 4) | 0x1; break;
      case IR_DisplayShow:                          // 2  LDC
        dispData = (dispData << 4) | 0x2; break;
      case IR_ChanelUp:                             // 3  AND
        dispData = (dispData << 4) | 0x3; break;
      case IR_10:                                   // 4  ANDC
        dispData = (dispData << 4) | 0x4; break;
      case IR_11:                                   // 5  OR
        dispData = (dispData << 4) | 0x5; break;
      case IR_12:                                   // 6  ORC
        dispData = (dispData << 4) | 0x6; break;
      case IR_7:                                    // 7  XNOR
        dispData = (dispData << 4) | 0x7; break;
      case IR_8:                                    // 8  STO
        dispData = (dispData << 4) | 0x8; break;
      case IR_9:                                    // 9  STOC
        dispData = (dispData << 4) | 0x9; break;
      case IR_4:                                    // A  IEN
        dispData = (dispData << 4) | 0xA; break;
      case IR_5:                                    // B  OEN
        dispData = (dispData << 4) | 0xB; break;
      case IR_6:                                    // C  JMP
        dispData = (dispData << 4) | 0xC; break;
      case IR_1:                                    // D  RTN
        dispData = (dispData << 4) | 0xD; break;
      case IR_2:                                    // E  SKZ
        dispData = (dispData << 4) | 0xE; break;
      case IR_3:                                    // F  NOPF
        dispData = (dispData << 4) | 0xF; break;
      case IR_ChanelDown:                           // write increment
        rom[pc].opcode = dispData >> 8;       
        rom[pc].operand = dispData & 0xFF;
        pc = (pc + 1) & 0xFF;
        dispData = ((word)rom[pc].opcode << 8) | rom[pc].operand;
        break;
      case IR_CursorUp:                             // ++pc
        pc = (pc + 1) & 0xFF;
        dispData = ((word)rom[pc].opcode << 8) | rom[pc].operand;
        break;
      case IR_CursorDown:                           // --pc
        pc = (pc - 1) & 0xFF;
        dispData = ((word)rom[pc].opcode << 8) | rom[pc].operand;
        break;
      case IR_SoundMute:                            // address set
        pc = dispData & 0xFF;
        dispData = ((word)rom[pc].opcode << 8) | rom[pc].operand;
        break;
      case IR_Power:                                // exec
        execFlg = true;
        break;
      case IR_ChanelGuide:                          // step Exec
        cpu.setPC(pc);
        pc = cpu.exec();
        dispData = ((word)rom[pc].opcode << 8) | rom[pc].operand;
        break;
      case IR_Blue:                                 // 1ワード挿入
        for(int i = 254; i >= (int)pc; --i){
          rom[i + 1] = rom[i];
        }
        rom[pc].opcode = dispData >> 8;
        rom[pc].operand = dispData & 0xFF;
        break;
      case IR_Red:                                  // 1ワード削除
        for(word i = pc; i < 255; ++i){
          rom[i] = rom[i + 1];        
        }
        dispData = ((word)rom[pc].opcode << 8) | rom[pc].operand;
        break;
    }

    // 入力ポートの内容スキャン。無くても動作には問題ないがプログラム停止中の入力ポート状態が反映されるのでDBに便利
    for(byte i = ST_INPUT; i <= END_INPUT; ++i){
      cpu.bitInput(i);
    }
  }
  dispData &= 0x0FFF;

  // 液晶表示
  const char *opcodeName[] = {
    "NOP0", "LD", "LDC", "AND", "ANDC", "OR", "ORC", "XNOR", 
    "STO", "STOC", "IEN", "OEN", "JMP", "RTN", "SKZ", "NOPF" };
  tRegister r;
  cpu.getRegister(r);
  byte *ram = cpu.getRam();

  lcd.clear();
  lcd.printf("%02X %03X %-4s %02X %c", pc, dispData, opcodeName[dispData >> 8], dispData & 0xFF, execFlg? '*' : '-');
  lcd.printf("%c%c%c ", r.rr? 'R' : '_', r.ien? 'I' : '_', r.oen? 'O' : '_');
  lcd.printf("%02X %02X%02X %02X%02X", ram[4], ram[3], ram[2], ram[1], ram[0]);
  
  lcd.update();
}

