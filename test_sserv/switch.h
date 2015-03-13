#ifndef SWITCH_H_
#define SWITCH_H_

#include <stdint.h>

/* @brief 定义包
*/
typedef struct proto_pkg {
	int len;
	uint16_t cmd;
	int id;
	int seq;
	int ret;
	uint8_t data[];
} __attribute__((packed)) proto_pkg_t;


int handle_switch(int fd, void *msg, int len);

#endif 
