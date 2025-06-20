#include "pc_protocol.h"
#include <stdint.h>
#include "fdcan.h"
#include "usart.h"
#include "gimbal.h"
#include "thread_socket.h"


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

static void pc_gimbal_control(uint8_t *buff);				// functioncode = PC_GIMBAL_CONTROL 0x00C3
static void pc_gimbal_angle_query(uint8_t *buff);			// functioncode = PC_GIMBAL_ANGLE_QUERY 0x00C4
static void pc_gimbal_aux_switch_control(uint8_t *buff);	// functioncode = PC_GIMBAL_AUX_SWITCH_CONTROL 0x00C5
static void pc_gimbal_preset_point(uint8_t *buff);			// functioncode = PC_GIMBAL_PRESET_POINT 0x00CA
static void pc_gimbal_camera_param_query(uint8_t *buff);	// functioncode = PC_GIMBAL_CAMERA_PARAM_QUERY 0x00CB
static void pc_gimbal_camera_param_set(uint8_t *buff);		// functioncode = PC_GIMBAL_CAMERA_PARAM_SET 0x00CC
static void pc_gimbal_camera_adjust(uint8_t *buff);			// functioncode = PC_GIMBAL_CAMERA_ADJUST 0x00CD
void (*pc2gimbal_pack[])(uint8_t *buff) =
{
	pc_gimbal_control,
	pc_gimbal_angle_query,
	pc_gimbal_aux_switch_control,
	pc_gimbal_preset_point,
	pc_gimbal_camera_param_query,
	pc_gimbal_camera_param_set,
	pc_gimbal_camera_adjust,
};


static void send_to_pc(uint16_t function_code, uint8_t *buff, uint16_t len)
{
	uint16_t crc = 0;
	uint8_t send_buf[sizeof(struct gimbal_protocol_t) + 8 + 2] = {0};	// 预留8个字节数据，2个字节crc

	struct gimbal_protocol_t gimbal_protocol;
	gimbal_protocol.head = pc_protocol_head;
	gimbal_protocol.source_addr = mcu_addr;
	gimbal_protocol.target_addr = pc_addr;
	gimbal_protocol.function_code = function_code;
	gimbal_protocol.data_length = len;

	memcpy(send_buf, &gimbal_protocol, sizeof(struct gimbal_protocol_t));

	if(len > 0 && buff != NULL && len < 8){
		memcpy(send_buf + sizeof(struct gimbal_protocol_t), buff, len);
	}

	crc = CRC16(send_buf, sizeof(struct gimbal_protocol_t) + len);
	memcpy(send_buf + sizeof(struct gimbal_protocol_t) + len, &crc, 2);
	
	nx_send(&tcp_socket, send_buf, sizeof(struct gimbal_protocol_t) + len + 2);

}

static void pc_gimbal_control(uint8_t *buff)
{
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

	send_to_pc(PC_GIMBAL_CONTROL, NULL, 0);

}

static void pc_gimbal_angle_query(uint8_t *buff)
{
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

	send_to_pc(PC_GIMBAL_ANGLE_QUERY, data, 5);
	
}

static void pc_gimbal_aux_switch_control(uint8_t *buff)
{
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

	send_to_pc(PC_GIMBAL_AUX_SWITCH_CONTROL, NULL, 0);
}

static void pc_gimbal_preset_point(uint8_t *buff)
{
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

	send_to_pc(PC_GIMBAL_PRESET_POINT, NULL, 0);
	
}

static void pc_gimbal_camera_param_query(uint8_t *buff)
{
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
	send_to_pc(PC_GIMBAL_CAMERA_PARAM_QUERY, data, sizeof(data));
}

static void pc_gimbal_camera_param_set(uint8_t *buff)
{
	uint16_t value = (uint16_t)((buff[1]<<8)+buff[2]);

	if(buff[0] == 0){
		gimbal_set_zoom(&gimbal,value);
	}
	else if(buff[0] == 1){
		gimbal_set_focus(&gimbal,value);
	}
	else{
		
	}

	send_to_pc(PC_GIMBAL_CAMERA_PARAM_SET, NULL, 0);

}

static void pc_gimbal_camera_adjust(uint8_t *buff)
{
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

	send_to_pc(PC_GIMBAL_CAMERA_ADJUST, NULL, 0);

}

