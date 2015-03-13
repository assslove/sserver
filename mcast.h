#ifndef MCAST_H_
#define MCAST_H_

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

/* @brief 服务通知信息
 */
typedef struct serv_noti {
	uint32_t ip;	//服务ip
	uint16_t port;	//服务端口号
	uint16_t id;	//服务id
	char servname[32]; //服务名称
} __attribute__((packed)) serv_noti_t;

/* @brief 重载业务逻辑信息
 */
typedef struct reload_so {
	uint16_t id;	 //服务器id 0表示所有进程
	char servname[32]; //服务名字
} __attribute__((packed)) reload_so_t;

/* @brief 重载业务逻辑名字
 */
typedef struct reload_conf {
	uint16_t id;		//服务器id 0表示所有进程
	uint16_t conf_id;	//conf文件id;
} __attribute__((packed)) reload_conf_t;

/* @brief 缓冲地址结构体
 */
typedef struct addrcach {
	int serv_id;		//服务id
	uint32_t ip;		//ip
	uint16_t port;		//port
	uint32_t last_syn;  //上一次同步时间
} __attribute__((packed)) addrcach_t;


typedef struct servcach {
	char serv_name[16]; //服务名字
	GHashTable *addrs;	//所有服务地址
} __attribute__((packed)) servcach_t;

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
