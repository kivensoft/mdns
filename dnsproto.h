#pragma once
#ifndef __DNSPROTO_H__
#define __DNSPROTO_H__

#include <stdint.h>
#include "net.h"

/** 域名最大允许长度 */
#define HOST_MAX 64
#define DNS_PACKET_MAX 512

typedef struct dns_head_t {
    uint16_t id;                // dns事务id，应答报文原样返回，客户通过标识字段来确定DNS响应是否与查询请求匹配

    union {
        uint16_t flags_uint16;
        struct {
            uint8_t qr: 1;          // 操作类型： 0：查询报文, 1：响应报文
            uint8_t opcode: 4;      // 查询类型： 0：标准查询, 1：反向查询, 2：服务器状态查询, 3～15：保留未用
            uint8_t aa: 1;          // 应答报文使用, 若置位，则表示该域名解析服务器是授权回答该域的
            uint8_t tc: 1;          // 若置位，则表示报文被截断, 使用UDP传输时，应答的总长度超过512字节时，只返回报文的前512个字节内容
            uint8_t rd: 1;          // 客户端希望域名服务器采取的解析方式： 0：希望采取迭代解析 1：希望采取递归解析

            uint8_t ra: 1;          // 域名解析服务器采取的解析方式： 0：采取迭代解析 1：采取递归解析
            uint8_t z: 3;           // 全部置0，保留未用
            uint8_t rcode: 4;       // 响应类型： 0：无差错 1：查询格式错 2：服务器失效 3：域名不存在 4：查询没有被执行 5：查询被拒绝 6-15: 保留未用
        } flags;   // 报文标志
    };

    uint16_t qtcount;           // 报文请求段中的问题记录数
    uint16_t ancount;           // 报文回答段中的回答记录数
    uint16_t nscount;           // 报文授权段中的授权记录数
    uint16_t arcount;           // 报文附加段中的附加记录数
} dns_head_t;

/** dns协议解析服务初始化函数
 * @param func 域名查找回调接口地址
 */
extern void dns_init(uint32_t (*find_func) (const char* host));

/** dns解析处理函数, 解析dns报文, 查找域名, 填充返回内容
 * @param req dns报文地址
 * @param req_size 报文长度
 * @param res 写入回复消息的地址
 * @param res_size 写入回复消息地址的可写长度
 * @return 写入长度, 0: 忽略消息, 无需回复
 */
extern uint16_t dns_process(const void *req, size_t req_size, uint8_t res[DNS_PACKET_MAX]);

#endif //__DNSPROTO_H__