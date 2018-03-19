/*
 * drivers/media/video/gc2145.c
 *
 * Copyright (C) ROCKCHIP, Inc.
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "generic_sensor.h"

/*
*      Driver Version Note
*v0.0.1: this driver is compatible with generic_sensor
*v0.0.3:
*        add sensor_focus_af_const_pause_usr_cb;
*/
static int version = KERNEL_VERSION(0, 0, 3);
module_param(version, int, S_IRUGO);

static int debug;
module_param(debug, int, S_IRUGO | S_IWUSR);

#define dprintk(level, fmt, arg...) do {			\
	if (debug >= level) 					\
	printk(KERN_WARNING fmt, ## arg);			\
} while (0)

/* Sensor Driver Configuration Begin */
#define SENSOR_NAME RK29_CAM_SENSOR_GC2145
#define SENSOR_V4L2_IDENT V4L2_IDENT_GC2145
#define SENSOR_ID 0x2145
#define SENSOR_BUS_PARAM	(V4L2_MBUS_MASTER |\
				 V4L2_MBUS_PCLK_SAMPLE_RISING |\
				 V4L2_MBUS_HSYNC_ACTIVE_HIGH |\
				 V4L2_MBUS_VSYNC_ACTIVE_LOW |\
				 V4L2_MBUS_DATA_ACTIVE_HIGH |\
				 SOCAM_MCLK_24MHZ)
#define SENSOR_PREVIEW_W				800
#define SENSOR_PREVIEW_H				600
#define SENSOR_PREVIEW_FPS				15000	// 15fps
#define SENSOR_FULLRES_L_FPS				7500	// 7.5fps
#define SENSOR_FULLRES_H_FPS				7500	// 7.5fps
#define SENSOR_720P_FPS 				15000
#define SENSOR_1080P_FPS				0

#define SENSOR_REGISTER_LEN				1	// sensor register address bytes
#define SENSOR_VALUE_LEN				1	// sensor register value bytes
static unsigned int SensorConfiguration =
    (CFG_WhiteBalance | CFG_Effect | CFG_Scene);
static unsigned int SensorChipID[] = { SENSOR_ID };
/* Sensor Driver Configuration End */

#define SENSOR_NAME_STRING(a) STR(CONS(SENSOR_NAME, a))
#define SENSOR_NAME_VARFUN(a) CONS(SENSOR_NAME, a)

#define SensorRegVal(a, b) CONS4(SensorReg, SENSOR_REGISTER_LEN, Val, SENSOR_VALUE_LEN)(a, b)
#define sensor_write(client, reg, v) CONS4(sensor_write_reg, SENSOR_REGISTER_LEN, val, SENSOR_VALUE_LEN)(client, (reg), (v))
#define sensor_read(client, reg, v) CONS4(sensor_read_reg, SENSOR_REGISTER_LEN, val, SENSOR_VALUE_LEN)(client, (reg), (v))
#define sensor_write_array generic_sensor_write_array

struct sensor_parameter {
	unsigned int PreviewDummyPixels;
	unsigned int CaptureDummyPixels;
	unsigned int preview_exposure;
	unsigned short int preview_line_width;
	unsigned short int preview_gain;

	unsigned short int PreviewPclk;
	unsigned short int CapturePclk;
	char awb[6];
};

struct specific_sensor {
	struct generic_sensor common_sensor;
	//define user data below
	struct sensor_parameter parameter;
	u16 shutter;

};

/*
*  The follow setting need been filled.
*
*  Must Filled:
*  sensor_init_data :				Sensor initial setting;
*  sensor_fullres_lowfps_data : 	Sensor full resolution setting with best auality, recommand for video;
*  sensor_preview_data :			Sensor preview resolution setting, recommand it is vga or svga;
*  sensor_softreset_data :			Sensor software reset register;
*  sensor_check_id_data :			Sensir chip id register;
*
*  Optional filled:
*  sensor_fullres_highfps_data: 	Sensor full resolution setting with high framerate, recommand for video;
*  sensor_720p: 					Sensor 720p setting, it is for video;
*  sensor_1080p:					Sensor 1080p setting, it is for video;
*
*  :::::WARNING:::::
*  The SensorEnd which is the setting end flag must be filled int the last of each setting;
*/

/* Sensor initial setting */
static struct rk_sensor_reg sensor_init_data[] = {
	//SENSORDB("GC2145_Sensor_Init"},
	{0xfe, 0xf0},
	{0xfe, 0xf0},
	{0xfe, 0xf0},
	{0xfc, 0x06},
	{0xf6, 0x00},
	{0xf7, 0x1d},
	{0xf8, 0x84},
	{0xfa, 0x00},
	{0xf9, 0xfe},
	{0xf2, 0x00},
	/////////////////////////////////////////////////
	//////////////////ISP reg//////////////////////

