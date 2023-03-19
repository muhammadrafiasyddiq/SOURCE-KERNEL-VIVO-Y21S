/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/string.h>
#include <linux/kernel.h>
#include "lcm_drv.h"
#include "mtkfb.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/leds.h>

static struct LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)	(lcm_util.set_reset_pin((v)))
#define MDELAY(n)              	(lcm_util.mdelay(n))
#define UDELAY(n)		(lcm_util.udelay(n))

#define dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V22(cmdq, cmd, count, ppara, force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) \
		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd) lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums) \
		lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd) \
	  lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size) \
		lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#define tp_reset_setting(cmd) \
		lcm_util.tp_reset_setting(cmd)
#define lcm_reset_setting(cmd) \
			lcm_util.lcm_reset_setting(cmd)
#define lcm_enp_setting(cmd) \
			lcm_util.lcm_enp_setting(cmd)
#define lcm_enn_setting(cmd) \
			lcm_util.lcm_enn_setting(cmd)
#define lcm_bkg_setting(cmd) \
			lcm_util.lcm_bkg_setting(cmd)

extern void lcm_bias_set_avdd_n_avee(int value);
extern unsigned int phone_shutdown_state;
/* static unsigned char lcd_id_pins_value = 0xFF; */
static const unsigned char LCD_MODULE_ID = 0x01;
#define LCM_DSI_CMD_MODE									0
#define FRAME_WIDTH										(720)
#define FRAME_HEIGHT									(1600)

/* physical size in um */
#define LCM_PHYSICAL_WIDTH		(67932)
#define LCM_PHYSICAL_HEIGHT		(150960)
#define LCM_DENSITY											(480)
#define VFP (140)
#define VSA (4)
#define VBP (26)
#define HFP (166)
#define HSA (4)
#define HBP (68)
#define REGFLAG_DELAY		0xFFFC
#define REGFLAG_UDELAY	0xFFFB
#define REGFLAG_END_OF_TABLE	0xFFFD
#define REGFLAG_RESET_LOW	0xFFFE
#define REGFLAG_RESET_HIGH	0xFFFF

#define WLED_CABC_ENABLE_LEVEL			403

/*static LCM_DSI_MODE_SWITCH_CMD lcm_switch_mode_cmd;*/

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif


struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};



extern unsigned int panel_reset_state;
extern unsigned int panel_off_state;
extern unsigned int lcm_software_id;
extern unsigned int bl_brightness_hal;
static unsigned int dimming_enable = 1;
extern unsigned int bl_lvl;
extern unsigned int cabc_state_for_upper_set;
extern unsigned int lcm_cabc_status;

static struct LCM_setting_table lcm_suspend_setting[] = {
{0x26, 1,{0x08}},
{0x28, 0,{0x00}},
{REGFLAG_DELAY, 20,{}},
{0x10, 0,{0x00}},
{REGFLAG_DELAY, 120,{}},
};

