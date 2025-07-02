#ifndef __PC_PROTOCOL_H__
#define __PC_PROTOCOL_H__

#include "main.h"


#define pc_protocol_head 0xff

#define pc_addr 0x03
#define mcu_addr 0x01

// can id
// 实际canid为canid<<16 + key
#define gimbal_can_id 0x07
#define mcu_can_id 0x02

/// 云台 CAN 通信 Key 枚举（协议 V2.9）
enum gimbal_key_e{
    //—— 云台状态信息 ——//
    GIMBAL_KEY_STATE1         = 0x0001,  ///< 云台状态1（补光灯档位、雨刷档位、加热器状态、自动开关、软件版本号）
    GIMBAL_KEY_STATE2         = 0x0002,  ///< 云台状态2（温度、光照度）
    GIMBAL_KEY_ANGLE          = 0x0003,  ///< 云台角度（横滚、俯仰、编码器校准值、软限位开关）
    GIMBAL_KEY_SPEED          = 0x0004,  ///< 云台最大速度 & 上下限位标定值
    GIMBAL_KEY_LIDAR_DIST     = 0x0005,  ///< 测距雷达（错误码 + 距离 mm）
    GIMBAL_KEY_METHANE        = 0x0006,  ///< 激光甲烷遥测（浓度、光强等）
    GIMBAL_KEY_ERROR          = 0x0007,  ///< 故障信息（电机、编码器、相机、存储、雷达等报警位）

    //—— 云台状态指令 ——//
    GIMBAL_KEY_SET_LIGHT      = 0x1000,  ///< 设置补光灯档位
    GIMBAL_KEY_SET_WIPER      = 0x1001,  ///< 设置雨刷档位
    GIMBAL_KEY_HEATER_MANUAL  = 0x1002,  ///< 云台加热器手动开关
    GIMBAL_KEY_HEATER_AUTO    = 0x1003,  ///< 云台加热器自动开关
    GIMBAL_KEY_READ_VERSION   = 0x1004,  ///< 读取云台软件版本号

    //—— 云台控制指令 ——//
    GIMBAL_KEY_STOP           = 0x2000,  ///< 云台停止
    GIMBAL_KEY_TURN_LEFT      = 0x2001,  ///< 云台左转（速度模式）
    GIMBAL_KEY_TURN_RIGHT     = 0x2002,  ///< 云台右转（速度模式）
    GIMBAL_KEY_TILT_UP        = 0x2003,  ///< 云台上转（速度模式）
    GIMBAL_KEY_TILT_DOWN      = 0x2004,  ///< 云台下转（速度模式）
    GIMBAL_KEY_RELATIVE_MOVE  = 0x2005,  ///< 相对位置模式（左右&上下转）
    GIMBAL_KEY_SET_MAX_SPEED  = 0x2006,  ///< 设置最大运动速度（横滚 & 俯仰）
    GIMBAL_KEY_ABSOLUTE_MOVE  = 0x2007,  ///< 绝对位置模式（左右&上下转）
    GIMBAL_KEY_RESET          = 0x200A,  ///< 云台复位（运动回到初始化位置）
    GIMBAL_KEY_CALIBRATE      = 0x200B,  ///< 云台标定（将当前位置设为初始位置）
    GIMBAL_KEY_ENCODER_CAL    = 0x200C,  ///< 云台编码器校准（X、Y 补偿值）
    GIMBAL_KEY_PITCH_UP_LIM   = 0x200D,  ///< 俯仰轴上限位标定
    GIMBAL_KEY_PITCH_DOWN_LIM = 0x200E,  ///< 俯仰轴下限位标定
    GIMBAL_KEY_SOFT_LIMIT_SW  = 0x200F,  ///< 俯仰轴软件限位开关

