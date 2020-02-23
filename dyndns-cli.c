#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "net.h"
#include "getopt.h"
#include "log.h"
#include "md5.h"

#define DN_MAX 64
#define KEY_MAX 128
#define IP_MAX 16
#ifndef _MAX_FNAME
#	define _MAX_FNAME 256
#endif

//命令行参数
typedef struct {
	bool help;	                  // 显示帮助
	bool debug;                   // 调试模式
	int  port;	                  // DNS服务器端口
	char server[DN_MAX];          // DNS服务器地址
	char host[DN_MAX];            // 指定更新的域名
	char key[KEY_MAX];	          // 动态更新DNS的密钥
	char logfile[_MAX_FNAME];     // 日志文件名
	char ip[IP_MAX];              // 指定更新的IP
} config_t;

const char APP[] = "dyndns-cli";
const char MAGIC[] = "ddyn";
static char HEX[] = "0123456789abcdef";

config_t g_conf = { .port = 53,
#ifdef _WIN32
	.logfile = "dyndns-cli.log"
#else
	.logfile = "/var/log/dyndns-cli.log"
#endif
};

char g_buf[512];

inline static const char* _b2s(bool b) {
	return b ? "true" : "false";
}

inline static void _uint64_to_hex(char dst[16], uint64_t val) {
	uint32_t v1 = val >> 32, v2 = val;
	dst[ 0] = HEX[v1 >> 28];
	dst[ 1] = HEX[(v1 >> 24) & 0xf];
	dst[ 2] = HEX[(v1 >> 20) & 0xf];
	dst[ 3] = HEX[(v1 >> 16) & 0xf];
	dst[ 4] = HEX[(v1 >> 12) & 0xf];
	dst[ 5] = HEX[(v1 >>  8) & 0xf];
	dst[ 6] = HEX[(v1 >>  4) & 0xf];
	dst[ 7] = HEX[v1 & 0xf];
	dst[ 8] = HEX[v2 >> 28];
	dst[ 9] = HEX[(v2 >> 24) & 0xf];
	dst[10] = HEX[(v2 >> 20) & 0xf];
	dst[11] = HEX[(v2 >> 16) & 0xf];
	dst[12] = HEX[(v2 >> 12) & 0xf];
	dst[13] = HEX[(v2 >>  8) & 0xf];
	dst[14] = HEX[(v2 >>  4) & 0xf];
	dst[15] = HEX[v2 & 0xf];
}

/** 使用帮助 */
static void usage() {
	printf("Usage: %s [OPTION]... -k <key> -h <host> -s <server>\n", APP);
	printf("dynamic dns update client, version 1.3, copyleft by kivensoft 2017-2020.\n\n");
	printf("Options:\n");
	printf("  -d                    enabled logger mode, default %s\n", _b2s(g_conf.debug));
	printf("  -g <log filename>     log file name, default %s\n", g_conf.logfile);
	printf("  -h <host>             dynamic update domain name, example: user.myip.com\n");
	printf("  -i <ip>               ip address for update, default auto detect\n");
	printf("  -k <key>              dynamic dns update key\n");
	printf("  -p <port>             DNS port port, default %d\n", g_conf.port);
	printf("  -s <server>           DNS server address, example: dns.myip.com\n");
}

/** 解析命令行参数 */
static bool parse_cmd_line(int argc, char **argv, config_t *dst) {
	int c;
	while ((c = getopt(argc, argv, "dg:h:i:k:p:s:?")) != -1) {
		switch (c) {
			case 'd':
				dst->debug = 1; break;
			case 'g':
				if (strlen(optarg) >= _MAX_FNAME) {
					printf("log file name too long!\n");
					return false;
				}
				strcpy(dst->logfile, optarg);
				break;
			case 'h':
				if (strlen(optarg) >= DN_MAX) {
					printf("host too long!\n");
					return false;
				}
				strcpy(dst->host, optarg);
				break;
			case 'i':
				if (strlen(optarg) >= IP_MAX) {
					printf("ip too long!\n");
					return false;
				}
				strcpy(dst->ip, optarg);
				break;
			case 'k':
				if (strlen(optarg) >= KEY_MAX) {
					printf("key too long!\n");
					return false;
				}
				strcpy(dst->key, optarg);
				break;
			case 'p':
				dst->port = atoi(optarg); break;
			case 's':
				if (strlen(optarg) >= DN_MAX) {
					printf("server too long!\n");
					return false;
				}
				strcpy(dst->server, optarg);
				break;
			case '?':
				dst->help = 1; break;
			default:
				printf("Try %s -? for more informaton.\n", APP);
				return false;
		}
	}
	return true;
}

static size_t _mk_dyndns_req() {
	char buf[512], *p = buf, *b = g_buf, digest[33];
	memcpy(p, MAGIC, sizeof(MAGIC));
	memcpy(b, MAGIC, sizeof(MAGIC));
	p += sizeof(MAGIC) - 1;
	b += sizeof(MAGIC) - 1;

	time_t t = time(NULL);
	_uint64_to_hex(p, t);
	memcpy(b, p, 16);
	p += 16;
	b += 16;

	char *nb = b + 32;

	size_t len = strlen(g_conf.host);
	memcpy(p, g_conf.host, len);
	memcpy(nb, g_conf.host, len);
	p += len;
	nb += len;

	if (g_conf.ip[0] != '\0') {
		*p++ = ' ';
		*nb++ = ' ';
		len = strlen(g_conf.ip);
		memcpy(p, g_conf.ip, len);
		memcpy(nb, g_conf.ip, len);
		p += len;
		nb += len;
	}

	len = strlen(g_conf.key);
	memcpy(p, g_conf.key, len);
	p += len;

	md5_string(digest, buf, p - buf);
	memcpy(b, digest, 32);

	return nb - g_buf;
}

int main(int argc, char **argv) {
	// 解析命令行参数
	if (!parse_cmd_line(argc, argv, &g_conf))
		return -1;

	// 如果参数是显示帮助
	if (g_conf.help) {
		usage();
		return 0;
	}
	// windows平台初始化winsocket
	if (!init_socket())
		return -1;

	// 配置日志
	if (g_conf.debug)
		log_set_file(g_conf.logfile, 1024 * 1024);

	// 创建socket
	socket_t fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == -1) {
		log_error("%s can't listen port %d", APP, g_conf.port);
		return -1;
	}

	// 设置读取超时时间
	sock_recv_timeout(fd, 5);

	// 设置对端网络地址与端口
	unsigned long ip = inet_addr(g_conf.server);
	sockaddr_in_t addr = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = ip,
		.sin_port = htons(g_conf.port)};
	socklen_t addrlen = sizeof(addr);

	int count = _mk_dyndns_req();
	// 发送数据
	count = sendto(fd, g_buf, count, 0, (sockaddr_t *) &addr, addrlen);
	g_buf[count] = 0;
	log_debug("send data: %s", g_buf);
	// 读取服务器响应
	count = recvfrom(fd, g_buf, sizeof(g_buf), 0, (sockaddr_t *) &addr, &addrlen);
	close_socket(fd);
	if (count < 0) {
		log_debug("udp connection timeout");
		return 0;
	}
	log_dump_text("recv data: ", g_buf, count);

	return 0;
}