static struct LCM_setting_table init_setting[] = {
{0xF0, 2,{0x5A,0x59}},
{0xF1, 2,{0xA5,0xA6}},
{0xB0,29,{0x86,0x85,0x84,0x83,0x8B,0x8A,0x02,0x01,0x22,0x22,0x22,0x22,0x00,0x01,0x01,0x86,0x01,0x01,0x03,0x05,0x04,0x03,0x02,0x01,0x02,0x03,0x04,0x00,0x00}},
{0xB1,32,{0x12,0x42,0x89,0x85,0x00,0x00,0x00,0x7D,0x00,0x00,0x04,0x08,0x54,0x00,0x00,0x00,0x44,0x40,0x02,0x01,0x40,0x02,0x01,0x40,0x02,0x01,0x40,0x02,0x01,0x00,0x00,0x00}},
{0xB2,17,{0x54,0xC4,0x82,0x05,0x40,0x02,0x01,0x40,0x02,0x01,0x05,0x05,0x54,0x0C,0x0C,0x0D,0x0B}},
{0xB3,31,{0x02,0x0B,0x08,0x0B,0x08,0x26,0x26,0x91,0xA2,0x33,0x44,0x00,0x26,0x00,0x18,0x01,0x02,0x08,0x20,0x30,0x08,0x09,0x44,0x20,0x40,0x20,0x40,0x08,0x09,0x22,0x33}},
{0xB4,28,{0x00,0xDC,0xDD,0x08,0xC8,0x00,0x00,0x04,0x06,0x12,0x10,0x0E,0x0C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFC,0x00,0x00,0x00}},
{0xB5,28,{0x00,0xDC,0xDD,0x09,0xC9,0x00,0x00,0x05,0x07,0x13,0x11,0x0F,0x0D,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFC,0x00,0x00,0x00}},
{0xB8,24,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
{0xBB,13,{0x01,0x05,0x09,0x11,0x0D,0x19,0x1D,0x55,0x25,0x69,0x00,0x21,0x25}},
{0xBC,14,{0x00,0x00,0x00,0x00,0x02,0x20,0xFF,0x00,0x03,0x13,0x01,0x73,0x33,0x00}},
{0xBD,10,{0xE9,0x02,0x4F,0xCF,0x72,0xA4,0x08,0x44,0xAE,0x15}},
{0xBE,10,{0x73,0x73,0x50,0x32,0x0C,0x77,0x43,0x07,0x0E,0x0E}},
{0xBF, 8,{0x07,0x25,0x07,0x25,0x7F,0x00,0x11,0x04}},
{0xC0, 9,{0x10,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0xFF,0x00}},
{0xC1,19,{0xC0,0x1A,0x20,0x8C,0x04,0x44,0xA6,0x04,0x2A,0x40,0x36,0x00,0x07,0xC0,0x10,0xFF,0x90,0x01,0xC0}},
{0xC2, 1,{0x00}},
{0xC3, 9,{0x06,0x00,0xFF,0x00,0xFF,0x00,0x00,0x81,0x01}},
{0xC4,11,{0x84,0x01,0x2B,0x41,0x00,0x3C,0x00,0x03,0x03,0x2E,0x00}},
{0xC5,11,{0x03,0x1C,0xB8,0xB8,0x30,0x10,0x64,0x44,0x09,0x0F,0x14}},
{0xC6,10,{0x8E,0x19,0x22,0x29,0x29,0x33,0x64,0x24,0x08,0x04}},
{0xC9, 6,{0x43,0x00,0x1F,0xFF,0x3F,0x03}},
{0xCB,17,{0x05,0x40,0x55,0x40,0x04,0x40,0x35,0x43,0x43,0x50,0x46,0x40,0x40,0x43,0x43,0x64,0x46}},
{0xD0, 8,{0x80,0x0D,0xFF,0x0F,0x63,0x0B,0x08,0x0C}},
{0xD2,15,{0x42,0x0C,0x30,0x01,0x80,0x26,0x04,0x00,0x00,0xC3,0x00,0x00,0x00,0x00,0x20}},
{0xD7, 1,{0xDE}},
{0xE1,23,{0xEF,0xFE,0xEE,0xFE,0xFE,0xEE,0xF0,0x67,0x33,0xFF,0xE1,0xE1,0xE1,0x5D,0xE1,0x33,0xE1,0xE1,0xE1,0xFF,0x00,0x08,0x37}},
{0xE0,26,{0x30,0x00,0x00,0x18,0x11,0x1F,0x22,0x62,0xDF,0xA0,0x04,0xCC,0x01,0xFF,0xFA,0xFF,0xF8,0xF8,0xF8,0xFA,0xF8,0xFA,0xFA,0xFA,0xFA,0xFF}},
{0xE5,22,{0x03,0x2B,0xF0,0x72,0xFB,0x47,0xFF,0xC0,0x40,0xF0,0x4E,0xA2,0xF8,0x00,0x00,0x3F,0xAA,0xFC,0x5A,0xEC,0x00,0x00}},
{0xE6,26,{0xFA,0xF8,0xF6,0xF4,0xF2,0xEE,0xEC,0xE4,0xDC,0xCC,0xBD,0x9D,0x7D,0x7C,0x5D,0x3D,0x2E,0x1E,0x16,0x0E,0x0A,0x06,0x04,0x02,0x00,0x00}},
{0xE7,26,{0xFD,0xFC,0xFB,0xF8,0xF5,0xF1,0xEE,0xE6,0xDE,0xCE,0xBD,0x9D,0x7D,0x7C,0x5D,0x3D,0x2D,0x1D,0x16,0x0F,0x0B,0x07,0x05,0x03,0x01,0x00}},
{0xE8,26,{0xFF,0xFD,0xFB,0xF8,0xF6,0xF2,0xEE,0xE6,0xDE,0xCE,0xBE,0x9F,0x7F,0x7E,0x5E,0x3E,0x2E,0x1E,0x16,0x0F,0x0B,0x07,0x05,0x03,0x01,0x00}},
{0xEA, 3,{0x46,0x50,0x00}},
{0xFE, 4,{0xFF,0xFF,0xFF,0x3D}},
{0xF1, 2,{0x5A,0x59}},
{0xF0, 2,{0xA5,0xA6}},
{0x51, 2,{0x00,0x00}},
{0x53, 1,{0x24}},
{0x35, 1,{0x00}},
{0x11, 0,{0x00}},
{REGFLAG_DELAY, 120,{}},
{0x29, 0,{0x00}},
{REGFLAG_DELAY, 35,{}},
{0x26, 1,{0x01}},
{REGFLAG_END_OF_TABLE,0x00,{}} 


};


static struct LCM_setting_table bl_level_12bit[] = {
	{0x51, 2, {0x0F, 0xFF} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};

static struct LCM_setting_table lcm_cmd_backlight_dimming_enable[] = {
	{0x53, 1, {0x2C} },
};

static struct LCM_setting_table lcm_cmd_backlight_dimming_disable[] = {
	{0x53, 1, {0x24} },
};

static struct LCM_setting_table lcm_cmd_cabc_level1[] = {
	{0x53, 1, {0x2c} },
	{0x55, 1, {0x01} },
};

static struct LCM_setting_table lcm_cmd_cabc_level2[] = {
	{0x53, 1, {0x2c} },
	{0x55, 1, {0x02} },
};
static struct LCM_setting_table lcm_cmd_cabc_level3[] = {
	{0x53, 1, {0x2c} },
	{0x55, 1, {0x03} },
};

static struct LCM_setting_table lcm_cmd_cabc_off[] = {
	{0x53, 1, {0x2c} },
	{0x55, 1, {0x00} },
};

static void push_table(void *cmdq, struct LCM_setting_table *table,
	unsigned int count, unsigned char force_update)
{
	unsigned int i;
	unsigned cmd;

	for (i = 0; i < count; i++) {
		cmd = table[i].cmd;

		switch (cmd) {

		case REGFLAG_DELAY:
			if (table[i].count <= 10)
				MDELAY(table[i].count);
			else
				MDELAY(table[i].count);
			break;

		case REGFLAG_UDELAY:
			UDELAY(table[i].count);
			break;

		case REGFLAG_END_OF_TABLE:
			break;

		default:
			dsi_set_cmdq_V22(cmdq, cmd, table[i].count, table[i].para_list, force_update);
		}
	}
}


static void lcm_set_util_funcs(const struct LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(struct LCM_UTIL_FUNCS));
}


static void lcm_get_params(struct LCM_PARAMS *params)
{
	memset(params, 0, sizeof(struct LCM_PARAMS));

	params->type = LCM_TYPE_DSI;

	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;
	params->physical_width = LCM_PHYSICAL_WIDTH/1000;
	params->physical_height = LCM_PHYSICAL_HEIGHT/1000;
	params->physical_width_um = LCM_PHYSICAL_WIDTH;
	params->physical_height_um = LCM_PHYSICAL_HEIGHT;
	params->density            = LCM_DENSITY;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode = CMD_MODE;
	params->dsi.switch_mode = SYNC_PULSE_VDO_MODE;
	lcm_dsi_mode = CMD_MODE;
#else
	params->dsi.mode = SYNC_PULSE_VDO_MODE;
	params->dsi.switch_mode = CMD_MODE;
	lcm_dsi_mode = SYNC_PULSE_VDO_MODE;
#endif
	LCM_INFO("lcm_get_params lcm_dsi_mode %d\n", lcm_dsi_mode);
	params->dsi.switch_mode_enable = 0;

	/* DSI */
	/* Command mode setting */
	params->dsi.LANE_NUM = LCM_THREE_LANE;
	/* The following defined the fomat for data coming from LCD engine. */
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

	/* Highly depends on LCD driver capability. */
	params->dsi.packet_size = 256;
	/* video mode timing */

	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.vertical_sync_active = VSA;
	params->dsi.vertical_backporch = VBP;
	params->dsi.vertical_frontporch = VFP;
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = HSA; //14
	params->dsi.horizontal_backporch = HBP;   //24
	params->dsi.horizontal_frontporch = HFP;  //36
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	params->dsi.HS_PRPR = 10;
	params->dsi.HS_TRAIL = 9;
	params->dsi.ssc_disable  = 1;
	params->dsi.data_rate = 860;
	params->dsi.PLL_CLOCK = 430; //300
	params->dsi.clk_lp_per_line_enable = 0;
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;
	params->dsi.esd_check_reload_firmware = 1;
	params->dsi.lcm_esd_check_table[0].cmd = 0x0a;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9c;

	params->dsi.lcm_esd_check_table[1].cmd = 0x0d;
	params->dsi.lcm_esd_check_table[1].count = 1;
	params->dsi.lcm_esd_check_table[1].para_list[0] = 0x00;
#ifdef CONFIG_MTK_ROUND_CORNER_SUPPORT
	params->round_corner_en = 1;
	params->corner_pattern_width = FRAME_WIDTH;
	params->corner_pattern_height = 16;
#endif
}

static void lcm_init_power(void)
{
	LCM_INFO("panel_reset_state = %d, lcm_software_id = 0x%x\n", panel_reset_state, lcm_software_id);
	if (panel_reset_state == 0) {
		lcm_enp_setting(1);
		MDELAY(12);
		lcm_enn_setting(1);
		MDELAY(10);
		lcm_bias_set_avdd_n_avee(60);
		panel_reset_state = 1;
	}
}

static void lcm_suspend_power(void)
{
	if (phone_shutdown_state) {
		LCM_INFO("[LCM]lcm_suspend_power reset keep low level\n");
		MDELAY(36);
		lcm_reset_setting(0);
	}
	MDELAY(10);
	lcm_enn_setting(0);
	MDELAY(5);
	lcm_enp_setting(0);
	MDELAY(8);
	LCM_INFO("[LCM]lcm_suspend_power icn9911c- vddi keep high level\n");
	panel_reset_state = 0;
	LCM_INFO("\n");
}

static void lcm_resume_power(void)
{
	lcm_init_power();
}

static void lcm_init(void)
{
	panel_off_state = 0;
	lcm_reset_setting(1);
	MDELAY(3);
	lcm_reset_setting(0);
	MDELAY(3);
	lcm_reset_setting(1);
	MDELAY(3);
	lcm_reset_setting(0);
	MDELAY(3);
	lcm_reset_setting(1);
	MDELAY(15);
	push_table(NULL, init_setting, sizeof(init_setting) / sizeof(struct LCM_setting_table), 1);
	LCM_INFO("icn9911c ----lcm_init----lcm mode = cmd mode :%d----\n", lcm_dsi_mode);
}

static void lcm_suspend(void)
{
	LCM_INFO("icn9911c lcm_software_id = 0x%x\n", lcm_software_id);
	push_table(NULL, lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
	panel_off_state = 1;
}

static void lcm_resume(void)
{
	lcm_bkg_setting(1);
	MDELAY(5);
	lcm_init();
}


static void lcm_update(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0 >> 8) & 0xFF);
	unsigned char x0_LSB = (x0 & 0xFF);
	unsigned char x1_MSB = ((x1 >> 8) & 0xFF);
	unsigned char x1_LSB = (x1 & 0xFF);
	unsigned char y0_MSB = ((y0 >> 8) & 0xFF);
	unsigned char y0_LSB = (y0 & 0xFF);
	unsigned char y1_MSB = ((y1 >> 8) & 0xFF);
	unsigned char y1_LSB = (y1 & 0xFF);

