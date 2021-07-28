#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include "log.h"
#include "dnsproto.h"

#define DNS_HEAD_LEN 12
#define TTL 60

typedef const uint8_t* pcuint8_t;

// dns查询类型定义
enum dns_qt_t {
	DNS_QT_A = 1,
	DNS_QT_NS = 2,
	DNS_QT_CNAME = 5,
	DNS_QT_PTR = 12,
	DNS_QT_MX = 15,
	DNS_QT_AAAA = 28
};

// dns返回码定义
enum dns_rcode_t {
	DNS_RCODE_OK = 0,
	DNS_RCODE_QUERY_ERROR = 1,
	DNS_RCODE_SVR_FAILURE = 2,
	DNS_RCODE_NAME_ERROR = 3
};

/** dns查询问题结构 */
typedef struct dns_query_t {
	uint32_t	ip;					// 查询结果，记录到此
	uint16_t	offset;				// 查询在请求报文中的偏移地址，用于创建应答报文时的地址引用
	uint16_t	type;              	// 查询类型
	uint16_t	class;             	// 查询类, 通常为1, 固定为internet类
	char 		host[HOST_MAX];   	// 域名
} dns_query_t;

static uint32_t (*g_dns_find_func) (const char* host) = NULL;

void dns_init(uint32_t (*find_func) (const char* host)) {
	g_dns_find_func = find_func;
}

/** 校验报文长度是否有效, 最小需要12个字节以上 */
inline static bool dns_check_len(size_t len) { return len > DNS_HEAD_LEN; }

/** 校验报文是否dns查询报文, 0: 查询, 1: 响应 */
inline static unsigned dns_get_query(pcuint8_t req) { return ((req[2] >> 7) & 0x1); }

/** 获取报文操作码, 4位, 0:标准查询, 1:反向查询, 2: 服务器状态请求 */
inline static unsigned dns_get_opcode(pcuint8_t req) { return (req[2] >> 3) & 0xF; }

/** 获取报文要查询的域名数量 */
inline static unsigned dns_get_questions(pcuint8_t req) { return ntohs(*(uint16_t*)(req + 4)); }

/** 读取dns请求的queries查询区域
 * @param data 要读取起始地址(非报文起始地址, 第一次读取, 应该是queries区域的起始地址)
 * @param data_end 结尾地址，即允许读取的最大地址加1
 * @param dst 回写域名查询请求结构地址
 * @return 返回data的下一次读取地址, 返回NULL表示读取失败, 可能是格式有误或其它错误
 */
static const uint8_t* dns_get_queries(pcuint8_t data, pcuint8_t data_end, dns_query_t *dst) {
	// 传入参数错误
	if (data >= data_end) {
		log_warn("%s error: data[%" PRIxPTR "] >= data_end[%" PRIxPTR "]!", __func__, data, data_end);
		return NULL; 
	}

	size_t pos = 0, len;
	char *host = dst->host;

	while (data < data_end && pos < HOST_MAX - 1) {
		// 读取域名分段长度
		len = (size_t)(*data++);
		
		// 读取到长度为0，表明域名读取结束
		if (!len) {
			// 最后一个字符是'.'，改成0
			host[pos - 1] = '\0';
			break;
		}
		
		// 域名长度超出报文长度，读取失败
		if (data + len > data_end) {
			log_warn("%s error: read domain name error, prefix len[%u] invalid!", __func__, (uint32_t)len);
			return NULL;
		}

		// 将分段域名写入host
		while (len--) host[pos++] = *data++;
		host[pos++] = '.';
	}

	// 读取查询类型type和查询类class
	dst->type = ntohs(*(uint16_t*)data);
	dst->class = ntohs(*(uint16_t*)(data + 2));

	return data + 4;
}

/** 创建dns响应报文的头部, 共12个字节
 * @param res 响应报文地址
 * @param req 请求报文地址
 * @param rcode 响应报文的返回码值
 * @param ancount 响应报文的回答区域数量
*/
static void dns_build_header(pcuint8_t req, uint8_t *res, uint16_t rcode, uint16_t ancount) {
	// 头部12字节清零
	*(uint64_t*)res = 0, *(uint32_t*)(res + 8) = 0;

	// 0,1 两字节为id，从请求中获取
	*(uint16_t*)res = *(const uint16_t*)req;

	// 2,3 两字节为标志位，设置为(高位到低位) QR(1)_0000_AA(1)_00_0000_RCODE(4)
	// QR: 0: 查询, 1: 应答, AA 1: 授权回答, 0: 非授权回答,  RCODE 响应码
	*(res + 2) = 0x84; // 0x84 = 10000100, 应答报文，且是授权应答
	*(res + 3) = (uint8_t) rcode;

	// 4,5,6,7,8,9,10,11为4个两字节长度的（请求、回答、授权、附加）数量
	*(uint16_t*)(res + 4) = *(const uint16_t*)(req + 4);
	*(uint16_t*)(res + 6) = htons(ancount);
}

