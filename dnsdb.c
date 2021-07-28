#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"
#include "log.h"
#include "net.h"
#include "dnsdb.h"

#define _I2S_TMP(x) #x
#define _I2S(x) _I2S_TMP(x)

#define SCAN_FMT ("%" _I2S(HOST_MAX) "s %" _I2S(HOST_MAX) "s")

// 存放域名ip信息的结构
typedef struct dnsdb_rec_t {
	LIST_FIELDS;
	char host[HOST_MAX];
	uint32_t ip;
} dnsdb_rec_t;

// 域名ip存放链表
static LIST_HEAD(_dns_recs);
static bool _db_modified = false; // 数据库改动标志
static char* _db_filename = NULL; // 数据库文件名

static inline void dnsdb_append_rec(const char* host, size_t hlen, uint32_t ip) {
	dnsdb_rec_t *r = malloc(sizeof(dnsdb_rec_t));
	memcpy(r->host, host, hlen + 1);
	r->ip = ip;
	list_add_tail((list_head_t*) r, &_dns_recs);
}

static inline dnsdb_rec_t* dnsdb_get(const char* host, size_t hlen) {
	if (++hlen <= HOST_MAX) {
		dnsdb_rec_t *pos;
		list_foreach(pos, &_dns_recs) {
			if (!memcmp(pos->host, host, hlen))
				return pos;
		}
	} else {
		log_warn("%s fail: host[%s] too long", __func__, host);
	}
	return NULL;
}

bool dnsdb_load(const char* filename) {
	if (_db_filename) {
		log_error("%s error: %s already load!", __func__, filename);
	}
	FILE* fp = fopen(filename, "r");
	if (!fp) {
		fp = fopen(filename, "w");
		if (!fp) {
			log_error("%s error: can't open file %s!", __func__, filename);
			return false;
		}
	}

	char host[HOST_MAX], ip[HOST_MAX];
	uint32_t ip_num;

	while (fscanf(fp, SCAN_FMT, host, ip) == 2) {
		log_trace("read record host=%s, ip=%s", host, ip);

		ip_num = inet_addr(ip);
		if (ip_num == INADDR_NONE) {
			log_warn("host[%s], ip[%s] is invalid.", host, ip);
		} else {
			size_t hl = strlen(host) + 1;
			if (hl > HOST_MAX) {
				hl = HOST_MAX;
				host[HOST_MAX - 1] = '\0';
			}
			dnsdb_append_rec(host, hl, ip_num);
		}
	}

	fclose(fp);
	_db_modified = false;
	size_t dfs = strlen(filename) + 1;
	_db_filename = malloc(dfs);
	memcpy(_db_filename, filename, dfs);
	log_info("load dnsdb records success: %s", filename);
	return true;
}

bool dnsdb_save() {
	if (!_db_modified) return true;

	if (!_db_filename) {
		log_error("%s error: dnsdb file name is NULL!", __func__);
		return false;
	}

	FILE* fp = fopen(_db_filename, "w");
	if (!fp) {
		log_error("%s error: can't open file %s", __func__, _db_filename);
		return false;
	}

	struct in_addr addr;
	dnsdb_rec_t *pos;
	list_foreach(pos, &_dns_recs) {
		addr.s_addr = pos->ip;
		char* ip = inet_ntoa(addr);
		log_trace("write record host[%s], ip[%s]", pos->host, ip);
		fprintf(fp, "%s %s\n", pos->host, ip);
	}

	fclose(fp);
	_db_modified = false;
	log_debug("save record success: %s", _db_filename);
	return true;
}

void dnsdb_free() {
	if (_db_filename)
		free(_db_filename);

	list_head_t *pos, *tmp;
	list_foreach_reverse_safe(pos, tmp, &_dns_recs) {
		free(pos);
	}

	_db_filename = NULL;
	list_head_init(&_dns_recs);
}

uint32_t dnsdb_find(const char* host) {
	if (host) {
		size_t hl = strlen(host);
		dnsdb_rec_t *p = dnsdb_get(host, hl);
		return p ? p->ip : INADDR_NONE;
	}
	return INADDR_NONE;
}

bool dnsdb_findby_ip(uint32_t ip, char dst[HOST_MAX]) {
	if (ip != INADDR_NONE) {
		dnsdb_rec_t *pos;
		list_foreach(pos, &_dns_recs) {
			if (ip == pos->ip) {
				strcpy(dst, pos->host);
				return true;
			}
		}
	}
	return false;
}

bool dnsdb_update(const char* host, uint32_t ip) {
	size_t hl = strlen(host);
	dnsdb_rec_t *p = dnsdb_get(host, hl);
	if (p) {
		if (p->ip == ip) return true;
		p->ip = ip;
	} else {
		dnsdb_append_rec(host, hl, ip);
	}
	_db_modified = true;

	if (log_is_trace_enabled())
		log_trace("%s update success: host[%s], ip[%s]", __func__, host, net_ip_tostring(ip));

	return true;
}

bool dnsdb_delete(const char* host) {
	size_t hl = strlen(host);
	dnsdb_rec_t *p = dnsdb_get(host, hl);

	if (!p) {
		log_debug("%s fail: host[%s] can't find!", __func__, host);
		return false;
	}

	list_del((list_head_t*) p);
	_db_modified = 1;

	return true;
}

void dnsdb_foreach(bool (*callback) (const char* host, uint32_t ip)) {
	dnsdb_rec_t *pos;
	list_foreach(pos, &_dns_recs) {
		if (!callback(pos->host, pos->ip))
			break;
	}
}

//----------------------------------------
// #define DNSDB_TEST
#ifdef DNSDB_TEST
int main() {
	// log_set_level(LOG_TRACE);
	const char *h1 = "home.kivensoft.cn", *h2 = "xx.home.kivensoft.cn";
	dnsdb_load("minidns.conf");
	dnsdb_update(h1, 0x01020304);
	dnsdb_update(h2, 0x05060708);
	dnsdb_save();
	dnsdb_free();

	dnsdb_load("minidns.conf");
	log_debug("host[%s] -> ip[%s]", h1, ip_string(dnsdb_find(h1)));
	log_debug("host[%s] -> ip[%s]", h1, ip_string(dnsdb_find(h2)));

    return 0;
}

#endif // DNSDB_TEST