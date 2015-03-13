/*
 * =====================================================================================
 *
 *       Filename:  net_util.c
 *
 *    Description:  real net util
 *
 *        Version:  1.0
 *        Created:  2014年11月24日 22时26分23秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:	xiaohou , houbin-12@163.com
 *   Organization:  XiaoHou, Inc. ShangHai CN. All rights reserved.
 *
 * =====================================================================================
 */

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <sys/socket.h>

#include <libnanc/log.h>

#include "net_util.h"
#include "util.h"
#include "fds.h"
#include "global.h"
#include "outer.h"


extern int handle_cli(int fd);
extern int do_proc_svr(int fd);
extern int do_fd_open(int fd);

int set_sock_snd_timeo(int sockfd, int millisec)
{
	struct timeval tv;

	tv.tv_sec  = millisec / 1000;
	tv.tv_usec = (millisec % 1000) * 1000;

	return setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
}

int set_sock_rcv_timeo(int sockfd, int millisec)
{
	struct timeval tv;

	tv.tv_sec  = millisec / 1000;
	tv.tv_usec = (millisec % 1000) * 1000;

	return setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

int set_sock_sndbuf_size(int sockfd, int size)
{
	return setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(int));
}

int set_sock_rcvbuf_size(int sockfd, int size)
{
	return setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(int));
}

int safe_tcp_recv_n(int sockfd, void* buf, int total)
{
	assert(total > 0);

	int recv_bytes, cur_len = 0;

	for (recv_bytes = 0; recv_bytes < total; recv_bytes += cur_len)	{
		cur_len = recv(sockfd, buf + recv_bytes, total - recv_bytes, 0);
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

int safe_tcp_send_n(int sockfd, const void* buf, int total)
{
	assert(total > 0);

	int send_bytes, cur_len;

	for (send_bytes = 0; send_bytes < total; send_bytes += cur_len) {
		cur_len = send(sockfd, buf + send_bytes, total - send_bytes, 0);
		if (cur_len == -1) {
			if (errno == EINTR) {
				cur_len = 0;
			} else if (errno == EAGAIN || errno == EWOULDBLOCK) {
				return send_bytes;
			} else {
				return -1;
			}
		}
	}

	return send_bytes;
}

int safe_socket_listen(const char* ipaddr, in_port_t port, int type, int backlog, int bufsize)
{
	TRACE(0, "backlog=%d,bufsize=%u", backlog, bufsize);
	assert((backlog > 0) && (bufsize > 0) && (bufsize <= (10 * 1024 * 1024)));

	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family  = AF_INET;
	servaddr.sin_port    = htons(port);
	if (ipaddr) {
		inet_pton(AF_INET, ipaddr, &servaddr.sin_addr);
	} else {	
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	}

	int listenfd;
	if ((listenfd = socket(AF_INET, type, 0)) == -1) {
		return -1;
	}

	int err;
	if (type != SOCK_DGRAM) {
		int reuse_addr = 1;	
		err = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));
		if (err == -1) {
			goto ret;
		}
	}

	err = setsockopt(listenfd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(int));
	if (err == -1) {
		goto ret;
	}
	err = setsockopt(listenfd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(int));
	if (err == -1) {
		goto ret;
	}

	err = bind(listenfd, (void*)&servaddr, sizeof(servaddr));
	if (err == -1) {
		goto ret;
	}

	if ((type == SOCK_STREAM) && (listen(listenfd, backlog) == -1)) {
		err = -1;
		goto ret;
	}

ret:
	if (err) {
		err      = errno;
		close(listenfd);
		listenfd = -1;
		errno    = err;		
	}

	return listenfd;
}

int safe_tcp_accept(int sockfd, struct sockaddr_in* peer, int nonblock)
{
	int err;
	int newfd;

	for ( ; ; ) {
		socklen_t peer_size = sizeof(*peer);
		newfd = accept(sockfd, (void*)peer, &peer_size);
		if (newfd >= 0) {
			break;
		} else if (errno != EINTR) {
			return -1;
		}
	}

	if (nonblock && (set_io_nonblock(newfd, 1) == -1)) {
		err   = errno;
		close(newfd);
		errno = err;
		return -1;
	}

	return newfd;
}

