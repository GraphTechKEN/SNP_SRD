//V2.1 ピンアサイン変更、鳴動テストを回生開放SW(小田急modeOER時)と兼用化
//V2.2 ATS-P単体の電源を導入
//V2.3 SIMPLEモード、BZ21鳴動時間設定を追加、ATS_P_West_Delay時間を追加
//V2.4 ATS-S,ATS-P-Westの鳴動時間EEPROM初期設定を追加 

#define PIN_Pdengen 14  //P表示灯 電源
#define PIN_Pettern 15  //P表示灯 パターン接ZZAA近
#define PIN_Break 16    //P表示灯 ブレーキ動作
#define PIN_Free 17     //P表示灯 ブレーキ開放
#define PIN_ATS_P 18    //P表示灯 ATS-P
#define PIN_Broken 19   //P表示灯 故障
#define PIN_Bell 20     //ベル出力
#define PIN_Dengen 11   //ATS電源入力
#define PIN_P_Dengen 3  //ATS-P電源入力

#define PIN_ATS_TEST 10      //ATS鳴動テスト
#define PIN_ATS_TEST_BZ21 9  //BZ21警報持続テスト(B接点)
#define PIN_ATS_ERR 5        //ATS警報器用リレー
#define PIN_ATS_BZ21 4       //BZ21警報用リレー
#define PIN_ATS_MITOUNYU 8   //ATS未投入警報用リレー

//#define PIN_Free_Mask 3    //解放マスクリレー 温泉V1用
//#define PIN_Broken_Mask 6  //故障マスクリレー 温泉V1用
#define SIMPLE  //鳴動テスト、P電源開放省略常時入

#include <Arduino.h>
#include <EEPROM.h>

bool USB_MON = 0;

uint16_t ATS_P_Dengen_Auto = 1;
uint16_t ATS_P_East = 1;
uint16_t ATS_P_West_Delay = 1000;

uint16_t speed = 0;
String str1 = "";
String str2 = "";
bool bell = false;
bool bell_latch = false;
unsigned long bell_timer = 0;

uint8_t P_Dengen_Tounyu_Step = 0;
uint8_t S_Dengen_Tounyu_Step = 0;
bool S_dengen_tounyu = false;
bool P_dengen_tounyu = false;
unsigned long S_Dengen_Tounyu_Timer = 0;
unsigned long P_Dengen_Tounyu_Timer = 0;
bool ATS_Dengen = false;

bool ATS_P_Dengen = 0;
bool ATS_P_Pettern = 0;
bool ATS_P_Break = 0;
bool ATS_P_Free = 0;
bool ATS_P_Mode = 0;
bool ATS_P_Broken = 0;

uint8_t ATS_Norm = 0;
uint8_t TrainMode = 0;

bool flgAtsErr = false;

uint8_t flgAtsErrTest = 0;

bool flgAtsMitounyu = false;
bool flgAtsMitounyuBell = false;
uint16_t ATS_Mitounyu_Mode = 0;  //ATS未投入防止 (1)警報器(0)警報装置

bool BZ21 = false;
bool ATS_Dengen_In = false;
bool ATS_Dengen_In_latch = false;
bool ATS_P_Dengen_In = false;
bool ATS_P_Dengen_In_latch = false;
bool bve_ATS_Err = false;
bool ATS_test_BZ21_latch = false;
uint16_t ATS_ERR_TIMER = 750;
uint16_t BZ21_stop_time = 10000;  //206

bool modeOER = false;

bool P_dengen_off = false;
bool P_dengen_off_Step = 0;
unsigned long P_dengen_off_timer = 0;

