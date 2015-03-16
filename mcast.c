/*
 * =====================================================================================
 *
 *       Filename:  mcast.c
 *
 *    Description:  组播使用 (1-服务发现; 2-重载业务逻辑 3-重载配置)
 *
 *        Version:  1.0
 *        Created:  2015年03月05日 15时33分07秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:	houbin , houbin-12@163.com
 *   Organization:  Houbin, Inc. ShangHai CN. All rights reserved.
 *
 * =====================================================================================
 */
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>

#include <glib.h>

#include <libnanc/log.h>

#include "mcast.h"
#include "global.h"

int mcast_cli_init(char *mcast_ip, uint16_t mcast_port, char *local_ip) 
{
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd == -1) {
		ERROR(0, "failed create mcast fd [%s]", strerror(errno));
		return -1;
	}

	struct sockaddr_in local_sa;
	local_sa.sin_family = AF_INET;
	local_sa.sin_port = htons(mcast_port);
	local_sa.sin_addr.s_addr = INADDR_ANY;

	int flag = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) == -1) {
		ERROR(0, "set sock reuseaddr failed [%s]", strerror(errno));
		return -1;
	}

	int ret = bind(sockfd, (struct sockaddr*)&local_sa, sizeof(struct sockaddr_in));
	if (ret == -1) {
		ERROR(0, "mcast bind failed [%s]", strerror(errno));
		return -1;
	}

	if (set_mcast_ttl(sockfd, 1) == -1) {
		ERROR(0, "set mcast ttl failed [%s]", strerror(errno));
		return -1;
	}

	if (set_mcast_loop(sockfd, 1) == -1) {
		ERROR(0, "set mcast loop failed [%s]", strerror(errno));
		return -1;
	}

	struct ip_mreq req;
	req.imr_multiaddr.s_addr = inet_addr(mcast_ip);
	req.imr_interface.s_addr = inet_addr(local_ip);

	if (join_mcast(sockfd, &req) == -1) {
		ERROR(0, "join mcast failed [%s]", strerror(errno));
		return -1;
	}

	return sockfd;
}

int send_pkg_to_mcast(char *mcast_ip, uint16_t mcast_port, char *local_ip, int mcast_type, int len, void *data)
{
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd == -1) {
		ERROR(0, "create serv socket failed [%s]", strerror(errno));
		return -1;
	}

	if (set_mcast_ttl(sockfd, 1) == -1) {
		ERROR(0, "set mcast ttl failed [%s]", strerror(errno));
		return -1;
	}

	if (set_mcast_loop(sockfd, 1) == -1) {
		ERROR(0, "set mcast loop failed [%s]", strerror(errno));
		return -1;
	}

	struct sockaddr_in mcast_sa;
	mcast_sa.sin_family = AF_INET;
	mcast_sa.sin_port = htons(mcast_port);
	mcast_sa.sin_addr.s_addr = inet_addr(mcast_ip);

	struct in_addr local_addr;
	local_addr.s_addr = inet_addr(local_ip);
	if (set_mcast_if(sockfd, local_addr) == -1) {
		ERROR(0, "set mcast if failed [%s]", strerror(errno));
		return -1;
	}

	static char buff[1024] = {'\0'};
	mcast_pkg_t *pkg = (mcast_pkg_t *)buff;
	pkg->len = len + sizeof(mcast_pkg_t);
	pkg->mcast_type = mcast_type;
	memcpy(pkg->data, data, len);

	int size = sendto(sockfd, buff, pkg->len, 0, (struct sockaddr*)&mcast_sa, sizeof(struct sockaddr_in));
	if (size != pkg->len) {
		ERROR(0, "send error[sendsize=%u][realsize=%u]", size, pkg->len);
		return -1;
	}

	DEBUG(0, "send to [%s:%u][type=%u]", mcast_ip, mcast_port, mcast_type);

	close(sockfd);

	return 0;	
}

/* @brief 设置组播ttl
*/
int set_mcast_ttl(int fd, int ttl)
{
	return setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
}

/* @brief 设置组播loop
*/
int set_mcast_loop(int fd, int loop)
{
	return setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
}

/* @brief 设置组播接口
*/
int set_mcast_if(int fd, struct in_addr in)
{
	return setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &in, sizeof(struct in_addr));
}

/* @brief 加入组播
*/
int join_mcast(int fd, struct ip_mreq *req)
{
	return setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, req, sizeof(struct ip_mreq));
}

/* @brief 离开组播
*/
int leave_mcast(int fd, struct ip_mreq *req)
{
	return setsockopt(fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, req, sizeof(struct ip_mreq));
}

int do_mcast_serv_noti(mcast_pkg_t *pkg) 
{
	serv_noti_t *serv = (serv_noti_t *)pkg->data;
	add_serv_to_cach(serv->servname, serv->id, serv->ip, serv->port);
	return 0;
}

void do_free_serv(void *item)
{
	servcach_t *serv = (servcach_t *)item;
	if (serv) {
		if (serv->addrs) {
			g_hash_table_destroy(serv->addrs);
		}
		g_slice_free1(sizeof(servcach_t), item);
	}
}

void do_free_addr(void *item) 
{
	g_slice_free1(sizeof(addrcach_t), item);
}

void init_mcast_servs()
{
	g_servs = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, do_free_serv);	
}

void add_serv_to_cach(const char* servname, uint32_t serv_id, uint32_t ip, uint16_t port)
{
	servcach_t *serv = g_hash_table_lookup(g_servs, servname);		
	if (!serv) {
		serv = g_slice_alloc(sizeof(servcach_t));
		if (!serv) {
			ERROR(0, "can't alloc servcach_t");
			return ;
		}
		strcpy(serv->serv_name, servname);
		serv->addrs = g_hash_table_new_full(g_int_hash, g_int_equal, NULL, do_free_addr);
		addrcach_t *addr = g_slice_alloc(sizeof(addrcach_t));
		if (!addr) {
			ERROR(0, "can't alloc addrcach_t");
			return ;
		}
		addr->serv_id = serv_id;
		addr->ip = ip;
		addr->port = port;
		addr->last_syn = time(NULL);
		g_hash_table_insert(serv->addrs, &addr->serv_id, addr);
		g_hash_table_insert(g_servs, serv->serv_name, serv);
	} else {
		addrcach_t *tmp = g_hash_table_lookup(serv->addrs, &serv_id);
		if (tmp) {
			tmp->serv_id = serv_id;
			tmp->ip = ip;
			tmp->port = port;
			tmp->last_syn = time(NULL);
		} else {
			addrcach_t *addr = g_slice_alloc(sizeof(addrcach_t));
			if (!addr) {
				ERROR(0, "can't alloc addrcach_t");
				return ;
			}
			addr->serv_id = serv_id;
			addr->ip = ip;
			addr->port = port;
			addr->last_syn = time(NULL);

			g_hash_table_insert(serv->addrs, &addr->serv_id, addr);
		}
	}

	INFO(0, "ADD SERVADDR [servname=%s][id=%u][ip=%s][port=%u]", \
				servname, serv_id, inet_ntoa(*((struct in_addr *)&ip)), port);
}
