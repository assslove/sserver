/*
 * =====================================================================================
 *
 *       Filename:  epoll_cli.c
 *
 *    Description:  使用epoll cli进行测试
 *
 *        Version:  1.0
 *        Created:  12/19/2014 05:44:03 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:	houbin , houbin-12@163.com
 *   Organization:  Houbin, Inc. ShangHai CN. All rights reserved.
 *
 * =====================================================================================
 */


#include <stdio.h>
#include <sys/epoll.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>
#include <map>
#include <unordered_map>
#include <iostream>

std::map<uint32_t, timeval> seq_time_map;

typedef struct proto_pkg {
	int len;
	uint16_t cmd;
	int id;
	int seq;
	int ret;
	char data[];
} __attribute__((packed))proto_pkg_t;

void gen_str(char buf[], int n)
{
	int i;
	for (i = 0; i < n; i++) {
		buf[i] = (char)('a' + rand() % 26);
	}
}

int safe_tcp_recv_n(int sockfd, void* buf, int total)
{
	assert(total > 0);

	int recv_bytes, cur_len = 0;

	for (recv_bytes = 0; recv_bytes < total; recv_bytes += cur_len)	{
		cur_len = recv(sockfd, (char *)buf + recv_bytes, total - recv_bytes, 0);
		if (cur_len == 0) { 
			return 0;
		} else if (cur_len == -1) { // errno 
			if (errno == EINTR) {
				cur_len = 0;
			} else if (errno == EAGAIN || errno == EWOULDBLOCK)	{
				return recv_bytes;
			} else {
				return -1;
			}
		}
	}

	return recv_bytes;
}

int set_io_nonblock(int fd, int nonblock)
{
	int val;
	if (nonblock) {
		val = (O_NONBLOCK | fcntl(fd, F_GETFL));
	} else {
		val = (~O_NONBLOCK & fcntl(fd, F_GETFL));
	}
	return fcntl(fd, F_SETFL, val);
}

int seq = 0;

int main(int argc, char* argv[]) 
{
	srand(time(NULL));
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1) {
		printf("%s\n", strerror(errno));
	}

	struct sockaddr_in servaddr;
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(9001 + rand() % 50);
	inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);

	int ret = connect(fd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr));

	if (ret == -1) {
		printf("%s", strerror(errno));
		return 0;
	}

	int epfd = epoll_create(1024);
	if (epfd == -1) {
		printf("create failed");
		return 0;
	}

	set_io_nonblock(fd, 1);

	struct epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET;
	ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
	struct epoll_event *evs = (struct epoll_event *)malloc(sizeof(struct epoll_event) * 200);	
	int done = 0;

	
	while (!done) {
		int i;
recv_again:
		int n = epoll_wait(epfd, evs, 1024, 100);
		if (n == -1 && errno != EINTR) {
			printf("%s", strerror(errno));
			return 0;
		}


		for (i = 0; i < n; ++i) {
			int fd = evs[i].data.fd;
			printf("fd=%u type=%u\n", fd, evs[i].events);
			if (evs[i].events && EPOLLIN) {
				char recvbuf[102400];
				int ret = safe_tcp_recv_n(fd, recvbuf, 102400);
				if (ret == 0) {
					printf("fd closed");
					return 0;
				} else if (ret > 0) {
					printf("recvbuf len=%d\n", ret);
					int len = ret;
					int readlen = 0;
					struct timeval end;
					struct timeval start; 
					while (readlen < len) {
						gettimeofday(&end, NULL);
						char*  ptr = recvbuf;
						proto_pkg_t *msg = (proto_pkg_t *)(ptr + readlen);
						start = seq_time_map[msg->seq];
						printf("recv: %d,%d,%d,%d,%d,%s:%lu,%lu\n", 
								msg->id, 
								msg->cmd, 
								msg->seq,
								msg->ret, 
								msg->len,
								msg->data,
								msg->len - sizeof(proto_pkg_t), 
								(end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec)
							  );
						readlen += msg->len;
						seq_time_map.erase(msg->seq);
					}
				} else {
					printf("recv error");
					return 0;
				}
			} else if (evs[i].events && EPOLLOUT) {

			}
		}
		if (seq_time_map.size()) {
			goto recv_again;
		}

		sleep(1);
		//getchar();
		char input[200] = {'\0'};
		int num = rand() % 200+ 1;
		//int num = 30;
		gen_str(input, num);
		//		scanf("%s", input);
		char buf[1024];
		for (i = 0; i < 200; ++i) {
			proto_pkg_t *pkg = (proto_pkg_t *)buf;	
			pkg->id =  i;
			pkg->cmd = i + 1;
			pkg->ret = i + 2;
			pkg->seq = ++seq;

			struct timeval start;
			gettimeofday(&start, NULL);
			seq_time_map[pkg->seq] = start;

			pkg->len = sizeof(proto_pkg_t) + strlen(input) + 1;
			input[strlen(input)] = '\0';
			memcpy(pkg->data, input, strlen(input) + 1);

			send(fd, buf, pkg->len, 0);
			printf("send: id=%u,cmd=%u,seq=%u,ret=%u,%s:%lu\n\n", pkg->id, pkg->cmd, pkg->seq, pkg->ret, input, strlen(input) + 1);

			//			getchar();
		}
		//		sleep(1);
		//if (rand() % 2) {
		//}
	}

	free(evs);
	close(epfd);
	close(fd);

	return 0;
}