int safe_tcp_connect(const char* ipaddr, in_port_t port, int bufsize, int timeout)
{
	struct sockaddr_in peer;

	memset(&peer, 0, sizeof(peer));
	peer.sin_family  = AF_INET;
	peer.sin_port    = htons(port);
	if (inet_pton(AF_INET, ipaddr, &peer.sin_addr) <= 0) {
		return -1;
	}

	int sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if ( sockfd == -1 ) {
		return -1;
	}

	if (timeout > 0) {
		set_sock_snd_timeo(sockfd, timeout);
		set_sock_rcv_timeo(sockfd, timeout);
	}

	if (connect(sockfd, (void*)&peer, sizeof(peer)) == -1) {
		close(sockfd);
		return -1;
	}

	if (bufsize) {
		set_sock_sndbuf_size(sockfd, bufsize);
		set_sock_rcvbuf_size(sockfd, bufsize);
	}

	return sockfd;
}

int add_fd_to_epinfo(int epfd, int fd, int events)
{
	set_io_nonblock(fd, 1);

	struct epoll_event event;
	event.data.fd = fd;
	event.events = events | EPOLLET;

	return epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);	
}

int mod_fd_to_epinfo(int epfd, int fd, int events)
{
	struct epoll_event event;
	event.data.fd = fd;
	event.events = events | EPOLLET;

	return epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &event);	
}

int add_pfd_to_epinfo(int epfd, void *pfd, int events)
{
	fdsess_t *sess = pfd;
	set_io_nonblock(sess->fd, 1);

	struct epoll_event event;
	event.data.ptr = pfd;
	event.events = events | EPOLLET;

	return epoll_ctl(epfd, EPOLL_CTL_ADD, sess->fd, &event);	
}

int mod_pfd_to_epinfo(int epfd, void *pfd, int events)
{
	fdsess_t *sess = pfd;
	struct epoll_event event;
	event.data.ptr = pfd;
	event.events = events | EPOLLET;

	return epoll_ctl(epfd, EPOLL_CTL_MOD, sess->fd, &event);	
}


int send_to_cli(struct fdsess *sess, const void *msg, int const len)
{
	mem_block_t blk;

	blk.id = sess->id;
	blk.fd = sess->fd;
	blk.type = BLK_DATA;
	blk.len = len + blk_head_len;

	if (mq_push(&workmgr.works[blk.id].sq, &blk, msg) == -1) {
		ERROR(0, "%s error [len=%d]", __func__, len);
		return -1;
	}

	return 0;
}

int send_to_serv(int fd, void *msg, int len)
{
	return do_fd_send(fd, msg, len);
}


int connect_to_serv(const char *ip, int port, int bufsize, int timeout)
{
	struct sockaddr_in sockaddr;
	memset(&sockaddr, 0, sizeof(struct sockaddr_in));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(port);
	if (inet_pton(AF_INET, ip, &sockaddr.sin_addr) <= 0) {
		ERROR(0, "inet_pton error [ip=%s]", ip);
		return -1;
	}

	int fd = safe_tcp_connect(ip, port, bufsize, timeout);
	if (fd > 0) {
		INFO(0, "connect to [%s:%d]", ip, port);
		add_fdinfo_to_epinfo(fd, 0, fd_type_svr, sockaddr.sin_addr.s_addr, port); //idx无用由work处理
	} else {
		ERROR(0, "connect to [%s:%d] failed", ip, port);
		return -1;
	}

	return fd;
}

void free_buff(fd_buff_t *buff)
{
	if (buff->sbf) {
		free(buff->sbf);
		buff->sbf = NULL;
	}

	if (buff->rbf) {
		munmap(buff->rbf, setting.max_msg_len);
		buff->rbf = NULL;
	}

	buff->slen = 0;
	buff->rlen = 0;
	buff->msglen = 0;
	buff->sbf_size = 0;
}