    //—— 可见光摄像头指令 ——//
    GIMBAL_KEY_ZOOM_STOP          = 0x3000,  ///< 变倍停止
    GIMBAL_KEY_ZOOM_IN            = 0x3001,  ///< 变倍+（默认速度）
    GIMBAL_KEY_ZOOM_OUT           = 0x3002,  ///< 变倍-（默认速度）
    GIMBAL_KEY_ZOOM_IN_SPEED      = 0x3003,  ///< 变倍+（指定速度）
    GIMBAL_KEY_ZOOM_OUT_SPEED     = 0x3004,  ///< 变倍-（指定速度）
    GIMBAL_KEY_ZOOM_TO            = 0x3005,  ///< 变倍到指定倍率坐标
    GIMBAL_KEY_ZOOM_FOCUS_TO      = 0x3006,  ///< 变倍 & 变焦到指定坐标
    GIMBAL_KEY_DIGITAL_ZOOM_ON    = 0x3007,  ///< 数字变倍开
    GIMBAL_KEY_DIGITAL_ZOOM_OFF   = 0x3008,  ///< 数字变倍关
    GIMBAL_KEY_FOCUS_STOP         = 0x3009,  ///< 焦距停止
    GIMBAL_KEY_FOCUS_IN           = 0x300A,  ///< 焦距+
    GIMBAL_KEY_FOCUS_OUT          = 0x300B,  ///< 焦距-
    GIMBAL_KEY_FOCUS_TO           = 0x300C,  ///< 焦距到指定坐标
    GIMBAL_KEY_FOCUS_AUTO         = 0x300D,  ///< 自动聚焦模式
    GIMBAL_KEY_FOCUS_MANUAL       = 0x300E,  ///< 手动聚焦模式
    GIMBAL_KEY_FOCUS_MODE_SWITCH  = 0x300F,  ///< 聚焦自动/手动切换
    GIMBAL_KEY_FOCUS_SINGLE       = 0x3010,  ///< 一次聚焦模式
    GIMBAL_KEY_FOCUS_INFINITY     = 0x3011,  ///< 无穷远聚焦模式
    GIMBAL_KEY_FOCUS_DIST_SET     = 0x3012,  ///< 最近可聚焦距离设置
    GIMBAL_KEY_AF_SENS_NORMAL     = 0x3013,  ///< 自动聚焦灵敏度普通
    GIMBAL_KEY_AF_SENS_LOW        = 0x3014,  ///< 自动聚焦灵敏度低
    GIMBAL_KEY_AREA_FOCUS         = 0x3015,  ///< 区域聚焦
    GIMBAL_KEY_DAY_MODE           = 0x3016,  ///< 白天模式
    GIMBAL_KEY_NIGHT_MODE         = 0x3017,  ///< 黑夜模式
    GIMBAL_KEY_ZOOM_POS_QUERY     = 0x3018,  ///< 变倍坐标查询
    GIMBAL_KEY_DIGI_ZOOM_QUERY    = 0x3019,  ///< 数字变倍开关查询
    GIMBAL_KEY_FOCUS_MODE_QUERY   = 0x301A,  ///< 聚焦模式查询
    GIMBAL_KEY_FOCUS_POS_QUERY    = 0x301B,  ///< 聚焦坐标查询
    GIMBAL_KEY_FOCUS_STATUS_QUERY = 0x301C,  ///< 聚焦状态查询
    GIMBAL_KEY_OPTICAL_ZOOM_QUERY = 0x301D,  ///< 光学倍率值查询

    //—— 激光测距雷达指令 ——//
    GIMBAL_KEY_LIDAR_OFF       = 0x4000,  ///< 关闭测距
    GIMBAL_KEY_LIDAR_ON        = 0x4001,  ///< 启动测距

    //—— 激光甲烷遥测指令 ——//
    GIMBAL_KEY_METHANE_OFF     = 0x5000,  ///< 关闭甲烷测量
    GIMBAL_KEY_METHANE_ON      = 0x5001   ///< 启动甲烷测量
};

/// @brief pc通信功能码枚举（7.x 节）
enum protocol_function_code_e
{
    PC_GIMBAL_CONTROL               = 0x00C3,  ///< 7.1 云台控制
    PC_GIMBAL_ANGLE_QUERY           = 0x00C4,  ///< 7.2 云台角度查询
    PC_GIMBAL_AUX_SWITCH_CONTROL    = 0x00C5,  ///< 7.3.1 云台辅助开关控制（雨刷/补光灯/透雾/模式/防冻）
    PC_GIMBAL_PRESET_POINT          = 0x00CA,	///< 7.3.2 云台预置点设置（设置/召回/删除）
    PC_GIMBAL_CAMERA_PARAM_QUERY    = 0x00CB,	///< 7.3.3 云台相机参数查询（变倍值/聚焦值）
    PC_GIMBAL_CAMERA_PARAM_SET      = 0x00CC,	///< 7.3.4 云台相机参数设置（变倍值/聚焦值）
    PC_GIMBAL_CAMERA_ADJUST         = 0x00CD,	///< 7.3.5 云台相机±量设置（变倍-/变倍+、变焦-/变焦+、光圈-/光圈+）

    // user
    PC_MAC_ADDR_SET               = 0x01F1,  /// 自定义功能码，设置mac地址
    PC_IP_ADDR_SET                = 0x01F2,  /// 自定义功能码，设置ip地址
    PC_MASK_ADDR_SET              = 0x01F3,  /// 自定义功能码，设置掩码地址
    PC_GATEWAY_ADDR_SET           = 0x01F4,  /// 自定义功能码，设置网关地址
};

// gimbal protocol
struct __attribute__((packed)) pc_comm_protocol_t
{
	uint8_t head;
	uint8_t source_addr;
	uint8_t target_addr;
	uint16_t function_code;
	uint16_t data_length;
};

struct pc_unpack_data_t
{
	uint16_t function_code;
	uint8_t data[8];
	uint16_t data_length;
	uint8_t comm_type;
};

int get_index_gimbal_key(uint16_t key);
int get_index_pc_function_code(uint16_t function_code);

extern void (*pc2gimbal_pack[])(struct pc_unpack_data_t *pc_unpack_data);


#endif