	////////////////////////////////////////////////////
	{0xfe, 0x00},
	{0x03, 0x04},
	{0x04, 0xe2},
	{0x09, 0x00},
	{0x0a, 0x00},
	{0x0b, 0x00},
	{0x0c, 0x00},
	{0x0d, 0x04},
	{0x0e, 0xc0},
	{0x0f, 0x06},
	{0x10, 0x52},
	{0x12, 0x2e},
	{0x17, 0x14},		//mirror
	{0x18, 0x22},
	{0x19, 0x0e},
	{0x1a, 0x01},
	{0x1b, 0x4b},
	{0x1c, 0x07},
	{0x1d, 0x10},
	{0x1e, 0x88},
	{0x1f, 0x78},
	{0x20, 0x03},
	{0x21, 0x40},
	{0x22, 0xa0},
	{0x24, 0x16},
	{0x25, 0x01},
	{0x26, 0x10},
	{0x2d, 0x60},
	{0x30, 0x01},
	{0x31, 0x90},
	{0x33, 0x06},
	{0x34, 0x01},
	/////////////////////////////////////////////////
	//////////////////ISP reg////////////////////
	/////////////////////////////////////////////////
	{0xfe, 0x00},
	{0x80, 0x7f},
	{0x81, 0x26},
	{0x82, 0xfa},
	{0x83, 0x00},
	{0x84, 0x00},
	{0x86, 0x02},
	{0x88, 0x03},
	{0x89, 0x03},
	{0x85, 0x08},
	{0x8a, 0x00},
	{0x8b, 0x00},
	{0xb0, 0x55},
	{0xc3, 0x00},
	{0xc4, 0x80},
	{0xc5, 0x90},
	{0xc6, 0x3b},
	{0xc7, 0x46},
	{0xec, 0x06},
	{0xed, 0x04},
	{0xee, 0x60},
	{0xef, 0x90},
	{0xb6, 0x01},
	{0x90, 0x01},
	{0x91, 0x00},
	{0x92, 0x00},
	{0x93, 0x00},
	{0x94, 0x00},
	{0x95, 0x04},
	{0x96, 0xb0},
	{0x97, 0x06},
	{0x98, 0x40},
	/////////////////////////////////////////
	/////////// BLK ////////////////////////
	/////////////////////////////////////////
	{0xfe, 0x00},
	{0x40, 0x42},
	{0x41, 0x00},
	{0x43, 0x5b},
	{0x5e, 0x00},
	{0x5f, 0x00},
	{0x60, 0x00},
	{0x61, 0x00},
	{0x62, 0x00},
	{0x63, 0x00},
	{0x64, 0x00},
	{0x65, 0x00},
	{0x66, 0x20},
	{0x67, 0x20},
	{0x68, 0x20},
	{0x69, 0x20},
	{0x76, 0x00},
	{0x6a, 0x08},
	{0x6b, 0x08},
	{0x6c, 0x08},
	{0x6d, 0x08},
	{0x6e, 0x08},
	{0x6f, 0x08},
	{0x70, 0x08},
	{0x71, 0x08},
	{0x76, 0x00},
	{0x72, 0xf0},
	{0x7e, 0x3c},
	{0x7f, 0x00},
	{0xfe, 0x02},
	{0x48, 0x15},
	{0x49, 0x00},
	{0x4b, 0x0b},
	{0xfe, 0x00},
	////////////////////////////////////////
	/////////// AEC ////////////////////////
	////////////////////////////////////////
	{0xfe, 0x01},
	{0x01, 0x04},
	{0x02, 0xc0},
	{0x03, 0x04},
	{0x04, 0x90},
	{0x05, 0x30},
	{0x06, 0x90},
	{0x07, 0x30},
	{0x08, 0x80},
	{0x09, 0x00},
	{0x0a, 0x82},
	{0x0b, 0x11},
	{0x0c, 0x10},
	{0x11, 0x10},
	{0x13, 0x7b},
	{0x17, 0x00},
	{0x1c, 0x11},
	{0x1e, 0x61},
	{0x1f, 0x35},
	{0x20, 0x40},
	{0x22, 0x40},
	{0x23, 0x20},
	{0xfe, 0x02},
	{0x0f, 0x04},
	{0xfe, 0x01},
	{0x12, 0x35},
	{0x15, 0xb0},
	{0x10, 0x31},
	{0x3e, 0x28},
	{0x3f, 0xb0},
	{0x40, 0x90},
	{0x41, 0x0f},

	/////////////////////////////
	//////// INTPEE /////////////
	/////////////////////////////
	{0xfe, 0x02},
	{0x90, 0x6c},
	{0x91, 0x03},
	{0x92, 0xcb},
	{0x94, 0x33},
	{0x95, 0x84},
	{0x97, 0x45},		//65 kent
	{0xa2, 0x11},
	{0xfe, 0x00},
	/////////////////////////////
	//////// DNDD///////////////
	/////////////////////////////
	{0xfe, 0x02},
	{0x80, 0xc1},
	{0x81, 0x08},
	{0x82, 0x1f},		//08 by kent
	{0x83, 0x10},		//05 by kent
	{0x84, 0x0a},
	{0x86, 0xf0},
	{0x87, 0x50},
	{0x88, 0x15},
	{0x89, 0xb0},
	{0x8a, 0x30},
	{0x8b, 0x10},
	/////////////////////////////////////////
	/////////// ASDE ////////////////////////
	/////////////////////////////////////////
	{0xfe, 0x01},
	{0x21, 0x04},
	{0xfe, 0x02},
	{0xa3, 0x50},
	{0xa4, 0x20},
	{0xa5, 0x40},
	{0xa6, 0x80},
	{0xab, 0x40},
	{0xae, 0x0c},
	{0xb3, 0x46},
	{0xb4, 0x64},
	{0xb6, 0x38},
	{0xb7, 0x01},
	{0xb9, 0x2b},
	{0x3c, 0x04},
	{0x3d, 0x15},
	{0x4b, 0x06},
	{0x4c, 0x20},
	{0xfe, 0x00},
	/////////////////////////////////////////
	/////////// GAMMA       ////////////////////////
	/////////////////////////////////////////