void setup() {

  Serial.begin(115200);
  Serial.setTimeout(10);
  Serial1.begin(115200);
  Serial1.setTimeout(10);
  Serial2.begin(115200);
  Serial2.setTimeout(10);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  pinMode(PIN_Pdengen, OUTPUT);
  pinMode(PIN_Pettern, OUTPUT);
  pinMode(PIN_Break, OUTPUT);
  pinMode(PIN_Free, OUTPUT);
  pinMode(PIN_ATS_P, OUTPUT);
  pinMode(PIN_Broken, OUTPUT);
  pinMode(PIN_Bell, OUTPUT);
  pinMode(PIN_ATS_TEST, INPUT_PULLUP);
  pinMode(PIN_ATS_TEST_BZ21, INPUT_PULLUP);

  pinMode(PIN_ATS_ERR, OUTPUT);   //ATS警報器用リレー
  pinMode(PIN_ATS_BZ21, OUTPUT);  //BZ21警報器用リレー
  pinMode(PIN_ATS_MITOUNYU, OUTPUT);

  pinMode(PIN_Dengen, INPUT_PULLUP);    //ATS電源入力
  pinMode(PIN_P_Dengen, INPUT_PULLUP);  //ATS-P電源入力
  //pinMode(PIN_Free_Mask, OUTPUT);
  //pinMode(PIN_Broken_Mask, OUTPUT);
  //digitalWrite(PIN_Free_Mask, 0);
  //digitalWrite(PIN_Broken_Mask, 0);

  digitalWrite(PIN_ATS_ERR, 0);       //ATS警報器用リレー
  digitalWrite(PIN_ATS_BZ21, 0);      //BZ21警報器用リレー
  digitalWrite(PIN_ATS_MITOUNYU, 0);  //ATS未投入防止リレー

  EEPROM.get(140, ATS_ERR_TIMER);  //ATS-S電源投入時間
  if (ATS_ERR_TIMER == 65535) {
    EEPROM.put(140, 750);
  }
  EEPROM.get(200, ATS_P_Dengen_Auto);
  EEPROM.get(202, ATS_P_East);         //ATS-P East(1)/West(0)
  EEPROM.get(204, ATS_Mitounyu_Mode);  //ATS未投入防止 (1)警報器(0)警報装置
  EEPROM.get(206, BZ21_stop_time);     //BZ21鳴動タイマ(TA-TA)
  EEPROM.get(208, ATS_P_West_Delay);   //ATS-P West点灯遅延
  if (ATS_P_West_Delay == 65535) {
    EEPROM.put(208, 1000);  //ATS-P West点灯遅延
  }
#ifndef SIMPLE
  ATS_Dengen_In = !digitalRead(PIN_Dengen);
  ATS_Dengen = ATS_Dengen_In;
#endif
}

void loop() {
  if (Serial.available() > 0) {
    String str = Serial.readStringUntil('\r');
    if (str.startsWith("MON")) {
      USB_MON = str.substring(4).toInt();
      Serial.print("OK MON ");
      Serial.println(USB_MON);
    } else {
      Uart_Comm(Serial, Serial, str, "USBTest:");
    }
  }

  if (Serial1.available() > 0) {
    str1 = Serial1.readStringUntil('\r');
    Uart_Comm(Serial1, Serial2, str1, "Uart1:");
  }
  if (Serial2.available() > 0) {
    str2 = Serial2.readStringUntil('\r');
    Uart_Comm(Serial2, Serial1, str2, "Uart2:");
  }

#ifndef SIMPLE
  ATS_Dengen_In = !digitalRead(PIN_Dengen);
  if (ATS_Dengen_In && !ATS_Dengen_In_latch) {
    //電源投入フラグ
    if (!ATS_Dengen) {
      ATS_Dengen = true;
      P_dengen_tounyu = true;
      S_dengen_tounyu = true;
      P_dengen_off = false;
      P_Dengen_Tounyu_Step = 0;
      S_Dengen_Tounyu_Step = 0;
      P_dengen_off_Step = 0;
    }
  } else if (!ATS_Dengen_In && ATS_Dengen_In_latch) {
    //電源断フラグ
    if (ATS_Dengen) {
      ATS_Dengen = false;
      P_dengen_tounyu = false;
      S_dengen_tounyu = true;
      P_Dengen_Tounyu_Step = 0;
      S_Dengen_Tounyu_Step = 0;
      P_dengen_off = true;
      P_dengen_off_Step = 0;
    }
  }
  ATS_Dengen_In_latch = ATS_Dengen_In;

  //ATS-P電源単体スイッチ
  ATS_P_Dengen_In = digitalRead(PIN_P_Dengen);
  if (ATS_P_Dengen_In && !ATS_P_Dengen_In_latch) {
    //ATS-P電源投入フラグ
    if (!ATS_P_Dengen) {
      P_dengen_tounyu = true;
      P_dengen_off = false;
      P_Dengen_Tounyu_Step = 0;
      P_dengen_off_Step = 0;
    }
  } else if (!ATS_P_Dengen_In && ATS_P_Dengen_In_latch) {
    //ATS-P電源断フラグ
    if (ATS_P_Dengen) {
      P_dengen_tounyu = false;
      P_dengen_off = true;
      P_Dengen_Tounyu_Step = 0;
      P_dengen_off_Step = 0;
    }
  }
  ATS_P_Dengen_In_latch = ATS_P_Dengen_In;

  ATS_Test();
#endif
  SRD();
  S_Dengen_Tounyu();
  P_Dengen_Tounyu();
  P_Dengen_Off();
  Bell();
  digitalWrite(PIN_Pdengen, ATS_P_Dengen);
  digitalWrite(PIN_Pettern, ATS_P_Pettern);
  digitalWrite(PIN_Break, ATS_P_Break);
  digitalWrite(PIN_Free, ATS_P_Free);
  //digitalWrite(PIN_Free_Mask, !ATS_P_Free);
  digitalWrite(PIN_ATS_P, ATS_P_Mode);
  digitalWrite(PIN_Broken, ATS_P_Broken);
  //digitalWrite(PIN_Broken_Mask, !ATS_P_Broken);


  digitalWrite(PIN_ATS_BZ21, !BZ21 && ATS_Dengen);
  digitalWrite(PIN_ATS_ERR, bve_ATS_Err || flgAtsErr || ((ATS_Mitounyu_Mode & 1) && !ATS_Dengen && flgAtsMitounyuBell));  //ATS警報器動作
  digitalWrite(PIN_ATS_MITOUNYU, !(ATS_Mitounyu_Mode & 1) && !ATS_Dengen && flgAtsMitounyu);
}

