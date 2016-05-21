
// プロトタイプ宣言
extern void dump(byte* dt, int);
extern void disp_prog(uint8_t*);
extern void dump(void);
extern void disp_prog(void);
extern bool SD_init(void);
extern void setFileNameFromAddress(char* , uint8_t, uint16_t);
extern bool SD_open_file_for_write(char*);
extern bool SD_open_file_for_read(char*);
extern void SD_close_file(void);
extern bool SD_write_to_file(uint16_t, uint8_t*, uint8_t);
extern bool SD_read_from_file(uint16_t, uint8_t*, uint8_t);
extern bool SD_write_to_file_as_text(uint8_t*, uint8_t);
extern int16_t readLine(uint8_t*, uint8_t) ;
extern int16_t text2bin(uint8_t*, uint8_t);
extern bool SD_read_from_file_as_text(uint16_t, uint8_t*, uint8_t);

// 定数宣言
#define MAXTEXTLEN 128          // IcigoJamプログラムの1行最大長 
#define PCTSIZE    32           // IcigoJam I2C通信パケット長
