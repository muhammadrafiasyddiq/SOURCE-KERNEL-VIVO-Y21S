/*
 * Copyright (C) 2017 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include "gpio.h"

struct GPIO_PINCTRL gpio_pinctrl_list_cam[
			GPIO_CTRL_STATE_MAX_NUM_CAM] = {
	/* Main */
	{"pnd1"},
	{"pnd0"},
	{"rst1"},
	{"rst0"},
	{"ldo_vcama_1"},
	{"ldo_vcama_0"},
	{"ldo_vcamois_1"},
	{"ldo_vcamois_0"},
	{"ldo_vmois_1"},
	{"ldo_vmois_0"},
#ifdef CONFIG_REGULATOR_FAN53870
	{"ldo_plugic_en_1"},
	{"ldo_plugic_en_0"},
#endif
	{"ldo_vcamd_1"},
	{"ldo_vcamd_0"},
	{"ldo_vcamio_1"},
	{"ldo_vcamio_0"},
};

#ifdef MIPI_SWITCH
struct GPIO_PINCTRL gpio_pinctrl_list_switch[
			GPIO_CTRL_STATE_MAX_NUM_SWITCH] = {
	{"cam_mipi_switch_en_1"},
	{"cam_mipi_switch_en_0"},
	{"cam_mipi_switch_sel_1"},
	{"cam_mipi_switch_sel_0"}
};
#endif

static struct GPIO gpio_instance;

static enum IMGSENSOR_RETURN gpio_init(
	void *pinstance,
	struct IMGSENSOR_HW_DEVICE_COMMON *pcommon)
{
	int    i, j;
	struct GPIO            *pgpio            = (struct GPIO *)pinstance;
	enum   IMGSENSOR_RETURN ret              = IMGSENSOR_RETURN_SUCCESS;
	char str_pinctrl_name[LENGTH_FOR_SNPRINTF];
	char *lookup_names = NULL;

	pgpio->pgpio_mutex = &pcommon->pinctrl_mutex;

	pgpio->ppinctrl = devm_pinctrl_get(&pcommon->pplatform_device->dev);
	if (IS_ERR(pgpio->ppinctrl)) {
		PK_PR_ERR("%s : Cannot find camera pinctrl!", __func__);
		return IMGSENSOR_RETURN_ERROR;
	}

	for (j = IMGSENSOR_SENSOR_IDX_MIN_NUM;
		j < IMGSENSOR_SENSOR_IDX_MAX_NUM;
		j++) {
		for (i = 0 ; i < GPIO_CTRL_STATE_MAX_NUM_CAM; i++) {
			lookup_names =
			gpio_pinctrl_list_cam[i].ppinctrl_lookup_names;

			if (lookup_names) {
				snprintf(str_pinctrl_name,
				sizeof(str_pinctrl_name),
				"cam%d_%s",
				j,
				lookup_names);
				pgpio->ppinctrl_state_cam[j][i] =
					pinctrl_lookup_state(
						pgpio->ppinctrl,
						str_pinctrl_name);
			}

			if (pgpio->ppinctrl_state_cam[j][i] == NULL ||
				IS_ERR(pgpio->ppinctrl_state_cam[j][i])) {
				pr_info(
					"ERROR: %s : pinctrl err, %s\n",
					__func__,
					str_pinctrl_name);
				ret = IMGSENSOR_RETURN_ERROR;
			}
		}
	}
#ifdef MIPI_SWITCH
	for (i = 0; i < GPIO_CTRL_STATE_MAX_NUM_SWITCH; i++) {
		if (gpio_pinctrl_list_switch[i].ppinctrl_lookup_names) {
			pgpio->ppinctrl_state_switch[i] =
				pinctrl_lookup_state(
					pgpio->ppinctrl,
			gpio_pinctrl_list_switch[i].ppinctrl_lookup_names);
		}

		if (pgpio->ppinctrl_state_switch[i] == NULL ||
			IS_ERR(pgpio->ppinctrl_state_switch[i])) {
			PK_PR_ERR("%s : pinctrl err, %s\n", __func__,
			gpio_pinctrl_list_switch[i].ppinctrl_lookup_names);
			ret = IMGSENSOR_RETURN_ERROR;
		}
	}
#endif

#ifdef CONFIG_MTK_CAM_PD2083F_EX // for gpio share use by changcheng 2021.3.7
	for (i = 0;i < GPIO_CTRL_STATE_MAX_NUM_CAM;i++) {
		pgpio->enable_cnt[i] = 0;
		PK_INFO("%s : gpio_init enable_cnt=%d\n",__FUNCTION__,pgpio->enable_cnt[i]);
	}
#endif
	return ret;
}

static enum IMGSENSOR_RETURN gpio_release(void *pinstance)
{
#ifdef CONFIG_MTK_CAM_PD2083F_EX // for gpio share use by changcheng 2021.3.7
	int i=0;
	struct GPIO            *pgpio            = (struct GPIO *)pinstance;
	for (i = 0;i < GPIO_CTRL_STATE_MAX_NUM_CAM;i++) {
		pgpio->enable_cnt[i] = 0;
		pr_info("%s : gpio_release enable_cnt=%d\n",__FUNCTION__,pgpio->enable_cnt[i]);
	}
#endif
	return IMGSENSOR_RETURN_SUCCESS;
}

static enum IMGSENSOR_RETURN gpio_set(
	void *pinstance,
	enum IMGSENSOR_SENSOR_IDX   sensor_idx,
	enum IMGSENSOR_HW_PIN       pin,
	enum IMGSENSOR_HW_PIN_STATE pin_state)
{
	struct pinctrl_state  *ppinctrl_state;
	struct GPIO           *pgpio = (struct GPIO *)pinstance;
	enum   GPIO_STATE      gpio_state;

