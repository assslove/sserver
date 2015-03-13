#ifndef WORK_H_
#define WORK_H_


extern int work_idx;

/* @brief
 */
int work_init(int i);

/* @brief 
 */
int work_dispatch(int i);

/* @brief
 */
int work_fini(int i);

/* @brief 读取共享队列读入
 */
int handle_mq_recv();

/* @brief 处理块读取
 */
int do_blk_msg(mem_block_t *blk);

/* @brief 处理块关闭
 */
int do_blk_close(mem_block_t *blk);

/* @brief 处理块打开
 */
int do_blk_open(mem_block_t *blk);

/* @brief 处理服务器端的数据处理
 */
int do_proc_svr(int fd);

/* @brief 处理管道
 */
int do_proc_pipe(int fd);

/* @brief 处理组播
 */
int do_proc_mcast(int fd);

/* @brief 关闭fd
 */
void close_cli(int fd);

/* @brief 用于同步相同组播地址可以知道本服务
 * @param idx 子进程索引
 */
void  do_syn_serv_addr();

/* @brief 获取本服务器ip
 */
uint32_t get_serv_ip();

/* @brief 获取本服务器port
 */
uint16_t get_serv_port();

/* @brief 获取本服务器id
 */
uint32_t get_serv_id();

/* @brief 获取本服务器地址
 */
const char* get_serv_ipstr();


#endif
