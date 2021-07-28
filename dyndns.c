#define _SVID_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <inttypes.h>
#include "log.h"
#include "md5.h"
#include "dyndns.h"

#define DD_HEAD_LEN 4
#define DD_TIME_LEN 16
#define DD_MD5_LEN 32
#define DD_MIN_LEN (DD_HEAD_LEN + DD_TIME_LEN + DD_MD5_LEN)
#define DD_HOST_MAX 63
#define DD_IP_MAX 15
#define DD_MAX_LEN (DD_MIN_LEN + DD_HOST_MAX + DD_IP_MAX)

typedef enum { CHK_OK, TIME_INVALID, SIGN_INVALID } chk_err_t;

typedef struct {
	char time[DD_TIME_LEN + 1];
	char md5[DD_MD5_LEN + 1];
	char host[DD_HOST_MAX + 1];
	char ip[DD_IP_MAX + 1];
	char host_ip[DD_HOST_MAX + DD_IP_MAX + 1];
	uint32_t ip_num;
	uint64_t time_num;
} dyndns_request_t;

static const char g_magic[] = "ddyn";
static const char g_zero_ip[] = "0.0.0.0";
static dyndns_upd_func g_dyndns_upd_func = NULL;
static char *g_key = "Mini DNS Server";

inline static uint32_t _h2(char c, unsigned shift) {
	return (uint32_t) (c >= '0' && c <= '9' ? c - 48 : c - 87) << shift;
}

/** 将16进制的字符串转换为64位整数, 这里用32位来处理是为了兼容32位程序 */
static int64_t _hex_to_int64(const char text[DD_TIME_LEN]) {
	return ((uint64_t) (
			  _h2(text[0 ], 28) | _h2(text[1 ], 24)
			| _h2(text[2 ], 20) | _h2(text[3 ], 16)
			| _h2(text[4 ], 12) | _h2(text[5 ], 8 )
			| _h2(text[6 ], 4 ) | _h2(text[7 ], 0 )
			) << 32)
			| _h2(text[8 ], 28) | _h2(text[9 ], 24)
			| _h2(text[10], 20) | _h2(text[11], 16)
			| _h2(text[12], 12) | _h2(text[13], 8 )
			| _h2(text[14], 4 ) | _h2(text[15], 0 );
}

inline static bool _dyndns_chk_magic(const char *src, size_t src_len) {
	return (src_len >= DD_HEAD_LEN
			&& src[0] == 'd' && src[1] == 'd'
			&& src[2] == 'y' && src[3] == 'n');
}

/** 检查头部标志是否动态更新标志 */
inline static bool _dyndns_chk_valid(const char *src, size_t src_len) {
	return (src_len > DD_MIN_LEN && src_len <= DD_MAX_LEN + 1
			&& src[DD_MIN_LEN] != 0);
}

static chk_err_t _dyndns_chk_sign(const dyndns_request_t* req) {
	// 时间校验, 正负10分钟内都算有效
	time_t now = time(NULL);
	time_t cmp = (time_t) req->time_num;
	if (cmp < now - 600 || cmp > now + 600) {
		log_warn("dyndns request error: time invalid!");
		return TIME_INVALID;
	}

	// md5签名校验
	size_t key_len = strlen(g_key);
	size_t host_ip_len = strlen(req->host_ip);
	size_t buf_len = DD_HEAD_LEN + DD_TIME_LEN + host_ip_len + key_len;
	char sign[33], buf[buf_len];
	memcpy(buf, g_magic, DD_HEAD_LEN);
	memcpy(buf + DD_HEAD_LEN, req->time, DD_TIME_LEN);
	memcpy(buf + DD_HEAD_LEN + DD_TIME_LEN, req->host_ip, host_ip_len);
	memcpy(buf + DD_HEAD_LEN + DD_TIME_LEN + host_ip_len , g_key, key_len);
	md5_string(sign, buf, buf_len);
	if (0 != strcmp(req->md5, sign)) {
		log_warn("dyndns request error: md5 sign invalid!");
		return SIGN_INVALID;
	}

	return CHK_OK;
}

