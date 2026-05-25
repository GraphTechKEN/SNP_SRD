#define PIN_Pdengen 14  //P表示灯 電源
#define PIN_Pettern 15  //P表示灯 パターン接ZZAA近
#define PIN_Break 16    //P表示灯 ブレーキ動作
#define PIN_Free 17     //P表示灯 ブレーキ開放
#define PIN_ATS_P 18    //P表示灯 ATS-P
#define PIN_Broken 19   //P表示灯 故障
#define PIN_Bell 20     //ベル出力
void setup() {

  pinMode(PIN_Pdengen, OUTPUT);
  pinMode(PIN_Pettern, OUTPUT);
  pinMode(PIN_Break, OUTPUT);
  pinMode(PIN_Free, OUTPUT);
  pinMode(PIN_ATS_P, OUTPUT);
  pinMode(PIN_Broken, OUTPUT);
  pinMode(PIN_Bell, OUTPUT);
}

void loop() {
  for (int i = 14; i <= 20; i++) {
    digitalWrite(i, 1);
    if (i > 14) {
      digitalWrite(i - 1, 0);
    } else {
      digitalWrite(20, 0);
    }
    delay(250);
  }
}
