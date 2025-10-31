#include "pc_protocol.h"
#include <stdint.h>
#include "fdcan.h"
#include "usart.h"
#include "gimbal.h"
#include "thread_socket.h"
#include "emb_flash.h"


const uint16_t gimbal_key[] = {
	GIMBAL_KEY_STATE1,
	GIMBAL_KEY_STATE2,
	GIMBAL_KEY_ANGLE,
	GIMBAL_KEY_SPEED,
	GIMBAL_KEY_LIDAR_DIST,
	GIMBAL_KEY_METHANE,
	GIMBAL_KEY_ERROR,
	GIMBAL_KEY_SET_LIGHT,
	GIMBAL_KEY_SET_WIPER,
	GIMBAL_KEY_HEATER_MANUAL,
	GIMBAL_KEY_HEATER_AUTO,
	GIMBAL_KEY_READ_VERSION,
	GIMBAL_KEY_STOP,
	GIMBAL_KEY_TURN_LEFT,
	GIMBAL_KEY_TURN_RIGHT,
	GIMBAL_KEY_TILT_UP,
	GIMBAL_KEY_TILT_DOWN,
	GIMBAL_KEY_RELATIVE_MOVE,
	GIMBAL_KEY_SET_MAX_SPEED,
	GIMBAL_KEY_ABSOLUTE_MOVE,
	GIMBAL_KEY_RESET,
	GIMBAL_KEY_CALIBRATE,
	GIMBAL_KEY_ENCODER_CAL,
	GIMBAL_KEY_PITCH_UP_LIM,
	GIMBAL_KEY_PITCH_DOWN_LIM,
	GIMBAL_KEY_SOFT_LIMIT_SW,
	GIMBAL_KEY_ZOOM_STOP,
	GIMBAL_KEY_ZOOM_IN,
	GIMBAL_KEY_ZOOM_OUT,
	GIMBAL_KEY_ZOOM_IN_SPEED,
	GIMBAL_KEY_ZOOM_OUT_SPEED,
	GIMBAL_KEY_ZOOM_TO,
	GIMBAL_KEY_ZOOM_FOCUS_TO,
	GIMBAL_KEY_DIGITAL_ZOOM_ON,
	GIMBAL_KEY_DIGITAL_ZOOM_OFF,
	GIMBAL_KEY_FOCUS_STOP,
	GIMBAL_KEY_FOCUS_IN,
	GIMBAL_KEY_FOCUS_OUT,
	GIMBAL_KEY_FOCUS_TO,
	GIMBAL_KEY_FOCUS_AUTO,
	GIMBAL_KEY_FOCUS_MANUAL,
	GIMBAL_KEY_FOCUS_MODE_SWITCH,
	GIMBAL_KEY_FOCUS_SINGLE,
	GIMBAL_KEY_FOCUS_INFINITY,
	GIMBAL_KEY_FOCUS_DIST_SET,
	GIMBAL_KEY_AF_SENS_NORMAL,
	GIMBAL_KEY_AF_SENS_LOW,
	GIMBAL_KEY_AREA_FOCUS,
	GIMBAL_KEY_DAY_MODE,
	GIMBAL_KEY_NIGHT_MODE,
	GIMBAL_KEY_ZOOM_POS_QUERY,
	GIMBAL_KEY_DIGI_ZOOM_QUERY,
	GIMBAL_KEY_FOCUS_MODE_QUERY,
	GIMBAL_KEY_FOCUS_POS_QUERY,
	GIMBAL_KEY_FOCUS_STATUS_QUERY,
	GIMBAL_KEY_OPTICAL_ZOOM_QUERY,
	GIMBAL_KEY_LIDAR_OFF,
	GIMBAL_KEY_LIDAR_ON,
	GIMBAL_KEY_METHANE_OFF,
	GIMBAL_KEY_METHANE_ON,
};

