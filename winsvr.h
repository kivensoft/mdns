#pragma once
#ifndef __WIN_SVR_H__
#define __WIN_SVR_H__

#ifdef _WIN32

/** 安装windows服务
 * @param cmd_arg 安装服务时指定的命令行参数
 */
extern void svc_install(const char *cmd_arg);

/** 以服务方式启动
 * 
 */
extern void svc_start();

#endif //_WIN32

#endif //__WIN_SVR_H__