#pragma once
#ifndef __LOG_H__
#define __LOG_H__
/** 日志单元, 支持windows/linux, 适用于控制台应用程序
 * @author kiven
 * @version 1.02
 * @describe 如果定义了NLOG, 将禁用所有日志单元的代码
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// 日志级别宏定义
#define LOG_DEBUG  0
#define LOG_INFO   1
#define LOG_WARN   2
#define LOG_ERROR  3
#define LOG_OFF    4

/** log_dump函数回调接口, 回调函数负责写入buf, 并返回写入长度
 * @param arg 回调函数时传递的变量
 * @param buf 回写缓冲区地址
 * @param size 回写缓冲区长度
*/
typedef int (*log_dump_func) (const void* arg, char* buf, int size);

#ifdef NLOG

#define log_set_level(X) ((void)0)
#define log_set_file(filename, maxsize) ((void)0)
#define log_is_debug_enabled(X) 0
#define log_is_info_enabled(X) 0
#define log_is_warn_enabled(X) 0
#define log_is_error_enabled(X) 0
#define log_log(levels, fmt, ...) ((void)0)
#define log_debug(fmt, ...) ((void)0)
#define log_info(fmt, ...) ((void)0)
#define log_warn(fmt, ...) ((void)0)
#define log_error(fmt, ...) ((void)0)
#define log_dump_hex(title, date, size) ((void)0)
#define log_dump_text(title, data, size) ((void)0)
#define log_dump(title, func) ((void)0)

#else // !NLOG

/** 设置日志级别, 支持运行期间动态设置
 * @param level 日志级别 LOG_DEBUG/LOG_INFO/LOG_WARN/LOG_ERROR/LOG_OFF
*/
extern void log_set_level(unsigned level);

/** 设置日志输出文件, 不设置默认只记录到控制台
 * filename 日志文件名称
 * maxsize 日志文件允许的最大长度, 超过将备份当前日志文件, 并新建日志文件进行记录
*/
extern void log_set_file(const char* filename, unsigned long maxsize);

/** 返回是否允许debug级别日志的值, 0: 不允许, 1: 允许 */
extern bool log_is_debug_enabled();

/** 返回是否允许info级别日志的值, 0: 不允许, 1: 允许 */
extern bool log_is_info_enabled();

/** 返回是否允许warn级别日志的值, 0: 不允许, 1: 允许 */
extern bool log_is_warn_enabled();

/** 返回是否允许error级别日志的值, 0: 不允许, 1: 允许 */
extern bool log_is_error_enabled();

/** 记录指定级别的日志, 会根据当前系统设置的级别判断是否需要记录
 * @param level 日志级别 LOG_DEBUG/LOG_INFO/LOG_WARN/LOG_ERROR
 * @param fmt 格式化字符串, 使用与printf同样的格式
 * @param ... 格式化参数
 */
extern void log_log(unsigned level, const char* fmt, ...);

/** 记录debug级别日志
 * @param fmt 格式化字符串, 使用与printf同样的格式
 * @param ... 格式化参数
 */
#define log_debug(fmt, ...) log_log(LOG_DEBUG, fmt, ##__VA_ARGS__)

/** 记录info级别日志
 * @param fmt 格式化字符串, 使用与printf同样的格式
 * @param ... 格式化参数
 */
#define log_info(fmt, ...) log_log(LOG_INFO, fmt, ##__VA_ARGS__)

/** 记录warn级别日志
 * @param fmt 格式化字符串, 使用与printf同样的格式
 * @param ... 格式化参数
 */
#define log_warn(fmt, ...) log_log(LOG_WARN, fmt, ##__VA_ARGS__)

/** 记录error级别日志
 * @param fmt 格式化字符串, 使用与printf同样的格式
 * @param ... 格式化参数
 */
#define log_error(fmt, ...) log_log(LOG_ERROR, fmt, ##__VA_ARGS__)

/** 将data的内容以hex方式输出到日志中, 方便调试
 * @param title 转储标题
 * @param data 要转储的内容
 * @param size 要转储的内容长度, 字节为单位
 */
extern void log_dump_hex(const char *title, const void *data, size_t size);

/** 将data的内容以text方式输出到日志中, 方便调试
 * @param title 转储标题
 * @param data 要转储的内容
 * @param size 要转储的内容长度, 字节为单位
 */
extern void log_dump_text(const char *title, const char *data, size_t size);

/** 将data的内容以回调函数方式输出到日志中, 方便调试
 * @param title 转储标题
 * @param arg 调用回调函数时传递的参数
 * @param func 回调函数
 */
extern void log_dump(const char *title, const void* arg, log_dump_func func);

#endif // NLOG

#endif // __LOG_H__