	unsigned int data_array[16];

	data_array[0] = 0x00053902;
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00053902;
	data_array[1] = (y1_MSB << 24) | (y0_LSB << 16) | (y0_MSB << 8) | 0x2b;
	data_array[2] = (y1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);
}

/******************************************lcmcabc start**************************************************/
static void lcm_vivo_set_cabc(void *handle, unsigned char levelsetting)
{
	if (bl_brightness_hal < WLED_CABC_ENABLE_LEVEL)
		levelsetting = 0;
	if ((levelsetting == 0) && (sizeof(lcm_cmd_cabc_off) > 0))
		push_table(handle, lcm_cmd_cabc_off, sizeof(lcm_cmd_cabc_off) / sizeof(struct LCM_setting_table), 1);
	else if ((levelsetting == 1) && (sizeof(lcm_cmd_cabc_level1) > 0))
		push_table(handle, lcm_cmd_cabc_level1, sizeof(lcm_cmd_cabc_level1) / sizeof(struct LCM_setting_table), 1);
	else if ((levelsetting == 2) && (sizeof(lcm_cmd_cabc_level2) > 0))
		push_table(handle, lcm_cmd_cabc_level2, sizeof(lcm_cmd_cabc_level2) / sizeof(struct LCM_setting_table), 1);
	else if ((levelsetting == 3) && (sizeof(lcm_cmd_cabc_level3) > 0))
		push_table(handle, lcm_cmd_cabc_level3, sizeof(lcm_cmd_cabc_level3) / sizeof(struct LCM_setting_table), 1);
	lcm_cabc_status = levelsetting;
	LCM_INFO("vincent=----lcmcabc----- level:%d\n",  levelsetting);
}