void Uart_Comm(Stream& serial_in, Stream& serial_out, String input_string, String name) {
  serial_out.print(input_string);
  serial_out.print('\r');
  if (&serial_out == &Serial) {
    serial_out.print('\n');
  }
  if (USB_MON) {
    Serial.print(name);
    Serial.println(input_string);
  }
  if (input_string.substring(4, 5) == "/") {
    speed = input_string.substring(0, 4).toInt();
  } else if (input_string.startsWith("ATS1")) {  //ATS電源ON
    if (!ATS_Dengen) {
      ATS_Dengen = true;
      P_dengen_tounyu = true;
      S_dengen_tounyu = true;
      P_dengen_off = false;
      P_Dengen_Tounyu_Step = 0;
      S_Dengen_Tounyu_Step = 0;
      P_dengen_off_Step = 0;
    }
  } else if (input_string.startsWith("ATS0")) {  //ATS電源OFF
    if (ATS_Dengen) {
      ATS_Dengen = false;
      P_dengen_tounyu = false;
      S_dengen_tounyu = true;
      P_Dengen_Tounyu_Step = 0;
      S_Dengen_Tounyu_Step = 0;
      P_dengen_off = true;
      P_dengen_off_Step = 0;
    }
  } else if (input_string.startsWith("ATSE")) {  //ATS警報器鳴動
    flgAtsErr = true;
  } else if (input_string.startsWith("ATSN")) {  //ATS警報器停止
    flgAtsErr = false;
  } else if (input_string.startsWith("ATSM3")) {  //ATS未投入(警報器)ON
    flgAtsMitounyuBell = true;
  } else if (input_string.startsWith("ATSM2")) {  //ATS未投入(警報器)OFF
    if (ATS_Mitounyu_Mode >> 1 & 1) {
      flgAtsMitounyuBell = true;
    } else {
      flgAtsMitounyuBell = false;
    }
  } else if (input_string.startsWith("ATSM1")) {  //ATS未投入(防止装置)ON
    flgAtsMitounyuBell = false;
    flgAtsMitounyu = true;
  } else if (input_string.startsWith("ATSM0")) {  //ATS未投入(防止装置)OFF
    flgAtsMitounyu = false;
  } else if (input_string.startsWith("ACT 1")) {  //BZ21停止ON
    BZ21 = true;
  } else if (input_string.startsWith("ACT 0")) {  //BZ21停止OFF
    BZ21 = false;
  }
  String s = ATS_P_Disp(input_string);
  if (s != "") {
    serial_in.print(s);
    serial_in.print('\r');
    //USB入力の場合
    if (&serial_in == &Serial) {
      serial_in.print('\n');
    }
    if (USB_MON) {
      Serial.print(name);
      Serial.println(s);
    }
  }
}

