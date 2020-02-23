#pragma once
#ifndef __MD5_H__
#define __MD5_H__

#include <stdint.h>

typedef struct MD5 MD5;
struct MD5 {
	/** state (ABCD)
	 * 四个32bits数，用于存放最终计算得到的消息摘要。当消息长度〉512bits时，也用于存放每个512bits的中间结果
	 */
	uint32_t state[4];

	/** number of bits, modulo 2^64 (lsb first) 
	 * 存储原始信息的bits数长度,不包括填充的bits，最长为 2^64 bits，因为2^64是一个64位数的最大值
	 */
	uint32_t count[2];

	/** input buffer
	 * 存放输入的信息的缓冲区，512bits
	 */
	uint8_t buffer[64];

	/** 初始化, 用于重新计算新值 */
	void (*init) (MD5 *this);

	/** 对input内容进行中间计算 */
	void (*update) (MD5 *this, const void *input, size_t len);

	/** 完成最终计算, 输出计算结果到digest中 */
	void (*final) (MD5 *this, uint8_t digest[16]);
};

/** 模拟C++的new 函数创建对象, 会自动调用对象的init函数 */
void md5_create(MD5 *value);

/** 计算md5值的快捷函数, 无需中间众多的转换步骤
 * @param dst 写入的地址
 * @param input 要计算的聂荣地址
 * @param len 要计算的长度(字节为单位)
 * @return 返回dst
 */
char* md5_string(char dst[33], const void *input, size_t len);

/** 转成16进制字符串
 * @param dst 写入的目标字符串地址
 * @param dstlen 目标地址最大可写长度
 * @param src 读取的源内容地址
 * @param srclen 读取的源内容的长度(字节为单位)
 * @return 写入长度, 返回0表示写入失败
 */
int to_hex(char *dst, size_t dstlen, const void *src, size_t srclen);

#endif /* __MD5_H__ */
