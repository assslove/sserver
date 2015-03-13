#ifndef MASTER_H_
#define MASTER_H_

struct mem_block;

/* @brief master init
 * @note 
 */
int master_init();

/* @brief 
 */
int master_listen(int i);

int master_mq_create(int i); 

/* @brief 
 */
int master_dispatch();

/* @brief 
 */
int master_fini();


/*  @brief 处理cli
 */
int handle_cli(int fd);


/* @brief 处理管道的读取
 * @note 直接就可以
 */
int handle_pipe(int fd);

/* @brief 处理往客户端发送消息
 */
void handle_mq_send();

/* @brief 发送消息块
 */
int do_blk_send(struct mem_block *blk);



/* @brief 执行fd打开
 */
int do_fd_open(int fd);

/* @brief 初始化配置信息
 */
int init_setting();

/* @brief master为work进程初始化 用于通信
 */
int master_init_for_work(int id);

/* @brief  将消息转化为blk
 */
void raw2blk(int fd, struct mem_block *blk);

/* @brief 处理信号
 * @return 0-success -1-error
 */
int handle_signal();

/* @brief 对应描述符被挂断 实现子进程重启
 * @note 由于父进程与子进程共享资源，所以不能在收到信号的时候处理
 *  信号收到时只处理僵尸进程
 */
void handle_hup(int fd);
#endif