const uint16_t pc_function_code[] = {
	PC_GIMBAL_CONTROL,
	PC_GIMBAL_ANGLE_QUERY,
	PC_GIMBAL_AUX_SWITCH_CONTROL,
	PC_GIMBAL_PRESET_POINT,
	PC_GIMBAL_CAMERA_PARAM_QUERY,
	PC_GIMBAL_CAMERA_PARAM_SET,
	PC_GIMBAL_CAMERA_ADJUST,
	PC_MAC_ADDR_SET,
	PC_IP_ADDR_SET,
	PC_MASK_ADDR_SET,
	PC_GATEWAY_ADDR_SET,
	PC_HARDFAULT_INFO_QUERY,
	PC_MAC_ADDR_QUERY,
	PC_IP_ADDR_QUERY,
	PC_MASK_ADDR_QUERY,
	PC_GATEWAY_ADDR_QUERY,
};


int get_index_gimbal_key(uint16_t key) // 获取云台key在数组中的索引
{
	for (uint16_t i = 0; i < sizeof(gimbal_key) / sizeof(gimbal_key[0]); i++)
	{
		if (gimbal_key[i] == key)
		{
			return i;
		}
	}
	return -1;
}

int get_index_pc_function_code(uint16_t function_code) // 获取pc功能码在数组中的索引
{
	for (uint16_t i = 0; i < sizeof(pc_function_code) / sizeof(pc_function_code[0]); i++)
	{
		if (pc_function_code[i] == function_code)
		{
			return i;
		}
	}
	return -1;
}


// 解包pc数据后通过can发送给云台

static void pc_gimbal_control(struct pc_unpack_data_t *pc_unpack_data);				// functioncode = PC_GIMBAL_CONTROL 0x00C3
static void pc_gimbal_angle_query(struct pc_unpack_data_t *pc_unpack_data);			// functioncode = PC_GIMBAL_ANGLE_QUERY 0x00C4
static void pc_gimbal_aux_switch_control(struct pc_unpack_data_t *pc_unpack_data);	// functioncode = PC_GIMBAL_AUX_SWITCH_CONTROL 0x00C5
static void pc_gimbal_preset_point(struct pc_unpack_data_t *pc_unpack_data);			// functioncode = PC_GIMBAL_PRESET_POINT 0x00CA
static void pc_gimbal_camera_param_query(struct pc_unpack_data_t *pc_unpack_data);	// functioncode = PC_GIMBAL_CAMERA_PARAM_QUERY 0x00CB
static void pc_gimbal_camera_param_set(struct pc_unpack_data_t *pc_unpack_data);		// functioncode = PC_GIMBAL_CAMERA_PARAM_SET 0x00CC
static void pc_gimbal_camera_adjust(struct pc_unpack_data_t *pc_unpack_data);			// functioncode = PC_GIMBAL_CAMERA_ADJUST 0x00CD
static void pc_mac_addr_set(struct pc_unpack_data_t *pc_unpack_data);			// functioncode = PC_MAC_ADDR_SET 0x01F1
static void pc_ip_addr_set(struct pc_unpack_data_t *pc_unpack_data);			// functioncode = PC_IP_ADDR_SET 0x01F2
static void pc_mask_addr_set(struct pc_unpack_data_t *pc_unpack_data);			// functioncode = PC_MASK_ADDR_SET 0x01F3
static void pc_gateway_addr_set(struct pc_unpack_data_t *pc_unpack_data);		// functioncode = PC_GATEWAY_ADDR_SET 0x01F4
static void pc_hardfault_info_query(struct pc_unpack_data_t *pc_unpack_data);	// functioncode = PC_HARDFAULT_INFO_QUERY 0x01F5
static void pc_mac_addr_query(struct pc_unpack_data_t *pc_unpack_data);		// functioncode = PC_MAC_ADDR_QUERY 0x01F6
static void pc_ip_addr_query(struct pc_unpack_data_t *pc_unpack_data);		// functioncode = PC_IP_ADDR_QUERY 0x01F7
static void pc_mask_addr_query(struct pc_unpack_data_t *pc_unpack_data);		// functioncode = PC_MASK_ADDR_QUERY 0x01F8
static void pc_gateway_addr_query(struct pc_unpack_data_t *pc_unpack_data);	// functioncode = PC_GATEWAY_ADDR_QUERY 0x01F9

