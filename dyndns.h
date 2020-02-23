#pragma once
#ifndef __DYNDNS_H__
#define __DYNDNS_H__

#include <stdbool.h>

#ifdef _WIN32
#	include <winsock2.h>
#else // no _WIN32
#	include <arpa/inet.h>
#	include <unistd.h>
#endif // _WIN32

typedef bool (*dyndns_upd_func) (const char* domain_name, uint32_t ip);

/** 初始化设置更新域名ip映射的回调函数 */
extern void dyndns_init(const char *key, dyndns_upd_func func);

/** dns动态更新处理函数
 * @param addr 客户端连接的socket信息
 * @param msg 消息报文地址
 * @param msg_size 消息报文长度
 * @param reply 回复报文地址, 如果是动态更新协议, 回复内容将写入此处
 * @param reply_size 回复报文地址最大可写入长度
 * @return 动态更新协议, 返回回写reply的内容长度, 否则返回-1
 */
extern int dyndns(const struct sockaddr_in *addr, const char *msg, size_t msg_size,
		char *reply, size_t reply_size);

#endif // __DYNDNS_H__