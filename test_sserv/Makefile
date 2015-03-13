# 简单服务器 一父多子 

INSTALL_DIR=/usr/local/include/sserv
LIBS = `pkg-config --cflags --libs glib-2.0`

all : test_serv.so data.so
	mv $< ../
test_serv.so: test_serv.cpp switch.cpp
	g++ -g -shared -fPIC test_serv.cpp switch.cpp -o test_serv.so `pkg-config --cflags --libs glib-2.0` -D_GNU_SOURCE -lnanc
data.so: data.c
	gcc -g -shared -fPIC data.c -o data.so
clean:
	rm -rf *.o simple_svr test_serv.so epoll_cli

install: 
	sudo rm -rf $(INSTALL_DIR)
	sudo mkdir $(INSTALL_DIR)
	sudo cp -r *.h $(INSTALL_DIR) 