void (*pc2gimbal_pack[])(struct pc_unpack_data_t *pc_unpack_data) =
{
	pc_gimbal_control,
	pc_gimbal_angle_query,
	pc_gimbal_aux_switch_control,
	pc_gimbal_preset_point,
	pc_gimbal_camera_param_query,
	pc_gimbal_camera_param_set,
	pc_gimbal_camera_adjust,
	pc_mac_addr_set,
	pc_ip_addr_set,
	pc_mask_addr_set,
	pc_gateway_addr_set,
	pc_hardfault_info_query,
	pc_mac_addr_query,
	pc_ip_addr_query,
	pc_mask_addr_query,
	pc_gateway_addr_query,
};


static void send_to_pc(uint16_t function_code, uint8_t *buff, uint16_t len, uint8_t comm_type)
{
	uint16_t crc = 0;
	uint8_t send_buf[256] = {0};

	struct pc_comm_protocol_t pc_protocol;
	pc_protocol.head = pc_protocol_head;
	pc_protocol.source_addr = mcu_addr;
	pc_protocol.target_addr = pc_addr;
	pc_protocol.function_code = function_code;
	pc_protocol.data_length = len;

	memcpy(send_buf, &pc_protocol, sizeof(struct pc_comm_protocol_t));
	memcpy(send_buf + sizeof(struct pc_comm_protocol_t), buff, len);
	crc = CRC16(send_buf, sizeof(struct pc_comm_protocol_t) + len);
	memcpy(send_buf + sizeof(struct pc_comm_protocol_t) + len, &crc, 2);
	
	if(comm_type == COMM_TYPE_TCP)
	{
		nx_send(&tcp_socket, send_buf, sizeof(struct pc_comm_protocol_t) + len + 2);
	}
	else if(comm_type == COMM_TYPE_UART)
	{
		serial_block_write(&usb_c, send_buf, sizeof(struct pc_comm_protocol_t) + len + 2);
	}

}

static void pc_gimbal_control(struct pc_unpack_data_t *pc_unpack_data)
{
	uint8_t *buff = pc_unpack_data->data;
	uint16_t yaw;
	uint16_t pitch;
	double yaw_fp;
	double pitch_fp;

	if(buff[0] == 0){
		gimbal_move(buff[1],buff[2]);
	}
	else if(buff[0] == 1){
		memcpy(&yaw,&buff[1],2);
		yaw_fp = yaw * 1.0;
		gimbal_hor_move_to(&gimbal,yaw_fp);
	}
	else if(buff[0] == 2){
		memcpy(&pitch,&buff[1],2);
		pitch_fp = pitch * 1.0;
		gimbal_ver_move_to(&gimbal,pitch_fp);
	}
	else if(buff[0] == 3){
		memcpy(&yaw,&buff[1],2);
		memcpy(&pitch,&buff[3],2);
		yaw_fp = yaw * 1.0;
		pitch_fp = pitch * 1.0;
		gimbal_sync_move_to(&gimbal,yaw_fp,pitch_fp);
	}

	send_to_pc(PC_GIMBAL_CONTROL, NULL, 0, pc_unpack_data->comm_type);

}

static void pc_gimbal_angle_query(struct pc_unpack_data_t *pc_unpack_data)
{
	uint8_t *buff = pc_unpack_data->data;
	uint16_t yaw;
	uint16_t pitch;
	uint8_t data[32] = {0};

	data[0] = buff[0];
	if(buff[0] == 0){
		gimbal_get_yaw(&gimbal);
		yaw = gimbal.yaw * 100;
		memcpy(&data[1],&yaw,2);
	}
	else if(buff[0] == 1){
		gimbal_get_pitch(&gimbal);
		pitch = gimbal.pitch * 100;
		memcpy(&data[1],&pitch,2);
	}
	else if(buff[0] == 2){
		gimbal_get_yaw(&gimbal);
		yaw = gimbal.yaw * 100;
		gimbal_get_pitch(&gimbal);
		pitch = gimbal.pitch * 100;
		memcpy(&data[1],&yaw,2);
		memcpy(&data[3],&pitch,2);
	}
	else{
		
	}

	send_to_pc(PC_GIMBAL_ANGLE_QUERY, data, 5, pc_unpack_data->comm_type);
	
}

