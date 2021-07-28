#include <stdlib.h>
#include <string.h>

#include "str.h"

#define MIN_CAP 32
#define LARGE_CAP 1024

str_t str_malloc(uint32_t capacity) {
    // 最小分配32字节，小于1024时倍数增长，大于1024时按1024增长
    uint32_t cap = MIN_CAP, min_cap = capacity + sizeof(struct _str_head_t) + 1;

    while (cap < min_cap)
        if (min_cap <= LARGE_CAP) cap <<= 1; else cap += LARGE_CAP;

    str_t p = (str_t) malloc(cap);
    p->ref = 1;
    p->len = 0;
    p->cap = cap - sizeof(struct _str_head_t) - 1;
    *p->data = '\0';

    return p;
}

void str_frees(int count, ...) {
    va_list args;
    va_start(args, count);
    while (count--)
        str_free(va_arg(args, str_t));
    va_end(args);
}

void str_expand(str_t* self, uint32_t capacity) {
    str_t s = *self;
    if (capacity <= s->cap) return;
    str_t p = str_malloc(capacity);

    memcpy(p->data, s->data, s->len + 1);
    p->len = s->len;
    str_free(s);

    *self = p;
}

str_t str_copy(str_t src) {
    str_t p = str_malloc(src ? src->len : 0);
    if (src) {
        memcpy(p->data, src->data, src->len + 1);
        p->len = src->len;
    }
    return p;
}

void str_write(str_t* self, const char *src, uint32_t len) {
    if (!src) return;

    uint32_t sl = (*self)->len, nl = sl + len;

    if (nl > (*self)->cap) str_expand(self, nl);
    memcpy((*self)->data + sl, src, len);
    (*self)->data[nl] = '\0';
    (*self)->len = nl;
}

void string_cats(str_t* self, int count, ...) {
    uint32_t len = (*self)->len, cap = (*self)->cap;

    uint32_t args_len[count];
    va_list args;

    // 计算所有字符串的总长度，加上自身长度，等于将要扩展的长度
    va_start(args, count);
    for (int i = 0, imax = count; i < imax; ++i) {
        char *p = va_arg(args, char*);
        if (p) {
            args_len[i] = (uint32_t) strlen(p);
            len += args_len[i];
        }
    }
    va_end(args);

    if (cap < len) str_expand(self, len);

    va_start(args, count);
    for (int i = 0, imax = count; i < imax; ++i)
        str_write(self, va_arg(args, char*), args_len[i]);
    va_end(args);
}
