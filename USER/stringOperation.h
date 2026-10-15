#ifndef 	__STRING_OPERATION_H_
#define 	__STRING_OPERATION_H_


// 函数声明
//=============================================================
// 整形数转成字符串
void STRING_IntToStr(uint32 param, uint8 *str);

// 字符串转数值
uint32 STRING_StrToVal(uint8 *str);

// 将两个字符串连接在一起
void STRING_CatStr(uint8 *dst, uint8 *src);

// 在指定的字符串中搜索子串，存在则返回字符串中首次出现子串的地址，否则返回空指针
int16 STRING_Find(const uint8 *mStr, const uint8 *subStr);

// 查找字符串中指定字符，成功返回该字符首次出现的下标位置，否则返回-1
int16 STRING_FindCh(const uint8 *str, uint8 ch);

// 查找字符串中指定字符的个数
int16 STRING_CalculateCh(const uint8 *str, uint8 ch);

// 判断两个字符串是否相等
BOOL STRING_CmpStr(const uint8 *str1, const uint8 *str2);

// 删除指定字符串中的指定字符，返回成功删除的个数
uint16 STRING_DeleteCh(uint8 str[], uint8 ch);

// 从指定的字符串的末尾1开始获取从右向左前 count 个字符
void STRING_Right(const char *str, uint8 count, char arr[]);

// 从指定的字符串的开始位置，截取count个字符为子串
void STRING_Left(const uint8 *str, uint8 count, uint8 arr[]);

// 从指定的字符串的指定下标位置开始，截取中间count个字符作为子串
void STRING_Mid(const uint8 *str, uint8 startNum, int8 count, uint8 arr[]);

// 从指定的字符串的第一个字符到第二个字符作为一个新的字串
BOOL STRING_FromTo(const uint8 *str, uint8 firstChar, uint8 secondChar, uint8 arr[]);

// 得到指定序号的逗号位置
int16 STRING_GPS_GetCommaPosition(const uint8 *str, uint16 num);

// 得到指定序号的第m个逗号和n个逗号之间的字符作为新字符串
BOOL STRING_GPS_FormCommaStr(const uint8 *str, uint16 mNum, uint16 nNum, uint8 arr[]);

uint8 STRING_CmpareN(const uint8 *str1, const uint8 *str2, uint8 len);

uint8 STRING_FindLimitSpeedh(uint8 *str, uint8 ch);
#endif

