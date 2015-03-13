#ifndef CONF_H_
#define CONf_H_

/* @brief 加载配置文件
 */
int load_conf();

/* @brief 释放空间
 */
void free_conf();

/* @brief 加载工程文件
 */
int load_simple_data();

/* @brief 打印工程配置
 */
void print_simple_conf();

/* @brief 加载work配置
 */
int load_work_data();

/* @brief 释放work配置
 */
void free_work_data();

/* @brief 获取字符串
 */
char* conf_get_str(const char *key);

/* @brief 获取整型
 */
int conf_get_int(const char *key);

/* @brief 获取整型
 */
int conf_get_int_def(const char *key, int def);

#endif
