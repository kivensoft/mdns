#pragma once
#ifndef __DNSDB_H__
#define __DNSDB_H__

#include <stdbool.h>
#include <stdint.h>

/** 域名最大允许长度 */
#define DOMAIN_NAME_MAX 64

/** 加载域名记录
 * @param fname 记录的数据库名
 * @return true: 加载成功, false: 加载失败
 */
extern bool dns_db_load(const char* fname);

/** 保存域名记录
 * @return true: 保存成功, false: 保存失败
 */
extern bool dns_db_save();

/** 查找域名记录
 * @param domain_name 域名
 * @return 成功返回ip, 失败返回0
 */
extern uint32_t dns_db_find(const char* domain_name);

/** 查找ip对应的域名
 * @param ip 查抄的ip地址
 * @param dst 找到ip后回写域名的地址
 * @param dst_size 回写地址最大空间
 * @return true: 成功, false: 失败
 */
extern bool dns_db_find_by_ip(uint32_t ip, char* dst, size_t dst_size);

/** 更新或添加记录, 只在内存中更新, 需要用户自己调用records_save来保存
 * @param domain_name 域名
 * @param ip ip地址
 * @return true: 成功, false: 失败
 */
extern bool dns_db_update(const char* domain_name, uint32_t ip);

/** 删除记录, 只在内存中删除
 * @param domain_name 域名
 * @return true: 成功, false: 失败
 */
extern bool dns_db_delete(const char* domain_name);

#endif //__DNSDB_H__