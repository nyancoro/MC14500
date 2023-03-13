#pragma once

// じゃんけんゲーム
//   コンピュータとじゃんけん

/*
  プログラム作成時は入力ポート、出力ポート、テンポラリレジスタの定義を行うこと！
*/

enum{
  // 入力ポート
  SWA	= 0,  // 役選択SW入力
  JUDGESW,	// 判定SW
  _padding0_, // paddingは無くても良いが液晶画面に分かりやすく表示するために使用
  _padding1_, // paddingで4bit毎に分ける

  // 出力ポート
  AWIN,     // Aの勝ち
  BWIN,     // B(コンピュータ)の勝ち
  DRAW,     // 引き分け
  _padding2_,
  
  // テンポラリレジスタ
  AG,       // グー選択(プレイヤー側)
  AT,       // チョキ選択
  AP,       // パー選択
  _padding3_,
  BG,       // グー選択(コンピュータ側)
  BT,       // チョキ選択
  BP,       // パー選択
  _padding4_,
  
  STATE,    // 状態	0:約の選択、1:判定
  ONE,      // 常に１
  SWABUF,
  JUDGESWBUF,
  TEMP,
};

// 忘れずに修正すること
#define ST_INPUT        SWA       // 入力ポートの開始アドレス
#define END_INPUT       _dummy1_   // 入力ポートの終了アドレス
#define ST_OUTPUT       AWIN      // 出力ポートの開始アドレス
#define END_OUTPUT      _dummy2_      // 出力ポートの終了アドレス
//
#define RR              0xFF      // リザルトレジスタ(変更しないこと！！)

const byte romData[] PROGMEM = {
// 初期化
ORC,  RR,
IEN,  RR,
OEN,  RR,
STO,  ONE,
STO,  AG,
STOC, AT,
STOC, AP,
STO,  BG,
STOC, BT,
STOC, BP,
STOC, STATE,

// main loop
ORC,  RR,
IEN,  RR,
OEN,  RR,

// stateの切替
LD,   JUDGESWBUF,
STO,  TEMP,
LDC,  JUDGESW,
STO,  JUDGESWBUF,
ANDC, TEMP,
OEN,  RR,
LDC,  STATE,
STO,  STATE,
OEN,  ONE,

// state = 0
// Aの手を選択
LD,   SWABUF,
STO,  TEMP,
LDC,  SWA,
STO,  SWABUF,
ANDC, TEMP,
ANDC, STATE,
OEN,  RR,
LD,   AT,   // A約の切替
STO,  TEMP,
LD,   AG,
STO,  AT,
LD,   AP,
STO,  AG,
LD,   TEMP,
STO,  AP,
OEN,  ONE,

// Bの手を選択
LDC,  STATE,
OEN,  RR,
LD,   BT,   // B役の切替
STO,  TEMP,
LD,   BG,
STO,  BT,
LD,   BP,
STO,  BG,
LD,   TEMP,
STO,  BP,
OEN,  ONE,

// state = 1
// DRAW 判定
LD,   AG,
AND,  BG,
STO,  TEMP,
LD,   AT,
AND,  BT,
OR,   TEMP,
STO,  TEMP,
LD,   AP,
AND,  BP,
OR,   TEMP,
AND,  STATE,
STO,  DRAW,

// Player WIN 判定
LD,   AG,
AND,  BT,
STO,  TEMP,
LD,   AT,
AND,  BP,
OR,   TEMP,
STO,  TEMP,
LD,   AP,
AND,  BG,
OR,   TEMP,
AND,  STATE,
STO,  AWIN,

// Computer WIN 判定
LDC,  AWIN,
ANDC, DRAW,
AND,  STATE,
STO,  BWIN,

NOPF, 0x00,   // wait 5ms
JMP,  11,     // main loop
};

const int romSize = sizeof(romData) / 2;