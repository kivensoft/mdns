/** 字符串库，创建的字符串对象具备长度属性，可动态扩展
 * 
 * @author kiven lee
 * @version 1.0
*/
#pragma once
#ifndef __STR_H__
#define __STR_H__

#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include "ppnarg.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 定义字符串复制功能，主要用于连续复制多个字符串
 * 
 * @param dst 目标字符串
 * @param src 源字符串
 * @return 目标字符串写入结束位置，及'\0'结束符所在位置
*/
static inline char* strcpy2(char *dst, const char *src) {
    while ((*dst = *src)) ++dst, ++src;
    return dst;
}

// str_t，带引用计数和容量、长度的字符串结构
typedef struct _str_head_t {
    uint32_t        ref;
    uint32_t        len;
    uint32_t        cap;
    char            data[];
} *str_t;


// usage: STR_FREE_MANY(a, b, c, d);
// expand: string_releases(4, a, b, c, d);
#define STR_FREES(...) str_frees(PP_ARGS(__VA_ARGS__))
#define STR_CATS(dst, ...) str_cats(dst, PP_ARGS(__VA_ARGS))

// 计算str_t所需要的内存
#define STR_SIZE(x) (sizeof(struct _str_header_t) + (x) + 1)

// 适用于函数中动态分配栈内存的str_t变量，退出时无需进行内存释放
#define STR_DECLARE(name, len) \
    char name##__1[sizeof(struct _str_head_t) + len + 1]; \
    ptr_t name = (ptr_t) name##__1; \
    name->ref = 0; \
    name->len = 0; \
    name->cap = len; \
    *name->data = '\0';

/** 分配str_t空间，容量为capacity
 * 
 * @param capacity      str_t的容量
*/
extern str_t str_malloc(uint32_t capacity);

/** str_t增加引用 */
static inline str_t str_addref(str_t self) { 
    ++self->ref;
    return self;
}

/** str_t引用计数减一，如果引用计数为0，则释放内存 */
static inline void str_free(str_t self) {
    if (self->ref && !--self->ref) free(self);
}

/** str_t引用计数减一，如果引用计数为0，则释放内存
 * 
 * @param count         参数个数
 * @param ...           不定参数
*/
extern void str_frees(int count, ...);

/** 分配str_t空间，原始内容为src内容 */
static inline str_t str_from_cstr(const char *src) {
    uint32_t len = src ? strlen(src) : 0;
    str_t p = str_malloc(len);

    if (src) {
        memcpy(p->data, src, len + 1);
        p->len = len;
    }

    return p;
}

/** 缩减字符串长度，只能用于截断，无法用于扩展
 * 
 * @param len           缩减长度
*/
static inline void str_trim(str_t self, uint32_t len) {
    if (len < self->len) {
        self->len = len;
        self->data[len] = '\0';
    }
}

/** str_t增加容量
 * 
 * @param capacity      新增容量
*/
extern void str_expand(str_t* self, uint32_t capacity);

/** 复制一个stirng，全新分配内存空间 */
extern str_t str_copy(str_t src);

/** 连接字符串
 * 
 * @param src           源字符串
 * @param len           src长度
*/
extern void str_write(str_t* self, const char *src, uint32_t len);

/** 连接字符串
 * 
 * @param src           源字符串
 * @return              新的str_t
*/
static inline void str_cat(str_t* self, const char *src) {
    if (src) str_write(self, src, strlen(src));
}

/** 连接字符串
 * 
 * @param count         字符串不定参数个数
 * @param ...           不定参数
 * @return              新的str_t
*/
extern void string_cats(str_t* self, int count, ...);

#ifdef __cplusplus
}
#endif

#endif // __STR_H__