void ATS_Test() {
  //ATS鳴動ボタンテスト 1000秒で警報器停止、BZ21警報はボタン押される(B接点)まで持続
  //ATS鳴動ボタンテスト
  bool ATS_test_Temp = !digitalRead(PIN_ATS_TEST);
  static bool ATS_test = ATS_test_Temp;
  static uint8_t ATS_test_Count_On = 0;
  static uint8_t ATS_test_Count_Off = 0;
  //ATS鳴動ボタンテストカウント開始
  if (ATS_test_Temp) {
    ATS_test_Count_On++;
    ATS_test_Count_Off = 0;
  } else {
    ATS_test_Count_Off++;
    ATS_test_Count_On = 0;
  }
  //ATS鳴動ボタンテストカウント判定
  if (ATS_test_Count_On > 10) {
    ATS_test_Count_On = 10;
    ATS_test = true;
  } else if (ATS_test_Count_Off > 10) {
    ATS_test_Count_Off = 10;
    ATS_test = false;
  }

  bool ATS_test_BZ21_Temp = digitalRead(PIN_ATS_TEST_BZ21);
  static bool ATS_test_BZ21 = ATS_test_BZ21_Temp;
  static uint8_t ATS_test_BZ21_Count_On = 0;
  static uint8_t ATS_test_BZ21_Count_Off = 0;
  //BZ21ボタンテストカウント開始
  if (ATS_test_BZ21_Temp) {
    ATS_test_BZ21_Count_On++;
    ATS_test_BZ21_Count_Off = 0;
  } else {
    ATS_test_BZ21_Count_Off++;
    ATS_test_BZ21_Count_On = 0;
  }
  //BZ21ボタンテストカウント判定
  if (ATS_test_BZ21_Count_On > 10) {
    ATS_test_BZ21_Count_On = 10;
    ATS_test_BZ21 = true;
  } else if (ATS_test_BZ21_Count_Off > 10) {
    ATS_test_BZ21_Count_Off = 10;
    ATS_test_BZ21 = false;
  }

  if (ATS_test_BZ21 != ATS_test_BZ21_latch) {
    if (modeOER) {
      Uart_Comm(Serial1, Serial2, "KEY 9", "Uart1:");
      Uart_Comm(Serial2, Serial1, "KEY 9", "Uart1:");
    } else {
      BZ21 = ATS_test_BZ21;
      //ATS鳴動テストシーケンスを解除
      if (ATS_test_BZ21) {
        flgAtsErrTest = 0;
        flgAtsErr = false;
      }
    }
  }
  ATS_test_BZ21_latch = ATS_test_BZ21;

  //ATS鳴動ボタンテストシーケンス
  static bool ATS_test_latch = ATS_test;
  static unsigned long elapse_ATS_test = millis();
  static unsigned long elapse_ATS_test_BZ21 = millis();
  if (flgAtsErrTest == 0 && ATS_Dengen && ATS_test && !ATS_test_latch) {
    //ATS警報器タイマースタート
    elapse_ATS_test = millis();
    //BZ21タイマースタート
    elapse_ATS_test_BZ21 = millis();
    flgAtsErrTest = 1;
    flgAtsErr = true;
  }
  ATS_test_latch = ATS_test;
  //ATS警報器タイマー停止
  if (flgAtsErrTest == 1 && (millis() - elapse_ATS_test) > 750) {
    flgAtsErrTest = 2;
    flgAtsErr = false;
  }
  //BZ21タイマー停止
  if (flgAtsErrTest == 2 && (millis() - elapse_ATS_test) > BZ21_stop_time) {
    flgAtsErrTest = 3;
    BZ21 = true;
  }
  //BZ21タイマー復帰
  if (flgAtsErrTest == 3 && (millis() - elapse_ATS_test) > (BZ21_stop_time + 500)) {
    flgAtsErrTest = 0;
    BZ21 = false;
  }
  //ATS鳴動ボタンテストシーケンスここまで
}

void SRD() {
  //SRD
  if (speed >= 50) {
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }
}