	/* PK_DBG("%s :debug pinctrl ENABLE, PinIdx %d, Val %d\n",
	 *	__func__, pin, pin_state);
	 */

	if (pin < IMGSENSOR_HW_PIN_PDN ||
#ifdef MIPI_SWITCH
	    pin > IMGSENSOR_HW_PIN_MIPI_SWITCH_SEL ||
#else
		pin > IMGSENSOR_HW_PIN_DOVDD ||
#endif
		pin_state < IMGSENSOR_HW_PIN_STATE_LEVEL_0 ||
		pin_state > IMGSENSOR_HW_PIN_STATE_LEVEL_HIGH ||
		sensor_idx < 0)
		return IMGSENSOR_RETURN_ERROR;

	gpio_state = (pin_state > IMGSENSOR_HW_PIN_STATE_LEVEL_0)
		? GPIO_STATE_H : GPIO_STATE_L;

#ifdef MIPI_SWITCH
	if (pin == IMGSENSOR_HW_PIN_MIPI_SWITCH_EN)
		ppinctrl_state = pgpio->ppinctrl_state_switch[
			GPIO_CTRL_STATE_MIPI_SWITCH_EN_H + gpio_state];
	else if (pin == IMGSENSOR_HW_PIN_MIPI_SWITCH_SEL)
		ppinctrl_state = pgpio->ppinctrl_state_switch[
			GPIO_CTRL_STATE_MIPI_SWITCH_SEL_H + gpio_state];
	else
#endif
	{
		ppinctrl_state =
			pgpio->ppinctrl_state_cam[sensor_idx][
			((pin - IMGSENSOR_HW_PIN_PDN) << 1) + gpio_state];
	}


	mutex_lock(pgpio->pgpio_mutex);

	if (ppinctrl_state != NULL && !IS_ERR(ppinctrl_state)) {
#ifdef CONFIG_MTK_CAM_PD2083F_EX // for gpio share use by changcheng 2021.3.7
		pr_info("%s : sensor_idx =%d GPIO_STATE  pin =%d gpio_state =%d enable_cnt=%d\n",__FUNCTION__,sensor_idx,pin,gpio_state ,pgpio->enable_cnt[((pin - IMGSENSOR_HW_PIN_PDN) << 1) + GPIO_STATE_H]);
		if ((gpio_state ==GPIO_STATE_H)&& ((pin == IMGSENSOR_HW_PIN_PLUGIC_EN) || (pin == IMGSENSOR_HW_PIN_OISVDD) ) ){
			if (pgpio->enable_cnt[((pin - IMGSENSOR_HW_PIN_PDN) << 1) + GPIO_STATE_H] == 0) {
				pinctrl_select_state(pgpio->ppinctrl, ppinctrl_state);
			}
			pgpio->enable_cnt[((pin - IMGSENSOR_HW_PIN_PDN) << 1) + GPIO_STATE_H] ++;
			pr_info("%s :GPIO_STATE_H pin =%d  enable_cnt++=%d\n",__FUNCTION__,pin,pgpio->enable_cnt[((pin - IMGSENSOR_HW_PIN_PDN) << 1) + GPIO_STATE_H]);

		} else if ((gpio_state ==GPIO_STATE_L)&& ((pin == IMGSENSOR_HW_PIN_PLUGIC_EN) || (pin == IMGSENSOR_HW_PIN_OISVDD) ) ) {
			if (pgpio->enable_cnt[((pin - IMGSENSOR_HW_PIN_PDN) << 1) + GPIO_STATE_H] >= 0&& pgpio->enable_cnt[((pin - IMGSENSOR_HW_PIN_PDN) << 1) + GPIO_STATE_H] <= 1) {
				pinctrl_select_state(pgpio->ppinctrl, ppinctrl_state);
				pgpio->enable_cnt[((pin - IMGSENSOR_HW_PIN_PDN) << 1) + GPIO_STATE_H] = 0;
				pr_info("%s :if 0<=cnt <=1 pin=%d GPIO_STATE_L  enable_cnt=%d\n",__FUNCTION__,pin,pgpio->enable_cnt[((pin - IMGSENSOR_HW_PIN_PDN) << 1) + GPIO_STATE_H]);
			} else if (pgpio->enable_cnt[((pin - IMGSENSOR_HW_PIN_PDN) << 1) + GPIO_STATE_H] > 1) {
				pgpio->enable_cnt[((pin - IMGSENSOR_HW_PIN_PDN) << 1) + GPIO_STATE_H] --;
				pr_info("%s :else cnt >1 pin=%d GPIO_STATE_L  enable_cnt=%d\n",__FUNCTION__,pin,pgpio->enable_cnt[((pin - IMGSENSOR_HW_PIN_PDN) << 1) + GPIO_STATE_H]);
			}
		} else {
				pinctrl_select_state(pgpio->ppinctrl, ppinctrl_state);
		}
#else
		pinctrl_select_state(pgpio->ppinctrl, ppinctrl_state);
#endif
	}
	else
		PK_PR_ERR("%s : pinctrl err, PinIdx %d, Val %d\n",
			__func__, pin, pin_state);

	mutex_unlock(pgpio->pgpio_mutex);

	return IMGSENSOR_RETURN_SUCCESS;
}

static struct IMGSENSOR_HW_DEVICE device = {
	.id        = IMGSENSOR_HW_ID_GPIO,
	.pinstance = (void *)&gpio_instance,
	.init      = gpio_init,
	.set       = gpio_set,
	.release   = gpio_release
};

enum IMGSENSOR_RETURN imgsensor_hw_gpio_open(
	struct IMGSENSOR_HW_DEVICE **pdevice)
{
	*pdevice = &device;
	return IMGSENSOR_RETURN_SUCCESS;
}

