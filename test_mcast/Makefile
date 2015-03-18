# 用于发送组播包
INSTALL_DIR=/usr/local/include/sserv
LIBS = `pkg-config --cflags --libs glib-2.0`
#CFLAGS =  -g -Wall -DENABLE_TRACE -ldl -rdynamic -lnanc
CFLAGS =  -g -Wall -ldl -rdynamic -lnanc  -lpthread
OBJS = reloadso_cli.o mcast_pub.o

all : mcast_serv

mcast_serv : $(OBJS)
	gcc $(OBJS) -o mcast_serv $(CFLAGS) $(LIBS) 

%.o : %.c
	gcc -c -o $@ $< $(CFLAGS)  $(LIBS) 

clean:
	rm -rf *.o mcast_serv
