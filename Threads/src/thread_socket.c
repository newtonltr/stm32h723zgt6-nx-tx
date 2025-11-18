#include "thread_socket.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "usart.h"
#include "pc_protocol.h"
#include "emb_flash.h"

// TCP socket相关参数定义在这个文件中
NX_TCP_SOCKET tcp_socket;
#define TCP_SERVER_PORT 7000  // 服务器监听端口

// 消息缓冲区
#define MAX_MESSAGE_SIZE 256
uint8_t command_buffer[MAX_MESSAGE_SIZE] = {0};
struct pc_unpack_data_t pc_unpack_data = {0};


UINT nx_send(NX_TCP_SOCKET *socket, uint8_t *data, uint32_t len)
{
    NX_PACKET *packet_ptr;
    UINT status = 0;

    // 分配数据包
    status |= nx_packet_allocate(&pool_0, &packet_ptr, NX_TCP_PACKET, NX_WAIT_FOREVER);
    if (status != NX_SUCCESS)
    {
        return status;
    }

    // 将消息附加到数据包
    status = nx_packet_data_append(packet_ptr, 
                                  (VOID *)data, 
                                  len, 
                                  &pool_0, 
                                  NX_WAIT_FOREVER);
    if (status != NX_SUCCESS)
    {
        nx_packet_release(packet_ptr);
        return status;
    }
    
    // 发送数据包
    status = nx_tcp_socket_send(socket, packet_ptr, NX_WAIT_FOREVER);
    if (status != NX_SUCCESS)
    {
        nx_packet_release(packet_ptr);
    }
    
    return status;

}

UINT nx_receive(NX_TCP_SOCKET *socket, uint8_t *data, ULONG *len, ULONG wait_option)
{
    NX_PACKET *packet_ptr;
    UINT status = 0;
    
    status = nx_tcp_socket_receive(socket, &packet_ptr, wait_option);
    if (status == NX_SUCCESS)
    {
         // 读取数据包内容
        status = nx_packet_data_retrieve(packet_ptr, data, len);
        // 释放数据包
        nx_packet_release(packet_ptr);
    }
    return status;
}

// 线程入口函数
void thread_socket_entry(ULONG thread_input)
{
    UINT status;
    NX_PACKET *receive_packet;
    ULONG bytes_read;
    static uint32_t no_recv_cnt = 0;
    
    // 创建TCP服务器套接字
    status = nx_tcp_socket_create(&ip_0, &tcp_socket, "TCP Server Socket", 
                                 NX_IP_NORMAL, NX_FRAGMENT_OKAY, NX_IP_TIME_TO_LIVE, 
                                 1024, NX_NULL, NX_NULL);
    if (status != NX_SUCCESS)
    {
        return;
    }
    
    // 绑定TCP套接字到服务器端口
    status = nx_tcp_server_socket_listen(&ip_0, TCP_SERVER_PORT, &tcp_socket, 5, NX_NULL);
    if (status != NX_SUCCESS)
    {
        nx_tcp_socket_delete(&tcp_socket);
        return;
    }
        
    while (1) {
        // 等待客户端连接
        status = nx_tcp_server_socket_accept(&tcp_socket, NX_NO_WAIT);
        sleep_ms(1);
        if (status != NX_SUCCESS)
        {
            nx_tcp_server_socket_unaccept(&tcp_socket);
            nx_tcp_server_socket_relisten(&ip_0, TCP_SERVER_PORT, &tcp_socket);
            continue;
        }

        no_recv_cnt = 0;
        while (1)
        {
            ULONG len = 0;
            status = nx_receive(&tcp_socket, command_buffer, &len, NX_NO_WAIT);
            if (status == NX_NOT_CONNECTED) {
                no_recv_cnt = 0;
                nx_tcp_server_socket_unaccept(&tcp_socket);
                nx_tcp_server_socket_relisten(&ip_0, TCP_SERVER_PORT, &tcp_socket);
                break;
            }
            if (status == NX_SUCCESS)
            {
                no_recv_cnt = 0;
                pc_command_unpack(command_buffer, COMM_TYPE_TCP);
            }
            else
            {
                no_recv_cnt++;
                if (no_recv_cnt > 20 * 1000) // 超过一定时间没有接收到数据，断开连接
                {
                    no_recv_cnt = 0;
                    nx_tcp_server_socket_unaccept(&tcp_socket);
                    nx_tcp_server_socket_relisten(&ip_0, TCP_SERVER_PORT, &tcp_socket);
                    break;
                }
            }
            
            sleep_ms(1);
        }  
    }
}


// 添加互斥锁，防止多线程访问
TX_MUTEX pc_unpack_mutex;
void pc_command_unpack(uint8_t *buffer, uint8_t comm_type)
{
    tx_mutex_get(&pc_unpack_mutex, TX_WAIT_FOREVER);
    struct pc_comm_protocol_t *command_packet = (struct pc_comm_protocol_t *)buffer;
    uint8_t data[8];
    uint16_t crc;

    if (command_packet->head != pc_protocol_head)
    {
        return;
    }

    if (command_packet->source_addr != pc_addr)
    {
        return;
    }

    if (command_packet->target_addr != mcu_addr)
    {
        return;
    }

    if (command_packet->data_length >= sizeof(data))
    {
        return;
    }

    memcpy(&crc, buffer+sizeof(struct pc_comm_protocol_t)+command_packet->data_length, 2);
    if (!checkRxCRC(buffer, sizeof(struct pc_comm_protocol_t)+command_packet->data_length, crc))
    {
        return;
    }

    memcpy(&data, buffer+sizeof(struct pc_comm_protocol_t), command_packet->data_length);
    pc_unpack_data.function_code = command_packet->function_code;
    pc_unpack_data.data_length = command_packet->data_length;
    memcpy(pc_unpack_data.data, data, command_packet->data_length);
    pc_unpack_data.comm_type = comm_type;
    tx_semaphore_put(&pc_unpack_semaphore);
    tx_mutex_put(&pc_unpack_mutex);
}




