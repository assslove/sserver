/*
 * =====================================================================================
 *
 *       Filename:  conf.c
 *
 *    Description:  配置文件读取 参考openssl
 *
 *        Version:  1.0
 *        Created:  12/10/2014 04:15:11 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:	houbin , houbin-12@163.com
 *   Organization:  Houbin, Inc. ShangHai CN. All rights reserved.
 *
 * =====================================================================================
 */

#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <malloc.h>
#include <stdlib.h>

#include <glib.h>

#include <libnanc/log.h>

#include "conf.h"
#include "global.h"
#include "net_util.h"

static void do_free_item(void* item)
{
	g_free(item);
	item = NULL;
}


int load_simple_data()
{
	sim_data = g_hash_table_new_full(g_str_hash, g_str_equal, do_free_item, do_free_item);
	int fd = open("etc/simple.conf", O_RDWR);
	if (fd == -1) {
		ERROR(0, "simple.conf not existed : %s", strerror(errno));
		return -1;
	}

	char line[1024];
	char key[64];
	char value[128];
	char *ptr = (char *)mmap((void *)-1, 1024 * 10, PROT_READ, MAP_SHARED, fd, 0);
	if (ptr == NULL) {
		ERROR(0, "error mmap");
		return 0;
	}

	int start = 0;
	int end = 0;
	int i = 0;
	int is_note = 0;
	for (; ptr[i] != '\0'; i++) {
		if (ptr[i] == '#') {
			is_note = 1;
		} else if (ptr[i] == '\n') {
			end = i - 1;
			if (is_note) {
				start = i + 1;
				i = start - 1;
				is_note = 0;
				continue;
			} else {
				int len = end - start + 1;
				memcpy(line, ptr + start, len);
				if (len == 0) {
					start = i + 1;
					is_note = 0;
					continue;
				}
				line[len] = '\0';
				sscanf(line, "%s %s\n", key, value);
				g_hash_table_insert(sim_data, strdup(key), strdup(value));
				//清下个状态
				memset(key, 0, 64);
				memset(value, 0, 128);
				start = i + 1;
				is_note = 0;
			}
		}
	}

	munmap(ptr, 1024 * 10);
	close(fd);

	return 0;
}

void free_sim_data()
{
	g_hash_table_destroy(sim_data);
}


void print(void *key, void *value, void *userdata)
{
	BOOT(0, "%s-%s", (char *)key, (char *)value);
}

void print_simple_conf()
{
	g_hash_table_foreach(sim_data, print, NULL);	
}

int load_work_data()
{
	int nr_work = 100;
	workmgr.nr_work = 0;
	workmgr.nr_used = 0;
	workmgr.works = (work_t *)malloc(sizeof(work_t) * nr_work);
	workmgr.nr_work = nr_work;
	
	int fd = open("etc/work.conf", O_RDWR);
	if (fd == -1) {
		ERROR(0, "work.conf not existed: %s", strerror(errno));
		return -1;
	}

	char line[1024];
	char *ptr = (char *)mmap((void *)-1, 1024 * 10, PROT_READ, MAP_SHARED, fd, 0);
	if (ptr == NULL) {
		ERROR(0, "error mmap");
		return 0;
	}

	int id = 0, port = 0;
	char ip[32];
	int start = 0;
	int end = 0;
	int i = 0;
	int is_note = 0;
	for (; ptr[i] != '\0'; i++) {
		if (ptr[i] == '#') {
			is_note = 1;
		} else if (ptr[i] == '\n') {
			end = i - 1;
			if (is_note) {
				start = i + 1;
				i = start - 1;
				is_note = 0;
				continue;
			} else {
				int len = end - start + 1;
				memcpy(line, ptr + start, len);
				if (len == 0) {
					start = i + 1;
					is_note = 0;
					continue;
				}
				line[len] = '\0';
				sscanf(line, "%d %s %d", &id, ip, &port);
				if (workmgr.nr_used >= workmgr.nr_work) {
					workmgr.works = (work_t *)realloc(workmgr.works, sizeof(work_t) * workmgr.nr_work + nr_work);
					workmgr.nr_work += nr_work;
				} else {
					workmgr.works[workmgr.nr_used].id = id;	
					workmgr.works[workmgr.nr_used].idx = i;	
					memcpy(workmgr.works[workmgr.nr_used].ip, ip, 32);
					workmgr.works[workmgr.nr_used].port = port;
					++workmgr.nr_used;
				}
				//清下个状态
				start = i + 1;
				is_note = 0;
			}
		}
	}

	munmap(ptr, 1024 * 10);
	close(fd);

	return 0;
}

void print_work_conf()
{
	int i = 0;
	for (; i < workmgr.nr_used; i++) {
		work_t *work = &workmgr.works[i];
		DEBUG(0, "id=%d, ip=%s, port=%d", work->id, work->ip, work->port);
	}
}

void free_work_data()
{
	free(workmgr.works);
}

char* conf_get_str(const char *key)
{
	char *value = g_hash_table_lookup(sim_data, key);
	if (value == NULL) {
		return "";
	}

	return value;
}

int conf_get_int(const char *key)
{
	char *value = g_hash_table_lookup(sim_data, key);
	if (value == NULL) {
		return 0;
	}

	return atoi(value);
}

int conf_get_int_def(const char *key, int def)
{
	char *value = g_hash_table_lookup(sim_data, key);
	if (value == NULL) {
		return def;
	}

	return atoi(value);
}


int load_conf()
{
	if (load_simple_data() || load_work_data()) {
		return -1;
	}

#ifdef ENABLE_TRACE
	print_simple_conf();
	print_work_conf();
#endif

	return 0;
}

void free_conf()
{
	free_sim_data();
	free_work_data();
}