	///////////////////gamma1////////////////////
#if 1
	{0xfe, 0x02},
	{0x10, 0x09},
	{0x11, 0x0d},
	{0x12, 0x13},
	{0x13, 0x19},
	{0x14, 0x27},
	{0x15, 0x37},
	{0x16, 0x45},
	{0x17, 0x53},
	{0x18, 0x69},
	{0x19, 0x7d},
	{0x1a, 0x8f},
	{0x1b, 0x9d},
	{0x1c, 0xa9},
	{0x1d, 0xbd},
	{0x1e, 0xcd},
	{0x1f, 0xd9},
	{0x20, 0xe3},
	{0x21, 0xea},
	{0x22, 0xef},
	{0x23, 0xf5},
	{0x24, 0xf9},
	{0x25, 0xff},
#else
	{0xfe, 0x02},
	{0x10, 0x0a},
	{0x11, 0x12},
	{0x12, 0x19},
	{0x13, 0x1f},
	{0x14, 0x2c},
	{0x15, 0x38},
	{0x16, 0x42},
	{0x17, 0x4e},
	{0x18, 0x63},
	{0x19, 0x76},
	{0x1a, 0x87},
	{0x1b, 0x96},
	{0x1c, 0xa2},
	{0x1d, 0xb8},
	{0x1e, 0xcb},
	{0x1f, 0xd8},
	{0x20, 0xe2},
	{0x21, 0xe9},
	{0x22, 0xf0},
	{0x23, 0xf8},
	{0x24, 0xfd},
	{0x25, 0xff},
	{0xfe, 0x00},
#endif
	{0xfe, 0x00},
	{0xc6, 0x20},
	{0xc7, 0x2b},
	///////////////////gamma2////////////////////
#if 1
	{0xfe, 0x02},
	{0x26, 0x0f},
	{0x27, 0x14},
	{0x28, 0x19},
	{0x29, 0x1e},
	{0x2a, 0x27},
	{0x2b, 0x33},
	{0x2c, 0x3b},
	{0x2d, 0x45},
	{0x2e, 0x59},
	{0x2f, 0x69},
	{0x30, 0x7c},
	{0x31, 0x89},
	{0x32, 0x98},
	{0x33, 0xae},
	{0x34, 0xc0},
	{0x35, 0xcf},
	{0x36, 0xda},
	{0x37, 0xe2},
	{0x38, 0xe9},
	{0x39, 0xf3},
	{0x3a, 0xf9},
	{0x3b, 0xff},
#else
	////Gamma outdoor
	{0xfe, 0x02},
	{0x26, 0x17},
	{0x27, 0x18},
	{0x28, 0x1c},
	{0x29, 0x20},
	{0x2a, 0x28},
	{0x2b, 0x34},
	{0x2c, 0x40},
	{0x2d, 0x49},
	{0x2e, 0x5b},
	{0x2f, 0x6d},
	{0x30, 0x7d},
	{0x31, 0x89},
	{0x32, 0x97},
	{0x33, 0xac},
	{0x34, 0xc0},
	{0x35, 0xcf},
	{0x36, 0xda},
	{0x37, 0xe5},
	{0x38, 0xec},
	{0x39, 0xf8},
	{0x3a, 0xfd},
	{0x3b, 0xff},
#endif
	///////////////////////////////////////////////
	///////////YCP ///////////////////////
	///////////////////////////////////////////////
	{0xfe, 0x02},
	{0xd1, 0x40},		//32 kent
	{0xd2, 0x40},		//32
	{0xd3, 0x48},		//40
	{0xd6, 0xf0},
	{0xd7, 0x10},
	{0xd8, 0xda},
	{0xdd, 0x14},
	{0xde, 0x86},
	{0xed, 0x80},
	{0xee, 0x00},
	{0xef, 0x3f},
	{0xd8, 0xd8},
	///////////////////abs/////////////////
	{0xfe, 0x01},
	{0x9f, 0x40},
	/////////////////////////////////////////////
	//////////////////////// LSC ///////////////
	//////////////////////////////////////////
	{0xfe, 0x01},
	{0xc2, 0x14},
	{0xc3, 0x0d},
	{0xc4, 0x0c},
	{0xc8, 0x15},
	{0xc9, 0x0d},
	{0xca, 0x0a},
	{0xbc, 0x24},
	{0xbd, 0x10},
	{0xbe, 0x0b},
	{0xb6, 0x25},
	{0xb7, 0x16},
	{0xb8, 0x15},
	{0xc5, 0x00},
	{0xc6, 0x00},
	{0xc7, 0x00},
	{0xcb, 0x00},
	{0xcc, 0x00},
	{0xcd, 0x00},
	{0xbf, 0x07},
	{0xc0, 0x00},
	{0xc1, 0x00},
	{0xb9, 0x00},
	{0xba, 0x00},
	{0xbb, 0x00},
	{0xaa, 0x01},
	{0xab, 0x01},
	{0xac, 0x00},
	{0xad, 0x05},
	{0xae, 0x06},
	{0xaf, 0x0e},
	{0xb0, 0x0b},
	{0xb1, 0x07},
	{0xb2, 0x06},
	{0xb3, 0x17},
	{0xb4, 0x0e},
	{0xb5, 0x0e},
	{0xd0, 0x09},
	{0xd1, 0x00},
	{0xd2, 0x00},
	{0xd6, 0x08},
	{0xd7, 0x00},
	{0xd8, 0x00},
	{0xd9, 0x00},
	{0xda, 0x00},
	{0xdb, 0x00},
	{0xd3, 0x0a},
	{0xd4, 0x00},
	{0xd5, 0x00},
	{0xa4, 0x00},
	{0xa5, 0x00},
	{0xa6, 0x77},
	{0xa7, 0x77},
	{0xa8, 0x77},
	{0xa9, 0x77},
	{0xa1, 0x80},
	{0xa2, 0x80},

	{0xfe, 0x01},
	{0xdf, 0x0d},
	{0xdc, 0x25},
	{0xdd, 0x30},
	{0xe0, 0x77},
	{0xe1, 0x80},
	{0xe2, 0x77},
	{0xe3, 0x90},
	{0xe6, 0x90},
	{0xe7, 0xa0},
	{0xe8, 0x90},
	{0xe9, 0xa0},
	{0xfe, 0x00},
	///////////////////////////////////////////////
	/////////// AWB////////////////////////
	///////////////////////////////////////////////
	{0xfe, 0x01},
	{0x4f, 0x00},
	{0x4f, 0x00},
	{0x4b, 0x01},
	{0x4f, 0x00},

