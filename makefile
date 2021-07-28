#mingw32-make
#CXX = g++
#CXXFLAGS = -Wall -std=c++17 -O2
#CXXFLAGS = -Wall -std=c11 -O2
#CXXFLAGS += -fexec-charset=GBK -finput-charset=UTF-8
#CXXFLAGS += --target=i686-W64-windows-gnu
#LDFLAGS = -static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++ -lpthread
#LDFLAGS = -static-libgcc -Wl,-Bstatic -lpthread -Wl,-Bdynamic -lws2_32 -s
#-Wl,-Bdynamic -lgcc_s
# Linux -static 可进行静态链接

# 命令行范例 make D="-DNLOG -DNDEBUG" CC=clang BITS=32
CC = gcc
CFLAGS += -Wall -std=c11 -O2 -D_DEFAULT_SOURCE
CFLAGS += $(D)
#BITS 可选值 32/64, 指明是编译成32位程序还是64位程序
BITS = 64

OSNAME = $(shell uname -s)
# linux config, linux平台采用静态链接方式, 避免对libc的依赖
ifeq ($(OSNAME), Linux)
	LDFLAGS = -static
# windows config
else
	EXT = .exe
	LDFLAGS = -lws2_32 -ladvapi32 -s
endif

CFLAGS += -m$(BITS)

#SOURCE = $(wildcard *.cpp)
#OBJS = $(patsubst %.cpp,%.o,$(SOURCE))

all: mdns dyndns-cli

#main: $(OBJS)
mdns: mdns.o log.o dnsdb.o dnsproto.o dyndns.o winsvr.o md5.o
	$(CC) $(CFLAGS) -o $@$(EXT) $^ $(LDFLAGS)

dyndns-cli: dyndns-cli.c md5.o log.o
	$(CC) $(CFLAGS) -o $@$(EXT) $^ $(LDFLAGS)

# test: test.o log.o dnsdb.o
# 	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
# run-test: test
# 	cmd /c test.exe

clean:
	rm -f *.o mdns$(EXT) dyndns-cli$(EXT) mdns.log
