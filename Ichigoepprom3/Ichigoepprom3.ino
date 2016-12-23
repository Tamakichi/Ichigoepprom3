//
// I2C EEPROMエミュレーション プロタイプ版 v1 level2
// 2016/04/26,4/30 作成 by Tamakitchi
// 2016/05/04 テキスト形式の保存対応 by Tamakichi 
//
// 注意：このスケッチの動作にはWireライブラリ修正版が必要
//

#include <Wire.h>
#include <sdfonts.h>
#include <sdbitmap.h>
#include "files.h"

#define MYDEBUG   0            // 0:デバッグ表示on(=1)/off(=0)
#define USE_CON   0            // コンソール出力 0:なし 1:あり
#define UART_BPS  115200       // デバッグ用シリアル通信ボーレート       
extern "C" {
#include <utility/twi.h>
}

#define DEVICE_ADDRESS 0x50  // 24LC1025 I2Cデバイスアドレス 
 
uint16_t adr;               // EEPROM アクセスアドレス
uint16_t cnt;               // 書込みバイト数
char fname[13];             // ファイル名
bool fsts;                  // ファイルステータス

// I2Cマスタからのデータ受信ハンドラ
void receiveEvent(int len) {
  uint8_t buf[PCTSIZE];         // プログラム保存領域
  uint16_t d1,d2;
  uint16_t slaveAdr;            // マスター要求スレーブアドレス
  uint8_t  i = 0;

  fsts = false;
  slaveAdr = twi_get_address(); // ターゲットスレーブアドレス
      
  // アドレス情報取得
  d1 = Wire.read();
  d2 = Wire.read(); 
  adr = (d1<<8) + d2;

  // エミュレション用SDカード上のファイルオープン
  if ( !(adr & 0x3ff) && (len > 2) ) {
    // データ書き込みモードでファイルオープン
    setFileNameFromAddress(fname, slaveAdr, adr);
    SD_open_file_for_write(fname);
    cnt = 0;
  } else if (len == 2) {
    // データ書き込みモードでファイルオープン
    setFileNameFromAddress(fname, slaveAdr, adr);
    fsts = SD_open_file_for_read(fname);
  }

#if MYDEBUG == 1 && USE_CON == 1
  Serial.print(F("adr="));
  Serial.print(adr,HEX);
  Serial.write(' ');
  Serial.println(len,DEC);
#endif

  if (len > 2) {
    // 送信されたデータを受信(データがある場合) 
    while (Wire.available()){
      buf[i++] = Wire.read();
    }
    SD_write_to_file(cnt , buf, i);
    adr += i;
    cnt += i;
    if (cnt >= 0x400) {
      SD_close_file();    
    }
  }
}

// I2Cマスタからの要求の処理
void requestEvent() {
  uint16_t n;
  uint8_t  sts;
  uint8_t  i;
  uint8_t  buf[PCTSIZE];

#if MYDEBUG  == 1 && USE_CON == 1
  Serial.print("req ");
  Serial.println(adr,HEX);
#endif

  if (fsts) {
    SD_read_from_file(adr & 0x3ff, buf, PCTSIZE);
  } else {
    memset(buf, 0, PCTSIZE);
  }
  Wire.write(buf, PCTSIZE);
  SD_close_file();
}

// セットアップ
void setup() {

#if USE_CON == 1
    Serial.begin(UART_BPS);
#endif 
    if (!SD_init()) {
#if USE_CON == 1
      Serial.println(F("SD initialization failed."));  
#endif 
    }

    adr = 0;    
    Wire.begin(DEVICE_ADDRESS) ;        // I2Cの初期化、自アドレスをA0とする
    twi_set_addressMask(B00001000);     // 複数アドレス対応 DEVICE_ADDRESS+0x04 も許可)
    Wire.onRequest(requestEvent) ;      // I2Cコマンド要求割込み関数の登録
    Wire.onReceive(receiveEvent) ;      // I2Cデータ受信割込み関数の登録
#if USE_CON == 1 
    Serial.println(F("start"));
#endif 
}

// メイン処理(デバッグ・メンテナンス用)
void loop() {
#if MYDEBUG == 3 && USE_CON == 1  
  byte c;
  uint8_t buf[MAXTEXTLEN];
  
  Serial.print(F("Debug Menu(1:text2bin 2:SD_read_from_file_as_text) ="));
  while ( !Serial.available() );
   c = Serial.read();
  if (c == '1') {
     Serial.println(F("1"));
    SD_open_file_for_read("110.DAT");
    uint8_t n;
    while(1) {
      n = readLine(buf, MAXTEXTLEN);
      if (!n)
        break;

      Serial.print(F("len=")); 
      Serial.print(n,DEC);
      Serial.write(' ');
      Serial.println((char *)buf);
      
      n = text2bin(buf, n);
      Serial.print(F("n="));
      Serial.println(n,DEC);
      dump(buf, n);
    }
    SD_close_file();
   } else if (c == '2') {
     Serial.println(F("2"));
     for (uint16_t i = 0; i <= 0x400 - PCTSIZE; i+= PCTSIZE) {
       SD_open_file_for_read("110.TXT");
       SD_read_from_file_as_text(i, buf, 32);
       Serial.print(F("adr="));
       Serial.println(i ,HEX);
       dump(buf, PCTSIZE);                
       SD_close_file();
     }
   } else {    
     Serial.println();
   } 
#endif 
}