	{0x4c, 0x01},		// D75
	{0x4d, 0x71},
	{0x4e, 0x01},
	{0x4c, 0x01},
	{0x4d, 0x91},
	{0x4e, 0x01},
	{0x4c, 0x01},
	{0x4d, 0x70},
	{0x4e, 0x01},
	{0x4c, 0x01},		// D65
	{0x4d, 0x90},
	{0x4e, 0x02},
	{0x4c, 0x01},
	{0x4d, 0xb0},
	{0x4e, 0x02},
	{0x4c, 0x01},
	{0x4d, 0x8f},
	{0x4e, 0x02},
	{0x4c, 0x01},
	{0x4d, 0x6f},
	{0x4e, 0x02},
	{0x4c, 0x01},
	{0x4d, 0xaf},
	{0x4e, 0x02},
	{0x4c, 0x01},
	{0x4d, 0xd0},
	{0x4e, 0x02},
	{0x4c, 0x01},
	{0x4d, 0xf0},
	{0x4e, 0x02},
	{0x4c, 0x01},
	{0x4d, 0xcf},
	{0x4e, 0x02},
	{0x4c, 0x01},
	{0x4d, 0xef},
	{0x4e, 0x02},
	{0x4c, 0x01},		//D50
	{0x4d, 0x6e},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0x8e},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0xae},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0xce},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0x4d},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0x6d},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0x8d},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0xad},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0xcd},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0x4c},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0x6c},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0x8c},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0xac},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0xcc},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0xcb},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0x4b},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0x6b},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0x8b},
	{0x4e, 0x03},
	{0x4c, 0x01},
	{0x4d, 0xab},
	{0x4e, 0x03},
	{0x4c, 0x01},		//CWF
	{0x4d, 0x8a},
	{0x4e, 0x04},
	{0x4c, 0x01},
	{0x4d, 0xaa},
	{0x4e, 0x04},
	{0x4c, 0x01},
	{0x4d, 0xca},
	{0x4e, 0x04},
	{0x4c, 0x01},
	{0x4d, 0xca},
	{0x4e, 0x04},
	{0x4c, 0x01},
	{0x4d, 0xc9},
	{0x4e, 0x04},
	{0x4c, 0x01},
	{0x4d, 0x8a},
	{0x4e, 0x04},
	{0x4c, 0x01},
	{0x4d, 0x89},
	{0x4e, 0x04},
	{0x4c, 0x01},
	{0x4d, 0xa9},
	{0x4e, 0x04},
	{0x4c, 0x02},		//tl84
	{0x4d, 0x0b},
	{0x4e, 0x05},
	{0x4c, 0x02},
	{0x4d, 0x0a},
	{0x4e, 0x05},
	{0x4c, 0x01},
	{0x4d, 0xeb},
	{0x4e, 0x05},
	{0x4c, 0x01},
	{0x4d, 0xea},
	{0x4e, 0x05},
	{0x4c, 0x02},
	{0x4d, 0x09},
	{0x4e, 0x05},
	{0x4c, 0x02},
	{0x4d, 0x29},
	{0x4e, 0x05},
	{0x4c, 0x02},
	{0x4d, 0x2a},
	{0x4e, 0x05},
	{0x4c, 0x02},
	{0x4d, 0x4a},
	{0x4e, 0x05},
	//{0x4c , 0x02}, //A
	//{0x4d , 0x6a},
	//{0x4e , 0x06},
	{0x4c, 0x02},
	{0x4d, 0x8a},
	{0x4e, 0x06},
	{0x4c, 0x02},
	{0x4d, 0x49},
	{0x4e, 0x06},
	{0x4c, 0x02},
	{0x4d, 0x69},
	{0x4e, 0x06},
	{0x4c, 0x02},
	{0x4d, 0x89},
	{0x4e, 0x06},
	{0x4c, 0x02},
	{0x4d, 0xa9},
	{0x4e, 0x06},
	{0x4c, 0x02},
	{0x4d, 0x48},
	{0x4e, 0x06},
	{0x4c, 0x02},
	{0x4d, 0x68},
	{0x4e, 0x06},
	{0x4c, 0x02},
	{0x4d, 0x69},
	{0x4e, 0x06},
	{0x4c, 0x02},		//H
	{0x4d, 0xca},
	{0x4e, 0x07},
	{0x4c, 0x02},
	{0x4d, 0xc9},
	{0x4e, 0x07},
	{0x4c, 0x02},
	{0x4d, 0xe9},
	{0x4e, 0x07},
	{0x4c, 0x03},
	{0x4d, 0x09},
	{0x4e, 0x07},
	{0x4c, 0x02},
	{0x4d, 0xc8},
	{0x4e, 0x07},
	{0x4c, 0x02},
	{0x4d, 0xe8},
	{0x4e, 0x07},
	{0x4c, 0x02},
	{0x4d, 0xa7},
	{0x4e, 0x07},
	{0x4c, 0x02},
	{0x4d, 0xc7},
	{0x4e, 0x07},
	{0x4c, 0x02},
	{0x4d, 0xe7},
	{0x4e, 0x07},
	{0x4c, 0x03},
	{0x4d, 0x07},
	{0x4e, 0x07},

	{0x4f, 0x01},
	{0x50, 0x80},
	{0x51, 0xa8},
	{0x52, 0x47},
	{0x53, 0x38},
	{0x54, 0xc7},
	{0x56, 0x0e},
	{0x58, 0x08},
	{0x5b, 0x00},
	{0x5c, 0x74},
	{0x5d, 0x8b},
	{0x61, 0xdb},
	{0x62, 0xb8},
	{0x63, 0x86},
	{0x64, 0xc0},
	{0x65, 0x04},
	{0x67, 0xa8},
	{0x68, 0xb0},
	{0x69, 0x00},
	{0x6a, 0xa8},
	{0x6b, 0xb0},
	{0x6c, 0xaf},
	{0x6d, 0x8b},
	{0x6e, 0x50},
	{0x6f, 0x18},
	{0x73, 0xf0},
	{0x70, 0x0d},
	{0x71, 0x60},
	{0x72, 0x80},
	{0x74, 0x01},
	{0x75, 0x01},
	{0x7f, 0x0c},
	{0x76, 0x70},
	{0x77, 0x58},
	{0x78, 0xa0},
	{0x79, 0x5e},
	{0x7a, 0x54},
	{0x7b, 0x58},
	{0xfe, 0x00},
	//////////////////////////////////////////
	///////////CC////////////////////////
	//////////////////////////////////////////
	{0xfe, 0x02},
	{0xc0, 0x01},
	{0xc1, 0x44},
	{0xc2, 0xfd},
	{0xc3, 0x04},
	{0xc4, 0xF0},
	{0xc5, 0x48},
	{0xc6, 0xfd},
	{0xc7, 0x46},
	{0xc8, 0xfd},
	{0xc9, 0x02},
	{0xca, 0xe0},
	{0xcb, 0x45},
	{0xcc, 0xec},
	{0xcd, 0x48},
	{0xce, 0xf0},
	{0xcf, 0xf0},
	{0xe3, 0x0c},
	{0xe4, 0x4b},
	{0xe5, 0xe0},
	//////////////////////////////////////////
	///////////ABS ////////////////////
	//////////////////////////////////////////
	{0xfe, 0x01},
	{0x9f, 0x40},
	{0xfe, 0x00},
	//////////////////////////////////////
	///////////  OUTPUT   ////////////////
	//////////////////////////////////////
	{0xfe, 0x00},
	{0xf2, 0x0f},
	///////////////dark sun////////////////////
	{0xfe, 0x02},
	{0x40, 0xbf},
	{0x46, 0xcf},
	{0xfe, 0x00},

	//////////////frame rate 50Hz/////////
	{0xfe, 0x00},
	{0x05, 0x01},
	{0x06, 0x56},
	{0x07, 0x00},
	{0x08, 0x32},
	{0xfe, 0x01},
	{0x25, 0x00},
	{0x26, 0xfa},

	{0x27, 0x04},
	{0x28, 0xe2},		//20fps
	{0x29, 0x07},
	{0x2a, 0xd0},		//14fps
	{0x2b, 0x09},
	{0x2c, 0xc4},		//12fps
	{0x2d, 0x0b},
	{0x2e, 0xb8},		//8fps
	{0xfe, 0x00},

	//SENSORDB("GC2145_Sensor_SVGA"},

	{0xfe, 0x00},
	{0xfd, 0x01},
	{0xfa, 0x00},
	//// crop window
	{0xfe, 0x00},
	{0x90, 0x01},
	{0x91, 0x00},
	{0x92, 0x00},
	{0x93, 0x00},
	{0x94, 0x00},
	{0x95, 0x02},
	{0x96, 0x58},
	{0x97, 0x03},
	{0x98, 0x20},
	{0x99, 0x11},
	{0x9a, 0x06},
	//// AWB
	{0xfe, 0x00},
	{0xec, 0x02},
	{0xed, 0x02},
	{0xee, 0x30},
	{0xef, 0x48},
	{0xfe, 0x02},
	{0x9d, 0x08},
	{0xfe, 0x01},
	{0x74, 0x00},
	//// AEC
	{0xfe, 0x01},
	{0x01, 0x04},
	{0x02, 0x60},
	{0x03, 0x02},
	{0x04, 0x48},
	{0x05, 0x18},
	{0x06, 0x50},
	{0x07, 0x10},
	{0x08, 0x38},
	{0x0a, 0x80},
	{0x21, 0x04},
	{0xfe, 0x00},
	{0x20, 0x03},
	{0xfe, 0x00},
	SensorEnd
};
/* Senor full resolution setting: recommand for capture */
static struct rk_sensor_reg sensor_fullres_lowfps_data[] = {

