/*
 * =====================================================================================
 *
 *       Filename:  out.c
 *
 *    Description:  用于动态调用so中函数，使业务逻辑能够嵌入到本网络框架中
 *
 *        Version:  1.0
 *        Created:  2014年12月29日 16时30分55秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:	houbin, houbin-12@163.com
 *   Organization:  Houbin, Inc. ShangHai CN. All rights reserved.
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include <libnanc/log.h>

#include "outer.h"

serv_if_t so;

#define DLFUNC_NO_ERROR(h, v, name) \
		do { \
			v = dlsym(h, name); \
			dlerror(); \
		} while (0)

#define DLFUNC(h, v, name) \
		do { \
			v = dlsym (h, name); \
			if ((error = dlerror ()) != NULL) { \
				ERROR(0, "dlsym error, %s", error); \
				dlclose(h); \
				h = NULL; \
				goto out; \
			} \
		} while (0)

int reg_data_so(const char* name)
{
	char* error; 
	int   ret_code = 0;
	if (name == NULL) {
		return 0;
	}

	so.data_handle = dlopen(name, RTLD_NOW | RTLD_GLOBAL);
	if ((error = dlerror()) != NULL) {
		ERROR(0, "dlopen error, %s", error);
		ret_code = 0;
	}

	BOOT(ret_code, "dlopen %s", name);

	return ret_code;
}
			
int reg_so(const char* name, int flag)
{
	char* error; 
	int   ret_code = -1;
	
	so.handle = dlopen(name, RTLD_NOW);
	if ((error = dlerror()) != NULL) {
		ERROR(0, "dlopen error, %s", error);
		goto out;
	}
	
	DLFUNC(so.handle, so.serv_init, "serv_init");
	DLFUNC(so.handle, so.serv_fini, "serv_fini");
	DLFUNC(so.handle, so.handle_timer, "handle_timer");

	DLFUNC(so.handle, so.get_msg_len, "get_msg_len");
	DLFUNC(so.handle, so.proc_cli_msg, "proc_cli_msg");
	DLFUNC(so.handle, so.proc_serv_msg, "proc_serv_msg");
	DLFUNC(so.handle, so.on_cli_closed, "on_cli_closed");
	DLFUNC(so.handle, so.on_serv_closed, "on_serv_closed");

	//DLFUNC_NO_ERROR(so.handle,	so.proc_mcast_msg, "proc_mcast_msg");

	ret_code = 0;

out:
	if (!flag) {
		BOOT(ret_code, "dlopen %s", name);
	} else {
		INFO(0, "RELOAD %s [%s]", name, (ret_code ? "FAIL" : "OK"));
	}
	return ret_code;
}

void unreg_data_so()
{
	if (so.data_handle != NULL){
		dlclose(so.data_handle);
		so.data_handle = NULL;
	}
}

void unreg_so()
{
	if (so.handle != NULL){
		dlclose(so.handle);
		so.handle = NULL;
	}
}

