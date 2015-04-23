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
	int num = 16*1024-18 - 1;
	char input[16*1024] = {'\0'};
	char buf[16*1024-18 + 1] = {'\0'};
	//int num = 30;
	gen_str(buf, num);

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1) {
		printf("%s\n", strerror(errno));
	}

	struct sockaddr_in servaddr;
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(9001);
	inet_pton(AF_INET, "10.0.2.15", &servaddr.sin_addr);

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
	char recvbuf[num];

	struct timeval start;
	struct timeval end;
	gettimeofday(&start, NULL);
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
			//printf("fd=%u type=%u\n", fd, evs[i].events);
			if (evs[i].events && EPOLLIN) {
				int ret = safe_tcp_recv_n(fd, recvbuf, 16 * 1024);
				if (ret == 0) {
					printf("fd closed");
					return 0;
				} else if (ret > 0) {
//					printf("recvbuf len=%d\n", ret);
					int len = ret;
					struct timeval end;
					struct timeval start; 
					int readlen = 0;
					while (readlen < len) {
						char*  ptr = recvbuf;
						proto_pkg_t *msg = (proto_pkg_t *)(ptr + readlen);
						if (msg->len != len) {
							printf("recv error");
							return 0;
						}
						readlen += msg->len;
						seq_time_map.erase(msg->seq);
						memcpy(buf, msg->data, num+1);
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

//		sleep(1);
		if (seq >= 100000) {
			gettimeofday(&end, NULL);
			int total = end.tv_sec - start.tv_sec;
			printf("total=1000000, msg_size=%u,throughput=%.4f M/s", 16 * 1024, total * 2 * 16 * 1024  * 1000000/ (1024.0 * 1024.0 * total));	
			return 0;
		}
		proto_pkg_t *pkg = (proto_pkg_t *)input;	
		pkg->id =  i;
		pkg->cmd = i + 1;
		pkg->ret = i + 2;
		pkg->seq = ++seq;

		seq_time_map[pkg->seq] = start;

		pkg->len = sizeof(proto_pkg_t) + strlen(buf) + 1;
		input[strlen(buf)] = '\0';
		memcpy(pkg->data, buf, strlen(buf) + 1);

		send(fd, input, pkg->len, 0);
		//printf("send: len=%u,id=%u,cmd=%u,seq=%u,ret=%u,%s:%lu\n\n", pkg->len, pkg->id, pkg->cmd, pkg->seq, pkg->ret, buf, strlen(buf) + 1);

	}


	free(evs);
	close(epfd);
	close(fd);

	return 0;
}
