/*
 * =====================================================================================
 *
 *       Filename:  switch.cpp
 *
 *    Description:  switch 测试
 *
 *        Version:  1.0
 *        Created:  2014年12月30日 15时36分28秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:	houbin , houbin-12@163.com
 *   Organization:  Houbin, Inc. ShangHai CN. All rights reserved.
 *
 * =====================================================================================
 */

#include <string.h>

extern "C" {
#include <libnanc/log.h>
#include <sserv/fds.h>
#include <sserv/net_util.h>
}

#include "switch.h"

int handle_switch(int fd, void *msg, int len)
{
	proto_pkg_t *pkg = reinterpret_cast<proto_pkg_t *>(msg);

	DEBUG(pkg->id, "switch callback len=%u,id=%u,seq=%u,cmd=%u,ret=%u, msg=%s", pkg->len, pkg->id, pkg->seq, pkg->cmd, pkg->ret, (char *)pkg->data);

	uint32_t  cli[1024];
	memcpy(cli, msg, pkg->len);

	return send_to_cli(get_fd(pkg->seq), cli, pkg->len);
}
