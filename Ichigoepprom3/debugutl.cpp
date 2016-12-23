//
// デバッグ用ツール・ライブラリ
//

#include <arduino.h>
#include <EEPROM.h>

//
// 保存プログラムのダンプ表示
// 引数
//   buf: プログラム保存領域先頭アドレス
// 
void disp_prog(uint8_t *buf) {
  uint16_t ptr = 0;  // 参照位置
  uint16_t nm,n1,n2; // 行番号
  uint8_t sz;        // 行サイズ
   Serial.println(F("Ichigojam program list"));
  while (1) {
    n1 = buf[ptr++];
    n2 = buf[ptr++];
    nm = n1 + (n2<<8);
    if (!nm)
      break;
    sz = buf[ptr]; ptr++;
    Serial.print(nm,DEC); Serial.write(' ');
    for (uint8_t i= 0; i < sz; i++) 
      Serial.write(buf[ptr++]);
    Serial.println();
    ptr++;  
  }  
}

//
// 保存プログラムのダンプ表示(EEPROM版)
//
void disp_prog(void) {
  uint16_t ptr = 0;  // 参照位置
  uint16_t nm,n1,n2; // 行番号
  uint8_t sz;        // 行サイズ
   Serial.println(F("Ichigojam program list"));
  while (1) {
    n1 = EEPROM[ptr++];
    n2 = EEPROM[ptr++];
    nm = n1 + (n2<<8);
    if (!nm)
      break;
    sz = EEPROM[ptr]; ptr++;
    Serial.print(nm,DEC); Serial.write(' ');
    for (uint8_t i= 0; i < sz; i++) 
      Serial.write(EEPROM[ptr++]);
    Serial.println();
    ptr++;  
  }  
}


//
// 書込みデータのダンプリスト
// 引数
//  dt : データ格納アドレス
//  n  : 表示バイトデータ数
//
void dump(uint8_t *dt, int n) {
  unsigned long sz;
  char buf[8];
  char asc[17];
  int clm = 0;
  uint8_t data;
  uint8_t sum;
  uint8_t vsum[16];
  uint8_t total =0;
  int saddr =0;
  int eaddr =n-1;
  sz = eaddr - saddr;
  
  Serial.println(F("----------------------------------------------------------"));
  for (uint8_t i=0;i<16;i++) vsum[i]=0;  
  for (unsigned long addr = saddr; addr <= eaddr; addr++) {
    data = dt[addr];
    if (clm == 0) {
      sum =0;
      sprintf(buf,"%05lx: ",addr);
      Serial.print(buf);
    }

    sum+=data;
    vsum[clm]+=data;  
    sprintf(buf,"%02x ",data);
    Serial.print(buf);

    if (data <0x20 || (data >= 0x7f && data < 0xa0) || data >0xdf) {
      asc[clm] = '.'; 
    } else {
      asc[clm] = data;
    }
    
    clm++;
    if (clm == 16) {
      sprintf(buf,"|%02x",sum);
      Serial.print(buf);
      Serial.write(': ');
      asc[clm]=0;
      Serial.print(asc);      
      Serial.println();
      clm = 0;
    }    
  }
  if (clm)
    Serial.println();
    
  Serial.println(F("----------------------------------------------------------"));
  Serial.print("       ");
  for (uint8_t i=0; i<16;i++) {
    total+=vsum[i];
    sprintf(buf,"%02x ",vsum[i]);
    Serial.print(buf);
  }
  sprintf(buf,"|%02x ",total);
  Serial.print(buf);      
  Serial.println();
  Serial.println();
}

//
// 書込みデータのダンプリスト(内部EEPROM版)
//
void dump(void) {
  unsigned long sz;
  char buf[8];
  char asc[17];
  int clm = 0;
  uint8_t data;
  uint8_t sum;
  uint8_t vsum[16];
  uint8_t total =0;
  int saddr =0;
  int eaddr =1023;
  sz = eaddr - saddr;
  
  Serial.println(F("----------------------------------------------------------"));
  for (uint8_t i=0;i<16;i++) vsum[i]=0;  
  for (unsigned long addr = saddr; addr <= eaddr; addr++) {
    data = EEPROM[addr];
    if (clm == 0) {
      sum =0;
      sprintf(buf,"%05lx: ",addr);
      Serial.print(buf);
    }

    sum+=data;
    vsum[clm]+=data;  
    sprintf(buf,"%02x ",data);
    Serial.print(buf);

    if (data <0x20 || (data >= 0x7f && data < 0xa0) || data >0xdf) {
      asc[clm] = '.'; 
    } else {
      asc[clm] = data;
    }
    
    clm++;
    if (clm == 16) {
      sprintf(buf,"|%02x",sum);
      Serial.print(buf);
      Serial.write(': ');
      asc[clm]=0;
      Serial.print(asc);      
      Serial.println();
      clm = 0;
    }    
  }
  Serial.println(F("----------------------------------------------------------"));
  Serial.print("       ");
  for (uint8_t i=0; i<16;i++) {
    total+=vsum[i];
    sprintf(buf,"%02x ",vsum[i]);
    Serial.print(buf);
  }
  sprintf(buf,"|%02x ",total);
  Serial.print(buf);      
  Serial.println();
  Serial.println();
}
