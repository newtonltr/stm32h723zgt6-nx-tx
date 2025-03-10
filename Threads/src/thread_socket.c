#include "thread_socket.h"
#include <stdio.h>
#include <string.h>

// TCP socket相关参数定义在这个文件中
NX_TCP_SOCKET tcp_socket;
#define TCP_SERVER_PORT 7000  // 服务器监听端口

// 消息缓冲区
#define MAX_MESSAGE_SIZE 256
static char message_buffer[MAX_MESSAGE_SIZE];

// 发送带时间戳的消息
UINT send_message_with_timestamp(const char* message)
{
    NX_PACKET *packet_ptr;
    UINT status;
    char timestamp_message[MAX_MESSAGE_SIZE];
    ULONG current_time = HAL_GetTick(); // 获取HAL时间戳
    
    // 格式化消息，添加时间戳
    snprintf(timestamp_message, MAX_MESSAGE_SIZE, "[%lu ms] %s", current_time, message);
    
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
    
    // 发送初始启动消息
    send_message_with_timestamp("TCP服务器已启动，等待连接...");
    
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
    send_message_with_timestamp("客户端已连接");
    
    // 主循环
    while (1)
    {
        // 接收数据包
        status = nx_tcp_socket_receive(&tcp_socket, &receive_packet, NX_WAIT_FOREVER);
        if (status == NX_SUCCESS)
        {
            // 读取数据包内容
            status = nx_packet_data_retrieve(receive_packet, message_buffer, &bytes_read);
            if (status == NX_SUCCESS && bytes_read > 0)
            {
                // 确保字符串以null结尾
                message_buffer[bytes_read < MAX_MESSAGE_SIZE ? bytes_read : MAX_MESSAGE_SIZE - 1] = '\0';
                
                // 添加时间戳并回显收到的消息
                send_message_with_timestamp(message_buffer);
            }
            
            // 释放数据包
            nx_packet_release(receive_packet);
        }
        else
        {
            // 检查套接字状态
            ULONG tcp_packets_sent, tcp_bytes_sent;
            ULONG tcp_packets_received, tcp_bytes_received;
            ULONG tcp_retransmit_packets, tcp_retransmit_bytes;
            ULONG tcp_socket_state, tcp_transmit_queue_depth;
            ULONG tcp_receive_window, tcp_transmit_window;
            ULONG tcp_window_size;
            
            // 获取套接字信息
            nx_tcp_socket_info_get(&tcp_socket, 
                                  &tcp_packets_sent, &tcp_bytes_sent,
                                  &tcp_packets_received, &tcp_bytes_received,
                                  &tcp_retransmit_packets, &tcp_retransmit_bytes,
                                  &tcp_socket_state, &tcp_transmit_queue_depth,
                                  &tcp_transmit_window, &tcp_receive_window,
                                  &tcp_window_size);
            
            // 如果连接断开，重新等待连接
            if (tcp_socket_state != NX_TCP_ESTABLISHED)
            {
                // 断开当前连接
                nx_tcp_socket_disconnect(&tcp_socket, NX_WAIT_FOREVER);
                nx_tcp_server_socket_unaccept(&tcp_socket);
                
                // 重新等待连接
                send_message_with_timestamp("客户端断开连接，等待新连接...");
                status = nx_tcp_server_socket_accept(&tcp_socket, NX_WAIT_FOREVER);
                if (status == NX_SUCCESS)
                {
                    send_message_with_timestamp("新客户端已连接");
                }
            }
        }
    }
}