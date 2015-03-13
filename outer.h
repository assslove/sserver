#ifndef OUT_H_
#define OUT_H_

#include <sys/types.h>

#include "fds.h"

/* @brief 提供对业务逻辑的接口
*/
#ifdef __cplusplus
extern "C" {
#endif

typedef struct ServInterface {
	void*   data_handle; 
	void*   handle;		

	void	(*handle_timer)();

	int		(*proc_cli_msg)(void* msg, int len, fdsess_t* fdsess);

	void	(*proc_serv_msg)(int fd, void* msg, int len);

	void	(*on_cli_closed)(int fd);

	void	(*on_serv_closed)(int fd);

	int 	(*serv_init)(int ismaster);

	int 	(*serv_fini)(int ismaster);

	int		(*get_msg_len)(int fd, const void *data, int len, int ismaster);
} serv_if_t;

extern serv_if_t so;

int  reg_data_so(const char* name);
int  reg_so(const char* name, int flag);
void unreg_data_so();
void unreg_so();

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif // OUTER_H_
