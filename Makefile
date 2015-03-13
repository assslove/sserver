# 简单服务器 一父多子 

INSTALL_DIR=/usr/local/include/sserv
LIBS = `pkg-config --cflags --libs glib-2.0`
CFLAGS =  -g -Wall -DENABLE_TRACE -ldl -rdynamic -lnanc
OBJS = simple_svr.o net_util.o mem_queue.o util.o master.o work.o global.o  fds.o outer.o mcast.o conf.o

all : simple_svr epoll_cli 

simple_svr: $(OBJS)
	gcc $(OBJS) -o simple_svr  $(LIBS) $(CFLAGS)

%.o : %.c
	gcc -c -o $@ $< $(LIBS) $(CFLAGS) 

epoll_cli: epoll_cli.c
	gcc -g -o epoll_cli epoll_cli.c

clean:
	rm -rf *.o simple_svr  epoll_cli

install: 
	sudo rm -rf $(INSTALL_DIR)
	sudo mkdir $(INSTALL_DIR)
	sudo cp -r *.h $(INSTALL_DIR) 