int do_fd_send(int fd, void *data, int len)
{
	fd_buff_t *buff = &epinfo.fds[fd].buff;	
	int is_send = 0;
	if (buff->slen > 0) {
		if (do_fd_write(fd) == -1) { //fd断了
			do_fd_close(fd, 1);
			return -1;	
		}
		is_send = 1;
	}
	
	int send_len = 0;
	if (buff->slen == 0) { //发送缓冲区没有数据发送
		send_len = safe_tcp_send_n(fd, data, len);
		if (send_len == -1) {
			ERROR(0, "write fd error[fd=%u, err=%s]", fd, strerror(errno));
			do_fd_close(fd, 1);
			return -1;
		} 
	}

	if (len > send_len) { //如果没有发送完
		int left_len = len - send_len;
		if (!buff->sbf) { //没有空间
			buff->sbf = (char *)malloc(left_len);
			if (buff->sbf) {
				ERROR(0, "malloc err, [err=%s]", strerror(errno));
				return -1;
			}
			buff->sbf_size = left_len;
		} else if (buff->sbf_size < buff->slen + left_len) {
			buff->sbf = (char *)realloc(buff->sbf, buff->slen + left_len);
			if (!buff->sbf) {
				ERROR(0, "realloc err, [err=%s]", strerror(errno));
				return -1;
			}
			buff->sbf_size = left_len + buff->slen;
		}

		memcpy(buff->sbf + buff->slen, (char *)data + send_len, left_len);
		buff->slen += left_len;

		if (setting.max_buf_len && setting.max_buf_len < buff->slen) { //如果大于最大发送缓冲区
			ERROR(0, "sendbuf is exceeded max[fd=%d,buflen=%d,slen=%d]", fd, setting.max_buf_len, buff->slen);
			do_fd_close(fd, 1);
			return -1;

		}
	}

	if (buff->slen > 0 && !is_send) { //没有写完 上次没有写过 如果写过 不修改原来事件
		mod_fd_to_epinfo(epinfo.epfd, fd, EPOLLIN | EPOLLOUT);	//当缓冲区不满时继续写
	} else if (buff->slen == 0) { //写完 修改为可读事件
		mod_fd_to_epinfo(epinfo.epfd, fd, EPOLLIN);	
	}

	return 0;
}

int do_fd_write(int fd)
{
	int send_len;
	fd_buff_t *buff = &epinfo.fds[fd].buff;
	send_len = safe_tcp_send_n(fd, buff->sbf, buff->slen);

	if (send_len == 0) {
		return 0;
	} else if (send_len > 0) {
		if (send_len < buff->slen) {
			memmove(buff->sbf, buff->sbf + send_len, buff->slen - send_len);
			buff->slen -= send_len;
		}
	} else {
		ERROR(0, "write fd error[fd=%u, err=%s]", fd, strerror(errno));
	}

	return send_len;
}


int do_fd_close(int fd, int ismaster)
{
	if (epinfo.fds[fd].type == fd_type_null) {
		return 0;
	}

	if (ismaster) { //通知子进程
		mem_block_t blk;
		blk.id = epinfo.fds[fd].idx;
		blk.fd = fd;
		blk.type = BLK_CLOSE;
		blk.len = blk_head_len;
		if (mq_push(&workmgr.works[blk.id].rq, &blk, NULL) == -1) {
			ERROR(0, "mq is full noti child failed [fd=%d]", fd);
		};
	} else if (ismaster == 0) { //子进程自己处理逻辑
		if (so.on_serv_closed) {
			so.on_serv_closed(fd);
		}
	}

	//从可读队列中删除
	do_del_from_readlist(fd);
	//从待关闭队列中删除
	do_del_from_closelist(fd);

	//释放缓冲区
	free_buff(&epinfo.fds[fd].buff);
	epinfo.fds[fd].type = fd_type_null;

	close(fd);
	--epinfo.count;

	//替换最大fd
	if (epinfo.maxfd == fd) {
		int i; 
		for (i = fd - 1; i >= 0; --i) {
			if (epinfo.fds[i].type == fd_type_null) {
				break;
			}
		}
		epinfo.maxfd = i;
	}

	INFO(0, "master close [fd=%d]", fd);

	return 0;
}

void do_add_to_readlist(int fd) 
{
	if (!epinfo.fds[fd].flag) {
		list_add_tail(&epinfo.fds[fd].node, &epinfo.readlist);
		epinfo.fds[fd].flag |= CACHE_READ;
		TRACE(0, "add to readlist [fd=%u]", fd);
	}
}

void do_del_from_readlist(int fd)
{
	if (epinfo.fds[fd].flag & CACHE_READ) {
		epinfo.fds[fd].flag = 0;
		list_del_init(&epinfo.fds[fd].node);
		TRACE(0, "del from readlist [fd=%u]", fd);
	}
}

void do_add_to_closelist(int fd) 
{
	do_del_from_readlist(fd);
	if (!(epinfo.fds[fd].flag & CACHE_CLOSE)) {
		list_add_tail(&epinfo.fds[fd].node, &epinfo.closelist);
		epinfo.fds[fd].flag |= CACHE_CLOSE;
		TRACE(0, "add to closelist[fd=%u]", fd);
	}
}

void do_del_from_closelist(int fd)
{
	if (epinfo.fds[fd].flag & CACHE_CLOSE) {
		epinfo.fds[fd].flag = 0;
		list_del_init(&epinfo.fds[fd].node);
		TRACE(0, "del from closelist[fd=%u]", fd);
	}
}

