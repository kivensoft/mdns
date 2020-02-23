#ifdef _WIN32

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <windows.h>
#include <winsvc.h>
#include "log.h"
#include "winsvr.h"

// #pragma comment(lib, "advapi32.lib")

#define SVCNAME "MDNS"
#define SVCDESC "Mini DNS Service"
#define SVC_ERROR       ((DWORD)0xC0020001L)

static SERVICE_STATUS          gSvcStatus;
static SERVICE_STATUS_HANDLE   gSvcStatusHandle;

extern int run();

void WINAPI SvcCtrlHandler(DWORD dwCtrl) {
	// Handle the requested control code. 
	switch (dwCtrl) {
		case SERVICE_CONTROL_STOP:
		case SERVICE_CONTROL_SHUTDOWN:
			gSvcStatus.dwWin32ExitCode = 0;
			gSvcStatus.dwCurrentState = SERVICE_STOPPED;
			log_debug("stop service success");
			break;
		default:
			gSvcStatus.dwCurrentState = SERVICE_RUNNING;
			break;
	}
	SetServiceStatus(gSvcStatusHandle, &gSvcStatus);
}

void WINAPI SvcMain(DWORD dwArgc, LPTSTR *lpszArgv) {
	// Register the handler function for the service
	gSvcStatusHandle = RegisterServiceCtrlHandlerA(SVCNAME, SvcCtrlHandler);

	if (!gSvcStatusHandle) {
		log_debug("RegisterServiceCtrlHandler failed.");
		return;
	}

	// These SERVICE_STATUS members remain as set here
	RtlZeroMemory(&gSvcStatus, sizeof(SERVICE_STATUS));
	gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
	gSvcStatus.dwCurrentState = SERVICE_RUNNING;
	SetServiceStatus(gSvcStatusHandle, &gSvcStatus);

	log_debug("start service success");
	run();
}

void svc_start() {
	SERVICE_TABLE_ENTRY table[] = {
			{ SVCNAME, SvcMain },
			{ NULL, NULL }
	};
	StartServiceCtrlDispatcherA(table);
}

void svc_install(const char* cmd_arg) {
	SC_HANDLE schSCManager;
	SC_HANDLE schService;
	char szPath[4096];
	DWORD len;

	if (strlen(cmd_arg) > 1024) {
		printf("Can't install service, command line too long.");
		return;
	}

	// 创建安装服务需要的命令行
	len = GetModuleFileNameA(NULL, szPath + 1, sizeof(szPath) - 1);
	if (len == 0) {
		printf("Cannot install service (%lu)\n", GetLastError());
		return;
	}
	szPath[0] = '"';
	sprintf(szPath + len + 1, "\" %s", cmd_arg);

	// Get a handle to the SCM database. 
	schSCManager = OpenSCManagerA(
		NULL,                    // local computer
		NULL,                    // ServicesActive database 
		SC_MANAGER_ALL_ACCESS);  // full access rights 
	if (NULL == schSCManager) {
		printf("OpenSCManager failed (%lu)\n", GetLastError());
		return;
	}

	// Create the service
	schService = CreateServiceA(
		schSCManager,              // SCM database 
		SVCNAME,                   // name of service 
		SVCDESC,                   // service name to display 
		SERVICE_ALL_ACCESS,        // desired access 
		SERVICE_WIN32_OWN_PROCESS, // service type 
		SERVICE_AUTO_START,        // start type 
		SERVICE_ERROR_NORMAL,      // error control type 
		szPath,                    // path to service's binary 
		NULL,                      // no load ordering group 
		NULL,                      // no tag identifier 
		NULL,                      // no dependencies 
		NULL,                      // LocalSystem account 
		NULL);                     // no password 

	if (schService == NULL) {
		printf("CreateService failed (%lu)\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return;
	} else
		printf("Service %s [%s] installed successfully\n", SVCNAME, SVCDESC);

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
}

#endif // _WIN32