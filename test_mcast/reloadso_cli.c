/*
 * =====================================================================================
 *
 *       Filename:  reloadso_cli.c
 *
 *    Description:  重载业务逻辑客户端
 *
 *        Version:  1.0
 *        Created:  2015年03月18日 19时14分03秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:	houbin , houbin-12@163.com
 *   Organization:  Houbin, Inc. ShangHai CN. All rights reserved.
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <unistd.h>
#include <getopt.h>

#include <sserv/mcast.h>
#include <sserv/net_util.h>

int main(int argc, char* argv[])
{
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd == -1) {
		printf("socket error");
		return 1;
	}

	set_mcast_ttl(sockfd, 1);
	set_mcast_loop(sockfd, 1);

	struct sockaddr_in mcast_sa;
	mcast_sa.sin_family = AF_INET;
	mcast_sa.sin_port = htons(8888);
	mcast_sa.sin_addr.s_addr = inet_addr("239.0.0.2");

	struct in_addr local_addr;
	local_addr.s_addr = inet_addr("127.0.0.1");

	int ret = set_mcast_if(sockfd, local_addr);

	if (ret == -1) {
		printf("set if error\n");
		return 1;
	}

	char buff[1024];
	mcast_pkg_t *pkg = (mcast_pkg_t *)buff;
	pkg->len = sizeof(mcast_pkg_t) + sizeof(reload_so_t);
	pkg->mcast_type = MCAST_RELOAD_SO;
	reload_so_t *reload = (reload_so_t *)pkg->data;
	reload->id = 1;
	strcpy(reload->servname, "online");
	strcpy(reload->soname, "/home/houbin/work/sserver/test_sserv/test_serv.so");

	int size = sendto(sockfd, buff, pkg->len, 0, (struct sockaddr *)&mcast_sa, sizeof(struct sockaddr_in));
	if (size < 0) {
		printf("send to error\n");
		return 1;
	}

	printf("send successful\n");

	close(sockfd);
	return 0;
}