void S_Dengen_Tounyu(void) {
  if (ATS_Dengen) {
    //ATS-SN
    if (S_dengen_tounyu && S_Dengen_Tounyu_Step == 0) {
      flgAtsErr = true;
      S_Dengen_Tounyu_Timer = millis();
      S_Dengen_Tounyu_Step = 1;
    } else if (S_Dengen_Tounyu_Step == 1 && (millis() - S_Dengen_Tounyu_Timer) > ATS_ERR_TIMER) {
      flgAtsErr = false;
      S_Dengen_Tounyu_Step = 2;
    } else if (S_Dengen_Tounyu_Step == 2 && !S_dengen_tounyu) {
      S_Dengen_Tounyu_Step = 0;
    }
  } else {
    S_Dengen_Tounyu_Step = 0;
    S_dengen_tounyu = false;
    flgAtsErr = false;
  }
}

void P_Dengen_Tounyu(void) {
  if (ATS_Dengen && ATS_P_Dengen_In) {
    if (P_dengen_tounyu) {
      //ATS-P
      uint16_t P_On_Delay = 0;
      if (!ATS_P_East) {
        P_On_Delay = ATS_P_West_Delay;
      }

      if (P_dengen_tounyu && P_Dengen_Tounyu_Step == 0) {
        ATS_P_Dengen = ATS_P_East;
        bell = ATS_P_East;
        P_Dengen_Tounyu_Timer = millis();
        P_Dengen_Tounyu_Step = 1;
      } else if (P_Dengen_Tounyu_Step == 1 && millis() - P_Dengen_Tounyu_Timer > P_On_Delay) {
        P_Dengen_Tounyu_Step = 2;
        ATS_P_Dengen = true;
      } else if (P_Dengen_Tounyu_Step == 2 && millis() - P_Dengen_Tounyu_Timer > (100 + P_On_Delay)) {
        ATS_P_Broken = true;
        if (!ATS_P_East) {
          ATS_P_Break = true;
        }
        bell = ATS_P_East;
        P_Dengen_Tounyu_Step = 3;
      } else if (P_Dengen_Tounyu_Step == 3 && millis() - P_Dengen_Tounyu_Timer > (300 + P_On_Delay)) {
        if (ATS_P_East) {
          bell = ATS_P_East;
        }
        P_Dengen_Tounyu_Step = 4;
      } else if (P_Dengen_Tounyu_Step == 4 && millis() - P_Dengen_Tounyu_Timer > (5250 + P_On_Delay)) {
        if (ATS_P_East) {
          ATS_P_Broken = false;
          ATS_P_Break = true;
        }
        P_Dengen_Tounyu_Step = 5;
      } else if (P_Dengen_Tounyu_Step == 5 && millis() - P_Dengen_Tounyu_Timer > (5300 + P_On_Delay)) {
        ATS_P_Break = false;
        if (!ATS_P_East) {
          ATS_P_Broken = false;
        }
        P_Dengen_Tounyu_Step = 0;
        P_dengen_tounyu = false;
      }
    }
  } else {
    P_Dengen_Tounyu_Step = 0;
    P_dengen_tounyu = false;
  }
}

void P_Dengen_Off(void) {
  if (P_dengen_off) {
    if (P_dengen_off_Step == 0) {
      ATS_P_Mode = false;
      ATS_P_Dengen = false;
      ATS_P_Break = false;
      ATS_P_Broken = true;
      P_dengen_off_timer = millis();
      P_dengen_off_Step = 1;
    }
    if (P_dengen_off_Step == 1 && (millis() - P_dengen_off_timer) > 50) {
      ATS_P_Broken = false;
      P_dengen_off = false;
    }
  }
}

void Bell(void) {
  if (ATS_P_East) {
    if (bell) {
      bell = false;
      digitalWrite(PIN_Bell, 1);
      bell_timer = millis();
    }
    if (!bell && millis() - bell_timer > 50) {
      digitalWrite(PIN_Bell, 0);
    }
  }
}

