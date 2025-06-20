#include "thread_init.h"
#include "nx_stm32_eth_driver.h"
#include "thread_socket.h"
#include "tx_api.h"
#include "usart.h"
#include "fdcan.h"
#include <stdint.h>
#include "pc_protocol.h"
#include "gimbal.h"

// ---------thread parameters
// thread init parameters
#define THREAD_INIT_STACK_SIZE		4096u
#define THREAD_INIT_PRIO			28u
TX_THREAD thread_init_block;
uint64_t thread_init_stack[THREAD_INIT_STACK_SIZE/8];
void thread_init(ULONG input);

// thread socket parameters
#define THREAD_SOCKET_STACK_SIZE    4096u
#define THREAD_SOCKET_PRIO          25u
TX_THREAD thread_socket_block;
uint64_t thread_socket_stack[THREAD_SOCKET_STACK_SIZE/8];


// tx kits create
// sem
TX_SEMAPHORE pc_unpack_semaphore;
void semaphore_create(void)
{
	// tx_semaphore_create(&rs422.rx_semaphore, "rs422_rx_semaphore", 0);
	// tx_semaphore_create(&rs485.rx_semaphore, "rs485_rx_semaphore", 0);
	// tx_semaphore_create(&rs232.rx_semaphore, "rs232_rx_semaphore", 0);
	// tx_semaphore_create(&usb_c.rx_semaphore, "usb_c_rx_semaphore", 0);
	tx_semaphore_create(&pc_unpack_semaphore, "pc_unpack_semaphore", 0);
}


// ---------netxduo parameters
NX_PACKET_POOL    pool_0;
NX_IP             ip_0;
#define NX_PACKET_POOL_SIZE ((1536 + sizeof(NX_PACKET)) * 8)
ULONG  packet_pool_area[NX_PACKET_POOL_SIZE/4 + 4] __attribute__((section(".NetXPoolSection")));
ULONG  arp_space_area[52*20 / sizeof(ULONG)] __attribute__((section(".NetXPoolSection")));

#define IP_ADDR0                        192
#define IP_ADDR1                        168
#define IP_ADDR2                        1
#define IP_ADDR3                        111

ULONG  ip0_address = IP_ADDRESS(IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);

#define  THREAD_NETX_IP0_PRIO0                          2u
#define  THREAD_NETX_IP0_STK_SIZE                     	1024*16u
static   uint64_t  thread_netx_ip0_stack[THREAD_NETX_IP0_STK_SIZE/8];

// ---------
void  tx_application_define(void *first_unused_memory)
{
	semaphore_create();

	// serial_start(&rs422, &huart2, SERIAL_RX_DMA_IDLE);
	// serial_start(&rs485, &huart1, SERIAL_RX_DMA_IDLE);
	// serial_start(&rs232, &huart6, SERIAL_RX_DMA_IDLE);
	// serial_start(&usb_c, &huart10, SERIAL_RX_DMA_IDLE);
	fdcan1_start();
	
	UINT nx_init_status = 0;

	HAL_ETH_DeInit(&heth);
	nx_system_initialize();
	nx_init_status |= nx_packet_pool_create(&pool_0,
									"NetX Main Packet Pool",
									1536,  (ULONG*)(((int)packet_pool_area + 15) & ~15) ,
									NX_PACKET_POOL_SIZE);
	nx_init_status |= nx_ip_create(&ip_0,
						"NetX IP0",
						ip0_address,
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

	ULONG gateway_ip = ip0_address;
	gateway_ip = (gateway_ip & 0xFFFFFF00) | 0x01;
	nx_ip_gateway_address_set(&ip_0, gateway_ip);

	sleep_ms(300);

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


static void send_to_gimbal(struct pc_unpack_data_t *pc_unpack_data);

void thread_init(ULONG input)  // 将UINT改为ULONG
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
		// 等待来自上位机gimbal unpack后的信号量并发送给gimbal can
		if (tx_semaphore_get(&pc_unpack_semaphore, TX_NO_WAIT) == TX_SUCCESS)
		{
			send_to_gimbal(&pc_unpack_data);
		}
		// 等待来自can的数据并根据协议发送给上位机
		if (fdcan_rx_flag == 1)
		{
			fdcan_rx_flag = 0;
			gimbal_parse(&fdcan_rx_frame, &gimbal, fdcan_rx_frame.data);
		}
		sleep_ms(1);
	}
}


static void send_to_gimbal(struct pc_unpack_data_t *pc_unpack_data)
{
	int index = get_index_pc_function_code(pc_unpack_data->function_code);
	if (index != -1)
	{
		pc2gimbal_pack[index](pc_unpack_data->data);
	}
}





