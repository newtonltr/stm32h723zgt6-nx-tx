#ifndef __GIMBAL_H__
#define __GIMBAL_H__

#include "main.h"
#include "fdcan.h"

struct gimbal_t
{
	char soft_ver[16];
	uint32_t error_code[8];
	uint8_t led;
	uint8_t wiper;
	uint8_t ptc;
	uint8_t reserve;
	uint16_t temp;
	uint16_t lux;
	float yaw;			//云台定义的坐标系
	float pitch;
	float yaw_local;	//本地右手坐标系
	float pitch_local;
	uint16_t zoom;
	uint16_t focus;
	uint16_t ch4;
	uint16_t intensity;
	float distance;		
};

int gimbal_parse(struct fdcan_rx_frame *rx_frame, struct gimbal_t *pdev, uint8_t *data);

// functioncode = PC_GIMBAL_CONTROL 0x00C3
void gimbal_move(uint8_t dir,uint16_t speed);
void gimbal_hor_move_to(struct gimbal_t *pdev, double angle);
void gimbal_ver_move_to(struct gimbal_t *pdev,double angle);
void gimbal_sync_move_to(struct gimbal_t *pdev,double x, double y);

// functioncode = PC_GIMBAL_ANGLE_QUERY 0x00C4
int gimbal_get_yaw(struct gimbal_t *pdev);
int gimbal_get_pitch(struct gimbal_t *pdev);

// functioncode = PC_GIMBAL_AUX_SWITCH_CONTROL 0x00C5
int gimbal_wiper(struct gimbal_t *pdev,uint8_t operation);
int gimbal_led(struct gimbal_t *pdev,uint16_t value);
int gimbal_fog(struct gimbal_t *pdev,uint8_t operation);
int gimbal_color_mode(struct gimbal_t *pdev,uint8_t operation);
int gimbal_antifreeze(struct gimbal_t *pdev,uint8_t operation);

// functioncode = PC_GIMBAL_PRESET_POINT 0x00CA
int gimbal_pre_set(struct gimbal_t *pdev,uint16_t no);
int gimbal_pre_recall(struct gimbal_t *pdev,uint16_t no);
int gimbal_pre_remove(struct gimbal_t *pdev,uint16_t no);

// functioncode = PC_GIMBAL_CAMERA_PARAM_QUERY 0x00CB
int gimbal_get_zoom(struct gimbal_t *pdev);
int gimbal_get_focus(struct gimbal_t *pdev);

// functioncode = PC_GIMBAL_CAMERA_PARAM_SET 0x00CC
int gimbal_set_zoom(struct gimbal_t *pdev,uint16_t value);
int gimbal_set_focus(struct gimbal_t *pdev,uint16_t value);	

// functioncode = PC_GIMBAL_CAMERA_ADJUST 0x00CD
int gimbal_zoom_add_sub(struct gimbal_t *pdev,uint8_t operation);
int gimbal_focus_add_sub(struct gimbal_t *pdev,uint8_t operation);
int gimbal_aperture_add_sub(struct gimbal_t *pdev,uint8_t operation);

extern struct gimbal_t gimbal;

#endif
