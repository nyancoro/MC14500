#pragma once
#include <Arduino.h>

class Timer{
  typedef unsigned long dword;
  public:
  void init(dword ta, dword tb, dword tbRand){
    this->ta = ta;
    this->tb = tb;
    this->tbRand = tbRand;
    timeOnA = false;
    timeOnB = false;
  }
  void setTimer(){
    st = millis();
    tbr = tb + micros() % tbRand;
    timeOnA = true;
    timeOnB = true;
  }
  bool isTimerOnA(){
    if(timeOnA && millis() - st >= ta)
      timeOnA = false;
    return timeOnA;
  }
  bool isTimerOnB(){
    if(timeOnB && millis() - st >= tbr)
      timeOnB = false;
    return timeOnB;
  }
  private:
  dword st;
  dword ta, tb, tbRand, tbr;
  bool timeOnA;
  bool timeOnB;
};
