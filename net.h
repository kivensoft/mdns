#pragma once
#ifndef __NET_H__
#define __NET_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
#include <winsock2.h>

#else // no _WIN32
#include <arpa/inet.h>
#include <unistd.h>

#endif // _WIN32

#ifdef _WIN32
#	define close_socket(fd) closesocket(fd)
	typedef SOCKET socket_t;
	typedef int socklen_t;
	typedef struct in_addr in_addr_t;
	/** 初始化加载ws2_32.dll */
	bool init_socket();
#else // no _WIN32
#	define init_socket() 1
#	define close_socket(fd) close(fd)
	typedef int socket_t;
#endif //_WIN32

typedef struct sockaddr_in sockaddr_in_t;
typedef struct sockaddr sockaddr_t;

/** 创建udp监听服务 */
extern bool sock_recv_timeout(socket_t socket, uint32_t seconds);
extern bool sock_send_timeout(socket_t socket, uint32_t seconds);


#endif // __NET_H__