static void pc_gimbal_aux_switch_control(struct pc_unpack_data_t *pc_unpack_data)
{
	uint8_t *buff = pc_unpack_data->data;
	uint16_t opt = buff[0];
	uint8_t val = buff[1];
	
	if(opt == 0){
		gimbal_wiper(&gimbal,val);
	}
	else if(opt == 1){
		gimbal_led(&gimbal,val);
	}
	else if(opt == 2){
		gimbal_fog(&gimbal,val);
	}
	else if(opt == 3){
		gimbal_color_mode(&gimbal,val);
	}
	else if(opt == 4){
		gimbal_antifreeze(&gimbal,val);
	}
	else{
		
	}

	send_to_pc(PC_GIMBAL_AUX_SWITCH_CONTROL, NULL, 0, pc_unpack_data->comm_type);
}

static void pc_gimbal_preset_point(struct pc_unpack_data_t *pc_unpack_data)
{
	uint8_t *buff = pc_unpack_data->data;
	uint16_t opt = buff[0];
	uint8_t val = buff[1];
	
	if(opt == 0){
		gimbal_pre_set(&gimbal,val);
	}
	else if(opt == 1){
		gimbal_pre_recall(&gimbal,val);
	}
	else if(opt == 2){
		gimbal_pre_remove(&gimbal,val);
	}
	else{
		
	}

	send_to_pc(PC_GIMBAL_PRESET_POINT, NULL, 0, pc_unpack_data->comm_type);
	
}

static void pc_gimbal_camera_param_query(struct pc_unpack_data_t *pc_unpack_data)
{
	uint8_t *buff = pc_unpack_data->data;
	uint8_t data[3] = {0};
	uint16_t value = 0;

	if(buff[0] == 0){
		gimbal_get_zoom(&gimbal);
	}
	else if(buff[0] == 1){
		gimbal_get_focus(&gimbal);
	}
	else{
		
	}

	sleep_ms(150);
	if(buff[0] == 0){
		value = gimbal.zoom;
		data[0] = 0x00;
	}
	if(buff[0] == 1){
		value = gimbal.focus;
		data[0] = 0x01;
	}

	data[1] = (value>>8) & 0xff;
	data[2] = (value>>0) & 0xff;
	send_to_pc(PC_GIMBAL_CAMERA_PARAM_QUERY, data, sizeof(data), pc_unpack_data->comm_type);
}

static void pc_gimbal_camera_param_set(struct pc_unpack_data_t *pc_unpack_data)
{
	uint8_t *buff = pc_unpack_data->data;
	uint16_t value = (uint16_t)((buff[1]<<8)+buff[2]);

	if(buff[0] == 0){
		gimbal_set_zoom(&gimbal,value);
	}
	else if(buff[0] == 1){
		gimbal_set_focus(&gimbal,value);
	}
	else{
		
	}

	send_to_pc(PC_GIMBAL_CAMERA_PARAM_SET, NULL, 0, pc_unpack_data->comm_type);

}

static void pc_gimbal_camera_adjust(struct pc_unpack_data_t *pc_unpack_data)
{
	uint8_t *buff = pc_unpack_data->data;
	uint16_t opt = buff[0];
	uint8_t val = buff[1];
	
	if(opt == 0){
		gimbal_zoom_add_sub(&gimbal,val);
	}
	else if(opt == 1){
		gimbal_focus_add_sub(&gimbal,val);
	}
	else if(opt == 2){
		gimbal_aperture_add_sub(&gimbal,val);
	}
	else{
		
	}

	send_to_pc(PC_GIMBAL_CAMERA_ADJUST, NULL, 0, pc_unpack_data->comm_type);

}


