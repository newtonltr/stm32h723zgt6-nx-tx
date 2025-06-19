#include "gimbal.h"
#include "fdcan.h"
#include "usart.h"
#include "pc_protocol.h"


struct gimbal_t gimbal;

static void gimbal_can_send(uint16_t key, uint8_t *data, uint16_t data_length)
{
	struct fdcan_tx_frame fdcan_tx_frame;
	fdcan_tx_frame.header.Identifier = mcu_can_id << 16 | key;
	fdcan_tx_frame.header.IdType = FDCAN_EXTENDED_ID;
	fdcan_tx_frame.header.TxFrameType = FDCAN_DATA_FRAME;
	fdcan_tx_frame.header.DataLength = (uint32_t)data_length;
	fdcan_tx_frame.header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
	fdcan_tx_frame.header.BitRateSwitch = FDCAN_BRS_OFF;
	fdcan_tx_frame.header.FDFormat = FDCAN_CLASSIC_CAN;
	fdcan_tx_frame.header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
	fdcan_tx_frame.header.MessageMarker = 0;
	memcpy(fdcan_tx_frame.data, data, data_length);
	fdcan1_send(&fdcan_tx_frame);
}

int gimbal_parse(struct fdcan_rx_frame *rx_frame, struct gimbal_t *pdev, uint8_t *data)
{
	if( ((rx_frame->header.Identifier>>16) & 0xff) != gimbal_can_id){
		return -1;
	}

	switch (rx_frame->header.Identifier&0xffff)
	{
	case GIMBAL_KEY_STATE1:
		pdev->led = data[0];
		pdev->wiper = data[1];
		pdev->ptc = data[2];
		if(pdev->soft_ver[0] == 0){
			snprintf(pdev->soft_ver, sizeof(pdev->soft_ver), "%u.%u.%u", data[4], data[5], data[6]);	
		}
		break;
	case GIMBAL_KEY_STATE2:
		pdev->temp = (uint16_t)(data[0]<<8)+data[1];
		pdev->lux = (uint16_t)(data[2]<<8)+data[3];
		break;
	case GIMBAL_KEY_ANGLE:
		pdev->yaw = (int16_t)((data[0]<<8)+data[1]) * 0.1f;
		pdev->pitch = (int16_t)((data[2]<<8)+data[3]) * 0.1f;
		pdev->yaw_local = pdev->yaw <= 180 ? pdev->yaw : (pdev->yaw-360);
		pdev->pitch_local = pdev->pitch <= 90 ? pdev->pitch : (pdev->pitch-360);
		break;
	case GIMBAL_KEY_SPEED:
		/* code */
		break;
	case GIMBAL_KEY_LIDAR_DIST:
		if(!data[0]){
			pdev->distance = (uint32_t)((data[1]<<16) + (data[2]<<8) + data[3]);
		}
		else{
			pdev->distance = 0;
		}
		break;
	case GIMBAL_KEY_METHANE:
		pdev->ch4 = (uint16_t)((data[0]<<8)+data[1]);
		pdev->intensity = (uint16_t)((data[2]<<8)+data[3]);
		break;
	case GIMBAL_KEY_ERROR:
		memcpy(pdev->error_code, data, 8);
		if(pdev->error_code[2]){
			// flag_set_bit(&err_info.sensor_err, ERR_SENSOR_CH4);
		}
		break;
	case GIMBAL_KEY_ZOOM_POS_QUERY:
		pdev->zoom = (uint16_t)((data[0]<<8)+data[1]);
		break;
	case GIMBAL_KEY_FOCUS_POS_QUERY:
		pdev->focus = (uint16_t)((data[0]<<8)+data[1]);
		break;
	
	default:
		break;
	}

	return 0;
}


