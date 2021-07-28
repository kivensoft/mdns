#pragma once
#ifndef __DNSDB_H__
#define __DNSDB_H__

#include <stdbool.h>
#include <stdint.h>
#include "dnsproto.h"

/** 加载域名记录
 * @param filename 记录的数据库名
 * @return true: 加载成功, false: 加载失败
 */
extern bool dnsdb_load(const char* filename);

/** 保存域名记录
 * @return true: 保存成功, false: 保存失败
 */
extern bool dnsdb_save();

/** 释放dnsdb所分配的内存 */
extern void dnsdb_free();

/** 查找域名记录
 * @param host 域名
 * @return 成功返回ip, 失败返回 INADDR_NONE
 */
extern uint32_t dnsdb_find(const char* host);

/** 查找ip对应的域名
 * @param ip 查抄的ip地址
 * @param dst 找到ip后回写域名的地址
 * @return true: 成功, false: 失败
 */
extern bool dnsdb_findby_ip(uint32_t ip, char dst[HOST_MAX]);

/** 更新或添加记录, 只在内存中更新, 需要用户自己调用records_save来保存
 * @param domain_name 域名
 * @param ip ip地址
 * @return true: 成功, false: 失败
 */
extern bool dnsdb_update(const char* host, uint32_t ip);

/** 删除记录, 只在内存中删除
 * @param host 域名
 * @return true: 成功, false: 失败
 */
extern bool dnsdb_delete(const char* host);

/** 对记录进行循环，循环中回调处理，回调函数返回true则继续循环，返回false取消循环
 * @param callback 回调函数
 */
extern void dnsdb_foreach(bool (*callback) (const char* host, uint32_t ip));

#endif //__DNSDB_H__