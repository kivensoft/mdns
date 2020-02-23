#pragma once
#ifndef __DNSPROTO_H__
#define __DNSPROTO_H__

#include <stdint.h>

#ifdef _WIN32
#	include <winsock2.h>
#else // NO _WIN32
#	include <arpa/inet.h>
#endif // _WIN32

typedef uint32_t (*dns_find_func) (const char* name);

/** dns协议解析服务初始化函数
 * @param func 域名查找回调接口地址
 */
extern void dns_init(dns_find_func func);

/** dns解析处理函数, 解析dns报文, 查找域名, 填充返回内容
 * @param msg dns报文地址
 * @param msg_size 报文长度
 * @param reply 写入回复消息的地址
 * @param reply_size 写入回复消息地址的可写长度
 * @return 写入长度, 0: 忽略消息, 无需回复
 */
extern int dns_process(const void *msg, size_t msg_size, void *reply, size_t reply_size);

#endif //__DNSPROTO_H__