/** 解析请求内容, 按协议解析到dst中 */
inline static void _dyndns_parse_request(const struct in_addr *addr, const char *src,
		size_t size, dyndns_request_t* dst) {
	strncpy(dst->time, src + DD_HEAD_LEN, DD_TIME_LEN);
	dst->time[DD_TIME_LEN] = '\0';

	strncpy(dst->md5, src + DD_HEAD_LEN + DD_TIME_LEN, DD_MD5_LEN);
	dst->md5[DD_MD5_LEN] = '\0';

	strncpy(dst->host_ip, src + DD_MIN_LEN, DD_HOST_MAX + DD_IP_MAX);
	dst->host_ip[DD_HOST_MAX + DD_IP_MAX] = '\0';

	int surplus = size - DD_MIN_LEN;
	const char* p = memchr(src + DD_MIN_LEN, ' ', surplus);
	// 更新请求自带ip
	if (p != NULL) {
		int n = p - (src + DD_MIN_LEN);
		strncpy(dst->host, src + DD_MIN_LEN, n);
		dst->host[n] = '\0';
		// 剩余数量要减去一个空格
		n = surplus - (n + 1);
		strncpy(dst->ip, p + 1, n);
		dst->ip[n] = '\0';
		dst->ip_num = inet_addr(dst->ip);
	}
	// 更新请求没有ip, 取客户端连接地址的ip
	else {
		strncpy(dst->host, src + DD_MIN_LEN, surplus);
		dst->host[surplus] = '\0';
		strncpy(dst->ip, inet_ntoa(*addr), DD_IP_MAX);
		dst->ip[DD_IP_MAX] = '\0';
		dst->ip_num = addr->s_addr;
	}

	// 计算时间值
	dst->time_num = _hex_to_int64(dst->time);
}

/** 将报文内容转储到日志中的日志回调函数 */
inline static void _dyndns_dump(const char* data, size_t size, const dyndns_request_t* req) {
	char buf[32];
	time_t t = (time_t) req->time_num;
	strftime(buf, sizeof(buf) - 1, "%Y-%m-%d %H:%M:%S", localtime(&t));
	log_debug("dyndns time: %s (%s)", req->time, buf);
	log_debug("dyndns md5: %s", req->md5);
	log_debug("dyndns domain name: %s", req->host);
	log_debug("dyndns ip: %s", req->ip);
}


void dyndns_init(const char *key, dyndns_upd_func func) {
	if (key) g_key = strdup(key);
	g_dyndns_upd_func = func;
}

/** 动态dns更新协议
 *  协议格式:
 *     请求报文:
 *         1.固定4字节的头部: ddyn
 *         2.固定16字节的时间毫秒值16进制, 取1970-1-1开始到现在的秒值, 兼容unix的time_t
 *         3.固定32字节的md5值16进制表示, 算法: 时间毫秒值16进制字符串 + 域名 + 密钥
 *         4.动态长度的域名 + ip, 直到结尾, 域名与ip中间用空格隔开, ip为可选项,
 *               如果没有ip, 则默认取客户端连接地址的ip作为要更新的ip
 *               例子1: home.kivensoft.cn
 *               例子2: home.kivensoft.cn 180.89.75.42
 *     应答报文:
 *         1.域名 + ip, 动态长度, 空格间隔, 如果更新失败, ip部分返回 0.0.0.0
 */
int dyndns(const struct sockaddr_in *addr, const char *msg, size_t msg_size,
		char *reply, size_t reply_size) {
	// 校验失败, 不是动态更新协议, 忽略退出
	if (!_dyndns_chk_magic(msg, msg_size))
		return -1;

	log_text(LOG_TRACE, "recv dynamic dns update request: ", msg, msg_size);
	// 有效标志头, 但内容无效或参数无效, 返回0, 表示抛弃该消息
	if (!_dyndns_chk_valid(msg, msg_size)
			|| reply_size < DD_HOST_MAX + DD_IP_MAX + 2) {
		log_debug("dyndns bad request, ignore this request!");
		return 0;
	}

	dyndns_request_t req;
	memset(&req, 0, sizeof(req));
	_dyndns_parse_request(&(addr->sin_addr), msg, msg_size, &req);
	_dyndns_dump(msg, msg_size, &req);

	// 校验时间和MD5是否正确
	chk_err_t _chk_err;
	if (CHK_OK != (_chk_err = _dyndns_chk_sign(&req))) {
		if (TIME_INVALID == _chk_err)
			strcpy(reply, "error invalid time.");
		else
			strcpy(reply, "error invalid sign.");
	} else {
		const char *p;
		if (g_dyndns_upd_func && g_dyndns_upd_func(req.host, req.ip_num))
			p = req.ip;
		else
			p = g_zero_ip;

		sprintf(reply, "ok %s %s", req.host, p);
	}

	log_debug("dyndns reply: %s", reply);
	return strlen(reply);
}
