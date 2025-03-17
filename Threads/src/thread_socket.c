#include "thread_socket.h"
#include <stdio.h>
#include <string.h>

// TCP socket相关参数定义在这个文件中
NX_TCP_SOCKET tcp_socket;
#define TCP_SERVER_PORT 7000  // 服务器监听端口

// 消息缓冲区
#define MAX_MESSAGE_SIZE 256
static char message_buffer[MAX_MESSAGE_SIZE];

// 发送带时间戳、IP地址和端口号的消息
UINT send_message_with_timestamp(const char* message)
{
    NX_PACKET *packet_ptr;
    UINT status;
    char timestamp_message[MAX_MESSAGE_SIZE];
    ULONG current_time = HAL_GetTick(); // 获取HAL时间戳
    ULONG ip_address = ip0_address;
    UINT port;
    
    // 获取当前IP地址和端口号
    nx_ip_address_get(&ip_0, &ip_address, NULL);
    port = TCP_SERVER_PORT;
    
    // 格式化消息，添加时间戳、IP地址和端口号
    snprintf(timestamp_message, MAX_MESSAGE_SIZE, "[%lu ms][%lu.%lu.%lu.%lu:%d] %s", 
             current_time, 
             (ip_address >> 24) & 0xFF,
             (ip_address >> 16) & 0xFF,
             (ip_address >> 8) & 0xFF,
             ip_address & 0xFF,
             port,
             message);
    
    // 分配数据包
    status = nx_packet_allocate(&pool_0, &packet_ptr, NX_TCP_PACKET, NX_WAIT_FOREVER);
    if (status != NX_SUCCESS)
    {
        return status;
    }
    
    // 将消息附加到数据包
    status = nx_packet_data_append(packet_ptr, 
                                  (VOID *)timestamp_message, 
                                  strlen(timestamp_message), 
                                  &pool_0, 
                                  NX_WAIT_FOREVER);
    if (status != NX_SUCCESS)
    {
        nx_packet_release(packet_ptr);
        return status;
    }
    
    // 发送数据包
    status = nx_tcp_socket_send(&tcp_socket, packet_ptr, NX_WAIT_FOREVER);
    if (status != NX_SUCCESS)
    {
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
        
    
    // 发送连接成功消息
    send_message_with_timestamp("client connected");
    
    while (1) {
        // 等待客户端连接
        status = nx_tcp_server_socket_accept(&tcp_socket, NX_WAIT_FOREVER);
        if (status != NX_SUCCESS)
        {
            nx_tcp_server_socket_unaccept(&tcp_socket);
            nx_tcp_server_socket_unlisten(&ip_0, TCP_SERVER_PORT);
            nx_tcp_socket_delete(&tcp_socket);
            return;
        }

        // 发送连接成功消息
        send_message_with_timestamp("client connected");

        while (1)
        {
            // 接收数据包
            status = nx_tcp_socket_receive(&tcp_socket, &receive_packet, NX_WAIT_FOREVER);
            if (status == NX_SUCCESS) {
                // 读取数据包内容
                status = nx_packet_data_retrieve(receive_packet, message_buffer, &bytes_read);
                if (status == NX_SUCCESS && bytes_read > 0) {
                    // 确保字符串以null结尾
                    message_buffer[bytes_read < MAX_MESSAGE_SIZE? bytes_read : MAX_MESSAGE_SIZE - 1] = '\0';
                    // 添加时间戳并回显收到的消息
                    send_message_with_timestamp(message_buffer);
                }
                // 释放数据包
                nx_packet_release(receive_packet);
            } else if (status == NX_NOT_CONNECTED) {
                // 接受新连接
                nx_tcp_server_socket_unaccept(&tcp_socket);
                nx_tcp_server_socket_relisten(&ip_0, TCP_SERVER_PORT, &tcp_socket);
                break;
            }
        }  
    }
}