	//SENSORDB("GC2145_Sensor_2M"},
	{0xfe, 0x00},
	{0xfd, 0x00},
	{0xfa, 0x11},
	//// crop window
	{0xfe, 0x00},
	{0x90, 0x01},
	{0x91, 0x00},
	{0x92, 0x00},
	{0x93, 0x00},
	{0x94, 0x00},
	{0x95, 0x04},
	{0x96, 0xb0},
	{0x97, 0x06},
	{0x98, 0x40},
	{0x99, 0x11},
	{0x9a, 0x06},
	//// AWB
	{0xfe, 0x00},
	{0xec, 0x06},
	{0xed, 0x04},
	{0xee, 0x60},
	{0xef, 0x90},
	{0xfe, 0x01},
	{0x74, 0x01},
	//// AEC
	{0xfe, 0x01},
	{0x01, 0x04},
	{0x02, 0xc0},
	{0x03, 0x04},
	{0x04, 0x90},
	{0x05, 0x30},
	{0x06, 0x90},
	{0x07, 0x30},
	{0x08, 0x80},
	{0x0a, 0x82},
	{0xfe, 0x01},
	{0x21, 0x15},
	{0xfe, 0x00},
	{0x20, 0x15},		//if 0xfa=11,then 0x21=15;else if 0xfa=00,then 0x21=04
	{0xfe, 0x00},
	SensorEnd
};

/* Senor full resolution setting: recommand for video */
static struct rk_sensor_reg sensor_fullres_highfps_data[] = {
	SensorEnd
};
/* Preview resolution setting*/
static struct rk_sensor_reg sensor_preview_data[] = {

	//SENSORDB("GC2145_Sensor_SVGA"},

	{0xfe, 0x00},
	{0xb6, 0x01},
	{0xfd, 0x01},
	{0xfa, 0x00},
	//// crop window
	{0xfe, 0x00},
	{0x90, 0x01},
	{0x91, 0x00},
	{0x92, 0x00},
	{0x93, 0x00},
	{0x94, 0x00},
	{0x95, 0x02},
	{0x96, 0x58},
	{0x97, 0x03},
	{0x98, 0x20},
	{0x99, 0x11},
	{0x9a, 0x06},
	//// AWB
	{0xfe, 0x00},
	{0xec, 0x02},
	{0xed, 0x02},
	{0xee, 0x30},
	{0xef, 0x48},
	{0xfe, 0x02},
	{0x9d, 0x08},
	{0xfe, 0x01},
	{0x74, 0x00},
	//// AEC
	{0xfe, 0x01},
	{0x01, 0x04},
	{0x02, 0x60},
	{0x03, 0x02},
	{0x04, 0x48},
	{0x05, 0x18},
	{0x06, 0x50},
	{0x07, 0x10},
	{0x08, 0x38},
	{0x0a, 0x80},
	{0x21, 0x04},
	{0xfe, 0x00},
	{0x20, 0x03},
	{0xfe, 0x00},
	SensorEnd
};
/* 1280x720 */
static struct rk_sensor_reg sensor_720p[] = {
#if 0
	{0xfe, 0x00},
	//{0xfa , 0x11},
	{0xb6, 0x01},
	{0xfd, 0x00},
	//// crop window
	{0xfe, 0x00},
	{0x99, 0x55},
	{0x9a, 0x06},
	{0x9b, 0x00},
	{0x9c, 0x00},
	{0x9d, 0x01},
	{0x9e, 0x23},
	{0x9f, 0x00},
	{0xa0, 0x00},
	{0xa1, 0x01},
	{0xa2, 0x23},

