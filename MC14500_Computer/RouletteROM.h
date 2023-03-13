#pragma once

// ルーレットゲーム

/*
  プログラム作成時は入力ポート、出力ポート、テンポラリレジスタの定義を行うこと！
*/

enum{
  // 入力ポート
  SWA	= 0,  // ルーレット開始
  // 出力ポート
  N_1,
  N_2,
  N_3,
  N_4,
  
  // テンポラリレジスタ
  STATE,
  TEMP,

  // ペリフェラル
  T0ONA,
  T0ONB,
  T0ON,
};

// 忘れずに修正すること
#define ST_INPUT        SWA      // 入力ポートの開始アドレス
#define END_INPUT       SWA      // 入力ポートの終了アドレス
#define ST_OUTPUT       N_1      // 出力ポートの開始アドレス
#define END_OUTPUT      N_4      // 出力ポートの終了アドレス
//
#define BITSET          0xFE      // 
#define RR              0xFF      // リザルトレジスタ(変更しないこと！！)

#define DELAY_100ms NOPF, 1
#define DELAY_300ms NOPF, 2
#define BEEP_ON     NOPF, 3

const byte romData[] PROGMEM = {
// 初期化
ORC,  RR,     // RR=1
IEN,  RR,     // IEN=1
OEN,  RR,     // OEN=1
STO,  N_1,
STOC, N_2,
STOC, N_3,
STOC, N_4,
STOC, STATE,  // state=0

// main loop
ORC,  RR,
IEN,  RR,
OEN,  RR,

// スタートSWオンで作動開始
LDC,  SWA,    // SWA==ON * STATE==0
ANDC, STATE,
OEN,  RR,
STO,  STATE,  // state=1
STO,  T0ON,   // T0NO ON
STOC, T0ON,

// ルーレット点灯移動
LD,   STATE,  // state == 1
OEN,  RR,
LD,   N_4,    // 1bit rotate shift
STO,  TEMP,
LD,   N_3,
STO,  N_4,
LD,   N_2,
STO,  N_3,
LD,   N_1,
STO,  N_2,
LD,   TEMP,
STO,  N_1,

// ルーレット移動毎にbeep on
LD,   STATE,
SKZ,  2,
BEEP_ON,      // BEEP

// 高速点灯移動時のタイマ
LD,   STATE,  // state == 1 * T0ONB
AND,  T0ONA,
SKZ,  2,
DELAY_100ms,  // delay = 100ms

// 低速点灯移動時のタイマ
LD,   STATE,  // state = 1 * /T0ONA * T0ONB
ANDC, T0ONA,
AND,  T0ONB,
SKZ,  2,
DELAY_300ms,  // delay = 300ms

// 終了判定
LD,   STATE,  // state == 1 * /T0ONA * /T0ONB
ANDC, T0ONA,
ANDC, T0ONB,
OEN,  RR,
STOC, STATE,  // state=0

JMP,  8,      // main loop
};

const int romSize = sizeof(romData) / 2;