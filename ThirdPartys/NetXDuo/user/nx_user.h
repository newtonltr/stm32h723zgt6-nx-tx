/*
 * nx_user.h
 *
 *  Created on: Aug 28, 2023
 *      Author: huang
 */

#ifndef NETXDUO_USER_NX_USER_H_
#define NETXDUO_USER_NX_USER_H_

//#define NX_ENABLE_INTERFACE_CAPABILITY

//#define NX_DEBUG

#define NX_IP_PERIODIC_RATE             1000

#define NX_ENABLE_TCP_KEEPALIVE
#define NX_TCP_KEEPALIVE_INITIAL        3        //建立链接后3s开始发送心跳包

#define NX_TCP_KEEPALIVE_RETRY          3        //每3s发送一次心跳

#define NX_TCP_KEEPALIVE_RETRIES        3       //重试3次，如果心跳均没有回应则断开连接

//#define NX_DISABLE_ICMPV4_TX_CHECKSUM
//#define NX_DISABLE_TCP_TX_CHECKSUM

//#define NX_DISABLE_TCP_TX_CHECKSUM
//#define NX_DISABLE_TCP_RX_CHECKSUM

#define NX_DISABLE_ERROR_CHECKING

#define NX_IP_REASSEMBLY_ENABLE
#define NX_IP_FRAGMENT_ENABLE



#endif /* NETXDUO_USER_NX_USER_H_ */
