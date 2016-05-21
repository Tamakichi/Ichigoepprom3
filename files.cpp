//
// SDカード ファイル操作
//

#include <arduino.h>
#include <SPI.h>
#include <SD.h>
#include <string.h>
#include "files.h"

#define SD_CS_PIN 10            // SDカード CSピン

File myFile;                    // ファイル操作オブジェクト
static volatile bool flgText;   // テキスト形式変換フラグ
static volatile bool flgEOP;    // プログラム終了フラグ
static volatile uint8_t hsize;  // 現在格納中のヘッダ
static uint8_t head[3];         // 行ヘッダ
static volatile uint8_t tsize;  // 現在処理済みのテキスト
static volatile uint16_t line;  // 行番号
static volatile uint8_t  tsz;   // テキスト部サイズ

//
// ファイルタイムスタンプコールバック関数
//
void dateTime(uint16_t* date, uint16_t* time) {
  *date = FAT_DATE(2016, 5, 1);
  *time = FAT_TIME(12, 0, 0);
}

//
// SDカードドライバのイニシャライズ
//
bool SD_init(void) {
  flgText = false;
  SdFile::dateTimeCallback( &dateTime );
  return SD.begin(SD_CS_PIN);
}

//
// ファイル名の生成
// I2Cアドレス,メモリ内アドレスからファイル名を生成する
// 引数
//  fname(OUT) : ファイル名: 100.DAT ～ 227.DAT or 100.BAS ～ 227.BAS  
//  device(IN) : I2Cアドレス(0x50 or 0x54)
//  addr(IN)   : メモリ内アドレス 0x0000 ～ 0xFFFF
//
void setFileNameFromAddress(char* fname, uint8_t device, uint16_t addr) {
  uint16_t nm = 100;
  if (device == 0x54) {
    nm += 64;
  }
  
  nm += (addr>>10); 
  sprintf(fname, "%03d.TXT", nm);
  
  // ファイル名の設定(XXX.TXTファイルが存在する場合、テキストモードとする)
  if (SD.exists(fname)) {
    flgText = true;
  } else {
    flgText = false;
    sprintf(fname, "%03d.DAT", nm);  
  }
}

//
// ファイルを書き込みモードでオープン
// 引数
//  fname(IN): ファイル名 (8.3形式)
// 戻り値
//  true 正常終了 false 異常終了
//
bool SD_open_file_for_write(char* fname) {
  if (SD.exists(fname)) {
    SD.remove(fname);
  }
  flgEOP = false;
  hsize = 0;
  tsize = 0;
  myFile = SD.open(fname, FILE_WRITE);
  if (!myFile) {
    return false;
  }
  return true;
}

//
// ファイルを読み込みモードでオープン
// 戻り値
//  true 正常終了 false 異常終了
//
bool SD_open_file_for_read(char* fname) {
  myFile = SD.open(fname, FILE_READ);
  if (!myFile) {
#if MYDEBUG == 1 && USE_CON == 1    
    Serial.print("cant open:");
    Serial.println(fname);
#endif
    return false;
  }
  return true;
}

//
// ファイルのクローズ
//
void SD_close_file(void) {
  myFile.close(); 
}

//
// データの書き込み
//
bool SD_write_to_file(uint16_t pos, uint8_t* dt, uint8_t sz) {
  if (flgText) {
     // テキストモードで書き込み
     return SD_write_to_file_as_text(dt, sz);
  } else {
    // バイナリーモードで書き込み
    if ( !myFile.seek(pos) )   
      return false;
    if ( !myFile.write(dt, sz) )
      return false;
  }
  return true;  
}

//
// テキストとしてデータ書き込み
//
bool SD_write_to_file_as_text(uint8_t* dt, uint8_t sz) {
  if (flgEOP) 
      return true;

  for (uint8_t i = 0; i < sz; i++) {      
    if (hsize < 3) {
      // ヘッダ部の処理
      head[hsize++] = dt[i];
      if ( hsize >= 3) {
        line = head[0] + (((uint16_t)head[1])<<8);
        tsz = head[2]+1;
        tsize = 0;
        if (line) {
          myFile.print(line,DEC); // 行番号出力 
          myFile.write(' ');
        } else {
          // プログラム終了
          flgEOP = true;
          break;          
        }
      }
    } else {
      // テキスト部の処理
      if (dt[i])
        myFile.write(dt[i]); 
      tsize++;
      if ( tsize >= tsz ) {
         myFile.println();
         hsize = 0;
      }
    }
  }
  return true;  
}