	{0x90, 0x01},
	{0x91, 0x00},
	{0x92, 0x78},
	{0x93, 0x00},
	{0x94, 0x00},
	{0x95, 0x02},
	{0x96, 0xd0},
	{0x97, 0x05},
	{0x98, 0x00},
	//// AWB
	{0xfe, 0x00},
	{0xec, 0x06},
	{0xed, 0x04},
	{0xee, 0x60},
	{0xef, 0x90},
	{0xfe, 0x01},
	{0x74, 0x01},
	//// AEC
	{0xfe, 0x01},
	{0x01, 0x04},
	{0x02, 0xc0},
	{0x03, 0x04},
	{0x04, 0x90},
	{0x05, 0x30},
	{0x06, 0x90},
	{0x07, 0x30},
	{0x08, 0x80},
	{0x0a, 0x82},
	{0xfe, 0x01},
	{0x21, 0x15},
	{0xfe, 0x00},
	{0x20, 0x15},		//if 0xfa=11,then 0x21=15;else if 0xfa=00,then 0x21=04
	{0xfe, 0x00},
#endif
	SensorEnd
};

/* 1920x1080 */
static struct rk_sensor_reg sensor_1080p[] = {
	SensorEnd
};

static struct rk_sensor_reg sensor_softreset_data[] = {
	SensorRegVal(0xfe, 80),
	SensorWaitMs(5),
	SensorEnd
};

static struct rk_sensor_reg sensor_check_id_data[] = {
	SensorRegVal(0xf0, 0),
	SensorRegVal(0xf1, 0),
	SensorEnd
};
/*
*  The following setting must been filled, if the function is turn on by CONFIG_SENSOR_xxxx
*/
static struct rk_sensor_reg sensor_WhiteB_Auto[] = {
	{0xfe, 0x00},
	{0xb3, 0x61},
	{0xb4, 0x40},
	{0xb5, 0x61},
	{0x82, 0xfe},
	SensorEnd
};
/* Cloudy Colour Temperature : 6500K - 8000K  */
static struct rk_sensor_reg sensor_WhiteB_Cloudy[] = {
	{0xfe, 0x00},
	{0x82, 0xfc},
	{0xb3, 0x58},
	{0xb4, 0x40},
	{0xb5, 0x50},

	SensorEnd
};
/* ClearDay Colour Temperature : 5000K - 6500K	*/
static struct rk_sensor_reg sensor_WhiteB_ClearDay[] = {
	//Sunny
	{0xfe, 0x00},
	{0x82, 0xfc},
	{0xb3, 0x70},
	{0xb4, 0x40},
	{0xb5, 0x50},
	SensorEnd
};
/* Office Colour Temperature : 3500K - 5000K  */
static struct rk_sensor_reg sensor_WhiteB_TungstenLamp1[] = {
	//Office
	{0xfe, 0x00},
	{0x82, 0xfc},
	{0xb3, 0x50},
	{0xb4, 0x40},
	{0xb5, 0xa8},
	SensorEnd
};
/* Home Colour Temperature : 2500K - 3500K	*/
static struct rk_sensor_reg sensor_WhiteB_TungstenLamp2[] = {
	//Home
	{0xfe, 0x00},
	{0x82, 0xfc},
	{0xb3, 0xa0},
	{0xb4, 0x45},
	{0xb5, 0x40},
	SensorEnd
};

static struct rk_sensor_reg *sensor_WhiteBalanceSeqe[] = {
	sensor_WhiteB_Auto,
	sensor_WhiteB_Cloudy,
	sensor_WhiteB_ClearDay,
	sensor_WhiteB_TungstenLamp1,
	sensor_WhiteB_TungstenLamp2,
	NULL,
};

static struct rk_sensor_reg sensor_Brightness0[] = {
	// Brightness -2

	{0xfe, 0x01},
	{0x13, 0x10},
	{0xfe, 0x00},
	SensorEnd
};

static struct rk_sensor_reg sensor_Brightness1[] = {
	// Brightness -1

	{0xfe, 0x01},
	{0x13, 0x20},
	{0xfe, 0x00},

	SensorEnd
};

static struct rk_sensor_reg sensor_Brightness2[] = {
	//      Brightness 0

	{0xfe, 0x01},
	{0x13, 0x30},
	{0xfe, 0x00},

	SensorEnd
};

static struct rk_sensor_reg sensor_Brightness3[] = {
	// Brightness +1
	{0xfe, 0x01},
	{0x13, 0x40},
	{0xfe, 0x00},

	SensorEnd
};

static struct rk_sensor_reg sensor_Brightness4[] = {
	//      Brightness +2
	{0xfe, 0x01},
	{0x13, 0x45},
	{0xfe, 0x00},

	SensorEnd
};

static struct rk_sensor_reg sensor_Brightness5[] = {
	//      Brightness +3
	{0xfe, 0x01},
	{0x13, 0x50},
	{0xfe, 0x00},
	SensorEnd
};
static struct rk_sensor_reg *sensor_BrightnessSeqe[] = {
	sensor_Brightness0,
	sensor_Brightness1,
	sensor_Brightness2,
	sensor_Brightness3,
	sensor_Brightness4,
	sensor_Brightness5,
	NULL,
};

static struct rk_sensor_reg sensor_Effect_Normal[] = {
	{0xfe, 0x00},
	{0x83, 0xe0},
	SensorEnd
};

static struct rk_sensor_reg sensor_Effect_WandB[] = {
	{0xfe, 0x00},
	{0x83, 0x12},
	SensorEnd
};

static struct rk_sensor_reg sensor_Effect_Sepia[] = {
	{0xfe, 0x00},
	{0x83, 0x82},
	SensorEnd
};

static struct rk_sensor_reg sensor_Effect_Negative[] = {
	//Negative
	{0xfe, 0x00},
	{0x83, 0x01},
	SensorEnd
};
static struct rk_sensor_reg sensor_Effect_Bluish[] = {
	// Bluish
	{0xfe, 0x00},
	{0x83, 0x62},
	SensorEnd
};

static struct rk_sensor_reg sensor_Effect_Green[] = {
	//      Greenish
	{0xfe, 0x00},
	{0x83, 0x52},
	SensorEnd
};
static struct rk_sensor_reg *sensor_EffectSeqe[] = {
	sensor_Effect_Normal,
	sensor_Effect_WandB,
	sensor_Effect_Negative,
	sensor_Effect_Sepia,
	sensor_Effect_Bluish,
	sensor_Effect_Green,
	NULL,
};

