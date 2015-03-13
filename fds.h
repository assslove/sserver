#ifndef FDS_H_
#define FDS_H_


#include <sys/socket.h>
#include <stdint.h>

/* @brief 客户端fd信息
 */
typedef struct fdsess {
	int fd;
	int id; //work idx
	int ip;
	int port;
} fdsess_t;

/* @brief 保存fd
 */
void save_fd(fdsess_t *fdsess);

/* @brief 初始化fds
 */
int	init_fds();

/* @brief 获取fd
 */
fdsess_t *get_fd(int fd);

/* @brief remove fd
 */
void remove_fd(int fd);

#endif 
