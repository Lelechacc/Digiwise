#include "Common.h"
#include "stringOperation.h"

// ===================================================================================================
// 函数定义
// ===================================================================================================

// 整形数转成字符串
void STRING_IntToStr(uint32 param, uint8 *str)
{
	uint32 localParam = param;
	
	// param为1个字节的整数倍长度，预留最后一个字节存储'\0'，因此在基础上+1
	uint8 tempStr[(sizeof(param)*8+1)] = {0};
	
	// 临时字符串区的最后一个为0，代表字符串结尾，按照倒序方式，从倒数第二个位置开始向下标减小的方向填充字符
	// 数组个数为sizeof(param)*8+1时，下标为[0,sizeof(param)*8]，因此倒数第二个位置在[sizeof(param)*8+1-2]
	int8 index = (sizeof(param)*8+1) - 2;
	
	// 参数检验
	if (NULL == str)
	{
		return;
	}

	// 从倒数第二个位置开始，向下标减小的方向添加数据
	do{
		tempStr[index--] = (localParam%10) + '0';
		localParam /= 10;
	}while((0 != localParam)&&(index > -1));

	// 由于经过了上述的运算后，index实际指向了最前面的一个空位置，因此这里++，将其指向第一个非空字符
	index++;
	while(0 != tempStr[index])
	{
		*str++ = tempStr[index++];
	}
	*str = '\0';	// 添加结束符
}

// 字符串转数值
uint32 STRING_StrToVal(uint8 *str)
{
	uint32 value = 0;
	uint8 *localStr = str;
	
	// 参数检验
	if (NULL == str)
	{
		return 0;
	}
	
	while ('\0' != *localStr)
	{
		if(((*localStr) < '0') || ((*localStr) > '9'))
		{
			return 0;
		}

		value *= 10;
		value += (*localStr)-'0';
		localStr ++;
	}
	
	return value;
}


// 将两个字符串连接在一起
void STRING_CatStr(uint8 *dst, uint8 *src)
{
	uint16 i = 0;
	uint16 j = 0;
	
	// 参数检验
	if ((NULL == dst) || (NULL == src))
	{
		return;
	}

	while (dst[i] != '\0')
	{
		i++;
	}

	while (src[j] != '\0')
	{
		dst[i++] = src[j++];
	}
	dst[i+j] = '\0';	// 添加字符串结束标志'\0'
	
}


// 在指定的字符串中搜索子串，存在则返回字符串中首次出现子串的下标地址，否则返回-1
int16 STRING_Find(const uint8 *mStr, const uint8 *subStr)
{
	uint8 *pCh = (uint8*)mStr;		
	uint8 *str1, *str2;
	volatile uint16 i = 0;	 // 子串在母串中的下标

	// 参数检验
	if ((NULL == mStr) || (NULL == subStr))
	{
		return -1;
	}

	if ('\0' == *mStr)
	{
		return -1;
	}

	if ('\0' == *subStr)
	{
		return -1;
	}

	while (*pCh != '\0')
	{				 
		str1 = pCh; 			   
		str2 = (uint8*)subStr;
		
		while ((*str1 != '\0') && (*str2 != '\0') && (*str1 == *str2))
		{
			str1++; 
			str2++;
		}
		if (*str2 == '\0') 
		{
			i = pCh - mStr;

			return i;
		}
		
		pCh++;		  
	}
	
	return -1;
}

uint8 STRING_FindLimitSpeedh(uint8 *str, uint8 ch)
{
	uint8 i = 0;

	for (i = 0;i < 100;i++)
	{
		if (str[i] == ch)
		{
			return i;
		}
		i++;
	}
	return 0;
}

// 查找字符串中指定字符，成功返回该字符首次出现的下标位置，否则返回-1
int16 STRING_FindCh(const uint8 *str, uint8 ch)
{
	uint16 i = 0;
	
	// 参数检验
	if (NULL == str)
	{
		return -1;
	}

	while (str[i] != '\0')
	{
		if (str[i] == ch)
		{
			return i;
		}
		i++;
	}

	return -1;
}

// 查找字符串中指定字符的个数
int16 STRING_CalculateCh(const uint8 *str, uint8 ch)
{
	uint16 i = 0;
	uint16 cnt = 0;
	
	// 参数检验
	if (NULL == str)
	{
		return -1;
	}

	while (str[i] != '\0')
	{
		if (str[i] == ch)
		{
			cnt++;
		}
		i++;
	}

	return cnt;
}

// 判断两个字符串是否相等
BOOL STRING_CmpStr(const uint8 *str1, const uint8 *str2)
{
	uint8 *localStr1 = (uint8*)str1;
	uint8 *localStr2 = (uint8*)str2;
	
	// 参数检验
	if ((NULL == str1) || (NULL == str2))
	{
		return FALSE;
	}

	while ((*localStr1 != '\0') && (*localStr2 != '\0'))
	{
		// 不区分大小写，如果是字母，需统一转换为大写
		if ((*localStr1 >= 'a')&&(*localStr1 <= 'z'))
		{
			*localStr1 = *localStr1 - 32;
		}

		if ((*localStr2 >= 'a')&&(*localStr2 <= 'z'))
		{
			*localStr2 = *localStr2 - 32;
		}

		if (*localStr1++ != *localStr2++)
		{
			return FALSE;
		} 
	}

	if ((*localStr1 == '\0') && (*localStr2 == '\0'))
	{
		return TRUE;
	}

	return FALSE;
}

