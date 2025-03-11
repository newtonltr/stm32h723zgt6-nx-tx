stm32h723zgt6搭配DP83848I PHY使用NetXDuo实现TCP服务器指南

---
### 写在前面

在使用h7系列的以太网时，mpu和cache的设置是绕不开的话题，h7的mpu和cache设置可以关闭也可以打开，但是对于以太网这种数据量巨大的程序，为了数据处理速度还是有必要打开cache的，不然就浪费了h7的特色性能了。

由于h7的cache和mpu的存在，以太网的配置和协议栈的移植多了很多需要注意的地方。以下为基于h723zgt6配合netxduo搭建tcp通信的工程（phy为DP83848I），实现用tcp客户端向stm32tcp服务器发送消息并使stm32接收消息后原路发送回来的示例。

该示例会详细讲解关于h723zgt6的mpu，cache配置，threadx，netxduo的移植，threadx线程的创建，基于netxduo的tcp服务器的搭建，tcp应用的编写，以及flash.ld文件的修改。

本文的工程环境是基于stm32cubemx，stm32cubeclt，vscode或其衍生编辑器（cursor，trae）搭建的，如果想用keil mdk搭建该工程只需将本文对flash.ld文件自定义内存区域的修改移植到keil mdk的.sct文件即可。

本项目不支持热插拔，请连接好网线之后再上电。

---

# 一、准备工作
## 软件环境及第三方库：
 1. stm32cubemx
 2. stm32cubeclt
 3. vscode，cursor或者trae并安装stm32 vs code extensions插件
 4. 网络调试软件（sscom或其他）
 5. netxduo软件包（m7系列）
 6. threadx软件包（m7系列）

# 二、cubemx配置
## 1.mpu，cache配置
配置mpu之前，首先要搞清楚h7系列的ram分布，根据链接文件或者查手册可知：

```typescript
MEMORY
{
DTCMRAM (xrw)  : ORIGIN = 0x20000000, LENGTH = 128K
RAM (xrw)      : ORIGIN = 0x24000000, LENGTH = 128K
RAM_D2 (xrw)   : ORIGIN = 0x30000000, LENGTH = 32K
RAM_D3 (xrw)   : ORIGIN = 0x38000000, LENGTH = 16K
ITCMRAM (xrw)  : ORIGIN = 0x00000000, LENGTH = 64K
FLASH (rx)     : ORIGIN = 0x8000000, LENGTH = 1024K
}

```
众所周知，DMA是不能访问DTCM的，因此以太网相关的DMA描述符以及NetXDuo需要的内存池都不能放在这个区域，为了方便和安全最好将以太网的DMA描述符和内存池放在RAM_D2，RAM和DTCM留给普通程序。

