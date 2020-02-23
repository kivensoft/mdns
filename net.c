#include <sys/time.h>

#include "log.h"
#include "net.h"

#ifdef _WIN32
#	define _TIME(name, secs) uint32_t name = 1000 * secs
#else
#	define _TIME(name, secs) struct timeval name = { .tv_sec = secs, .tv_usec = 0 }
#endif

#ifdef _WIN32

static void wsa_cleanup() {
	WSACleanup();
}

bool init_socket() {
	WSADATA wd;
	if (WSAStartup(MAKEWORD(2, 2), &wd) != 0) {
		log_error("load ws2_32.dll error!");
		return false;
	}
	atexit(wsa_cleanup);
	return true;
}

#else // no _WIN32


#endif //_WIN32

bool sock_recv_timeout(socket_t socket, uint32_t seconds) {
	_TIME(t, seconds);
	return setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char*)(&t), sizeof(t)) >= 0;
}

bool sock_send_timeout(socket_t socket, uint32_t seconds) {
	_TIME(t, seconds);
	return setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (char*)(&t), sizeof(uint32_t)) >= 0;
}