//
// データの読み込み
//
bool SD_read_from_file(uint16_t pos, uint8_t* dt, uint8_t sz) {
  if (flgText) {
     // テキストモードで書き込み
     return SD_read_from_file_as_text(pos,dt, sz);
  } else {
    if ( !myFile.seek(pos) )   
      return false;
  
    for (uint8_t i = 0 ; i < sz; i++) {
      if ( !myFile.available() ) 
        return false;
      dt[i] = myFile.read();  
    }
  }
  return true;  
}

//
// テキストからのデータ読み込み
// 引数
//  pos : 先頭からのオフセット(バイナリ換算)
//  dt  : 格納バッファ
//  sz  : バッファサイズ
// 戻り値
//  true :正常終了 false :異常終了
// 
bool SD_read_from_file_as_text(uint16_t pos, uint8_t* dt, uint8_t sz) {
  uint8_t buf[MAXTEXTLEN];  // 行バッファ
  int16_t n;                // 1行テキスト長  
  int16_t bn;               // 1行バイナリ長
  int16_t cnt = 0;          // 先頭からのデータ数
  uint8_t sn  = 0;          // 読み込み済数
  
  // ファイル内先頭に移動
  if ( !myFile.seek(0) )   
    return false;

  while ( 1 ) {
    // 1行分データ取得
    
    n = readLine(buf, MAXTEXTLEN-1) ;
    if (!n) {
      //return false;
      bn = MAXTEXTLEN-1;
      memset(buf, 0, MAXTEXTLEN-1);    
    } else {
      // バイナリ変換
      bn = text2bin(buf, n);     
    }
    
#if MYDEBUG  == 2 && USE_CON == 1
    Serial.print("pos="); Serial.print(pos,DEC);
    Serial.print(" cnt="); Serial.print(cnt,DEC);
    Serial.print(" bn="); Serial.print(bn,DEC);
    Serial.print(" sz="); Serial.print(sz,DEC);
    Serial.print(" sn="); Serial.println(sn,DEC);
#endif

    if ( (pos + sz) <= (cnt + bn) ) { 
      // 読み込むデータが全てバッファ内にある
      memmove(&dt[sn], &buf[pos-cnt], sz);
      return true;
      
    } else if ( pos <= (cnt + bn) ) {
      // データの一部がバッファ内にある
      memmove(&dt[sn], &buf[pos-cnt], cnt + bn - pos);
      sz  -= (cnt + bn - pos);
      sn  += (cnt + bn - pos);      
      pos += (cnt + bn - pos);
    }
    cnt += bn;
  }
  if (!sn)
    return false;
  return true;  
}

//
// 1行分データのバイナリ化
// 引数
//  dt:    読み込みデータ格納バッファ
//  sz:    格納データサイズ
// 戻り値 格納バイナリデータサイズ
// 
int16_t text2bin(uint8_t* dt, uint8_t sz) {
  uint16_t lineno = 0;  // 行番号
  uint8_t  p = 0;       // 参照用位置
  uint8_t  len;         // テキスト部サイズ
  uint16_t n;           // 全体サイズ

  // 行番号の取得
  lineno = 0;
  while ( isdigit(dt[p]) ) {
    lineno *= 10;
    lineno += (dt[p] - '0');
    p++;
  }
  
  while( isspace (dt[p]) )
    p++;

  // テキスト部のサイズ計算
  len = sz - p;
  if (len & 1) {
    // バウンダリー調整
    len ++;
    dt[p+len-1] = 0;
  }
  
  // 全体サイズの計算
  n = len + 4;

  // テキスト部移動
  if (p != 3) {
    memmove(&dt[3], &dt[p],len);
  }
  
  // ヘッダの設定
  dt[0] = (uint8_t)(lineno & 0xff);
  dt[1] = (uint8_t)(lineno>>8);
  dt[2] = len;

  // フッタの設定
  dt[n-1] = 0;
  
  return n;
}

//
// テキストデータ１行読み込み
// 引数
//  dt: 読み込みデータ格納バッファ
//  sz: 最大読み込みサイズ 
// 戻り値
//  文字列長
//
int16_t readLine(uint8_t* dt, uint8_t sz) {
  uint16_t p = 0;
  uint16_t d;
  
  // 1行読み込み 
  while (myFile.available()) {
    d = myFile.read();
    if (d < 0) 
      break;
       
    if ( (d == 0x0d) && (myFile.peek() == 0x0a) ) {
      // 改行コードを検出
      myFile.read();
      dt[p] = 0x00;
      break;  
    }
    dt[p++] = d;
    if (p >= sz) 
      break;
  }
  return p;
}



