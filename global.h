#ifndef GLOBAL_H_
#define GLOBAL_H_

#include <glib.h>

struct work_mgr;
struct epoll_info;
struct svr_setting;
struct servcach;

/* 定义的一些常量
 */
enum CONST_DEF{
	MAX_WORKS = 1024, 
	MCAST_MSG_LEN = 4096, 
	PIPE_MSG_LEN = 1024
};

/* @brief 返回类型
 */
enum RTYPE {
	SUCCESS = 0,
	ERROR = -1 
};


extern int g_argc;
extern char **g_argv;
extern char *argv_start;
extern char *argv_end;
extern char *env_end;

extern int chl_pids[MAX_WORKS];
extern struct work_mgr workmgr;
extern struct epoll_info  epinfo;
extern struct svr_setting  setting;
extern GHashTable *sim_data;
extern GHashTable *fds;
extern int stop;
extern GHashTable *g_servs;
extern int work_idx;
#endif
