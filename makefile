#mingw32-make
#CXX = g++
#CXXFLAGS = -Wall -std=c++17 -O2
#CXXFLAGS += -fexec-charset=GBK -finput-charset=UTF-8
#CXXFLAGS += --target=i686-W64-windows-gnu
#LDFLAGS = -static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++ -lpthread
#LDFLAGS = -static-libgcc -Wl,-Bstatic -lpthread -Wl,-Bdynamic -lws2_32 -s
#-Wl,-Bdynamic -lgcc_s

# 命令行范例 make D="-DNLOG -DNDEBUG" CC=clang BITS=32
CC = gcc
CFLAGS = -Wall -std=c99 -O2
CFLAGS += $(D)
#BITS 可选值 32/64, 指明是编译成32位程序还是64位程序
BITS = 64

OSNAME = $(shell uname -s)
ifeq ($(OSNAME), Linux)
	# linux config, linux平台采用静态链接方式, 避免对libc的依赖
	LDFLAGS = -static
else
	# windows config
ifeq ($(CC), clang)
		# clang config, clang编译器采用32位编译, 因为只装了32位的clang
		CFLAGS += --target=i686-pc-windows-gnu
		BITS = 32
endif
	LDFLAGS = -lws2_32 -ladvapi32 -s
endif

CFLAGS += -m$(BITS)

#SOURCE = $(wildcard *.cpp)
#OBJS = $(patsubst %.cpp,%.o,$(SOURCE))

all: mdns dyndns-cli

#main: $(OBJS)
mdns: mdns.o log.o net.o dnsdb.o dnsproto.o dyndns.o getopt.o winsvr.o md5.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

dyndns-cli: dyndns-cli.c net.o md5.o log.o getopt.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# test: test.o log.o dnsdb.o
# 	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
# run-test: test
# 	cmd /c test.exe

clean:
	rm *.o