因此我们只需要对0x30000000起始的大小为32k的区域进行mpu配置，分为两个区域进行配置：
region 0 的配置：
![region 0 的配置](https://i-blog.csdnimg.cn/direct/6753482481144fb081c46379e6181dca.png)
region 1 的配置：
![region 1 的配置](https://i-blog.csdnimg.cn/direct/a09f9030862c4a39b2ae0517c0284a13.png)
打开dcache和icache，分别为数据和指令的缓存。

mpu control mode设置为允许特定的程序（即内核）访问后台未被mpu配置的内存，对于stm32来说只要是你存在片内的程序都可以访问，并且在出现故障时禁止mpu功能。

首先是对RAM_D2域的全局配置，打开mpu访问权限，打开指令执行访问权限，关闭共享权限（对于多核系统才有影响，普通stm32的话这里全部关闭就行了打开也没影响），打开cache，关闭缓冲区功能。

然后对以太网的dma描述符区域进行单独配置，为什么大小是256B，这里先放eth的配置：
![eth](https://i-blog.csdnimg.cn/direct/890e0554bb3349dc821d4c0b65e2f003.png)
可见从0x30000000开始大小为256b的ram空间为以太网dma收发描述符所在区域，为了数据一致性，关闭该区域的cache功能。

生成代码后在程序中的体现为：

```c
void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x30000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_32KB;
  MPU_InitStruct.SubRegionDisable = 0x0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER1;
  MPU_InitStruct.Size = MPU_REGION_SIZE_256B;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

```
可见REGION1对REGION0的前256B大小的ram进行了重新配置，覆盖了REGION0的原配置，其他未重新赋值的成员变量则继承REGION0的数值。

## 2. 其他重要的cubemx配置

 - 以太网io口速度必须全部设为very high
![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/806cb13fd5d8444194f9c52a13dd27d5.png)
 - 单独配置复位引脚![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/25b43204e2b84f2583d5c8f5df0a40f8.png)
 - 系统时钟时基使用TIM6，其他也可以，用hal库别用systick，时钟图如下：![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/e9240b87770d41638214dfef053e3c10.png)
 - 取消生成systick和pend sv中断处理函数，因为后面要使用threadx来接管这些中断。![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/d024c6ecbada4486877b6e9707c9ad87.png)
到这里已经可以点击生成代码了。

# 三、Threadx, NetXDuo移植
软件包可在文末我上传到github的完整工程中获取。以下讲解移植的几个重要过程。

 1. Threadx或者NetXDuo都有port文件夹，其中分为GNU和AC5两个文件夹，用cmake方案gcc编译的话保留GNU文件夹，用keil mdk保留ac5文件夹![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/d9488aca2e0b430682bf3a9f7c2f8da8.png)
 2. threadx中tx_initialize_low_level.S文件的修改：
 

```c
SYSTEM_CLOCK      =   520000000
SYSTICK_CYCLES    =   ((SYSTEM_CLOCK / 1000) -1)

```
修改为你自己的时钟频率

# 四、开启ThreadX线程，初始化NetXDuo

 1. 使用ThreadX要定义tx_application_define函数，我们在该函数中完成NetXDuo的初始化和init线程的创建。
```c
// thread init parameters
#define THREAD_INIT_STACK_SIZE		4096u
#define THREAD_INIT_PRIO			28u
TX_THREAD thread_init_block;
uint64_t thread_init_stack[THREAD_INIT_STACK_SIZE/8];
void thread_init(ULONG input);

// ---------netxduo parameters
NX_PACKET_POOL    pool_0;
NX_IP             ip_0;
#define NX_PACKET_POOL_SIZE ((1536 + sizeof(NX_PACKET)) * 8)
ULONG  packet_pool_area[NX_PACKET_POOL_SIZE/4 + 4] __attribute__((section(".NetXPoolSection")));
ULONG  arp_space_area[52*20 / sizeof(ULONG)] __attribute__((section(".NetXPoolSection")));

#define IP_ADDR0                        192
#define IP_ADDR1                        168
#define IP_ADDR2                        0
#define IP_ADDR3                        135

#define  THREAD_NETX_IP0_PRIO0                          2u
#define  THREAD_NETX_IP0_STK_SIZE                     	1024*16u
static   uint64_t  thread_netx_ip0_stack[THREAD_NETX_IP0_STK_SIZE/8];


void  tx_application_define(void *first_unused_memory)
{
	UINT nx_init_status = 0;

	HAL_ETH_DeInit(&heth);
	nx_system_initialize();
	nx_init_status |= nx_packet_pool_create(&pool_0,
									"NetX Main Packet Pool",
									1536,  (ULONG*)(((int)packet_pool_area + 15) & ~15) ,
									NX_PACKET_POOL_SIZE);
	nx_init_status |= nx_ip_create(&ip_0,
						"NetX IP0",
						IP_ADDRESS(IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3),
						0xFFFFFF00UL,
						&pool_0, nx_stm32_eth_driver,
						(UCHAR*)thread_netx_ip0_stack,
						sizeof(thread_netx_ip0_stack),
						THREAD_NETX_IP0_PRIO0);
	nx_init_status |= nx_arp_enable(&ip_0, (void *)arp_space_area, sizeof(arp_space_area));
	nx_init_status |= nx_ip_fragment_enable(&ip_0);
	nx_init_status |= nx_tcp_enable(&ip_0);
	nx_init_status |= nx_udp_enable(&ip_0);
	nx_init_status |= nx_icmp_enable(&ip_0);

	ULONG gateway_ip = IP_ADDRESS(IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
	gateway_ip = (gateway_ip & 0xFFFFFF00) | 0x01;
	nx_ip_gateway_address_set(&ip_0, gateway_ip);

	tx_thread_create(&thread_init_block, 
		"tx_init", 
		thread_init, 
		0, 
		&thread_init_stack[0],
		THREAD_INIT_STACK_SIZE, 
		THREAD_INIT_PRIO, 
		THREAD_INIT_PRIO, 
		TX_NO_TIME_SLICE, 
		TX_AUTO_START);

}

```

 2. 在init线程中完成其他线程的创建，本示例仅创建用于tcp服务器的线程。
 

```c
// thread socket parameters
#define THREAD_SOCKET_STACK_SIZE    4096u
#define THREAD_SOCKET_PRIO          25u
TX_THREAD thread_socket_block;
uint64_t thread_socket_stack[THREAD_SOCKET_STACK_SIZE/8];

void thread_init(ULONG input)  //
{
	// 创建socket线程
	tx_thread_create(&thread_socket_block,
		"tx_socket",
		thread_socket_entry,
		0,
		&thread_socket_stack[0],
		THREAD_SOCKET_STACK_SIZE,
		THREAD_SOCKET_PRIO,
		THREAD_SOCKET_PRIO,
		TX_NO_TIME_SLICE,
		TX_AUTO_START);
	
	while (1) {
		sleep_ms(100);
	}
}

```

 3. 在socket线程中tcp服务器 socket，实现将客户端发送的数据包加上时间戳后再打包发送的功能。
 

```c
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
```
send_message_with_timestamp的实现如下：

```c
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

```

需要注意的是处理字符数据时，如果要使用strlen要将字符末尾添加终止符

```c
// 确保字符串以null结尾
	message_buffer[bytes_read < MAX_MESSAGE_SIZE ? bytes_read : MAX_MESSAGE_SIZE - 1] = '\0';
```

# 五、STM32H723XG_FLASH.ld 文件修改
在user_heap_stack配置下一行加入这三段：

```typescript
  .RxDescripSection 0x30000000:
  {
    . = ALIGN(4);
    __RxDescripSection_Start = .;
    *(.RxDescripSection);
    . = ALIGN(4);
    __RxDescripSection_End = .;
  } >RAM_D2

  .TxDescripSection 0x30000080:
  {
    . = ALIGN(4);
    __TxDescripSection_Start = .;
    *(.TxDescripSection);
    . = ALIGN(4);
    __TxDescripSection_End = .;
  } >RAM_D2

  .NetXPoolSection 0x30000100:
  {
    . = ALIGN(4);  /* 对齐到 4 字节边界 */
    __NetXPoolSection_start = .;  /* 定义全局符号，表示段的起始地址 */
    *(.NetXPoolSection)  /* 将所有 .my_data_section 段的内容放入此处 */
    . = ALIGN(4);  /* 对齐到 4 字节边界 */
    __NetXPoolSection_end = .;  /* 定义全局符号，表示段的结束地址 */
  } >RAM_D2  

	__RAM_segment_used_end__ = .;

```
前两者用于定义以太网的dma描述符所在的内存区域，后者用于netxduo协议栈需要使用的内存池。


# 六、测试网络

 1. 测试能否ping通![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/4b07d498d9d343ceb6d3ea8f872ad8a4.png)

 2. 测试转发功能![在这里插入图片描述](https://i-blog.csdnimg.cn/direct/b94eed58b04340ebb7f9cb8555382c20.png)



---

# 总结
本工程最需要注意的就是要把以太网引脚设为very high，以及对mpu和cache进行正确配置，这两者配置好了，h7的使用就基本没有问题了。

本项目github地址：
[stm32h723zgt6-nx-tx](https://github.com/newtonltr/stm32h723zgt6-nx-tx)