static void pc_mac_addr_set(struct pc_unpack_data_t *pc_unpack_data)
{
	HAL_StatusTypeDef ret;
	uint8_t *buff = pc_unpack_data->data;
	socket_param_data.mac_address[0] = buff[0];
	socket_param_data.mac_address[1] = buff[1];
	socket_param_data.mac_address[2] = buff[2];
	socket_param_data.mac_address[3] = buff[3];
	socket_param_data.mac_address[4] = buff[4];
	socket_param_data.mac_address[5] = buff[5];
	
	ret = emb_flash_write(socket_param_data_address, (uint32_t*)&socket_param_data, sizeof(struct socket_param_t));

	if(ret == HAL_OK)
	{
		// 读取写入的数据，再将新旧mac地址比较，如果相同就发送pc新写入的mac地址，否则返回空数据
		struct socket_param_t socket_param_data_read;
		emb_flash_read(socket_param_data_address, (uint32_t*)&socket_param_data_read, sizeof(struct socket_param_t));
		if(socket_param_data_read.flash_head == FLASH_HEAD && socket_param_data_read.flash_tail == FLASH_TAIL
		 && memcmp(socket_param_data_read.mac_address, socket_param_data.mac_address, 6) == 0)
		{
			send_to_pc(PC_MAC_ADDR_SET, buff, 6, pc_unpack_data->comm_type);
		}
		else
		{
			// 写入默认mac地址
			memcpy(socket_param_data.mac_address, default_mac_address, 6);
			ret = emb_flash_write(socket_param_data_address, (uint32_t*)&socket_param_data, sizeof(struct socket_param_t));
			send_to_pc(PC_MAC_ADDR_SET, NULL, 0, pc_unpack_data->comm_type);
		}
	}
	else
	{	// 写入失败，返回空数据
		send_to_pc(PC_MAC_ADDR_SET, NULL, 0, pc_unpack_data->comm_type);
	}
}

static void pc_ip_addr_set(struct pc_unpack_data_t *pc_unpack_data)
{
	HAL_StatusTypeDef ret;
	uint8_t *buff = pc_unpack_data->data;
	socket_param_data.ip_address = (uint32_t)((buff[0])+(buff[1]<<8)+(buff[2]<<16)+(buff[3]<<24));
	ret = emb_flash_write(socket_param_data_address, (uint32_t*)&socket_param_data, sizeof(struct socket_param_t));

	if(ret == HAL_OK)
	{
		// 读取写入的数据，再将新旧ip地址比较，如果相同就发送pc新写入的ip地址，否则返回空数据
		struct socket_param_t socket_param_data_read;
		emb_flash_read(socket_param_data_address, (uint32_t*)&socket_param_data_read, sizeof(struct socket_param_t));
		if(socket_param_data_read.flash_head == FLASH_HEAD && socket_param_data_read.flash_tail == FLASH_TAIL
		 && socket_param_data_read.ip_address == socket_param_data.ip_address)
		{
			send_to_pc(PC_IP_ADDR_SET, buff, 4, pc_unpack_data->comm_type);
		}
		else
		{
			// 写入默认ip地址
			socket_param_data.ip_address = default_ip_address;
			ret = emb_flash_write(socket_param_data_address, (uint32_t*)&socket_param_data, sizeof(struct socket_param_t));
			send_to_pc(PC_IP_ADDR_SET, NULL, 0, pc_unpack_data->comm_type);
		}
	}
	else
	{	// 写入失败，返回空数据
		send_to_pc(PC_IP_ADDR_SET, NULL, 0, pc_unpack_data->comm_type);
	}

}	

static void pc_mask_addr_set(struct pc_unpack_data_t *pc_unpack_data)
{
	HAL_StatusTypeDef ret;
	uint8_t *buff = pc_unpack_data->data;
	socket_param_data.mask_address = (uint32_t)((buff[0])+(buff[1]<<8)+(buff[2]<<16)+(buff[3]<<24));
	ret = emb_flash_write(socket_param_data_address, (uint32_t*)&socket_param_data, sizeof(struct socket_param_t));

	if(ret == HAL_OK)
	{
		// 读取写入的数据，再将新旧mask地址比较，如果相同就发送pc新写入的mask地址，否则返回空数据
		struct socket_param_t socket_param_data_read;
		emb_flash_read(socket_param_data_address, (uint32_t*)&socket_param_data_read, sizeof(struct socket_param_t));
		if(socket_param_data_read.flash_head == FLASH_HEAD && socket_param_data_read.flash_tail == FLASH_TAIL
		 && socket_param_data_read.mask_address == socket_param_data.mask_address)
		{
			send_to_pc(PC_MASK_ADDR_SET, buff, 4, pc_unpack_data->comm_type);
		}
		else
		{
			// 写入默认mask地址
			socket_param_data.mask_address = default_mask_address;
			ret = emb_flash_write(socket_param_data_address, (uint32_t*)&socket_param_data, sizeof(struct socket_param_t));
			send_to_pc(PC_MASK_ADDR_SET, NULL, 0, pc_unpack_data->comm_type);
		}
	}
	else
	{
		send_to_pc(PC_MASK_ADDR_SET, NULL, 0, pc_unpack_data->comm_type);
	}
}


