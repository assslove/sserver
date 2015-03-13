/*
 * =====================================================================================
 *
 *       Filename:  util.c
 *
 *    Description:  实用函数
 *
 *        Version:  1.0
 *        Created:  12/11/2014 01:52:42 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:	houbin , houbin-12@163.com
 *   Organization:  Houbin, Inc. ShangHai CN. All rights reserved.
 *
 * =====================================================================================
 */

#include <fcntl.h>
#include <sys/prctl.h>

#include <libnanc/log.h>

#include "util.h"

int init_rlimit()
{
	struct rlimit rlim;
	rlim.rlim_cur = rlim.rlim_max = RLIM_INFINITY;
	setrlimit(RLIMIT_CORE, &rlim);
	return 0;
}

int chg_proc_title(const char *fmt, ...) 
{
	char title[128] = {'\0'};

	va_list ap;
	va_start(ap, fmt);
	vsprintf(title, fmt, ap);
	va_end(ap);

	int len = strlen(title);
	memcpy(argv_start, title, len);
	argv_start[len] = '\0';
	//clear old environ
	memset(argv_start + len, '\0', (argv_end - argv_start) - len);

	prctl(PR_SET_NAME, title);
	return 0;
}

int save_args(int argc, char* argv[])
{
	g_argc = argc;
	g_argv = (char **)malloc(sizeof(char*) * argc);

	int i = 0;
	for (; i < argc; i++) {
		g_argv[i] = strdup(argv[i]);
	}

	argv_start = (char *)argv[0];
	argv_end = (char*)(argv[argc-1]) + strlen(argv[argc-1]) + 1;

	int env_size = 0;
	for (i = 0; environ[i]; i++) {
		env_size += strlen(environ[i]) + 1;
	}

	char *env_new = (char *)malloc(sizeof(char) * env_size);;
	if (env_new == NULL) {
		ERROR(0, "error malloc env");
		return 0;
	}

	for (i = 0; environ[i]; i++) {
		int len = strlen(environ[i]) + 1;
		argv_end += len;

		memcpy(env_new, environ[i], len);
		environ[i] = env_new;
		env_new += len;
	}

	return 0;
}

void free_args() 
{
	int i = 0;
	for (; i < g_argc; i++) {
		free(g_argv[i]);
	}
	free(g_argv);
}

void print_args()
{
	int i = 0;
	for (; i < g_argc; i++) {
		printf("%s\n", (g_argv[i]));
	}
}

void  print_env() 
{
	int i = 0;
	for (; environ[i]; i++) {
		printf("%s\n", environ[i]);
	}

	printf("-----------------------");
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

