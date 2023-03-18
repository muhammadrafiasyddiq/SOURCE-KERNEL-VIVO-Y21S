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

#ifndef __DISP_DTS_GPIO_H__
#define __DISP_DTS_GPIO_H__

/*
 * This module helps you to set GPIO pin according to linux device tree (DTS).
 * To use this module, you MUST init this module once before any operation.
 */

#include <linux/platform_device.h>	/* struct platform_device */

/* DTS state */
enum DTS_GPIO_STATE {
#ifdef CONFIG_LCM_PANEL_TYPE_TFT
	DTS_GPIO_STATE_TFT_RST_OUT0 = 0,
	DTS_GPIO_STATE_TFT_RST_OUT1,
	DTS_GPIO_STATE_TFT_ENP_OUT0,
	DTS_GPIO_STATE_TFT_ENP_OUT1,
	DTS_GPIO_STATE_TFT_ENN_OUT0,
	DTS_GPIO_STATE_TFT_ENN_OUT1,
	DTS_GPIO_STATE_TFT_BKG_OUT0,
	DTS_GPIO_STATE_TFT_BKG_OUT1,
	DTS_GPIO_STATE_TFT_TPRST_OUT0,
	DTS_GPIO_STATE_TFT_TPRST_OUT1,
	DTS_GPIO_STATE_MAX,	
#else
	DTS_GPIO_STATE_LCM_RST_OUT0_GPIO = 0,
	DTS_GPIO_STATE_LCM_RST_OUT1_GPIO,
	DTS_GPIO_STATE_LCM_VCI_EN0_GPIO,
	DTS_GPIO_STATE_LCM_VCI_EN1_GPIO,
	DTS_GPIO_STATE_MAX,		/* for array size */
#endif
};

/**
 * this function MUST be called in mtkfb_probe.
 *  @param *pdev    - reference of struct platform_device which contains pinctrl
 *                    state information of GPIO
 *  @return         - 0 for OK, otherwise returns PTR_ERR(pdev).
 */
long disp_dts_gpio_init(struct platform_device *pdev);

/**
 * set gpio according sepcified DTS state.
 *  @notice         - to call this function, you MUST init this module first.
 *                    If not, we will trigger BUG_ON(0).
 *  @param s        - state which describes GPIO statement.
 *  @return         - 0 for OK, otherwise returns PTR_ERR(pdev).
 */
long disp_dts_gpio_select_state(enum DTS_GPIO_STATE s);

/* repo of initialization */
#ifdef CONFIG_MTK_LEGACY
#  define disp_dts_gpio_init_repo(x)	(0)
#else
#  define disp_dts_gpio_init_repo(x)	(disp_dts_gpio_init(x))
#endif

#endif /* __DISP_DTS_GPIO_H__ */
