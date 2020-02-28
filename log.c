#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

#include "log.h"

// 条件编译语句
#ifndef NLOG

#define LOG_NAME_MAX 256
#define LOG_REC_SIZE 4096
#define LOG_HEX_LINE  (4 + 16 * 3)

/** 记录日志文件名, 0长度表示只记录到控制台 */
static char g_log_name[LOG_NAME_MAX];
/** 日志文件指针 */
static FILE *g_log_fp;
/** 允许的最大日志文件长度, 超出将重建 */
static unsigned long g_log_max_size;
/** 当前日志文件长度 */
static unsigned long g_log_cur_size;
/** 当前日志级别 */
static unsigned g_log_level = LOG_DEBUG;
static char g_log_levels[][9] = { "[DEBUG] ", "[INFO ] ", "[WARN ] ", "[ERROR] " };
static const char HEX[] = "0123456789abcdef";

void log_set_level(unsigned level) {
	g_log_level = level;
}

/** 日志文件超出长度, 重命名为bak文件 */
static void log_truncate() {
	fclose(g_log_fp);
	// 重命名
	char bak_name[LOG_NAME_MAX];
	strcpy(bak_name, g_log_name);
	strcat(bak_name, ".bak");
	remove(bak_name);
	rename(g_log_name, bak_name);
	// 新建日志文件
	g_log_fp = fopen(g_log_name, "wb");
}

static void log_deinit() {
	if (g_log_fp != NULL)
		fclose(g_log_fp);
}

void log_set_file(const char* filename, unsigned long maxsize) {
	// 校验参数
	do {
		if (g_log_fp != NULL)
			printf("%s error, can't call again!", __func__);
		else if (filename == NULL)
			printf("%s error, filename is null", __func__);
		else if (strlen(filename) > LOG_NAME_MAX - 4)
			printf("%s error, filename too long!", __func__);
		else
			break;
		return;
	} while (0);

	// 打开或创建日志文件, 并移动指针到文件末尾
	g_log_fp = fopen(filename, "rb+");
	if (g_log_fp == NULL)
		g_log_fp = fopen(filename, "wb");
	if (g_log_fp == NULL) {
		printf("%s can't open log file %s\n", __func__, filename);
		return;
	}
	fseek(g_log_fp, 0, SEEK_END);

	// 设置日志单元的一些全局变量的初始值
	g_log_cur_size = (unsigned long) ftell(g_log_fp);
	strcpy(g_log_name, filename);
	g_log_max_size = maxsize;
	atexit(log_deinit);
}

/** 往日志中写入一个字符 */
inline static void log_putc(char c) {
	if (g_log_fp) {
		fputc(c, g_log_fp);
		++g_log_cur_size;
	}
	fputc(c, stdout);
}

/** 写入内容到日志中 */
inline static void log_write(const void* data, size_t size) {
	if (g_log_fp) {
		fwrite(data, 1, size, g_log_fp);
		g_log_cur_size += size;
	}
	fwrite(data, 1, size, stdout);
}

/** 刷新日志文件缓存 */
inline static void log_flush() {
	if (g_log_fp)
		fflush(g_log_fp);
}

/** 写入日志头部 */
static void log_header(unsigned levels) {
	char buf[64];
	time_t t = time(NULL);
	size_t c = strftime(buf, sizeof(buf), "[%Y-%m-%d %H:%M:%S] ", localtime(&t));
	strcpy(buf + c, g_log_levels[levels]);
	log_write(buf, c + sizeof(g_log_levels[0]) - 1);
}

void log_log(unsigned level, const char* fmt, ...) {
	if (level < g_log_level)
		return;
	if (g_log_fp && g_log_cur_size >= g_log_max_size)
		log_truncate();

	log_header(level);

	char buf[LOG_REC_SIZE];
	// 写入记录头部的记录时间和日志级别
	va_list args;
	va_start(args, fmt);
	int pos = vsnprintf(buf, LOG_REC_SIZE - 2, fmt, args);
	va_end(args);
	if (pos < 0)
		pos = LOG_REC_SIZE - 2;
	buf[pos++] = '\n';
	buf[pos] = '\0';

	log_write(buf, pos);
	log_flush();
}

inline static bool log_is_enabled(unsigned level) {
	return level >= g_log_level;
}

bool log_is_debug_enabled() {
	return log_is_enabled(LOG_DEBUG);
}

bool log_is_info_enabled() {
	return log_is_enabled(LOG_INFO);
}

bool log_is_warn_enabled() {
	return log_is_enabled(LOG_WARN);
}

bool log_is_error_enabled() {
	return log_is_enabled(LOG_ERROR);
}

void log_dump_hex(const char *title, const void *data, size_t size) {
	if (!log_is_enabled(LOG_DEBUG))
		return;

	log_header(LOG_DEBUG);
	log_write(title, strlen(title));
	log_putc('\n');

	char buf[LOG_HEX_LINE]; // 一行hex显示格式所需大小
	memset(buf, ' ', LOG_HEX_LINE);
	buf[LOG_HEX_LINE - 1] = '\n';

	for (const char *p = data, *pe = data + size; p < pe;) {
		int pos = 4;
		// 按一行输出
		for (; p < pe && pos < LOG_HEX_LINE; ++p) {
			buf[pos++] = HEX[*p >> 4];
			buf[pos++] = HEX[*p & 0xf];
			++pos;
		}
		buf[pos - 1] = '\n';
		log_write(buf, pos);
	}
	log_flush();
}

void log_dump_text(const char *title, const char *data, size_t size) {
	if (!log_is_enabled(LOG_DEBUG))
		return;
	log_header(LOG_DEBUG);
	log_write(title, strlen(title));
	log_write(data, size);
	log_putc('\n');
	log_flush();
}

void log_dump(const char *title, const void* arg, log_dump_func func) {
	if (!log_is_enabled(LOG_DEBUG))
		return;
	log_header(LOG_DEBUG);
	log_write(title, strlen(title));

	char buf[LOG_REC_SIZE];
	int count;
	while ((count = func(arg, buf, LOG_REC_SIZE)) > 0)
		log_write(buf, count);

	log_flush();
}

#endif // NLOG