int handle_read(int fd)
{
#ifdef ENABLE_TRACE
	TRACE(0, "%s", __func__);
#endif
	fd_buff_t *buff = &epinfo.fds[fd].buff;
	if (!buff->rbf) {
		buff->msglen = 0;
		buff->rlen = 0;
		buff->rbf = mmap(0, setting.max_msg_len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0); 
		if (buff->rbf == MAP_FAILED) {
			ERROR(0, "%s mmap error", __func__);
			return -1;
		}
	}

	//判断缓存区是否已满
	if (setting.max_msg_len <= buff->rlen) {
		ERROR(0, "recv buff full [fd=%u]", fd);
		return 0;
	}

	//接收消息
	int recv_len = safe_tcp_recv_n(fd, buff->rbf + buff->rlen, setting.max_msg_len - buff->rlen);

	if (recv_len > 0) { //有消息
		buff->rlen += recv_len;
	} else if (recv_len == 0) { //对端关闭
		ERROR(0, "[fd=%u,ip=%s] has closed", fd, inet_ntoa(*((struct in_addr *)&epinfo.fds[fd].addr.ip)));
		return -1;
	} else { //
		ERROR(0, "recv error[fd=%u,error=%u]", fd, strerror(errno));
		recv_len = 0;
		return -1;
	}

	if (buff->rlen == setting.max_msg_len) {
		//增加到可读队列里面
		do_add_to_readlist(fd);	
	} else {
		//从可读队列里面删除
		do_del_from_readlist(fd);	
	}

	return recv_len;
}

int handle_readlist(int ismaster)
{
	typedef int (*handle_msg)(int fd);
	static handle_msg func[2] = {do_proc_svr, handle_cli}; //处理函数不一样

	fd_wrap_t *pfd, *tmpfd;
	list_for_each_entry_safe(pfd, tmpfd, &epinfo.readlist, node) {
		DEBUG(0, "%s [fd=%u]", __func__, pfd->fd);
		if (unlikely(pfd->type == fd_type_listen)) { //打开连接 只有主进程才打开连接
			while(do_fd_open(pfd->fd)) {}	 //应该不会执行
		} else if (func[ismaster](pfd->fd) == -1) { //处理客户端的请求 读取
			do_fd_close(pfd->fd, ismaster); //接收失败 关闭
		}
	}

	return 0;
}

int handle_closelist(int ismaster)
{
	fd_wrap_t *pfd, *tmpfd;
	list_for_each_entry_safe(pfd, tmpfd, &epinfo.closelist, node) {
		DEBUG(0, "%s [fd=%u]", __func__, pfd->fd);
		if (pfd->buff.slen > 0) {	//不再接收
			if (do_fd_write(pfd->fd) == -1) {//写入缓存区
				pfd->buff.slen = 0; //写入错误不再写入
			};		
		}
		do_fd_close(pfd->fd, ismaster);
	}

	return 0;
}

int add_fdinfo_to_epinfo(int fd, int idx, int type, int ip, uint16_t port)
{
	set_io_nonblock(fd, 1);

	struct epoll_event event;
	event.data.fd = fd;

	switch (type) {
		case fd_type_listen:
		case fd_type_pipe:
		case fd_type_cli:
		case fd_type_svr:
			event.events = EPOLLIN | EPOLLET;
			break;
		default:
			event.events = EPOLLIN | EPOLLOUT | EPOLLET;
	}

	int ret = epoll_ctl(epinfo.epfd, EPOLL_CTL_ADD, fd, &event);	
	if (ret == -1) {
		ERROR(0, "err ctl add [epfd=%u][fd=%u][type=%u][%s]", epinfo.epfd, fd, type, strerror(errno));
		return -1;
	}

	//初始化fdinfo
	fd_wrap_t *pfd = &epinfo.fds[fd];
	++epinfo.seq;
	pfd->type = type;
	pfd->idx = idx;
	pfd->fd = fd;
	pfd->addr.ip = ip;
	pfd->addr.port = port;
	pfd->flag = 0;
	//初始化缓冲区
	fd_buff_t *buff = &pfd->buff;
	buff->slen = buff->rlen = 0;
	buff->sbf = buff->rbf = NULL;
	buff->msglen = 0;
	buff->sbf_size = 0;
	
	if (epinfo.maxfd < fd) epinfo.maxfd = fd;

	return 0;
}

