#ifndef MCAST_PUB_H_
#define MCAST_PUB_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <glib.h>

/* @brief 组播类型定义
 */
enum MCAST_CMD {
	MCAST_SERV_NOTI = 1, //服务发现
	MCAST_RELOAD_SO = 2, //重载业务逻辑
	MCAST_RELOAD_CONF = 3 //重载配置文件
};

/* @brief 组播包格式定义
 */
typedef struct mcast_pkg {
	int len;			//协议长度
	uint8_t mcast_type; //组播类型,见MCAST_TYPE
	uint8_t data[];		//协议数据
} __attribute__((packed)) mcast_pkg_t;

/* @brief 重载业务逻辑信息
 */
typedef struct reload_so {
	uint16_t id;	 //服务器id 0表示所有进程
	char servname[32]; //服务名字
	char soname[64];   //so名字
} __attribute__((packed)) reload_so_t;

/* @brief 重载业务逻辑名字
 */
typedef struct reload_conf {
	uint16_t id;		//服务器id 0表示所有进程
	char servname[32];  //服务名字
	uint16_t conf_id;	//conf文件id;
} __attribute__((packed)) reload_conf_t;

/* @brief 设置组播ttl
*/
int set_mcast_ttl(int fd, int ttl);

/* @brief 设置组播loop
*/
int set_mcast_loop(int fd, int loop);

/* @brief 设置组播接口
*/
int set_mcast_if(int fd, struct in_addr in);

/* @brief 加入组播
*/
int join_mcast(int fd, struct ip_mreq *req);

/* @brief 离开组播
*/
int leave_mcast(int fd, struct ip_mreq *req);


/* @brief 初始化组播客户端接口
 * @param mcast_ip 组播ip
 * @param mcast_port 组播端口号
 * @param local_ip 本地ip
 *
 * @return >0 组播fd -1 error
 */
int mcast_cli_init(char *mcast_ip, uint16_t mcast_port, char *local_ip); 

/* @brief 发送数据给组播
 * @param mcast_ip 组播ip
 * @param mcast_port 组播端口号
 * @param local_ip 本地ip
 * @param mcast_type
 * @param len 
 * @param data
 */
int send_pkg_to_mcast(char *mcast_ip, uint16_t mcast_port, char *local_ip, int mcast_type, int len, void *data);



#endif
