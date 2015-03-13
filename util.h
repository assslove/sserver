#ifndef UTIL_H_
#define UTIL_H_

#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdarg.h>

extern int g_argc;
extern char **g_argv;
extern char *argv_start;
extern char *argv_end;
extern char *env_end;
extern char **environ;

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

/* @brief 初始化rlimit信息
 */
inline int init_rlimit();

/* @brief 修改进程名字
 */
int chg_proc_title(const char *fmt, ...);

/* @brief 保存参数列表
 */
int save_args(int argc, char* argv[]);
void free_args();
void print_args();
void print_env();

/* @brief 
 */
int set_io_nonblock(int fd, int nonblock);

#endif