static void pc_gateway_addr_set(struct pc_unpack_data_t *pc_unpack_data)
{
	HAL_StatusTypeDef ret;
	uint8_t *buff = pc_unpack_data->data;
	socket_param_data.gateway_address = (uint32_t)((buff[0])+(buff[1]<<8)+(buff[2]<<16)+(buff[3]<<24));
	ret = emb_flash_write(socket_param_data_address, (uint32_t*)&socket_param_data, sizeof(struct socket_param_t));

	if(ret == HAL_OK)
	{
		// 读取写入的数据，再将新旧网关地址比较，如果相同就发送pc新写入的网关地址，否则返回空数据
		struct socket_param_t socket_param_data_read;
		emb_flash_read(socket_param_data_address, (uint32_t*)&socket_param_data_read, sizeof(struct socket_param_t));
		if(socket_param_data_read.flash_head == FLASH_HEAD && socket_param_data_read.flash_tail == FLASH_TAIL
		 && socket_param_data_read.gateway_address == socket_param_data.gateway_address)
		{
			send_to_pc(PC_GATEWAY_ADDR_SET, buff, 4, pc_unpack_data->comm_type);
		}
		else
		{
			// 写入默认网关地址
			socket_param_data.gateway_address = default_gateway_address;
			ret = emb_flash_write(socket_param_data_address, (uint32_t*)&socket_param_data, sizeof(struct socket_param_t));
			send_to_pc(PC_GATEWAY_ADDR_SET, NULL, 0, pc_unpack_data->comm_type);
		}
	}
	else
	{
		send_to_pc(PC_GATEWAY_ADDR_SET, NULL, 0, pc_unpack_data->comm_type);
	}
}

static void pc_hardfault_info_query(struct pc_unpack_data_t *pc_unpack_data)
{
	hardfault_info_t fault_info = {0};
	HardFault_ReadInfo(&fault_info);
	uint8_t data[sizeof(hardfault_info_t)] = {0};
	memcpy(data, &fault_info, sizeof(hardfault_info_t));
	send_to_pc(PC_HARDFAULT_INFO_QUERY, data, sizeof(hardfault_info_t), pc_unpack_data->comm_type);
}

uint8_t mac_query[6];

static void pc_mac_addr_query(struct pc_unpack_data_t *pc_unpack_data)
{
	uint32_t mac_high = ETH->MACA0HR;
	uint32_t mac_low = ETH->MACA0LR;

	mac_query[5] = (uint8_t)((mac_high >> 8) & 0xFF);
	mac_query[4] = (uint8_t)(mac_high & 0xFF);
	mac_query[3] = (uint8_t)((mac_low >> 24) & 0xFF);
	mac_query[2] = (uint8_t)((mac_low >> 16) & 0xFF);
	mac_query[1] = (uint8_t)((mac_low >> 8) & 0xFF);
	mac_query[0] = (uint8_t)(mac_low & 0xFF);

	send_to_pc(PC_MAC_ADDR_QUERY, mac_query, 6, pc_unpack_data->comm_type);
}

static void pc_ip_addr_query(struct pc_unpack_data_t *pc_unpack_data)
{
	send_to_pc(PC_IP_ADDR_QUERY, (uint8_t*)&socket_param_data.ip_address, 4, pc_unpack_data->comm_type);
}

static void pc_mask_addr_query(struct pc_unpack_data_t *pc_unpack_data)
{
	send_to_pc(PC_MASK_ADDR_QUERY, (uint8_t*)&socket_param_data.mask_address, 4, pc_unpack_data->comm_type);
}

static void pc_gateway_addr_query(struct pc_unpack_data_t *pc_unpack_data)
{
	send_to_pc(PC_GATEWAY_ADDR_QUERY, (uint8_t*)&socket_param_data.gateway_address, 4, pc_unpack_data->comm_type);
}

