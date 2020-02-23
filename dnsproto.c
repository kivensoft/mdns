#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "log.h"
#include "dnsproto.h"

#define DOMAIN_NAME_MAX 64
#define DNS_HEAD_LEN 12
#define TTL 60

// dns查询类型定义
#define DNS_QT_A        1
#define DNS_QT_NS       2
#define DNS_QT_CNAME    5
#define DNS_QT_PTR      12
#define DNS_QT_MX       15
#define DNS_QT_AAAA     28

// dns返回码定义
#define DNS_RCODE_OK          0
#define DNS_RCODE_SVR_FAILURE 2
#define DNS_RCODE_NAME_ERROR  3

/** dns查询问题结构 */
typedef struct {
	uint16_t offset;              // 域名在报文中的偏移地址(从0开始计算)
	uint16_t type;                // 查询类型
	uint16_t class;               // 查询类, 通常为1, 固定为internet类
	char name[DOMAIN_NAME_MAX];   // 域名
} dns_query_t;

static dns_find_func g_dns_find_func = (dns_find_func) NULL;

void dns_init(dns_find_func func) {
	g_dns_find_func = func;
}

/** 创建dns响应报文的头部
 * @param reply 响应报文地址
 * @param msg 请求报文地址
 * @param rcode 响应报文的返回码值
 * @param ancount 响应报文的回答区域数量
*/
static void _dns_build_header(uint8_t *reply, const uint8_t *msg, uint16_t rcode, uint16_t ancount) {
	// 保留0,1,4,5字节与请求内容一致
	memcpy(reply, msg, 6);

	// 保留opcode, 设置响应标志(QR)及授权回答标志(AA)
	reply[2] = (msg[2] & 0x78) | 0x84;	 // 01111000 | 10000100
	// 设置返回码
	reply[3] = rcode;

	// 设置回答区域数量
	*(uint16_t*)(reply + 6) = htons(ancount);

	// 授权区域数量与附加区域数量均设置为0
	memset(reply + 8, 0, 4);
}

/** 把查询内容复制到响应报文中 */
inline static bool _dns_build_queries(uint8_t *reply, size_t reply_size, const uint8_t *msg, size_t msg_size) {
	if (reply_size < msg_size) return false;
	memcpy(reply + DNS_HEAD_LEN, msg + DNS_HEAD_LEN, msg_size - DNS_HEAD_LEN);
	return true;
}

/** 创建一个域名错误的回答 */
inline static size_t _dns_build_reply(uint8_t *reply, size_t reply_size,
		const uint8_t *msg, size_t msg_size, uint16_t rcode) {
	_dns_build_header(reply, msg, rcode, 0);
	_dns_build_queries(reply, reply_size, msg, msg_size);
	return msg_size;
}

/** 校验报文长度是否有效, 最小需要12个字节以上 */
inline static bool _dns_len_valid(size_t len) {
	return len > DNS_HEAD_LEN;
}

/** 校验报文是否dns查询报文, 0:查询, 1:响应 */
inline static bool _dns_is_query(const uint8_t *data) {
	return (data[2] & 0x80) == 0;
}

/** 获取报文操作码, 4位, 0:标准查询, 1:反向查询, 2: 服务器状态请求 */
inline static unsigned _dns_opcode(const uint8_t *data) {
	return (data[2] >> 3) & 0xf;
}

/** 获取报文要查询的域名数量 */
inline static uint16_t _dns_questions(const uint8_t *data) {
	return ntohs(*((uint16_t*)(data + 4)));
}

/** 读取dns请求的queries查询区域
 * @param data 要读取起始地址(非报文起始地址, 第一次读取, 应该是queries区域的起始地址)
 * @param max data最大可读取数量
 * @param dst 回写域名查询请求结构地址
 * @return 返回data的下一次读取地址, 返回NULL表示读取失败, 可能是格式有误或其它错误
 */
static const uint8_t* _dns_queries(const uint8_t *data, const uint8_t *data_end, dns_query_t *dst) {
	int max = data_end - data;
	char *p = dst->name;
	uint8_t len, total = 0, first = 1;
	while(data < data_end && (len = *data++) != 0) {
		total += len + 1;
		// 域名超过64个字符, 报错退出函数
		if (total > DOMAIN_NAME_MAX || total > max) {
			log_debug("read dns queries fail: too long");
			return NULL;
		}
		if (first) first = 0;
		else *p++ = '.';
		// 复制分段的域名内容
		memcpy(p, data, len);
		data += len;
		p += len ;
	}
	*p = '\0';

	// 读取查询类型type和查询类class
	dst->type = ntohs(*(uint16_t*)data);
	dst->class = ntohs(*(uint16_t*)(data + 2));

	return data + 4;
}

