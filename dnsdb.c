#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#else // NO _WIN32
#include <arpa/inet.h>
#endif // _WIN32

#include "log.h"
#include "dnsdb.h"

// 可存放的记录总容量
#define DNS_DB_CAPACITY 128

#define _I2S_TMP(x) #x
#define _I2S(x) _I2S_TMP(x)

#define SCAN_FMT ("%" _I2S(DOMAIN_NAME_MAX) "s %" _I2S(DOMAIN_NAME_MAX) "s")

// 存放域名ip信息的结构
typedef struct {
	char host[DOMAIN_NAME_MAX];
	uint32_t ip;
} dns_rec_t;

// 域名ip存放数组, 全局变量
static dns_rec_t g_db_recs[DNS_DB_CAPACITY];
// 数据库改动标志
static bool g_db_modified = false;
// 数据库文件名
static char* g_db_filename = NULL;

/** 查找域名对应记录的索引位置, 找不到返回-1 */
static int _dns_db_indexof(const char* domain_name) {
	if (NULL != domain_name) {
		for (int i = 0; i < DNS_DB_CAPACITY && g_db_recs[i].host[0]; ++i) {
			if (strcmp(g_db_recs[i].host, domain_name) == 0)
				return i;
		}
	}
	return -1;
}

bool dns_db_load(const char* fname) {
	FILE* fp = fopen(fname, "r");
	if (fp == NULL) {
		log_error("%s can't open file %s to read!", __func__, fname);
		return false;
	}
	log_debug("load records from %s ...", fname);

	char host[DOMAIN_NAME_MAX], ip[DOMAIN_NAME_MAX];
	uint32_t ip_num;
	memset(g_db_recs, 0, sizeof(dns_rec_t) * DNS_DB_CAPACITY);

	for (unsigned i = 0; i < DNS_DB_CAPACITY; ++i) {
		if (fscanf(fp, SCAN_FMT, host, ip) != 2)
			break;
		log_debug("read record host=%s, ip=%s", host, ip);

		ip_num = inet_addr(ip);
		if (ip_num == INADDR_NONE) {
			log_warn("host %s ip address %s is invalid.", host, ip);
		} else {
			strcpy(g_db_recs[i].host, host);
			g_db_recs[i].ip = ip_num;
		}
	}

	fclose(fp);
	g_db_modified = 0;
	g_db_filename = malloc(strlen(fname) + 1);
	strcpy(g_db_filename, fname);
	log_debug("load records success");
	return true;
}

bool dns_db_save() {
	if (!g_db_modified)
		return true;

	if (!g_db_filename) {
		log_error("%s fail, save can't after load success!", __func__);
		return false;
	}

	FILE* fp = fopen(g_db_filename, "w");
	if (fp == NULL) {
		log_error("%s fail, can't open file %s", __func__, g_db_filename);
		return false;
	}

	log_debug("save record to %s ...", g_db_filename);
	char *ip;
	struct in_addr addr;
	for (unsigned i = 0; i < DNS_DB_CAPACITY && g_db_recs[i].host[0]; ++i) {
		addr.s_addr = g_db_recs[i].ip;
		ip = inet_ntoa(addr);
		log_debug("write record host=%s, ip=%s", g_db_recs[i].host, ip);
		fprintf(fp, "%s %s\n", g_db_recs[i].host, ip);
	}

	fclose(fp);
	g_db_modified = 0;
	log_debug("save record success");
	return true;
}

uint32_t dns_db_find(const char* domain_name) {
	int idx = _dns_db_indexof(domain_name);
	return idx == -1 ? 0 : g_db_recs[idx].ip;
}

bool dns_db_find_by_ip(uint32_t ip, char* dst, size_t dst_size) {
	if (ip != 0) {
		for (unsigned i = 0; i < DNS_DB_CAPACITY && g_db_recs[i].host[0]; ++i) {
			if (ip == g_db_recs[i].ip) {
				if (dst != NULL && dst_size > 1) {
					strncpy(dst, g_db_recs[i].host, --dst_size);
					dst[dst_size] = '\0';
				}
				return true;
			}
		}
	}
	return false;
}

bool dns_db_update(const char* domain_name, uint32_t ip) {
	// 验证域名长度
	if (strlen(domain_name) >= DOMAIN_NAME_MAX) {
		log_error("records update error, host name %s too long.", domain_name);
		return false;
	}

	// 找到对应的主机记录, 进行更新操作
	int idx = _dns_db_indexof(domain_name);
	if (idx != -1) {
		if (g_db_recs[idx].ip != ip) {
			g_db_recs[idx].ip = ip;
			g_db_modified = 1;
		}
		return true;
	}

	// 没找到, 进行新增操作
	for (unsigned i = 0; i < DNS_DB_CAPACITY; ++i) {
		if (g_db_recs[i].host[0] == '\0') {
			dns_rec_t* rec = &g_db_recs[i];
			strcpy(rec->host, domain_name);
			rec->ip = ip;
			g_db_modified = 1;
			return true;
		}
	}

	log_debug("records update error, capacity is full.");
	return false;
}

bool dns_db_delete(const char* domain_name) {
	int idx = _dns_db_indexof(domain_name);
	if (idx == -1) {
		log_error("records delete error, can't find host %s!", domain_name);
		return false;
	}

	g_db_modified = 1;

	// 如果要删除的记录是最后一条
	int next_idx = idx + 1;
	if (next_idx == DNS_DB_CAPACITY || g_db_recs[next_idx].host[0] == '\0') {
		g_db_recs[idx].host[0] = '\0';
		return true;
	}

	// 找到最后一条记录的位置
	while (next_idx < DNS_DB_CAPACITY && g_db_recs[next_idx].host[0] != '\0')
		++next_idx;
	--next_idx;

	// 把数组中的最后一条记录移动到被删除项的位置
	dns_rec_t *del_rec = &g_db_recs[idx], *move_rec = &g_db_recs[next_idx];
	strcpy(del_rec->host, move_rec->host);
	del_rec->ip = move_rec->ip;
	move_rec->host[0] = '\0';
	move_rec->ip = 0;
	return true;
}
