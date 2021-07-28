/** 智能指针，包括共享指针、独占指针与弱指针三种
 * @author kiven lee
 * @version 1.0
*/
#pragma once
#ifndef __PTR_H__
#define __PTR_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ptr_head_t {
    uint32_t    ref;
    uint32_t    len;
    uint8_t     data[];
} *ptr_t;

/** 分配内存资源并创建强引用
 * 
 * @param size 资源长度
 */
inline ptr_t ptr_malloc(size_t size) {
    ptr_t ret = (ptr_t) malloc(sizeof(struct _ptr_head_t) + size);
    ret->ref = 1;
    ret->len = size;
    return ret;
}

/** 资源的引用计数加1 */
inline ptr_t ptr_addref(ptr_t self) {
    ++self->ref;
    return self;
}

/** 释放资源，引用计数减1，当引用计数为0时释放资源 */
inline void ptr_free(ptr_t self) {
    if (self->ref && !--self->ref) free(self);
}

/** 对资源进行扩容
 * 
 * @param size 增加的容量 
 */
inline void ptr_expand(ptr_t* self, uint32_t size) {
    ptr_t s = *self;
    *self = ptr_malloc(s->len + size);
    memcpy((*self)->data, s->data, s->len);
    ptr_free(s);
}

/** 写入内容
 * 
 * @param pos 写入位置
 * @param src 要写入的内容
 * @param len 要写入内容的长度
 */
inline void ptr_write(ptr_t* self, uint32_t pos, void* src, uint32_t len) {
    if ((*self)->len < pos + len)
        ptr_expand(self, pos + len);
    memcpy((*self)->data + pos, src, len);
}

#ifdef __cplusplus
}
#endif

#endif //__PTR_H__