uint8 STRING_CmpareN(const uint8 *str1, const uint8 *str2, uint8 len)
{
	uint16 i = 0;

	while(i < len)
	{
		if(*(str1 +i) != *(str2 +i))
		{
			return 0;
		}
		i++;
	}
	return 1;
}


// 删除指定字符串中的指定字符，返回成功删除的个数
uint16 STRING_DeleteCh(uint8 str[], uint8 ch)
{
	uint16 i, j;
	uint16 cnt = 0;
	
	for (i = 0; str[i] != '\0'; i++)
	{
		if (str[i] == ch) 
		{
			for(j = i; str[j] != '\0'; j++) 
			{
				str[j] = str[j+1];
			}
			cnt++;
			i--;
		}		 
	}

	return cnt;
}

// 从指定的字符串的末尾1开始获取从右向左前 count 个字符
void STRING_Right(const char *str, uint8 count, char arr[])
{
	uint8 *localStr = (uint8*)str;
	uint8 length = strlen((char*)str);
	
	// 参数检验
	if (NULL == str)
	{
		return;
	}

	// 长度不能越界
	if (count > length)
	{
		count = length;
	}

	localStr += (length - count);	// 从右边第n个字符开始，到'\0'结束 

	while (*localStr != '\0')
	{
		*arr++ = *localStr++;
	}
	*arr = '\0';

}

// 从指定的字符串的开始位置，截取count个字符为子串
void STRING_Left(const uint8 *str, uint8 count, uint8 arr[])
{
	uint8 *localStr = (uint8*)str;
	uint8 length = strlen((char*)str);
	uint8 i;
	
	// 参数检验
	if (NULL == str)
	{
		return;
	}

	// 长度不能越界
	if (count > length)
	{
		count = length;
	}

	for (i = 0; i < count; i++)
	{
	   arr[i] = *localStr++; 
	}
	
	arr[i] = '\0';
}


// 从指定的字符串的指定下标位置开始，截取中间count个字符作为子串
void STRING_Mid(const uint8 *str, uint8 startNum, int8 count, uint8 arr[])
{
	uint8 *localStr = (uint8*)str;
	uint8 length = strlen((char*)str);
	
	// 参数检验
	if ((NULL == str) || (startNum > length))
	{
		return;
	}

	// 长度不能越界
	if (count > length)
	{
		count = length - startNum; // 从第m个到最后
	}

	localStr += startNum;

	if (count > 0)
	{
		while ((count--)&&((*localStr) != '\0'))
		{
		   *arr++ = *localStr++; 
		}
	}
	else
	{
		while ((*localStr) != '\0')
		{
		   *arr++ = *localStr++; 
		}
	}
	
	*arr = '\0';
}

// 从指定的字符串的第一个字符到第二个字符作为一个新的字串
BOOL STRING_FromTo(const uint8 *str, uint8 firstChar, uint8 secondChar, uint8 arr[])
{
	uint8 *localStr = (uint8*)str;
	uint8 *localarr = arr;
	uint8 length = strlen((char*)str);
	int16 j, k;
	uint16 i;

	// 参数检验
	if ((NULL == localStr) || ('\0' == firstChar) || ('\0' == secondChar))
	{
		return FALSE;
	}

	if (firstChar == secondChar)
	{
		// 数量不足2个
		if (STRING_CalculateCh(localStr, firstChar) < 2)
		{
			return FALSE;
		}
	}

	j = STRING_FindCh(localStr, firstChar);
	k = STRING_FindCh(localStr, secondChar);
	if ((-1 == j) || (-1 == k))
	{
		return FALSE;
	}
	
	// 取第二个字符之前的字符作为新字符串
	for (i = 0, ++j; localStr[j] != secondChar; i++, j++)
	{
		localarr[i] = localStr[j];
	}
	localarr[i] = '\0';

	return TRUE;
}

// 得到指定序号的逗号位置
int16 STRING_GPS_GetCommaPosition(const uint8 *str, uint16 num)
{
	uint16 i, j;
	uint16 len = strlen((char*)str);

	// 参数检验
	if ((NULL == str) || (num > len))
	{
		return -1;
	}
	
	for (i = 0, j = 0; i < len; i++)
	{
		if (str[i] == ',')
		{
			 j++;
		}

		if (j == num)
		{
			return i;
		}
	}
	
	return -1;
}

// 得到指定序号的第m个逗号和n个逗号之间的字符作为新字符串
BOOL STRING_GPS_FormCommaStr(const uint8 *str, uint16 mNum, uint16 nNum, uint8 arr[])
{
	int16 i, j; 
	uint16 len = strlen((char*)str);

	// 参数检验
	if ((NULL == str) || (mNum > len) || (nNum > len) || (mNum > nNum))
	{
		return FALSE;
	}

	// 找不到逗号
	i = STRING_GPS_GetCommaPosition(str, mNum);
	j = STRING_GPS_GetCommaPosition(str, nNum);
	if ((i < 0) || (j < 0))
	{
		return FALSE;
	}

	// 两个逗号之间无数据
	if (1 == (j - i))
	{
		return FALSE;
	}

	// 截取两个逗号之间的字符组成新字符串
	if (STRING_FromTo(&str[i], ',', ',', arr))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}







