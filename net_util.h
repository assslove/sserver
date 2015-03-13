#ifndef _NET_UTIL_H
#define _NET_UTIL_H

#include <sys/socket.h> /* basic socket definition */
#include <netinet/in.h> /* sockaddr_in and other internet def */
#include <sys/types.h>
#include <sys/epoll.h>
#include <stdint.h>

#include <libnanc/list.h>

#include "mem_queue.h"

struct fdsess;

/* @brief 服务器类型
 */
enum SERV_TYPE {
	SERV_MASTER = 0, 
	SERV_WORK = 0
};

/* @brief 缓存队列的类型
 */
enum CACHE_TYPE {
	CACHE_READ = 1, 	
	CACHE_CLOSE = 2
};

/* fd 类型
 */
enum fd_type {
	fd_type_listen = 0,
	fd_type_connect,
	fd_type_mcast, 
	fd_type_pipe, 
	fd_type_cli,  //客户端fd
	fd_type_svr,  //服务端fd
	fd_type_null, //空类型
};

/* @brief 工作进程配置项
 */
typedef struct work {
	uint32_t id;  //配置id
	uint16_t idx; //索引
	char ip[32];
	uint16_t port;
	uint8_t proto_type; 
	mem_queue_t rq;	 //接收队列
	mem_queue_t sq;  //发送队列

	uint32_t next_syn_addr;			//下一次更新地址时间
	uint32_t next_del_expire_addr;  //下一次删除过期地址时间
}__attribute__((packed)) work_t;

/* @brief work配置项
 */
typedef struct work_mgr {
	int nr_work;
	int nr_used;
	work_t *works; //配置项
} __attribute__((packed)) work_mgr_t;

/* @brief 连接缓存区 用于读写
 */
typedef struct fd_buff {
	int slen;		//发送缓冲长度
	int rlen;		//接收缓冲区长度
	char *sbf;			//发送缓冲区
	char *rbf;			//接收缓冲区
	int msglen;			//消息长度
	int sbf_size;		//大小
}__attribute__((packed)) fd_buff_t;

/* @brief 地址结构
 */
typedef struct fd_addr {
	uint32_t ip;
	uint16_t port;
} __attribute__((packed)) fd_addr_t;

/* @brief fd信息
 */
typedef struct fd_wrap {
	uint8_t type;
	int fd;
	int idx; //epinfo->fds index
	//int id;  //work id index
	uint8_t flag; //标志 CACHE_TYPE
	fd_buff_t buff; //缓存
	fd_addr_t addr; //地址信息
	list_head_t node;   //在可读或者关闭链表的位置
} __attribute__((packed)) fd_wrap_t;

/* @brief 对epoll的封装
 */
typedef struct epoll_info {
	int epfd;
	struct epoll_event *evs;
	fd_wrap_t *fds;
	int maxfd;
	int maxev;
	uint32_t seq; 
	uint32_t count;
	list_head_t readlist; //待读取链表
	list_head_t closelist; //待关闭链表
}__attribute__((packed)) epoll_info_t;

typedef struct svr_setting {
	int nr_max_event;	// 最大的事件类型 epoll_create ms不需要这个参数了
	int nr_max_fd;		// fd max num
	int mem_queue_len;	// 共享内存队列长度
	int max_msg_len;	// 最大消息长度
	int max_buf_len;	//发送(接收)缓冲区最大长度 超过报错
	char srv_name[32];	//服务器名字
	int mcast_msg_len;  //组播包长
	int raw_buf_len;	//socket buf 长度
	char text_so[32];	//text_so
	char data_so[32];	//data_so;
	uint8_t log_level;  //日志级别
	uint32_t log_maxfiles; //最大日志文件个数
	uint32_t log_size;	//日志个数
	char log_dir[32];	//日志目录
	char mcast_ip[16];  //组播地址
	uint16_t mcast_port; //组播端口号
	char mcast_out_ip[16]; //组播出口地址
} svr_setting_t;

/*	
 *	@brief net util
 *	@ref 
 */

/* @brief 设置sock send time out
 */
int set_sock_snd_timmo(int sockfd, int millisec);

/* @brief 设置sock recv time out
 */
int set_sock_rcv_timeo(int sockfd, int millisec);

/* @brief 设置发送缓存区
 */
int set_sock_sndbuf_size(int sockfd, int size);

/* @brief 设置接收缓存区
 */
int set_sock_rcvbuf_size(int sockfd, int size);

/* @brief 接收total字节数据到buf
 */
int safe_tcp_recv_n(int sockfd, void* buf, int total);

/* @brief 发送total字节数据到sockfd
 */
int safe_tcp_send_n(int sockfd, const void* buf, int total);

/* @brief 监听socket
 */
int safe_socket_listen(const char* ipaddr, in_port_t port, int type, int backlog, int bufsize);

/* @brief接收socket
 */
int safe_tcp_accept(int sockfd, struct sockaddr_in* peer, int nonblock);

/* @brief 连接指定的sock
 */
int safe_tcp_connect(const char* ipaddr, in_port_t port, int bufsize, int timeout);

/* @brief 加入epoll
 */
int add_fd_to_epinfo(int epfd, int fd, int events);

/* @brief 修改epoll的属性
 */
int mod_fd_to_epinfo(int epfd, int fd, int events);

/* @brief 
 */
int add_pfd_to_epinfo(int epfd, void *pfd, int events);

/* @brief 
 */
int mod_pfd_to_epinfo(int epfd, void *pfd, int events);

/* @brief work进程向客户端发送信息调用的接口
 * @note 保证发送的包长小于共享队列长度
 */
int send_to_cli(struct fdsess *sess, const void *msg, int const len);

/* @brief work进程向所连服务器发送调用
 * @note 保证发送缓冲区满时进行缓冲
 */
int send_to_serv(int fd, void *msg, int len);

/* @brief 释放缓存区
 */
void free_buff(fd_buff_t *buff);

/* @brief 向fd发送消息
 */
int do_fd_send(int fd, void *data, int len);

/* @brief 从缓存区向fd发送消息
 */
int do_fd_write(int fd);

/* @brief 把fd关掉 释放资源
 */
int do_fd_close(int fd, int ismaster);

/* @brif 将fd增加 可读队列里面
 */
void do_add_to_readlist(int fd);

/* @brief fd从可读队列删除
 */
void do_del_from_readlist(int fd);

/*  @brief 将fd增加到待关闭队列里面
*/
void do_add_to_closelist(int fd);

/* @brief 将fd从关闭队列里面删除
 */
void do_del_from_closelist(int fd);

/*  @brief 连接某个服务器 用于work进程
 */
int connect_to_serv(const char *ip, int port, int bufsize, int timeout);

/* @brief  handle read
 */
int handle_read(int fd);

/* @brief 处理可读队列
 */
int handle_readlist(int ismaster);

/* @brief 处理待关闭对列
 */
int handle_closelist(int ismaster);

/* @brief 增加fd到epinfo
 */
int add_fdinfo_to_epinfo(int fd, int idx, int type, int ip, uint16_t port);

#endif