/** 把查询内容写入到响应报文中, 返回写入查询内容后的总报文长度 */
static uint16_t dns_copy_queries(pcuint8_t req, uint8_t *res) {
	uint16_t count = (uint16_t) dns_get_questions(req), pos = DNS_HEAD_LEN;
	while (count--) {
		while ((res[pos] = req[pos]))
			for (uint16_t len = req[pos++], max = pos + len; pos < max; ++pos)
				res[pos] = req[pos];
		pos++;
		*(uint32_t*)(res + pos) = *(uint32_t*)(req + pos);
		pos += 4;
	}
	return pos;
}

/** 创建一个域名错误的回答, 返回生成的报文长度 */
inline static uint16_t dns_build_fail(pcuint8_t req, uint8_t* res, uint16_t rcode) {
	dns_build_header(req, res, rcode, 0);
	return dns_copy_queries(req, res);
}

/** 构建一个查询响应内容
 * @param reply 写入响应内容的起始地址
 * @param reply_end 最大可写入的结束地址
 * @param query 查询问题结构
 * @param ip 写入的响应ip地址
 * @return 写入长度
 */
static uint16_t dns_build_answer(uint8_t *data, uint8_t *data_end, const dns_query_t *query) {
	if (data_end - data < 16) return 0;
	uint16_t off = query->offset | 0xC000; // 1100_0000_0000_0000
	*(uint16_t*)(data) = htons(off);
	*(uint16_t*)(data + 2) = htons(query->type);
	*(uint16_t*)(data + 4) = htons(query->class);
	*(uint32_t*)(data + 6) = htonl(TTL);

	*(data + 10) = 0;
	*(data + 11) = 4;
	*(uint32_t*)(data + 12) = query->ip;
	return 16;
}

uint16_t dns_process(const void *req, size_t req_size, uint8_t res[DNS_PACKET_MAX]) {
	// 判断报文长度
	if (!dns_check_len(req_size)) {
		log_warn("dns request length[%" PRIu64 "] too small!", (uint64_t)req_size);
		return 0;
	}

	// 获取报文类型，判断是否查询请求, 0: 查询, 1: 响应
	if (dns_get_query(req)) {
		log_warn("dns request not query type!");
		return 0;
	}

	// 获取操作码, 0: 标准查询, 1: 反向查询, 2: 服务器状态请求
	unsigned opcode = dns_get_opcode(req);
	if (opcode > 1) {
		log_warn("dns request opcode[%d] unsupport!", opcode);
		return 0;
	}

	// 获取查询数量, 当前暂时只支持1个域名的查询, 一次性查多个域名暂不支持
	int questions = dns_get_questions(req);
	if (questions != 1) {
		log_warn("dns request multiple questions[%u] unsupport!", questions);
		return dns_build_fail(req, res, DNS_RCODE_NAME_ERROR);
	}

	// 解析查询请求
	const uint8_t *rp;
	dns_query_t quer = {.offset = DNS_HEAD_LEN, .ip = INADDR_NONE};
	rp = dns_get_queries(req + DNS_HEAD_LEN, req + req_size, &quer);
	if (rp == NULL) {
		log_warn("dns request queries format error!");
		return dns_build_fail(req, res, DNS_RCODE_NAME_ERROR);
	}
	// 解析查询请求中的域名信息错误或者请求的查询类型不是ipv4地址解析类型
	if (quer.type != DNS_QT_A) {
		log_warn("dns request query type unsupport: %s [type=%u,class=%u]",
				quer.host, quer.type, quer.class);
		return dns_build_fail(req, res, DNS_RCODE_NAME_ERROR);
	}
	log_debug("dns request query: %s [type=%u, class=%u]", quer.host, quer.type, quer.class);

	// 对成功解析的请求进行响应
	quer.ip = g_dns_find_func(quer.host);
	if (quer.ip == INADDR_NONE) {
		log_warn("dns query result: can't find %s", quer.host);
		return dns_build_fail(req, res, DNS_RCODE_NAME_ERROR);
	}

	// 生成应答包
	dns_build_header(req, res, 0, 1);
	uint16_t hlen = dns_copy_queries(req, res);
	uint16_t alen = dns_build_answer(res + hlen, res + DNS_PACKET_MAX - hlen, &quer);
	if (!alen) {
		log_warn("dns answer write res memory no enough.");
		return dns_build_fail(req, res, DNS_RCODE_SVR_FAILURE);
	}
	log_debug("dns anwser: %s -> %s", quer.host, net_ip_tostring(quer.ip));

	return hlen + alen;
}