static struct rk_sensor_reg sensor_Exposure0[] = {
	{0xfe, 0x01},
	{0x13, 0x60},
	{0xfe, 0x00},

	SensorEnd
};

static struct rk_sensor_reg sensor_Exposure1[] = {
	{0xfe, 0x01},
	{0x13, 0x65},
	{0xfe, 0x00},

	SensorEnd
};

static struct rk_sensor_reg sensor_Exposure2[] = {
	{0xfe, 0x01},
	{0x13, 0x70},
	{0xfe, 0x00},

	SensorEnd
};

static struct rk_sensor_reg sensor_Exposure3[] = {
	{0xfe, 0x01},
	{0x13, 0x7b},
	{0xfe, 0x00},
	SensorEnd
};

static struct rk_sensor_reg sensor_Exposure4[] = {
	{0xfe, 0x01},
	{0x13, 0x85},
	{0xfe, 0x00},

	SensorEnd
};

static struct rk_sensor_reg sensor_Exposure5[] = {
	{0xfe, 0x01},
	{0x13, 0x90},
	{0xfe, 0x00},

	SensorEnd
};

static struct rk_sensor_reg sensor_Exposure6[] = {
	{0xfe, 0x01},
	{0x13, 0x95},
	{0xfe, 0x00},

	SensorEnd
};

static struct rk_sensor_reg *sensor_ExposureSeqe[] = {
	sensor_Exposure0,
	sensor_Exposure1,
	sensor_Exposure2,
	sensor_Exposure3,
	sensor_Exposure4,
	sensor_Exposure5,
	sensor_Exposure6,
	NULL,
};

static struct rk_sensor_reg sensor_Saturation0[] = {
	SensorEnd
};

static struct rk_sensor_reg sensor_Saturation1[] = {
	SensorEnd
};

static struct rk_sensor_reg sensor_Saturation2[] = {
	SensorEnd
};
static struct rk_sensor_reg *sensor_SaturationSeqe[] = {
	sensor_Saturation0,
	sensor_Saturation1,
	sensor_Saturation2,
	NULL,
};

static struct rk_sensor_reg sensor_Contrast0[] = {
	//Contrast -3

	SensorEnd
};

static struct rk_sensor_reg sensor_Contrast1[] = {
	//Contrast -2

	SensorEnd
};

static struct rk_sensor_reg sensor_Contrast2[] = {
	// Contrast -1

	SensorEnd
};

static struct rk_sensor_reg sensor_Contrast3[] = {
	//Contrast 0

	SensorEnd
};

static struct rk_sensor_reg sensor_Contrast4[] = {
	//Contrast +1

	SensorEnd
};

static struct rk_sensor_reg sensor_Contrast5[] = {
	//Contrast +2

	SensorEnd
};

static struct rk_sensor_reg sensor_Contrast6[] = {
	//Contrast +3

	SensorEnd
};
static struct rk_sensor_reg *sensor_ContrastSeqe[] = {
	sensor_Contrast0,
	sensor_Contrast1,
	sensor_Contrast2,
	sensor_Contrast3,
	sensor_Contrast4,
	sensor_Contrast5,
	sensor_Contrast6,
	NULL,
};
static struct rk_sensor_reg sensor_SceneAuto[] = {
	{0xfe, 0x01},

	{0x3c, 0x40},
	{0xfe, 0x00},
	SensorEnd
};

static struct rk_sensor_reg sensor_SceneNight[] = {
	{0xfe, 0x01},

	{0x3c, 0x60},
	{0xfe, 0x00},
	SensorEnd
};
static struct rk_sensor_reg *sensor_SceneSeqe[] = {
	sensor_SceneAuto,
	sensor_SceneNight,
	NULL,
};

static struct rk_sensor_reg sensor_Zoom0[] = {
	SensorEnd
};

static struct rk_sensor_reg sensor_Zoom1[] = {
	SensorEnd
};

static struct rk_sensor_reg sensor_Zoom2[] = {
	SensorEnd
};

static struct rk_sensor_reg sensor_Zoom3[] = {
	SensorEnd
};
static struct rk_sensor_reg *sensor_ZoomSeqe[] = {
	sensor_Zoom0,
	sensor_Zoom1,
	sensor_Zoom2,
	sensor_Zoom3,
	NULL,
};

/*
* User could be add v4l2_querymenu in sensor_controls by new_usr_v4l2menu
*/
static struct v4l2_querymenu sensor_menus[] = {
};
/*
* User could be add v4l2_queryctrl in sensor_controls by new_user_v4l2ctrl
*/
static struct sensor_v4l2ctrl_usr_s sensor_controls[] = {
};

//MUST define the current used format as the first item
static struct rk_sensor_datafmt sensor_colour_fmts[] = {
	{MEDIA_BUS_FMT_UYVY8_2X8, V4L2_COLORSPACE_JPEG}
};
/*static struct soc_camera_ops sensor_ops;*/

/*
**********************************************************
* Following is local code:
*
* Please codeing your program here
**********************************************************
*/
/*
**********************************************************
* Following is callback
* If necessary, you could coding these callback
**********************************************************
*/
/*
* the function is called in open sensor
*/
static int sensor_activate_cb(struct i2c_client *client)
{

	return 0;
}

/*
* the function is called in close sensor
*/
static int sensor_deactivate_cb(struct i2c_client *client)
{
	//sensor_write(client, 0xfe, 0xf0);
	//sensor_write(client, 0xfe, 0xf0);
	//sensor_write(client, 0xfe, 0xf0);
	//sensor_write(client, 0xfa, 0x73);
	//sensor_write(client, 0x24, 0x00);
	return 0;
}

/*
* the function is called before sensor register setting in VIDIOC_S_FMT
*/
static unsigned int shutter_h, shutter_l, cap;
static int sensor_s_fmt_cb_th(struct i2c_client *client,
			      struct v4l2_mbus_framefmt *mf, bool capture)
{
	char value;
	unsigned int pid = 0, shutter, temp_reg;

