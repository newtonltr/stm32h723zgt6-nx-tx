#ifndef THREAD_SOCKET_H
#define THREAD_SOCKET_H

#include "main.h"
#include "nx_api.h"
#include <stdint.h>



// 函数声明
void thread_socket_entry(ULONG thread_input);

// 外部变量声明 - 这些变量在thread_init.c中定义
extern TX_THREAD thread_socket_block;
extern NX_IP ip_0;
extern ULONG ip0_address;
extern NX_PACKET_POOL pool_0;
extern TX_SEMAPHORE pc_unpack_semaphore;
extern TX_MUTEX pc_unpack_mutex;

// 外部变量声明 - 这些变量在thread_socket.c中定义
extern struct pc_unpack_data_t pc_unpack_data;
extern NX_TCP_SOCKET tcp_socket;


UINT nx_send(NX_TCP_SOCKET *socket, uint8_t *data, uint32_t len);
void pc_command_unpack(uint8_t *buffer, uint8_t comm_type);



#endif // THREAD_SOCKET_H