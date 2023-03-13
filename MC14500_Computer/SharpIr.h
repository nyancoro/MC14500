#pragma once
#include <Arduino.h>

/*
  TVリモコンはオーム電機製 AV-R570Nを使用
  メーカコード設定はシャープ0000に対応
  フレームの連続受信をリピートとして判定。※リピート専用コマンドには非対応。

  他のTV用リモコンに変更する時は、ソニー以外の日系メーカーを選ぶとプログラムの修正が少ない。
  シャープ、パナソニック、東芝？などが良い。

*/

// 受信コマンド
#define IR_Power		    0x16D1  // 電源
#define IR_SoundSelect	0x1831  // 音声切替
#define IR_InputSelect	0x1381  // 入力切替
#define IR_DigitalTV		0x8982  // 地デジ
#define IR_BS		        0x8AB2  // BS
#define IR_CS		        0x8BA2  // CS
#define IR_1		        0x4E32  // CH1
#define IR_2	          0x4F22
#define IR_3		        0x50C2
#define IR_4		        0x51D2
#define IR_5		        0x52E2
#define IR_6	          0x53F2
#define IR_7		        0x5482
#define IR_8		        0x5592
#define IR_9		        0x56A2
#define IR_10		        0x57B2
#define IR_11		        0x5842
#define IR_12		        0x5952  // CH12
#define IR_VolumeUp		  0x14F1  // 音量大
#define IR_VolumeDown	  0x15E1  // 音量小
#define IR_DisplayShow	0x1B01  // 画面表示
#define IR_SoundMute	  0x17C1  // 消音
#define IR_ChanelUp		  0x11A1  // チャネル＋
#define IR_ChanelDown	  0x1291  // チャネルー
#define IR_HomeMenu	    0xBB92  // ホーム／メニュー
#define IR_ChanelGuide	0x60F2  // 番組表
#define IR_d		        0x5E22  // ｄ
#define IR_CursorUp		  0x5781  // カーソル上
#define IR_CursorDown	  0x2081  // カーソル下
#define IR_CursorLeft		0xD701  // カーソル左
#define IR_CursorRight	0xD8F1  // カーソル右
#define IR_Select		    0x52D1  // 決定
#define IR_Back		      0xE401  // 戻る
#define IR_Blue		      0x8012  // 青
#define IR_Red		      0x8102  // 赤
#define IR_Green		    0x8232  // 緑
#define IR_Yellow		    0x8322  // 黃

class Sharp0000{
  typedef unsigned long dword;

  public:
  Sharp0000(byte irPort){ this->irPort = irPort; }
  void init();
  bool read();                      // 受信処理。非同期なので受信がなければ即戻る。
                                    // リーダーキャリアオン時間-500us以内の周期で呼ばないと取りこぼしが発生する
  word getCommand(){ return cmd; }  // 受信コマンド
  bool isRepeat(){ return repeat; } // リピートされた受信コマンド？

  private:
  const bool ON  = true;
  const bool OFF = false; 
  byte  irPort;
  byte  irData[6]; // 48bitの受信データ。
  dword endTime;
  word  cmd, oldCommand;
  bool  repeat;

  bool irCheck(dword &startTime, dword &deltaTime, dword timeShort, dword timeOver, bool careerOn); // キャリアのオン・オフ時間判定
};

void Sharp0000::init(){
  pinMode(irPort, INPUT_PULLUP);
  repeat = false;
  endTime = 0;
  cmd = oldCommand = 0x0000;
}

bool Sharp0000::read() {
  dword dt;

  dword startTime = micros();
  if(irCheck(startTime, dt,  500, 4000, ON)) return false;    // リーダーチェック
  if(irCheck(startTime, dt, 1400, 1800, OFF)) return false;
  for(byte i = 0; i < 48; ++i){ // データビットの読込
    if(irCheck(startTime, dt, 300, 600, ON)) return false;
    if(irCheck(startTime, dt, 200, 1400, OFF)) return false;
    irData[i >> 3] >>= 1;
    if(dt > 780){ // データ1の判定
      irData[i >> 3] |= 0x80;
    }
  }
  if(irCheck(startTime, dt, 300, 600, ON)) return false;
  
  cmd = (irData[4] << 8) | irData[5];
  if(oldCommand == cmd && startTime - endTime < 200000){
    repeat = true;
  }else{
    repeat = false;
  }
  oldCommand = cmd;
  endTime = startTime;
  return true;
}

bool Sharp0000::irCheck(dword &startTime, dword &deltaTime, dword timeShort, dword timeOver, bool careerOn){
  dword t;

  do{
    t = micros();
    deltaTime = t - startTime;
  }while(!digitalRead(irPort) == careerOn && deltaTime < timeOver);

  if(deltaTime < timeShort || deltaTime > timeOver){
    return true;  // タイムショート・オーバーエラー
  }
  startTime = t;
  return false;   // 有効時間幅
}