void gimbal_move(uint8_t dir,uint16_t speed)
{
	uint8_t data[8] = {0};
	uint32_t yaw_speed;
	uint32_t pitch_speed;
	uint16_t pitch;
	uint16_t yaw;

	yaw_speed = speed * 10 * 40 / 45;
	pitch_speed = speed * 10 * 10 / 45;

	yaw_speed = yaw_speed > 400 ? 400 : yaw_speed;
	pitch_speed = pitch_speed > 100 ? 100 : pitch_speed;

	data[0] = (yaw_speed>>8) & 0xff;
	data[1] = (yaw_speed>>0) & 0xff;
	data[2] = (pitch_speed>>8) & 0xff;
	data[3] = (pitch_speed>>0) & 0xff;
	gimbal_can_send(GIMBAL_KEY_SET_MAX_SPEED, data, 0x08);

	//left
	if(dir == 0x03){
		gimbal_can_send(GIMBAL_KEY_TURN_LEFT, data, 0x08);
	}	
	//right
	else if(dir == 0x04){
		gimbal_can_send(GIMBAL_KEY_TURN_RIGHT, data, 0x08);
	}
	//up
	else if(dir == 0x01){
		gimbal_can_send(GIMBAL_KEY_TILT_UP, data, 0x08);
	}
	//down
	else if(dir == 0x02){
		gimbal_can_send(GIMBAL_KEY_TILT_DOWN, data, 0x08);
	}
	//stop
	else if(dir == 0x00){
		gimbal_can_send(GIMBAL_KEY_STOP, data, 0x08);
	}
	else{
	}
	
}

//set qt ptz roll, limit [0-360]
void gimbal_hor_move_to(struct gimbal_t *pdev, double angle)
{
	uint8_t data[8] = {0};
	uint16_t pitch = pdev->pitch * 10;
	uint16_t yaw = angle / 10;

	float yaw_local = (yaw * 0.1) <= 180 ? (yaw * 0.1) : (yaw * 0.1 - 360);
	float pitch_local = pdev->pitch_local;


	data[0] = 0x01;
	data[1] = 0x90;
	data[2] = 0x00;
	data[3] = 0x64;
	gimbal_can_send(GIMBAL_KEY_SET_MAX_SPEED, data, 0x08);

	data[0] = (yaw>>8) & 0xff;
	data[1] = yaw & 0xff;
	data[2] = (pitch >>8) & 0xff;
	data[3] = (pitch) & 0xff;
	gimbal_can_send(GIMBAL_KEY_ABSOLUTE_MOVE, data, 0x08);

}

//set qt ptz yaw, limit [270,90]
void gimbal_ver_move_to(struct gimbal_t *pdev,double angle)
{
	uint8_t data[8] = {0};
	uint16_t yaw = pdev->yaw * 10;
	uint16_t pitch = angle / 10;



	float yaw_local = pdev->yaw_local;
	float pitch_local = (pitch * 0.1f) <= 90 ? (pitch * 0.1f) : (pitch * 0.1f - 360);


	data[0] = 0x01;
	data[1] = 0x90;
	data[2] = 0x00;
	data[3] = 0x64;
	gimbal_can_send(GIMBAL_KEY_SET_MAX_SPEED, data, 0x08);

	data[0] = (yaw >>8) & 0xff;
	data[1] = (yaw) & 0xff;
	data[2] = (pitch>>8) & 0xff;
	data[3] = pitch & 0xff;
	gimbal_can_send(GIMBAL_KEY_ABSOLUTE_MOVE, data, 0x08);

}

void gimbal_sync_move_to(struct gimbal_t *pdev,double x, double y)
{
	uint8_t data[8] = {0};
	uint16_t yaw = x / 10;
	uint16_t pitch = y / 10;
	data[0] = 0x01;
	data[1] = 0x90;
	data[2] = 0x00;
	data[3] = 0x64;
	gimbal_can_send(GIMBAL_KEY_SET_MAX_SPEED, data, 0x08);



	float yaw_local = (yaw * 0.1) <= 180 ? (yaw * 0.1) : (yaw * 0.1 - 360);
	float pitch_local = (pitch * 0.1) <= 90 ? (pitch * 0.1) : (pitch * 0.1 - 360);



	data[0] = (yaw >>8) & 0xff;
	data[1] = (yaw) & 0xff;
	data[2] = (pitch>>8) & 0xff;
	data[3] = pitch & 0xff;
	gimbal_can_send(GIMBAL_KEY_ABSOLUTE_MOVE, data, 0x08);

}

int gimbal_get_yaw(struct gimbal_t *pdev)
{
	
	return 0;
}

int gimbal_get_pitch(struct gimbal_t *pdev)
{
	
	return 0;
}


int gimbal_wiper(struct gimbal_t *pdev,uint8_t operation)
{
	int ret = 0;
	uint8_t data[8] = {0};
	
	if(operation > 3){
		data[0] = 3;
	}
	else{
		data[0] = operation;
	}

	gimbal_can_send(GIMBAL_KEY_SET_WIPER, data, 0x08);
	
	return ret;
}