String ATS_P_Disp(String _str) {
  uint8_t i = 0;
  String s = "";

  if (_str.startsWith("WR ")) {
    if (_str.length() > 7) {
      uint8_t device = _str.substring(3, 6).toInt();
      int16_t num = _str.substring(7, 12).toInt();
      if (device >= 200 && device < 256 || device == 140) {
        switch (device) {

            //ATS-S電源投入時間
          case 140:
            if (num > 3000 || num < 0) {

              s = "E1 " + String(device);
            } else {
              s = rw_eeprom(device, &num, &ATS_ERR_TIMER, true);
            }
            break;

          //P電源Auto
          case 200:
            if (num < 0 || num > 1) {
              s = "E1 " + device;
            } else {
              s = rw_eeprom(device, &num, &ATS_P_Dengen_Auto, true);
            }
            break;

            //ATS-P East(1)/West(0)
          case 202:
            if (num < 0 || num > 1) {
              s = "E1 " + device;
            } else {
              s = rw_eeprom(device, &num, &ATS_P_East, true);
            }
            break;

            //ATS未投入防止装置モード
          case 204:
            if (num < 0 || num > 4) {
              s = "E1 " + device;
            } else {
              s = rw_eeprom(device, &num, &ATS_Mitounyu_Mode, true);
            }
            break;

            //BZ21鳴動時間
          case 206:
            if (num < 0 || num > 65535) {
              s = "E1 " + device;
            } else {
              s = rw_eeprom(device, &num, &BZ21_stop_time, true);
            }
            break;

            //BZ21鳴動時間
          case 208:
            if (num < 0 || num > 65535) {
              s = "E1 " + device;
            } else {
              s = rw_eeprom(device, &num, &ATS_P_West_Delay, true);
            }
            break;
        }
      }
    }

  } else if (_str.startsWith("RD ")) {
    if (_str.length() > 5) {
      uint8_t device = _str.substring(3, 6).toInt();
      if (device >= 200 && device < 256) {
        uint8_t nnn = 0;
        s = rw_eeprom(device, 0, (uint16_t)&nnn, false);
      }
    }
  } else {
    if (_str.length() > 17) {
      //ATS白色表示灯抽出
      ATS_Norm = _str.substring(14, 15).toInt();

      //JRモード抽出
      if (ATS_Norm == 1) {
        TrainMode = 0;
      }
      //小田急モード抽出
      if (_str.length() > 41) {
        if (_str.substring(40, 41).toInt() == 2) {
          TrainMode = 1;  //小田急
        }
      }
      if (_str.length() > 41) {
        if (!P_dengen_tounyu && ATS_Dengen) {
          //JRモード
          if (TrainMode == 0) {
            //ATS警報抽出
            bve_ATS_Err = _str.substring(15, 16).toInt();

            if (ATS_P_Dengen_Auto) {
              ATS_P_Dengen = _str.substring(42, 43).toInt();
            } else {
              ATS_P_Dengen = true;
            }
            if (ATS_P_Pettern != _str.substring(29, 30).toInt()) {
              ATS_P_Pettern = _str.substring(29, 30).toInt();
              bell = true;
              Bell();
            }
            if (ATS_P_Break != _str.substring(31, 32).toInt()) {
              ATS_P_Break = _str.substring(31, 32).toInt();
              bell = true;
              Bell();
            }
            ATS_P_Free = _str.substring(30, 31).toInt();
            if (ATS_P_Mode != _str.substring(32, 33).toInt()) {
              ATS_P_Mode = _str.substring(32, 33).toInt();
              bell = true;
              Bell();
            }
            ATS_P_Broken = _str.substring(33, 34).toInt();
          } else if (TrainMode == 1) {
            if (ATS_P_Dengen_Auto) {
              ATS_P_Dengen = _str.substring(42, 43).toInt();
            } else {
              ATS_P_Dengen = true;
            }
            ATS_P_Pettern = _str.substring(28, 29).toInt();
            ATS_P_Break = _str.substring(26, 27).toInt();
            ATS_P_Free = false;
            ATS_P_Mode = true;
            ATS_P_Broken = false;
          }
        }
      }
    }
  }
  return s;
}

String rw_eeprom(uint16_t dev, uint16_t* n, uint16_t* param, bool write) {
  if (write) {
    *param = *n;
    EEPROM.put(dev, *param);
  } else {
    EEPROM.get(dev, *param);
  }
  String s = "OK ";
  if (dev < 100) {
    s += "0";
  }
  if (dev < 10) {
    s += "0";
  }
  s += dev;
  s += " ";
  s += *param;

  return s;
}