// #pragma comment(lib, "advapi32.lib", "ws2_32.lib")
#define _SVID_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
#include "winsvr.h"
#else
#define __USE_BSD
#include <unistd.h>
#endif // _WIN32

#include "getopt.h"
#include "log.h"
#include "net.h"
#include "dnsdb.h"
#include "dnsproto.h"
#include "dyndns.h"

#ifndef _MAX_FNAME
#define _MAX_FNAME 256
#endif

#define KEY_LEN 33


#ifdef _WIN32
#	define _SVC_INST(x) if (x) { _svc_inst(); return 0; }
#	define _DAEMON(x) if(x) { svc_start(); return 0; }

	const char DEFAULT_LOG[] = "mdns.log";
	const char DEFAULT_CONF[] = "mdns.conf";
#elif __linux
#	define _SVC_INST(x) ((void)0)
#	define _DAEMON(x) if (x) daemon(1, 0)

	const char DEFAULT_LOG[] = "/var/log/mdns.log";
	const char DEFAULT_CONF[] = "/etc/mdns.conf";
#endif

const char DEFAULT_KEY[] = "Mini DNS Server";

/** 收发dns消息的两个变量 */
static uint8_t g_recv[DNS_PACKET_MAX], g_reply[DNS_PACKET_MAX];

//命令行参数
typedef struct config {
	bool  help;	    // 显示帮助
	int   level;    // 日志级别
	bool  daemon;   // linux下的守护进程模式
	bool  inst;	    // 安装wndows服务
	int   port;	    // DNS监听端口
	char* dbfile;   // DNS数据库文件名
	char* logfile;  // LOG文件名
	char* key;	    // DNS动态域名更新密钥
} config_t;

config_t g_conf = { .help = 0, .level = LOG_DEBUG, .daemon = 0, .inst = 0, .port = 53, .dbfile = (char*)DEFAULT_CONF, .key = (char*)DEFAULT_KEY };

/** 提供给dns动态更新协议的回调函数接口 */
static bool dyndns_update(const char* name, uint32_t ip) {
	bool ret = dnsdb_update(name, ip);
	if (ret) dnsdb_save();
	return ret;
}

inline static const char* b2s(bool b) {
	return b ? "true" : "false";
}

/** 使用帮助 */
static void usage() {
	printf("Usage: mdns [OPTION]...\n");
	printf("mini dns server, version 1.34, copyleft by kivensoft 2017-2021.\n\n");
	printf("Options:\n");
	printf("  -d                    run daemon mode, default %s\n", b2s(g_conf.daemon));
	printf("  -f <db filename>      dns db file name, default %s\n", DEFAULT_CONF);
	printf("  -g <log filename>     log file name, default %s\n", DEFAULT_LOG);
	printf("  -i                    install service, warning: windows only\n");
	printf("  -k <key>              dynamic dns update key, default %s\n", DEFAULT_KEY);
	printf("  -l <log level>        set log level, default debug\n");
	printf("  -p <port>             listen dns port, default %d\n", g_conf.port);
}

/** 解析命令行参数 */
static bool parse_cmd_line(int argc, char **argv, config_t *dst) {
	int c;
	while ((c = getopt(argc, argv, "df:g:ikl:p:?")) != -1) {
		switch (c) {
			case 'd':
				dst->daemon = 1; break;
			case 'f': dst->dbfile = strdup(optarg); break;
			case 'g': dst->logfile = strdup(optarg); break;
			case 'i': dst->inst = 1; break;
			case 'k': dst->key = strdup(optarg); break;
			case 'l': dst->level = log_get_level(optarg); break;
			case 'p': dst->port = atoi(optarg); break;
			case '?': dst->help = 1; break;
			default:
				puts("Try mdns -? for more informaton.");
				return false;
		}
	}

	return true;
}

static bool init() {
	// 配置日志
	log_start(g_conf.logfile, 1024 * 1024);
	log_set_level(g_conf.level);
	log_debug("optons daemon=%d, level=%d, dns_port=%d, dbname=%s, logfile=%s",
			g_conf.daemon, g_conf.level, g_conf.port, g_conf.dbfile, g_conf.logfile);

	// windows平台初始化winsocket
	socket_init();

	// 加载域名记录
	if (!dnsdb_load(g_conf.dbfile)) {
		log_error("init db file failed!");
		return false;
	}

	// 初始化dns协议的回调接口配置
	dns_init(dnsdb_find);

	// 初始化动态dns协议配置, 配置动态更新ip的回调函数
	dyndns_init(g_conf.key, dyndns_update);

	return true;
}

static socket_t create_udp_server(const char *host, uint16_t port) {
	socket_t fd = socket(AF_INET, SOCK_DGRAM, 0);
	unsigned long ip = host != NULL ? inet_addr(host) : INADDR_ANY;
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = htonl(ip),
		.sin_port = htons(port)};
	if (bind(fd, (struct sockaddr *)(&addr), sizeof(addr)) == -1) {
		socket_close(fd);
		fd = -1;
	}
	return fd;
}

int run() {
	// 监听dns服务端口
	socket_t fd = create_udp_server(NULL, g_conf.port);
	if (fd == -1) {
		log_error("mini dns can't listen port %d", g_conf.port);
		return -1;
	}
	log_debug("mini dns listen port %d", g_conf.port);

	sockaddr_in_t addr;
	socklen_t addrlen = sizeof(addr);
	int recv_count, reply_count, dump_flag = 0;

	// 进入服务处理模式
	while (1) {
		recv_count = recvfrom(fd, (char*)g_recv, sizeof(g_recv), 0, (sockaddr_t *) &addr, &addrlen);
		if (recv_count <= 0) {
			log_debug("udp recive count is 0, ignore");
			continue;
		}

		log_hex(LOG_TRACE, "dns recived data:", g_recv, recv_count);

		// 先使用动态dns协议判断是否动态dns更新协议
		reply_count = dyndns(&addr, (const char*)g_recv, recv_count, (char*)g_reply, sizeof(g_reply));

		// 不是动态dns协议报文, 转到正常dns处理
		if (reply_count == -1) {
			dump_flag = 1;
			reply_count = dns_process(g_recv, recv_count, g_reply);
		}

		// 处理结果有应答包, 则进行发送, 否则, 可能是外部攻击, 忽略
		if (reply_count > 0)
			sendto(fd, (char*)g_reply, reply_count, 0, (sockaddr_t *)&addr, addrlen);

		if (dump_flag) {
			dump_flag = 0;
			if (reply_count > 0)
				log_hex(LOG_TRACE, "dns answer data:", g_reply, reply_count);
			else
				log_info("dns drop this message, no reply!");
		}
	}

	return 0;
}

#ifdef _WIN32
static int _svc_inst() {
	char buf[1024], *p = buf;

	*p++ = '-';
	*p++ = 'd';
	*p++ = ' ';

	sprintf(p, "-p %d -f \"%s\" -g \"%s\"", g_conf.port, g_conf.dbfile, g_conf.logfile);

	svc_install(buf);
	return 0;
}
#endif

int main(int argc, char **argv) {
	// 解析命令行参数
	if (!parse_cmd_line(argc, argv, &g_conf))
		return -1;

	// 如果参数是显示帮助
	if (g_conf.help) {
		usage();
		return 0;
	}

	// windows平台, 添加服务安装代码
	_SVC_INST(g_conf.inst);

	// 初始化应用程序配置
	if (false == init())
		return -1;

	// linux平台, 切换成守护模式, windows平台, 启动服务
	_DAEMON(g_conf.daemon);

	return run();
}
