/*
 * =====================================================================================
 *
 *       Filename:  fds.c
 *
 *    Description:  子进程对fd的管理
 *
 *        Version:  1.0
 *        Created:  12/07/2014 09:31:18 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:	xiaohou, houbin-12@163.com
 *   Organization:  XiaoHou, Inc. ShangHai CN. All rights reserved.
 *
 * =====================================================================================
 */

#include <stdint.h>
#include <string.h>
#include <glib.h>

#include "global.h"
#include "fds.h"

void do_free_fdsess(void *fdsess)
{
	g_slice_free1(sizeof(fdsess_t), fdsess);
}

int	init_fds()
{
	fds = g_hash_table_new_full(g_int_hash, g_int_equal, 0, do_free_fdsess);
	return 0;
}

fdsess_t *get_fd(int fd)
{
	return g_hash_table_lookup(fds, &fd);
}

void save_fd(fdsess_t *sess)
{
	g_hash_table_insert(fds, &(sess->fd), sess);
}

void remove_fd(int fd)
{
	g_hash_table_remove(fds, &fd);	
}