int gimbal_led(struct gimbal_t *pdev,uint16_t value)
{
	int ret = 0;
	uint8_t data[8] = {0};
	
	if(value > 3){
		data[0] = 3;
	}
	else{
		data[0] = value;
	}

	gimbal_can_send(GIMBAL_KEY_SET_LIGHT, data, 0x08);
	
	return ret;
}

int gimbal_fog(struct gimbal_t *pdev,uint8_t operation)
{
	int ret = 0;
	
	if(operation == 0){

	}
	else if(operation == 1){

	}
	else{
		ret = -1;
	}
	
	return ret;
}

int gimbal_color_mode(struct gimbal_t *pdev,uint8_t operation)
{
	int ret = 0;
	
	if(operation == 0){

	}
	else if(operation == 1){

	}
	else{
		ret = -1;
	}
	
	return ret;
}

int gimbal_antifreeze(struct gimbal_t *pdev,uint8_t operation)
{
	int ret = 0;
	
	if(operation == 0){

	}
	else if(operation == 1){

	}
	else{
		ret = -1;
	}
	
	return ret;
}


int gimbal_pre_set(struct gimbal_t *pdev,uint16_t no)
{
	int ret = 0;

	return ret;
}

int gimbal_pre_recall(struct gimbal_t *pdev,uint16_t no)
{
	int ret = 0;
	
	return ret;
}

int gimbal_pre_remove(struct gimbal_t *pdev,uint16_t no)
{
	int ret = 0;
	
	return ret;
}

int gimbal_get_zoom(struct gimbal_t *pdev)
{
	int ret = 0;

	uint8_t data[8] = {0};

	gimbal_can_send(GIMBAL_KEY_ZOOM_POS_QUERY, data, 0x08);

	return ret;
}

int gimbal_get_focus(struct gimbal_t *pdev)
{
	int ret = 0;

	uint8_t data[8] = {0};
	gimbal_can_send(GIMBAL_KEY_FOCUS_POS_QUERY, data, 0x08);

	return ret;
}

int gimbal_set_zoom(struct gimbal_t *pdev,uint16_t value)
{
	int ret = 0;
	uint16_t zoom = value;
	uint8_t data[8] = {0};

	if(zoom > 0x7ac0){
		zoom = 0x7ac0;
	}

	data[0] = (zoom>>8) & 0xff;
	data[1] = zoom & 0xff;
	gimbal_can_send(GIMBAL_KEY_ZOOM_TO, data, 0x08);

	return ret;
}


int gimbal_set_focus(struct gimbal_t *pdev,uint16_t value)
{
	int ret = 0;
	uint16_t focus = value;
	uint8_t data[8] = {0};

	data[0] = (focus>>8) & 0xff;
	data[1] = focus & 0xff;
	gimbal_can_send(GIMBAL_KEY_FOCUS_TO, data, 0x08);

	return ret;
}


int gimbal_zoom_add_sub(struct gimbal_t *pdev,uint8_t operation)
{
	int ret = 0;

	uint8_t data[8] = {0};
	
	if(operation == 0){
		gimbal_can_send(GIMBAL_KEY_ZOOM_OUT, data, 0x08);
	}
	else if(operation == 1){
		gimbal_can_send(GIMBAL_KEY_ZOOM_IN, data, 0x08);
	}
	else{
		gimbal_can_send(GIMBAL_KEY_ZOOM_STOP, data, 0x08);
	}
	
	return ret;
}

int gimbal_focus_add_sub(struct gimbal_t *pdev,uint8_t operation)
{
	int ret = 0;

	uint8_t data[8] = {0};
	
	if(operation == 0){
		gimbal_can_send(GIMBAL_KEY_FOCUS_OUT, data, 0x08);
	}
	else if(operation == 1){
		gimbal_can_send(GIMBAL_KEY_FOCUS_IN, data, 0x08);
	}
	else{
		gimbal_can_send(GIMBAL_KEY_FOCUS_STOP, data, 0x08);
	}
	
	return ret;
}


int gimbal_aperture_add_sub(struct gimbal_t *pdev,uint8_t operation)
{
	int ret = 0;
	
	if(operation == 0){

	}
	else if(operation == 1){

	}
	else{
		ret = -1;
	}
	
	return ret;
}