/** 构建一个查询响应内容
 * @param reply 写入响应内容的起始地址
 * @param reply_end 最大可写入的结束地址
 * @param query 查询问题结构
 * @param ip 写入的响应ip地址
 * @return 下一次写入地址, 如果写入失败, 返回null, 通常是因为空间不足
 */
static uint8_t* _dns_build_answer(uint8_t *data, uint8_t *data_end, const dns_query_t *query, uint32_t ip) {
	if (data_end - data < 16) return NULL;
	uint16_t off = query->offset | 0xC000; // 1100_0000_0000_0000
	*(uint16_t*)(data) = htons(off);
	*(uint16_t*)(data + 2) = htons(query->type);
	*(uint16_t*)(data + 4) = htons(query->class);
	*(uint32_t*)(data + 6) = htonl(TTL);

	*(data + 10) = 0;
	*(data + 11) = 4;
	*(uint32_t*)(data + 12) = ip;
	return data + 16;
}

int dns_process(const void *msg, size_t msg_size, void *reply, size_t reply_size) {
	// 长度错误或者非查询请求 忽略
	if (!_dns_len_valid(msg_size) || !_dns_is_query(msg)) {
		log_debug("dns request length[%d] too small.", msg_size);
		return 0;
	}

	// 获取操作码, 0: 标准查询, 1: 反向查询, 2: 服务器状态请求
	int opcode = _dns_opcode(msg);
	if (opcode > 1) {
		log_debug("dns request opcode[%d] unsupport.", opcode);
		return 0;
	}

	// 获取查询数量, 当前暂时只支持1个域名的查询, 一次性查多个域名暂不支持
	int questions = _dns_questions(msg);
	if (questions != 1) {
		log_debug("dns request questions[%d] unsupport.", questions);
		return _dns_build_reply(reply, reply_size, msg, msg_size, DNS_RCODE_NAME_ERROR);
	}

	// 解析查询请求
	const uint8_t *rp;
	dns_query_t quer;
	quer.offset = DNS_HEAD_LEN; //rp - (uint8_t*) msg;
	rp = _dns_queries(msg + DNS_HEAD_LEN, msg + msg_size, &quer);
	if (rp == NULL) {
		log_debug("dns request queries format error");
		return _dns_build_reply(reply, reply_size, msg, msg_size, DNS_RCODE_NAME_ERROR);
	}
	// 解析查询请求中的域名信息错误或者请求的查询类型不是ipv4地址解析类型
	if (quer.type != DNS_QT_A) {
		log_debug("dns request query type unsupport: %s [type=%d,class=%d]",
				quer.name, quer.type, quer.class);
		return _dns_build_reply(reply, reply_size, msg, msg_size, DNS_RCODE_NAME_ERROR);
	}
	log_debug("dns request query: %s [type=%d, class=%d]",
			quer.name, quer.type, quer.class);

	// 对成功解析的请求进行响应
	uint8_t *wp;
	uint32_t ip = g_dns_find_func(quer.name);
	if (ip == 0) {
		log_debug("dns can't find %s ip", quer.name);
		return _dns_build_reply(reply, reply_size, msg, msg_size, DNS_RCODE_NAME_ERROR);
	}
	wp = _dns_build_answer(reply + msg_size, reply + reply_size, &quer, ip);
	if (wp == NULL) {
		log_debug("dns answer write reply memory no enough.");
		return _dns_build_reply(reply, reply_size, msg, msg_size, DNS_RCODE_SVR_FAILURE);
	}
	#ifndef NLOG
		struct in_addr addr;
		addr.s_addr = ip;
		log_debug("dns anwser: %s -> %s", quer.name, inet_ntoa(addr));
	#endif

	// 构建响应头
	_dns_build_header(reply, msg, DNS_RCODE_OK, (uint16_t) 1);
	// 构建响应内容的请求部分
	_dns_build_queries(reply, reply_size, msg, msg_size);

	return wp - (uint8_t*) reply;
}