static void lcm_setbacklight_cmdq(void *handle, unsigned int level)
{
	static unsigned int last_brightness_level;
	if (level == LED_FULL)
			level = 4095;
	else
		level = level * 2;
	if ((level != 0) && (dimming_enable == 0) && (last_brightness_level != 0)) {
		push_table(handle, lcm_cmd_backlight_dimming_enable, sizeof(lcm_cmd_backlight_dimming_enable) / sizeof(struct LCM_setting_table), 1);
		dimming_enable = 1;
		MDELAY(20);	//vivo  add by hehuan for dimming enable
		LCM_INFO("LastBacklight = %d, dimming_enable=%d, high_bit=0x%x, low_bit=0x%x\n", bl_brightness_hal, dimming_enable, bl_level_12bit[0].para_list[0], bl_level_12bit[0].para_list[1]);
	}
	if (level == 0) {
		push_table(handle, lcm_cmd_backlight_dimming_disable, sizeof(lcm_cmd_backlight_dimming_disable) / sizeof(struct LCM_setting_table), 1);
		dimming_enable = 0;
		LCM_INFO("LastBacklight = %d, dimming_enable=%d, high_bit=0x%x, low_bit=0x%x\n", bl_brightness_hal, dimming_enable, bl_level_12bit[0].para_list[0], bl_level_12bit[0].para_list[1]);
		MDELAY(40);
	}
	bl_lvl = level;
	last_brightness_level = level;
	bl_level_12bit[0].para_list[0] = (unsigned char)((level>>8)&0xFF);
	bl_level_12bit[0].para_list[1] = (unsigned char)(level&0xFFFF);
	LCM_INFO("LastBacklight = %d, level=%d, high_bit=0x%x, low_bit=0x%x\n", bl_brightness_hal, level, bl_level_12bit[0].para_list[0], bl_level_12bit[0].para_list[1]);
	push_table(handle, bl_level_12bit, sizeof(bl_level_12bit) / sizeof(struct LCM_setting_table), 1);
}
static void lcm_reset_for_touch(void)
{
	lcm_reset_setting(0);
	MDELAY(5);
	lcm_reset_setting(1);
	MDELAY(3);
	lcm_reset_setting(0);
	MDELAY(5);
	lcm_reset_setting(1);
	MDELAY(12);
}

static void lcm_vivo_MipiCmd_HS(void *handle, unsigned char cmdtype, unsigned char levelsetting)
{
	/*Set CABC when backlight is greater than WLED_CABC_ENABLE_LEVEL*/
	if (cmdtype == 0x81)
		lcm_vivo_set_cabc(handle, levelsetting);
}
static unsigned int lcm_get_id(unsigned char *lcm_flag)
{
	if (lcm_software_id == 0x01)
		return 0x11;
	else
		return 0xFF;
}

struct LCM_DRIVER pd2138_icn9911c_fhdplus_dsi_vdo_tm_lcm_drv = {
	.name = "pd2138_icn9911c_fhdplus_dsi_vdo_tm",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	.init_power = lcm_init_power,
	.resume_power = lcm_resume_power,
	.suspend_power = lcm_suspend_power,
	.update = lcm_update,
	//.switch_mode = lcm_switch_mode,
	//.validate_roi = lcm_validate_roi,
	.get_id	    = lcm_get_id,
	.lcm_MipiCmd_HS  = lcm_vivo_MipiCmd_HS,
	.lcm_reset = lcm_reset_for_touch,
	.set_backlight_cmdq = lcm_setbacklight_cmdq,
};