	if ((mf->width == 800 && mf->height == 600) && (cap == 1)) {
		cap = 0;
		sensor_write(client, 0xb6, 0x00);	// AEC off
		sensor_write(client, 0x03, shutter_h);
		sensor_write(client, 0x04, shutter_l);
		msleep(50);
		printk("set preview for rewrite 0x03");
	}
	if (mf->width == 1600 && mf->height == 1200) {

		cap = 1;
		sensor_write(client, 0xfe, 0x00);
		sensor_write(client, 0xb6, 0x00);
		sensor_read(client, 0x03, &value);
		shutter_h = value;
		pid |= (value << 8);
		sensor_read(client, 0x04, &value);
		shutter_l = value;
		pid |= (value & 0xff);
		shutter = pid;

		temp_reg = shutter / 2;

		if (temp_reg < 1)
			temp_reg = 1;

		sensor_write(client, 0x03, ((temp_reg >> 8) & 0x1f));
		sensor_write(client, 0x04, (temp_reg & 0xff));
	}

	return 0;
}

/*
* the function is called after sensor register setting finished in VIDIOC_S_FMT
*/
static int sensor_s_fmt_cb_bh(struct i2c_client *client,
			      struct v4l2_mbus_framefmt *mf, bool capture)
{
	/* add delay for rk312x */
	msleep(300);
	return 0;
}
static int sensor_softrest_usr_cb(struct i2c_client *client,
				  struct rk_sensor_reg *series)
{

	return 0;
}
static int sensor_check_id_usr_cb(struct i2c_client *client,
				  struct rk_sensor_reg *series)
{
	return 0;
}
static int sensor_try_fmt_cb_th(struct i2c_client *client,
				struct v4l2_mbus_framefmt *mf)
{
	return 0;
}
static int sensor_suspend(struct soc_camera_device *icd, pm_message_t pm_msg)
{
	//struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));

	if (pm_msg.event == PM_EVENT_SUSPEND) {
		SENSOR_DG("Suspend");

	} else {
		SENSOR_TR("pm_msg.event(0x%x) != PM_EVENT_SUSPEND\n",
			  pm_msg.event);
		return -EINVAL;
	}
	return 0;
}

static int sensor_resume(struct soc_camera_device *icd)
{

	SENSOR_DG("Resume");

	return 0;

}
static int sensor_mirror_cb(struct i2c_client *client, int mirror)
{
	char val;
	int err = 0;

	SENSOR_DG("mirror: %d", mirror);
	if (mirror) {
		sensor_write(client, 0xfe, 0);
		err = sensor_read(client, 0x17, &val);
		val -= 4;
		if (err == 0) {
			if ((val & 0x1) == 0) {
				err =
				    sensor_write(client, 0x17,
						 ((val | 0x1) + 4));
			} else
				err =
				    sensor_write(client, 0x17,
						 ((val & 0xfe) + 4));
		}
	} else {
		//do nothing
	}
	return err;
}

/*
* the function is v4l2 control V4L2_CID_HFLIP callback
*/
static int sensor_v4l2ctrl_mirror_cb(struct soc_camera_device *icd,
				     struct sensor_v4l2ctrl_info_s *ctrl_info,
				     struct v4l2_ext_control *ext_ctrl)
{
	struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));

	if (sensor_mirror_cb(client, ext_ctrl->value) != 0)
		SENSOR_TR("sensor_mirror failed, value:0x%x", ext_ctrl->value);

	SENSOR_DG("sensor_mirror success, value:0x%x", ext_ctrl->value);
	return 0;
}

static int sensor_flip_cb(struct i2c_client *client, int flip)
{
	char val;
	int err = 0;

	SENSOR_DG("flip: %d", flip);
	if (flip) {

		sensor_write(client, 0xfe, 0);
		err = sensor_read(client, 0x17, &val);
		val -= 4;
		if (err == 0) {
			if ((val & 0x2) == 0) {
				err =
				    sensor_write(client, 0x17,
						 ((val | 0x2) + 4));
			} else {
				err =
				    sensor_write(client, 0x17,
						 ((val & 0xfc) + 4));
			}
		}
	} else {
		//do nothing
	}

	return err;
}

/*
* the function is v4l2 control V4L2_CID_VFLIP callback
*/
static int sensor_v4l2ctrl_flip_cb(struct soc_camera_device *icd,
				   struct sensor_v4l2ctrl_info_s *ctrl_info,
				   struct v4l2_ext_control *ext_ctrl)
{
	struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));

	if (sensor_flip_cb(client, ext_ctrl->value) != 0)
		SENSOR_TR("sensor_flip failed, value:0x%x", ext_ctrl->value);

	SENSOR_DG("sensor_flip success, value:0x%x", ext_ctrl->value);
	return 0;
}

/*
* the functions are focus callbacks
*/
static int sensor_focus_init_usr_cb(struct i2c_client *client)
{
	return 0;
}

static int sensor_focus_af_single_usr_cb(struct i2c_client *client)
{
	return 0;
}

static int sensor_focus_af_near_usr_cb(struct i2c_client *client)
{
	return 0;
}

static int sensor_focus_af_far_usr_cb(struct i2c_client *client)
{
	return 0;
}

static int sensor_focus_af_specialpos_usr_cb(struct i2c_client *client, int pos)
{
	return 0;
}

static int sensor_focus_af_const_usr_cb(struct i2c_client *client)
{
	return 0;
}
static int sensor_focus_af_const_pause_usr_cb(struct i2c_client *client)
{
	return 0;
}
static int sensor_focus_af_close_usr_cb(struct i2c_client *client)
{
	return 0;
}

static int sensor_focus_af_zoneupdate_usr_cb(struct i2c_client *client,
					     int *zone_tm_pos)
{
	return 0;
}

/*
face defect call back
*/
static int sensor_face_detect_usr_cb(struct i2c_client *client, int on)
{
	return 0;
}

/*
*	The function can been run in sensor_init_parametres which run in sensor_probe, so user can do some
* initialization in the function.
*/
static void sensor_init_parameters_user(struct specific_sensor *spsensor,
					struct soc_camera_device *icd)
{
	return;
}

/*
* :::::WARNING:::::
* It is not allowed to modify the following code
*/

sensor_init_parameters_default_code();

sensor_v4l2_struct_initialization();

sensor_probe_default_code();

sensor_remove_default_code();

sensor_driver_default_module_code();
