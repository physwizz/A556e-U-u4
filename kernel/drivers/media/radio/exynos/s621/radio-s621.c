// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * drivers/media/radio/exynos/s621/radio-s621.c
 *
 * V4L2 driver for SAMSUNG s621 chip
 *
 * Copyright (c) 2022 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 */
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/atomic.h>
#include <linux/videodev2.h>
#include <linux/io.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/vmalloc.h>
#include <media/v4l2-common.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/v4l2-device.h>
#include <linux/version.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/pinctrl/consumer.h>

#if IS_ENABLED(CONFIG_SCSC_FM)
#include <scsc/scsc_mx.h>
#endif

#include "s621_io_map.h"
#include "fm_wakelock.h"
#include "fm_low_struc.h"
#include "radio-s621.h"

#if defined(USE_AUDIO_PM)
#define ABOX_CPU_GEAR_CALL_KERNEL	(0xCA12)
#endif

static unsigned int radio_region = 0;
module_param(radio_region, uint, 0644);
MODULE_PARM_DESC(radio_region, "Region: 0=Europe/US, 1=Japan, 2=Custom");

struct s621_radio;

/* global variable for radio structure */
struct s621_radio *gradio;

#define	FAC_VALUE	16000

static int s621_radio_g_volatile_ctrl(struct v4l2_ctrl *ctrl);
static int s621_radio_s_ctrl(struct v4l2_ctrl *ctrl);
static int s621_core_set_power_state(struct s621_radio *radio,
		u8 next_state);
static int fm_radio_runtime_resume(struct device *dev);

static int fm_clk_get(struct s621_radio *radio);
static int fm_clk_prepare(struct s621_radio *radio);
static int fm_clk_enable(struct s621_radio *radio);
static void fm_clk_unprepare(struct s621_radio *radio);
static void fm_clk_disable(struct s621_radio *radio);
static void fm_clk_put(struct s621_radio *radio);

#ifdef USE_PMIC_RW
static int fm_ldo_enable(struct s621_radio *radio);
static int fm_ldo_disable(struct s621_radio *radio);
#endif /* USE_PMIC_RW */

#ifdef USE_FM_PIN_CONTROL
static int fm_init_pinctrl_info(struct s621_radio *radio);
static int fm_pinctrl_state_active(struct s621_radio *radio);
static int fm_pinctrl_state_sleep(struct s621_radio *radio);
static int fm_pinctrl_state_elna(struct s621_radio *radio);
#endif /* USE_FM_PIN_CONTROL */

u32 *fm_spur_init;
u32 *fm_spur_trf_init;
u32 *fm_dual_clk_init;
u32 vol_level_init[FM_RX_VOLUME_GAIN_STEP] = {
	0, 16, 23, 32, 45, 64, 90, 128,
	181, 256, 362, 512, 724, 1024, 1447, 2047 };

static const struct v4l2_ctrl_ops s621_ctrl_ops = {
		.s_ctrl			= s621_radio_s_ctrl,
		.g_volatile_ctrl       = s621_radio_g_volatile_ctrl,

};

enum s621_ctrl_idx {
	S621_IDX_CH_SPACING  = 0x01,
	S621_IDX_CH_BAND    =  0x02,
	S621_IDX_SOFT_STEREO_BLEND = 0x03,
	S621_IDX_SOFT_STEREO_BLEND_COEFF = 0x04,
	S621_IDX_SOFT_MUTE_COEFF = 0x05,
	S621_IDX_RSSI_CURR	= 0x06,
	S621_IDX_SNR_CURR	= 0x07,
	S621_IDX_SEEK_CANCEL	= 0x08,
	S621_IDX_SEEK_MODE	= 0x09,
	S621_IDX_RDS_ON = 0x0A,
	S621_IDX_IF_COUNT1 = 0x0B,
	S621_IDX_IF_COUNT2 = 0x0C,
	S621_IDX_RSSI_TH	= 0x0D,
	S621_IDX_KERNEL_VER	= 0x0E,
	S621_IDX_SOFT_STEREO_BLEND_REF	= 0x0F,
	S621_IDX_REG_RW_ADDR = 0x10,
	S621_IDX_REG_RW = 0x11
};

static struct v4l2_ctrl_config s621_ctrls[] = {
		/**
		 * S621 during its station seeking(or tuning) process uses several
		 * parameters to detrmine if "the station" is valid:
		 *
		 *	- Signal's RSSI(in dBuV) must be greater than
		 *	#V4L2_CID_S621_RSSI_THRESHOLD
		 */
		[S621_IDX_CH_SPACING] = {/*0x01*/
				.ops	= &s621_ctrl_ops,
				.id	= V4L2_CID_S621_CH_SPACING,
				.type	= V4L2_CTRL_TYPE_INTEGER,
				.name	= "Channel Spacing",
				.min	= 0,
				.max	= 2,
				.step	= 1,
		},
		[S621_IDX_CH_BAND] = { /*0x02*/
				.ops	= &s621_ctrl_ops,
				.id	= V4L2_CID_S621_CH_BAND,
				.type	= V4L2_CTRL_TYPE_INTEGER,
				.name	= "Channel Band",
				.min	= 0,
				.max	= 0xFFFFFFF,
				.step	= 1,
		},
		[S621_IDX_SOFT_STEREO_BLEND] = { /*0x03*/
				.ops	= &s621_ctrl_ops,
				.id	= V4L2_CID_S621_SOFT_STEREO_BLEND,
				.type	= V4L2_CTRL_TYPE_INTEGER,
				.name	= "Soft Stereo Blend",
				.min	=     0,
				.max	= 1,
				.step	= 1,
		},
		[S621_IDX_SOFT_STEREO_BLEND_COEFF] = { /*0x04*/
				.ops	= &s621_ctrl_ops,
				.id	= V4L2_CID_S621_SOFT_STEREO_BLEND_COEFF,
				.type	= V4L2_CTRL_TYPE_INTEGER,
				.name	= "Soft Stereo Blend COEFF",
				.min	=     0,
				.max	= 0xffff,
				.step	= 1,
		},
		[S621_IDX_SOFT_MUTE_COEFF] = { /*0x05*/
				.ops	= &s621_ctrl_ops,
				.id	= V4L2_CID_S621_SOFT_MUTE_COEFF,
				.type	= V4L2_CTRL_TYPE_INTEGER,
				.name	= "Soft Mute COEFF Set",
				.min	=     0,
				.max	= 0xffff,
				.step	= 1,
		},
		[S621_IDX_RSSI_CURR] = { /*0x06*/
				.ops	= &s621_ctrl_ops,
				.id	= V4L2_CID_S621_RSSI_CURR,
				.type	= V4L2_CTRL_TYPE_INTEGER,
				.name	= "RSSI Current",
				.min	=     0,
				.max	= 0xffff,
				.step	= 1,
		},
		[S621_IDX_SNR_CURR] = { /*0x07*/
				.ops	= &s621_ctrl_ops,
				.id	= V4L2_CID_S621_SNR_CURR,
				.type	= V4L2_CTRL_TYPE_INTEGER,
				.name	= "SNR Current",
				.min	=     0,
				.max	= 0xffff,
				.step	= 1,
		},
		[S621_IDX_SEEK_CANCEL] = { /*0x08*/
				.ops	= &s621_ctrl_ops,
				.id	= V4L2_CID_S621_SEEK_CANCEL,
				.type	= V4L2_CTRL_TYPE_INTEGER,
				.name	= "Seek Cancel",
				.min	=     0,
				.max	= 1,
				.step	= 1,
		},
		[S621_IDX_SEEK_MODE] = { /*0x09*/
				.ops	= &s621_ctrl_ops,
				.id	= V4L2_CID_S621_SEEK_MODE,
				.type	= V4L2_CTRL_TYPE_INTEGER,
				.name	= "Seek Mode",
				.min	=     0,
				.max	= 4,
				.step	= 1,
		},
		[S621_IDX_RDS_ON] = { /*0x0A*/
				.ops	= &s621_ctrl_ops,
				.id = V4L2_CID_S621_RDS_ON,
				.type	= V4L2_CTRL_TYPE_INTEGER,
				.name	= "RDS ON",
				.min	=	  0,
				.max	= 0x0F,
				.step	= 1,
		},
		[S621_IDX_IF_COUNT1] = { /*0x0B*/
				.ops	= &s621_ctrl_ops,
				.id = V4L2_CID_S621_IF_COUNT1,
				.type	= V4L2_CTRL_TYPE_INTEGER,
				.name	= "IF_COUNT1",
				.min	=	  0,
				.max	= 0xffff,
				.step	= 1,
		},
		[S621_IDX_IF_COUNT2] = { /*0x0C*/
				.ops	= &s621_ctrl_ops,
				.id = V4L2_CID_S621_IF_COUNT2,
				.type	= V4L2_CTRL_TYPE_INTEGER,
				.name	= "IF_COUNT2",
				.min	=	  0,
				.max	= 0xffff,
				.step	= 1,
		},
		[S621_IDX_RSSI_TH] = { /*0x0D*/
				.ops	= &s621_ctrl_ops,
				.id = V4L2_CID_S621_RSSI_TH,
				.type	= V4L2_CTRL_TYPE_INTEGER,
				.name	= "RSSI Th",
				.min	=	  0,
				.max	= 0xffff,
				.step	= 1,
		},
		[S621_IDX_KERNEL_VER] = { /*0x0E*/
				.ops	= &s621_ctrl_ops,
				.id = V4L2_CID_S621_KERNEL_VER,
				.type	= V4L2_CTRL_TYPE_INTEGER,
				.name	= "KERNEL_VER",
				.min	=	  0,
				.max	= 0xffff,
				.step	= 1,
		},
		[S621_IDX_SOFT_STEREO_BLEND_REF] = { /*0x0F*/
				.ops	= &s621_ctrl_ops,
				.id = V4L2_CID_S621_SOFT_STEREO_BLEND_REF,
				.type	= V4L2_CTRL_TYPE_INTEGER,
				.name	= "Soft Stereo Blend Ref",
				.min	=	  0,
				.max	= 0xffff,
				.step	= 1,
		},
		[S621_IDX_REG_RW_ADDR] = { /*0x10*/
				.ops	= &s621_ctrl_ops,
				.id = V4L2_CID_S621_REG_RW_ADDR,
				.type	= V4L2_CTRL_TYPE_INTEGER,
				.name	= "REG_RW_ADDR",
				.min	=	  0,
				.max	= 0xffffff,
				.step	= 1,
		},
		[S621_IDX_REG_RW] = { /*0x11*/
				.ops	= &s621_ctrl_ops,
				.id = V4L2_CID_S621_REG_RW,
				.type	= V4L2_CTRL_TYPE_INTEGER,
				.name	= "REG_RW",
				.min	=	  0,
				.max	= 0x7fffffff,
				.step	= 1,
		},
};

static const struct v4l2_frequency_band s621_bands[] = {
		[0] = {
				.type		= V4L2_TUNER_RADIO,
				.index		= S621_BAND_FM,
				.capability	= V4L2_TUNER_CAP_LOW
				| V4L2_TUNER_CAP_STEREO
				| V4L2_TUNER_CAP_RDS
				| V4L2_TUNER_CAP_RDS_BLOCK_IO
				| V4L2_TUNER_CAP_FREQ_BANDS,
				/* default region Eu/US */
				.rangelow	= 87500*FAC_VALUE,
				.rangehigh	= 108000*FAC_VALUE,
				.modulation	= V4L2_BAND_MODULATION_FM,
		},
		[1] = {
				.type		= V4L2_TUNER_RADIO,
				.index		= S621_BAND_FM,
				.capability	= V4L2_TUNER_CAP_LOW
				| V4L2_TUNER_CAP_STEREO
				| V4L2_TUNER_CAP_RDS
				| V4L2_TUNER_CAP_RDS_BLOCK_IO
				| V4L2_TUNER_CAP_FREQ_BANDS,
				/* Japan region       */
				.rangelow	= 76000*FAC_VALUE,
				.rangehigh	= 90000*FAC_VALUE,
				.modulation	= V4L2_BAND_MODULATION_FM,
		},
		[2] = {
				.type		= V4L2_TUNER_RADIO,
				.index		= S621_BAND_FM,
				.capability = V4L2_TUNER_CAP_LOW
				| V4L2_TUNER_CAP_STEREO
				| V4L2_TUNER_CAP_RDS
				| V4L2_TUNER_CAP_RDS_BLOCK_IO
				| V4L2_TUNER_CAP_FREQ_BANDS,
				/* custom region */
				.rangelow	= 76000*FAC_VALUE,
				.rangehigh	= 108000*FAC_VALUE,
				.modulation = V4L2_BAND_MODULATION_FM,
		},
};

/* Region info */
static struct region_info region_configs[] = {
		/* Europe/US */
		{
			.chanl_space = FM_CHANNEL_SPACING_200KHZ * FM_FREQ_MUL,
			.bot_freq = 87500,	/* 87.5 MHz */
			.top_freq = 108000,	/* 108 MHz */
			.fm_band = 0,
		},
		/* Japan */
		{
			.chanl_space = FM_CHANNEL_SPACING_200KHZ * FM_FREQ_MUL,
			.bot_freq = 76000,	/* 76 MHz */
			.top_freq = 90000,	/* 90 MHz */
			.fm_band = 1,
		},
		/* Custom */
		{
			.chanl_space = FM_CHANNEL_SPACING_200KHZ * FM_FREQ_MUL,
			.bot_freq = 76000,	/* 76 MHz */
			.top_freq = 108000,	/* 108 MHz */
			.fm_band = 2,
		},

};

static inline bool s621_radio_freq_is_inside_of_the_band(u32 freq, int band)
{
	return freq >= region_configs[band].bot_freq &&
			freq <= region_configs[band].top_freq;
}

static inline struct s621_radio *
v4l2_dev_to_radio(struct v4l2_device *d)
{
	return container_of(d, struct s621_radio, v4l2dev);
}

static inline struct s621_radio *
v4l2_ctrl_handler_to_radio(struct v4l2_ctrl_handler *d)
{
	return container_of(d, struct s621_radio, ctrl_handler);
}

/*
 * s621_vidioc_querycap - query device capabilities
 */
static int s621_radio_querycap(struct file *file, void *priv,
		struct v4l2_capability *capability)
{
	struct s621_radio *radio = video_drvdata(file);

	FUNC_ENTRY(radio);

	s621_core_lock(radio->core);
	strscpy(capability->driver, radio->v4l2dev.name,
			sizeof(capability->driver));
	strscpy(capability->card, DRIVER_CARD, sizeof(capability->card));

	snprintf(capability->bus_info, sizeof(capability->bus_info),
			"platform:%s", radio->v4l2dev.name);
	s621_core_unlock(radio->core);

	FUNC_EXIT(radio);

	return 0;
}

static int s621_radio_enum_freq_bands(struct file *file, void *priv,
		struct v4l2_frequency_band *band)
{
	int ret;
	struct s621_radio *radio = video_drvdata(file);

	FUNC_ENTRY(radio);

	if (band->tuner != 0)
		return -EINVAL;

	s621_core_lock(radio->core);

	if (band->index == S621_BAND_FM) {
		*band = s621_bands[radio_region];
		ret = 0;
	} else {
		ret = -EINVAL;
	}

	s621_core_unlock(radio->core);

	FUNC_EXIT(radio);

	return ret;
}

static int s621_radio_g_tuner(struct file *file, void *priv,
		struct v4l2_tuner *tuner)
{
	int ret;
	struct s621_radio *radio = video_drvdata(file);
	u16 payload;

	FUNC_ENTRY(radio);

	if (tuner->index != 0)
		return -EINVAL;

	s621_core_lock(radio->core);

	tuner->type       = V4L2_TUNER_RADIO;
	tuner->capability = V4L2_TUNER_CAP_LOW
			| V4L2_TUNER_CAP_STEREO
			| V4L2_TUNER_CAP_HWSEEK_BOUNDED
			| V4L2_TUNER_CAP_HWSEEK_WRAP
			| V4L2_TUNER_CAP_HWSEEK_PROG_LIM;


	strscpy(tuner->name, "FM", sizeof(tuner->name));

	tuner->rxsubchans = V4L2_TUNER_SUB_RDS;
	tuner->capability |= V4L2_TUNER_CAP_RDS
			| V4L2_TUNER_CAP_RDS_BLOCK_IO
			| V4L2_TUNER_CAP_FREQ_BANDS;

	/* Read register : MONO, Stereo mode */
	/*ret = low_get_most_mode(radio, &payload);*/
	payload = radio->low->fm_state.force_mono ?
			0 : MODE_MASK_MONO_STEREO;
	radio->audmode = payload;
	tuner->audmode = radio->audmode;
	tuner->afc = 1;
	tuner->rangelow = s621_bands[radio_region].rangelow;
	tuner->rangehigh = s621_bands[radio_region].rangehigh;

	ret = low_get_search_lvl(radio, &payload);
	if (ret != 0) {
		dev_err(radio->v4l2dev.dev,
			"Failed to read reg for SEARCH_LVL\n");
		ret = -EIO;
		tuner->signal = 0;
	} else {
		tuner->signal = 0;
		if (payload & 0x80)
			tuner->signal = 0xFF00;
		else
			tuner->signal = 0;

		tuner->signal |= (payload & 0xFF);
	}

	s621_core_unlock(radio->core);

	FUNC_EXIT(radio);

	return ret;
}

static int s621_radio_s_tuner(struct file *file, void *priv,
		const struct v4l2_tuner *tuner)
{
	int ret;
	struct s621_radio *radio = video_drvdata(file);
	u16 payload;

	FUNC_ENTRY(radio);

	if (tuner->index != 0)
		return -EINVAL;

	s621_core_lock(radio->core);

	if (tuner->audmode == V4L2_TUNER_MODE_MONO ||
			tuner->audmode == V4L2_TUNER_MODE_STEREO)
		radio->audmode = tuner->audmode;
	else
		radio->audmode = V4L2_TUNER_MODE_STEREO;

	payload = radio->audmode;
	ret = low_set_most_mode(radio, payload);
	if (ret != 0) {
		dev_err(radio->v4l2dev.dev,
			"Failed to write reg for MOST MODE clear\n");
		ret = -EIO;
	}

	s621_core_unlock(radio->core);

	FUNC_EXIT(radio);

	return ret;
}

static int s621_radio_pretune(struct s621_radio *radio)
{
	int ret = 0;
	u16 payload = 0;

	FUNC_ENTRY(radio);

	ret = low_set_mute_state(radio, payload);
	if (ret != 0) {
		dev_err(radio->v4l2dev.dev,
				"Failed to write reg for MUTE state clean\n");
		return -EIO;
	}

	ret = low_set_most_blend(radio, payload);
	if (ret != 0) {
		dev_err(radio->v4l2dev.dev,
				"Failed to write reg for MOST blend clean\n");
		return -EIO;
	}

	fm_set_band(radio, payload);

	ret = low_set_demph_mode(radio, payload);
	if (ret != 0) {
		dev_err(radio->v4l2dev.dev,
			"Failed to write reg for FM_SUBADDR_DEMPH_MODE clean\n");
		return -EIO;
	}

	radio->low->fm_state.use_rbds = FALSE;
	radio->low->fm_state.save_eblks = FALSE;
	radio->low->fm_state.rds_mem_thresh = RDS_MEM_MAX_THRESH;

	FUNC_EXIT(radio);

	return ret;
}

static int s621_radio_g_frequency(struct file *file, void *priv,
		struct v4l2_frequency *f)
{
	struct s621_radio *radio = video_drvdata(file);
	u32 payload;

	FUNC_ENTRY(radio);

	if (f->tuner != 0 || f->type  != V4L2_TUNER_RADIO)
		return -EINVAL;

	s621_core_lock(radio->core);

	payload = radio->low->fm_state.freq;
	f->frequency = payload;
	f->frequency *= FAC_VALUE;

	s621_core_unlock(radio->core);

	FUNC_EXIT(radio);

	FDEBUG(radio,
		"%s():freq.tuner:%d, type:%d, freq:%d, real-freq:%d\n",
		__func__, f->tuner, f->type, f->frequency,
		f->frequency/FAC_VALUE);

	return 0;
}

int fm_set_frequency(struct s621_radio *radio, u32 freq)
{
	unsigned long timeleft;
	u32 curr_frq, intr_flag;
	u32 curr_frq_in_khz;
	int ret;
	u32 payload;
	u16 payload16;

	FUNC_ENTRY(radio);

	/* Calculate frequency with offset and set*/
	payload = real_to_api(freq);

	low_set_freq(radio, payload);

	/* Read flags - just to clear any pending interrupts if we had */
	payload = fm_get_flags(radio);

	/* Enable FR, BL interrupts */
	intr_flag = radio->irq_mask;
	radio->irq_mask = (FM_EVENT_TUNED | FM_EVENT_BD_LMT);

	low_get_search_lvl(radio, (u16 *) &payload16);
	if (!payload16) {
		payload16 = FM_DEFAULT_RSSI_THRESHOLD;
		low_set_search_lvl(radio, (u16) payload16);
	}

	if (radio->low->fm_config.search_conf.normal_ifca_m == 0)
		radio->low->fm_config.search_conf.normal_ifca_m =
			low_fm_conf_init.search_conf.normal_ifca_m;

	if (radio->low->fm_config.search_conf.weak_ifca_m == 0)
		radio->low->fm_config.search_conf.weak_ifca_m =
			low_fm_conf_init.search_conf.weak_ifca_m;

	FDEBUG(radio, "%s(): ifcount:W-%d N-%d\n",	__func__,
		radio->low->fm_config.search_conf.weak_ifca_m,
		radio->low->fm_config.search_conf.normal_ifca_m);

	if (!radio->low->fm_config.mute_coeffs_soft) {
		radio->low->fm_config.mute_coeffs_soft =
			low_fm_conf_init.mute_coeffs_soft;
	}

	if (!radio->low->fm_config.blend_coeffs_soft) {
		radio->low->fm_config.blend_coeffs_soft =
			low_fm_conf_init.blend_coeffs_soft;
	}

	if (!radio->low->fm_config.blend_coeffs_switch) {
		radio->low->fm_config.blend_coeffs_switch =
			low_fm_conf_init.blend_coeffs_switch;
	}

	if (!radio->low->fm_config.blend_coeffs_dis)
		radio->low->fm_config.blend_coeffs_dis =
		low_fm_conf_init.blend_coeffs_dis;

	fm_set_blend_mute(radio);

	init_completion(&radio->flags_set_fr_comp);

	/* Start tune */
	payload = FM_TUNER_PRESET_MODE;
	ret = low_set_tuner_mode(radio, payload);
	if (ret != 0) {
		ret = -EIO;
		goto exit;
	}

	timeleft = jiffies_to_msecs(FM_DRV_TURN_TIMEOUT);
	/* Wait for tune ended interrupt */
	timeleft = wait_for_completion_timeout(&radio->flags_set_fr_comp,
			FM_DRV_TURN_TIMEOUT);

	if (!timeleft) {
		dev_err(radio->v4l2dev.dev,
				"Timeout(%d sec),didn't get tune ended int\n",
				jiffies_to_msecs(FM_DRV_TURN_TIMEOUT) / 1000);
		ret = -ETIMEDOUT;
		goto exit;
	}

	/* Read freq back to confirm */
	curr_frq = radio->low->fm_state.freq;
	curr_frq_in_khz = curr_frq;
	if (curr_frq_in_khz != freq) {
		dev_err(radio->v4l2dev.dev,
				"%s(), Failed to set freq(%d). Requested freq(%d)\n",
				__func__,
				curr_frq_in_khz, freq);
		ret = -ENODATA;
		goto exit;
	}

	FDEBUG(radio,
			"%s(), Set frequency:%d Read frequency:%d\n",
			__func__,  freq, curr_frq_in_khz);

	/* Update local cache  */
	radio->freq = curr_frq_in_khz;
exit:
	/* Re-enable default FM interrupts */
	radio->irq_mask = intr_flag;

	FDEBUG(radio, "%s(), error ret:%d\n", __func__, ret);

	FUNC_EXIT(radio);

	return ret;
}

static int s621_radio_s_frequency(struct file *file, void *priv,
		const struct v4l2_frequency *f)
{
	int ret;
	u32 freq = f->frequency;
	struct s621_radio *radio = video_drvdata(file);

	FUNC_ENTRY(radio);

	if (f->tuner != 0 ||
			f->type  != V4L2_TUNER_RADIO)
		return -EINVAL;

	if (!wake_lock_active(&radio->wakelock))
		wake_lock(&radio->wakelock);

	s621_core_lock(radio->core);
	FDEBUG(radio, "%s():freq:%d, real-freq:%d\n",
			__func__, f->frequency, f->frequency/FAC_VALUE);

	freq /= FAC_VALUE;

	freq = clamp(freq,
			region_configs[radio_region].bot_freq,
			region_configs[radio_region].top_freq);
	if (!s621_radio_freq_is_inside_of_the_band(freq,
			radio_region)) {
		ret = -EINVAL;
		goto unlock;
	}

	ret = fm_set_frequency(radio, freq);

unlock:
	s621_core_unlock(radio->core);

	if (wake_lock_active(&radio->wakelock))
		wake_unlock(&radio->wakelock);

	FUNC_EXIT(radio);

	FDEBUG(radio,
			"%s():v4l2_frequency.tuner:%d,type:%d,frequency:%d\n",
			__func__, f->tuner, f->type, freq);

	return ret;
}

int fm_rx_seek(struct s621_radio *radio, u32 seek_upward,
		u32 wrap_around, u32 spacing, u32 freq_low, u32 freq_hi)
{
	u32 curr_frq, save_freq;
	u32 payload, int_reason, intr_flag, tune_mode;
	u32 space, upward;
	u16 payload16;
	unsigned long timeleft;
	int ret;
	int	bl_1st, bl_2nd, bl_3nd;
	static u32 pre_freq;

	FUNC_ENTRY(radio);

	pre_freq = 0;
	radio->seek_freq = 0;
	radio->wrap_around = 0;
	bl_1st = 0;
	bl_2nd = 0;
	bl_3nd = 0;

	payload = fm_get_flags(radio);

	/* Set channel spacing */
	if (spacing > 0 && spacing <= 50)
		payload = 0; /*CHANNEL_SPACING_50KHZ*/
	else if (spacing > 50 && spacing <= 100)
		payload = 1; /*CHANNEL_SPACING_100KHZ*/
	else
		payload = 2; /*CHANNEL_SPACING_200KHZ;*/

	FDEBUG(radio, "%s(): spacing: %d\n", __func__, payload);

	radio->low->fm_tuner_state.freq_step =
			radio->low->fm_freq_steps[payload];
	radio->region.chanl_space = 50 * (1 << payload);

	/* Check the offset in order to be aligned to the channel spacing*/
	space = radio->region.chanl_space;

	/* Set search direction (0:Seek Up, 1:Seek Down) */
	payload = (seek_upward ? FM_SEARCH_DIRECTION_UP :
		FM_SEARCH_DIRECTION_DOWN);
	upward = payload;
	radio->low->fm_state.search_down =
			!!(payload & SEARCH_DIR_MASK);
	FDEBUG(radio, "%s(): direction: %d\n",	__func__, payload);

	save_freq = freq_low;
	radio->seek_freq = save_freq;

	if ((((save_freq == region_configs[radio_region].bot_freq) &&
		(upward == FM_SEARCH_DIRECTION_UP)) ||
		((save_freq == region_configs[radio_region].top_freq) &&
		(upward == FM_SEARCH_DIRECTION_DOWN))) &&
		(pre_freq != save_freq) &&
		!wrap_around) {
		tune_mode = FM_TUNER_AUTONOMOUS_SEARCH_MODE;
	} else
		tune_mode = FM_TUNER_AUTONOMOUS_SEARCH_MODE_NEXT;

	pre_freq = save_freq;

	if (radio->low->fm_state.rssi_limit_search == 0)
		low_set_search_lvl(radio, (u16)FM_DEFAULT_RSSI_THRESHOLD);

	if (radio->low->fm_config.search_conf.normal_ifca_m == 0)
		radio->low->fm_config.search_conf.normal_ifca_m =
			low_fm_conf_init.search_conf.normal_ifca_m;

	if (radio->low->fm_config.search_conf.weak_ifca_m == 0)
		radio->low->fm_config.search_conf.weak_ifca_m =
			low_fm_conf_init.search_conf.weak_ifca_m;

		FDEBUG(radio, "%s(): ifcount:W-%d N-%d\n",	__func__,
		radio->low->fm_config.search_conf.weak_ifca_m,
		radio->low->fm_config.search_conf.normal_ifca_m);

again:
	curr_frq = freq_low;
	if ((freq_low == region_configs[radio_region].bot_freq)
		&& (upward == FM_SEARCH_DIRECTION_DOWN)) {
		curr_frq = region_configs[radio_region].top_freq;
		tune_mode = FM_TUNER_AUTONOMOUS_SEARCH_MODE;
	}

	if ((freq_low == region_configs[radio_region].top_freq)
		&& (upward == FM_SEARCH_DIRECTION_UP)) {
		curr_frq = region_configs[radio_region].bot_freq;
		tune_mode = FM_TUNER_AUTONOMOUS_SEARCH_MODE;
	}

	payload = curr_frq;
	low_set_freq(radio, payload);

	FDEBUG(radio, "%s(): curr_freq: %d, freq hi: %d\n",
		__func__, curr_frq, freq_hi);

	/* Enable FR, BL interrupts */
	intr_flag = radio->irq_mask;
	radio->irq_mask = (FM_EVENT_TUNED | FM_EVENT_BD_LMT);
	low_get_search_lvl(radio, (u16 *) &payload16);
	if (!payload16) {
		payload16 = FM_DEFAULT_RSSI_THRESHOLD;
		low_set_search_lvl(radio, (u16) payload16);
	}
	FDEBUG(radio, "%s(): SEARCH_LVL1: 0x%x\n",  __func__, payload16);

	if (!radio->low->fm_config.mute_coeffs_soft) {
		radio->low->fm_config.mute_coeffs_soft =
			low_fm_conf_init.mute_coeffs_soft;
	}

	if (!radio->low->fm_config.blend_coeffs_soft) {
		radio->low->fm_config.blend_coeffs_soft =
			low_fm_conf_init.blend_coeffs_soft;
	}

	if (!radio->low->fm_config.blend_coeffs_switch) {
		radio->low->fm_config.blend_coeffs_switch =
			low_fm_conf_init.blend_coeffs_switch;
	}

	if (!radio->low->fm_config.blend_coeffs_dis)
		radio->low->fm_config.blend_coeffs_dis =
		low_fm_conf_init.blend_coeffs_dis;

	fm_set_blend_mute(radio);

	reinit_completion(&radio->flags_seek_fr_comp);

	payload = tune_mode;
	FDEBUG(radio,
			"%s(): turn start mode: 0x%x\n",  __func__, payload);
	ret = low_set_tuner_mode(radio, (u16) payload);
	if (ret != 0)
		return -EIO;

	/* Wait for tune ended interrupt */
	timeleft =
		wait_for_completion_timeout(&radio->flags_seek_fr_comp,
		FM_DRV_SEEK_TIMEOUT);

	/* seek cancel status */
	if (radio->seek_status == FM_TUNER_STOP_SEARCH_MODE) {
		dev_info(radio->dev, ">>> rev seek cancel");
		return -ENODATA;
	}

	FDEBUG(radio,
		"FDEBUG > Seek done rev complete!! freq %d, irq_flag: 0x%x, bl:%d\n",
		radio->low->fm_state.freq, radio->irq_flag, bl_1st);

	int_reason = radio->low->fm_state.flags &
			(FM_EVENT_TUNED | FM_EVENT_BD_LMT);

	if ((save_freq == region_configs[radio_region].bot_freq)
		|| (save_freq == region_configs[radio_region].top_freq))
			bl_1st = 1;

	if ((save_freq == region_configs[radio_region].bot_freq)
		&& (upward == FM_SEARCH_DIRECTION_UP)) {
		bl_1st = 0;
		bl_2nd = 1;
		tune_mode = FM_TUNER_AUTONOMOUS_SEARCH_MODE;
	}
	if ((save_freq == region_configs[radio_region].top_freq)
		&& (upward == FM_SEARCH_DIRECTION_DOWN)) {
		bl_1st = 0;
		bl_2nd = 1;
		tune_mode = FM_TUNER_AUTONOMOUS_SEARCH_MODE;
	}

	if ((int_reason & FM_EVENT_BD_LMT) && (bl_1st == 0) && (bl_3nd == 0)) {
		if (wrap_around) {
			freq_low = radio->low->fm_state.freq;
			bl_1st = 1;
			if (bl_2nd)
				bl_3nd = 1;

			/* Re-enable default FM interrupts */
			radio->irq_mask = intr_flag;
			radio->wrap_around = 1;
			FDEBUG(radio, "> bl set %d, %d, save %d\n",
				radio->seek_freq,
				radio->wrap_around,
				save_freq);

			goto again;
		} else
			ret = -EINVAL;
	}

	/* Read freq to know where operation tune operation stopped */
	payload = radio->low->fm_state.freq;
	radio->freq = payload;

	dev_info(radio->v4l2dev.dev, "Seek freq %d\n", radio->freq);

	radio->seek_freq = 0;
	radio->wrap_around = 0;

	FUNC_EXIT(radio);

	return ret;
}

static int s621_radio_s_hw_freq_seek(struct file *file, void *priv,
		const struct v4l2_hw_freq_seek *seek)
{
	int ret = 0;
	struct s621_radio *radio = video_drvdata(file);
	u32 seek_low, seek_hi, seek_spacing;

	FUNC_ENTRY(radio);

	if (file->f_flags & O_NONBLOCK)
		return -EAGAIN;

	if (seek->tuner != 0 ||
			seek->type  != V4L2_TUNER_RADIO)
		return -EINVAL;

	if (seek->rangelow >= seek->rangehigh)
		ret = -EINVAL;

	seek_low = radio->low->fm_state.freq;
	if (seek->seek_upward)
		seek_hi = region_configs[radio_region].top_freq;
	else
		seek_hi = region_configs[radio_region].bot_freq;

	seek_spacing = seek->spacing / 1000;
	FDEBUG(radio, "%s(): get freq low: %d, freq hi: %d\n",
		__func__, seek_low, seek_hi);
	FDEBUG(radio,
			"%s(): upward:%d, warp: %d, spacing: %d\n",  __func__,
		seek->seek_upward, seek->wrap_around, seek->spacing);

	if (!wake_lock_active(&radio->wakelock))
		wake_lock(&radio->wakelock);

	ret = fm_rx_seek(radio, seek->seek_upward, seek->wrap_around,
			seek_spacing, seek_low, seek_hi);
	if (ret < 0)
		dev_err(radio->v4l2dev.dev, "RX seek failed - %d\n", ret);

	if (wake_lock_active(&radio->wakelock))
		wake_unlock(&radio->wakelock);

	FUNC_EXIT(radio);

	return ret;
}

/* Configures mute mode (Mute Off/On/Attenuate) */
int fm_set_mute_mode(struct s621_radio *radio, u8 mute_mode_toset)
{
	u16 payload = radio->mute_mode;
	int ret = 0;

	FUNC_ENTRY(radio);

	radio->mute_mode = mute_mode_toset;

	switch (radio->mute_mode) {
	case FM_MUTE_ON:
		payload = FM_RX_AC_MUTE_MODE;
		break;

	case FM_MUTE_OFF:
		payload = FM_RX_UNMUTE_MODE;
		break;
	}

	/* Write register : mute */
	ret = low_set_mute_state(radio, payload);
	if (ret != 0)
		ret = -EIO;

	FUNC_EXIT(radio);

	return ret;
}

/* Enable/Disable RDS */
int fm_set_rds_mode(struct s621_radio *radio, u8 rds_en_dis)
{
	int ret = 0;
	u16 payload;
	int ii;
	u32 fifo_tmp;

	FUNC_ENTRY(radio);

	if (rds_en_dis != FM_RDS_ENABLE && rds_en_dis != FM_RDS_DISABLE) {
		dev_err(radio->v4l2dev.dev, "Invalid rds option\n");
		return -EINVAL;
	}

	if (rds_en_dis == FM_RDS_ENABLE) {
		if (!wake_lock_active(&radio->rdswakelock))
			wake_lock(&radio->rdswakelock);
		msleep(10);
		atomic_set(&radio->is_rds_new, 0);
		radio->rds_cnt_mod = 0;
		radio->rds_n_count = 0;
		radio->rds_r_count = 0;
		radio->rds_read_cnt = 0;
		radio->rds_sync_loss_cnt = 0;

		init_waitqueue_head(&radio->core->rds_read_queue);

		payload = radio->core->power_mode;
		payload |= S621_POWER_ON_RDS;
		ret = s621_core_set_power_state(radio,
				payload);
		if (ret != 0) {
			dev_err(radio->v4l2dev.dev,
					"Failed to set for RDS power state\n");
			return -EIO;
		}

		/* Write register : RDS on */
		/* Turn on RDS and RDS circuit */
		fm_rds_on(radio);

		/* Write register : clear RDS cache */
		/* flush RDS buffer */
		payload = FM_RX_RDS_FLUSH_FIFO;
		ret = low_set_rds_cntr(radio, payload);
		if (ret != 0) {
			dev_err(radio->v4l2dev.dev,
					"Failed to write reg for default RDS_CNTR\n");
			return -EIO;
		}

		/* Write register : clear panding interrupt flags */
		/* Read flags - just to clear any pending interrupts. */
		payload = fm_get_flags(radio);

		/* Write register : set RDS memory depth */
		/* Set RDS FIFO threshold value */
		if (radio->rds_parser_enable)
			radio->low->fm_state.rds_mem_thresh =
				RDS_MEM_MAX_THRESH_PARSER;
		else
			radio->low->fm_state.rds_mem_thresh =
				RDS_MEM_MAX_THRESH;

		/* Write register : set RDS interrupt enable */
		/* Enable RDS interrupt */
		radio->irq_mask |= FM_EVENT_BUF_FUL;

		/* Update our local flag */
		radio->rds_flag = FM_RDS_ENABLE;

		spin_lock(&radio->rds_lock);

		/* RDS parser reset */
		if (radio->rds_parser_enable)
			fm_rds_parser_reset(&(radio->pi));

		/* FIFO clear */
		for (ii = 0; ii < 32; ii++)
			fifo_tmp = fmspeedy_get_reg_work(FM_RDS_DATA);

		spin_unlock(&radio->rds_lock);

#ifdef	RDS_POLLING_ENABLE
		fm_rds_periodic_update((unsigned long) radio);
#endif	/*RDS_POLLING_ENABLE*/
	} else if (
		rds_en_dis == FM_RDS_DISABLE) {
		payload = radio->core->power_mode;
		payload &= ~S621_POWER_ON_RDS;
		ret = s621_core_set_power_state(radio,
				payload);
		if (ret != 0) {
			dev_err(radio->v4l2dev.dev,
					"Failed to set for RDS power state\n");
			return -EIO;
		}

		/* Write register : RDS off */
		/* Turn off RX RDS */
		fm_rds_off(radio);

		/* Update RDS local cache */
		radio->irq_mask &= ~(FM_EVENT_BUF_FUL);
		radio->rds_flag = FM_RDS_DISABLE;
#ifdef	RDS_POLLING_ENABLE
		fm_rds_periodic_cancel((unsigned long) radio);
#endif	/*RDS_POLLING_ENABLE*/

		/* Service pending read */
		wake_up_interruptible(&radio->core->rds_read_queue);

		if (wake_lock_active(&radio->rdswakelock))
			wake_unlock(&radio->rdswakelock);
	}

	FUNC_EXIT(radio);

	return ret;
}

int fm_set_deemphasis(struct s621_radio *radio, u16 vol_to_set)
{
	int ret = 0;
	u16 payload;

	payload = vol_to_set;
	/* Write register : deemphasis */
	ret = low_set_demph_mode(radio, payload);
	if (ret != 0)
		return -EIO;

	radio->deemphasis_mode = vol_to_set;

	return ret;
}

static int s621_radio_g_volatile_ctrl(struct v4l2_ctrl *ctrl)
{
	struct s621_radio *radio = v4l2_ctrl_handler_to_radio(ctrl->handler);
	u16 payload;
	int ret = 0;

	FUNC_ENTRY(radio);
	if (!radio)
		return -ENODEV;

	s621_core_lock(radio->core);

	switch (ctrl->id) {
	case V4L2_CID_S621_CH_SPACING:
		payload = radio->low->fm_tuner_state.freq_step / 100;
		FDEBUG(radio, "%s(), FREQ_STEP val:%d, ret : %d\n",
				__func__, payload,  ret);
		ctrl->val = payload;
		break;
	case V4L2_CID_S621_CH_BAND:
		payload = radio->low->fm_state.band;
		FDEBUG(radio, "%s(), BAND val:%d, ret : %d\n",
				__func__, payload, ret);
		ctrl->val = payload;
		break;
	case V4L2_CID_S621_SOFT_STEREO_BLEND:
		payload = radio->low->fm_state.use_switched_blend ?
			MODE_MASK_BLEND : 0;
		FDEBUG(radio, "%s(), MOST_BLEND val:%d, ret : %d\n",
				__func__, ctrl->val, ret);
		ctrl->val = payload;
		break;
	case V4L2_CID_S621_SOFT_STEREO_BLEND_COEFF:
		payload = radio->low->fm_config.blend_coeffs_soft;
		FDEBUG(radio, "%s(),BLEND_COEFF_SOFT val:%d, ret: %d\n",
				__func__, ctrl->val, ret);
		ctrl->val = payload;
		break;
	case V4L2_CID_S621_SOFT_MUTE_COEFF:
		payload = radio->low->fm_config.mute_coeffs_soft;
		FDEBUG(radio, "%s(), MUTE_COEFF_SOFT val:%d, ret: %d\n",
				__func__, ctrl->val, ret);
		ctrl->val = payload;
		break;
	case V4L2_CID_S621_RSSI_CURR:
		fm_update_rssi(radio);
		payload = radio->low->fm_state.rssi;
		ctrl->val = (payload & 0x80) ? payload - 256 : payload;
		FDEBUG(radio, "%s(), RSSI_CURR val:%d, ret : %d\n",
				__func__, ctrl->val, ret);
		break;
	case V4L2_CID_S621_SNR_CURR:
		fm_update_snr(radio);
		payload = radio->low->fm_state.snr;
		FDEBUG(radio, "%s(), SNR_CURR val:%d, ret : %d\n",
				__func__, payload, ret);
		ctrl->val = payload;
		break;
	case V4L2_CID_S621_SEEK_CANCEL:
		ctrl->val = 0;
		break;
	case V4L2_CID_S621_SEEK_MODE:
		if (radio->seek_mode == FM_TUNER_AUTONOMOUS_SEARCH_MODE_NEXT)
			ctrl->val = 4;
		else
			ctrl->val = radio->seek_mode;

		FDEBUG(radio, "%s(), SEEK_MODE val:%d, ret : %d\n",
				__func__, ctrl->val,  ret);
		break;
	case V4L2_CID_S621_RDS_ON:
		if (radio->low->fm_state.fm_pwr_on & S621_POWER_ON_RDS)
			ctrl->val = FM_RDS_ENABLE;
		else
			ctrl->val = FM_RDS_DISABLE;
		FDEBUG(radio, "%s(), RDS_ON:%d, ret: %d\n", __func__,
				ctrl->val, ret);
		break;
	case V4L2_CID_S621_IF_COUNT1:
		payload = radio->low->fm_config.search_conf.weak_ifca_m;
		FDEBUG(radio, "%s(), IF_CNT1 val:%d, ret : %d\n",
			__func__, ctrl->val, ret);
		ctrl->val = payload;
		break;
	case V4L2_CID_S621_IF_COUNT2:
		payload = radio->low->fm_config.search_conf.normal_ifca_m;
		FDEBUG(radio, "%s(), IF_CNT2 val:%d, ret : %d\n",
			__func__, ctrl->val, ret);
		ctrl->val = payload;
		break;
	case V4L2_CID_S621_RSSI_TH:
		ctrl->val  = radio->low->fm_state.rssi;
		FDEBUG(radio, "%s(), RSSI_CURR val:%d, ret : %d\n",
				__func__, ctrl->val, ret);
		break;
	case V4L2_CID_S621_KERNEL_VER:
		ctrl->val |= FM_RADIO_RDS_PARSER_VER_CHECK;
		FDEBUG(radio, "%s(), KERNEL_VER val:%d, ret : %d\n",
				__func__, ctrl->val, ret);
		break;
	case V4L2_CID_S621_SOFT_STEREO_BLEND_REF:
		ctrl->val = radio->rssi_ref_enable;
		FDEBUG(radio, "%s(), SOFT_STEREO_BLEND_REF val:%d, ret : %d\n",
				__func__, ctrl->val,  ret);
		break;
	case V4L2_CID_S621_REG_RW_ADDR:
		ctrl->val = radio->speedy_reg_addr;
		FDEBUG(radio, "%s(), REG_RW_ADDR val:0x%xh, ret : %d\n",
			__func__, ctrl->val, ret);
		break;
	case V4L2_CID_S621_REG_RW:
		ctrl->val = fmspeedy_get_reg(radio->speedy_reg_addr);
		FDEBUG(radio, "%s(), REG_RW val:0x%xh, ret : %d\n",
			__func__, ctrl->val, ret);
		break;
	default:
		ret = -EINVAL;
		dev_err(radio->v4l2dev.dev,
				"g_volatile_ctrl Unknown IOCTL: %d\n",
				(int) ctrl->id);
		break;
	}

	s621_core_unlock(radio->core);
	FUNC_EXIT(radio);

	return ret;
}

static int s621_radio_s_ctrl(struct v4l2_ctrl *ctrl)
{
	int ret = 0;
	u16 payload = 0;
	struct s621_radio *radio = v4l2_ctrl_handler_to_radio(ctrl->handler);

	FUNC_ENTRY(radio);

	s621_core_lock(radio->core);
	switch (ctrl->id) {
	case V4L2_CID_AUDIO_MUTE:  /* set mute */
		ret = fm_set_mute_mode(radio, (u8)ctrl->val);
		FDEBUG(radio, "%s(), mute val:%d, ret : %d\n", __func__,
				ctrl->val, ret);
		if (ret != 0)
			ret = -EIO;
		break;
	case V4L2_CID_AUDIO_VOLUME:  /* set volume gain */
		fm_set_audio_gain(radio, (u8)ctrl->val);
		FDEBUG(radio, "%s(), volume val:%d, ret : %d\n", __func__,
				ctrl->val, ret);
		break;
	case V4L2_CID_TUNE_DEEMPHASIS:
		if (ctrl->val == 0)
			break;
		payload = (u16)ctrl->val;
		if (ctrl->val > 0)
			payload -= 1;
		ret = fm_set_deemphasis(radio, payload);
		FDEBUG(radio, "%s(), deemphasis val:%d, ret : %d, payload:%d\n",
			__func__,
				ctrl->val, ret, payload);
		if (ret != 0)
			ret = -EINVAL;
		break;
	case V4L2_CID_S621_CH_SPACING:
		radio->freq_step = ctrl->val;
		radio->low->fm_tuner_state.freq_step =
		radio->low->fm_freq_steps[ctrl->val];
		FDEBUG(radio, "%s(), FREQ_STEP val:%d, ret : %d\n",
			__func__, ctrl->val,  ret);
		break;
	case V4L2_CID_S621_CH_BAND:
		FDEBUG(radio, "%s(), ctrl->val: 0x%08X, ret : %d\n",
				__func__, ctrl->val);
		radio_region = (ctrl->val & 0xF);
		radio->radio_region = (ctrl->val & 0xF);
		if (radio->radio_region == 2) {
			payload = ((ctrl->val & 0xFFF0) >> 4) * 50;
			if (s621_radio_freq_is_inside_of_the_band(payload, radio->radio_region))
				region_configs[radio->radio_region].bot_freq = payload;
			payload = ((ctrl->val & 0xFFF0000) >> 16) * 50;
			if (s621_radio_freq_is_inside_of_the_band(payload, radio->radio_region))
				region_configs[radio->radio_region].top_freq = payload;
		}
		fm_set_band(radio, radio->radio_region);
		FDEBUG(radio, "%s(), BAND val:%d, ret : %d\n",
				__func__, radio->radio_region, ret);
		break;
	case V4L2_CID_S621_SOFT_STEREO_BLEND:
		ret = low_set_most_blend(radio, ctrl->val);
		FDEBUG(radio, "%s(), MOST_BLEND val:%d, ret : %d\n",
				__func__, ctrl->val,  ret);
		if (ret != 0)
			ret = -EIO;
		break;
	case V4L2_CID_S621_SOFT_STEREO_BLEND_COEFF:
		radio->low->fm_config.blend_coeffs_soft = (u16)ctrl->val;
		fm_set_blend_mute(radio);
		FDEBUG(radio, "%s(), BLEND_COEFF_SOFT val:%d,ret: %d\n",
				__func__, ctrl->val,  ret);
		break;
	case V4L2_CID_S621_SOFT_MUTE_COEFF:
		radio->low->fm_config.mute_coeffs_soft = (u16)ctrl->val;
		fm_set_blend_mute(radio);
		FDEBUG(radio, "%s(), SOFT_MUTE_COEFF val:%d, ret: %d\n",
				__func__, ctrl->val, ret);
		break;
	case V4L2_CID_S621_RSSI_CURR:
		ctrl->val = 0;
		break;
	case V4L2_CID_S621_SEEK_CANCEL:
		if (ctrl->val) {
			payload = FM_TUNER_STOP_SEARCH_MODE;
			low_set_tuner_mode(radio, payload);
			FDEBUG(radio, "%s(), SEEK_CANCEL val:%d, ret: %d\n",
					__func__, ctrl->val,  ret);
			radio->seek_mode = FM_TUNER_STOP_SEARCH_MODE;
		}
		break;
	case V4L2_CID_S621_SEEK_MODE:
		if (ctrl->val == 4)
			radio->seek_mode = FM_TUNER_AUTONOMOUS_SEARCH_MODE_NEXT;
		else
			radio->seek_mode = ctrl->val;

		FDEBUG(radio, "%s(), SEEK_MODE val:%d, ret : %d\n",
				__func__, ctrl->val,  ret);
		break;
	case V4L2_CID_S621_RDS_ON:
		payload = (u16)ctrl->val;
		if (payload & RDS_PARSER_ENABLE)
			radio->rds_parser_enable = TRUE;
		else
			radio->rds_parser_enable = FALSE;
		payload &= FM_RDS_ENABLE;
		ret = fm_set_rds_mode(radio, payload);
		FDEBUG(radio, "%s(), RDS_RECEPTION:%d, ret:%d parser:%d\n",
			__func__,
				payload, ret, radio->rds_parser_enable);
		if (ret != 0)
			ret = -EINVAL;
		break;
	case V4L2_CID_S621_SNR_CURR:
		ctrl->val = 0;
		break;
	case V4L2_CID_S621_IF_COUNT1:
		radio->low->fm_config.search_conf.weak_ifca_m =
				(u16)ctrl->val;
		FDEBUG(radio, "%s(), IF_CNT1  val:%d, ret : %d\n",
			__func__, ctrl->val,  ret);
		break;
	case V4L2_CID_S621_IF_COUNT2:
		radio->low->fm_config.search_conf.normal_ifca_m =
				(u16)ctrl->val;
		FDEBUG(radio, "%s(), IF_CNT2  val:%d, ret : %d\n",
			__func__, ctrl->val,  ret);
		break;
	case V4L2_CID_S621_RSSI_TH:
		low_set_search_lvl(radio, (u16)ctrl->val);
		FDEBUG(radio, "%s(), RSSI_TH val:%d, ret : %d\n",
				__func__, ctrl->val,  ret);
		break;
	case V4L2_CID_S621_KERNEL_VER:
		ctrl->val = 0;
		break;
	case V4L2_CID_S621_SOFT_STEREO_BLEND_REF:
		payload = (u16)ctrl->val;
		if (payload & RSSI_REF_ENABLE)
			radio->rssi_ref_enable = TRUE;
		else
			radio->rssi_ref_enable = FALSE;

		FDEBUG(radio, "%s(), SOFT_STEREO_BLEND_REF val:%d, ret : %d\n",
				__func__, ctrl->val,  ret);
		break;
	case V4L2_CID_S621_REG_RW_ADDR:
		radio->speedy_reg_addr = (u32)ctrl->val;
		radio->speedy_reg_addr |= 0x10000000;
		FDEBUG(radio, "%s(), REG_RW_ADDR  val:0x%xh, ret : %d\n",
			__func__, ctrl->val,  ret);
		break;
	case V4L2_CID_S621_REG_RW:
		if ((radio->speedy_reg_addr >= FM_SPEEDY_ADDR_MIN)
				&& (radio->speedy_reg_addr <= FM_SPEEDY_ADDR_MAX))
			fmspeedy_set_reg(radio->speedy_reg_addr, (u32)ctrl->val);
		else
			dev_err(radio->v4l2dev.dev,
				"%s(), V4L2_CID_S621_REG_RW, skip addr:0x%xh\n",
					__func__, radio->speedy_reg_addr);
		FDEBUG(radio, "%s(), REG_RW  val:0x%xh, ret : %d\n",
			__func__, ctrl->val, ret);
		break;
	default:
		ret = -EINVAL;
		dev_err(radio->v4l2dev.dev, "s_ctrl Unknown IOCTL: 0x%x\n",
				(int)ctrl->id);
		break;
	}

	s621_core_unlock(radio->core);
	FUNC_EXIT(radio);

	return ret;
}

#if IS_ENABLED(CONFIG_VIDEO_ADV_DEBUG)
static int s621_radio_g_register(struct file *file, void *fh,
		struct v4l2_dbg_register *reg)
{
	struct s621_radio *radio = video_drvdata(file);

	FUNC_ENTRY(radio);

	s621_core_lock(radio->core);

	reg->size = 4;
	reg->val = (__u64)fmspeedy_get_reg((u32)reg->reg);
	s621_core_unlock(radio->core);

	FUNC_EXIT(radio);

	return 0;
}
static int s621_radio_s_register(struct file *file, void *fh,
		const struct v4l2_dbg_register *reg)
{
	struct s621_radio *radio = video_drvdata(file);

	FUNC_ENTRY(radio);

	s621_core_lock(radio->core);
	fmspeedy_set_reg((u32)reg->reg, (u32)reg->val);
	s621_core_unlock(radio->core);

	FUNC_EXIT(radio);

	return 0;
}
#endif

int s621_core_set_power_state(struct s621_radio *radio,
		u8 next_state)
{
	int ret = 0;

	FUNC_ENTRY(radio);

	ret = low_set_power(radio, next_state);
	if (ret != 0) {
		dev_err(radio->v4l2dev.dev,
			"%s(), Failed to set for power state\n", __func__);
		ret = -EIO;
		goto ret_power;
	}
	radio->core->power_mode |= next_state;

ret_power:
	FUNC_EXIT(radio);
	return ret;
}

void  fm_prepare(struct s621_radio *radio)
{
	FUNC_ENTRY(radio);

	spin_lock_init(&radio->rds_buff_lock);

	/* Initial FM device variables  */
	/* Region info */
	radio->region = region_configs[radio_region];
	radio->mute_mode = FM_MUTE_OFF;
	radio->rf_depend_mute = FM_RX_RF_DEPENDENT_MUTE_OFF;
	radio->rds_flag = FM_RDS_DISABLE;
	radio->freq = FM_UNDEFINED_FREQ;
	radio->rds_mode = FM_RDS_SYSTEM_RDS;
	radio->af_mode = FM_RX_RDS_AF_SWITCH_MODE_OFF;
	radio->core->power_mode = S621_POWER_DOWN;
	radio->seek_weak_rssi = SCAN_WEAK_SIG_THRESHOLD;
	/* Reset RDS buffer cache if need */

	/* Initial wait queue for rds read */
	init_waitqueue_head(&radio->core->rds_read_queue);
	radio->idle_cnt_mod = 0;
	radio->rds_new_stat = 0;

	FUNC_EXIT(radio);

}

#ifdef USE_FM_LNA_ENABLE
static int set_eLNA_gpio(struct s621_radio *radio, int stat)
{
	if (radio->without_elna)
		return -EPERM;

	if (!gpio_is_valid(radio->elna_gpio))
		return -EINVAL;

	gpio_set_value(radio->elna_gpio, stat);
	dev_info(radio->v4l2dev.dev, "FM eLNA gpio:(%d)\n",
			gpio_get_value(radio->elna_gpio));

	return 0;
}
#endif /* USE_FM_LNA_ENABLE */

static int s621_radio_fops_open(struct file *file)
{
	int ret;
	struct s621_radio *radio = video_drvdata(file);

	FUNC_ENTRY(radio);

	ret = v4l2_fh_open(file);
	if (ret)
		return ret;

	if (v4l2_fh_is_singular_file(file)) {
#if IS_ENABLED(CONFIG_SCSC_FM)
		/* Start FM/WLBT LDO */
		ret = mx250_fm_request();
		if (ret < 0) {
			dev_err(radio->v4l2dev.dev,
				"mx250_fm_request() failed with err %d\n", ret);
			return ret;
		}
#endif /* CONFIG_SCSC_FM */
#ifdef USE_PMIC_RW
		if (radio->ldo_onoff) {
			if (radio->i2c_main || radio->i2c_sub) {
				ret = fm_ldo_enable(radio);
				if (ret < 0)
					dev_err(radio->v4l2dev.dev,
						"fm ldo enable failed with err %d\n", ret);
			}
		}
#endif /* USE_PMIC_RW */

#ifdef USE_FM_PIN_CONTROL
	fm_pinctrl_state_active(radio);
	if (radio->pin_control_available == true) {
		fm_pinctrl_state_elna(radio);
		radio->pin_control_available = false;
	}
#endif /* USE_FM_PIN_CONTROL */

#ifdef USE_FM_LNA_ENABLE
		if ((radio->without_elna != 1) && (radio->elna_gpio != -EINVAL)) {
			dev_info(radio->v4l2dev.dev,
				"%s(), FM eLNA gpio: %d\n",
			__func__, radio->elna_gpio);
			if (set_eLNA_gpio(radio, GPIO_HIGH))
				dev_info(radio->v4l2dev.dev,
				"Failed to set gpio HIGH for FM eLNA\n");
		}
#endif /* USE_FM_LNA_ENABLE */

		s621_core_lock(radio->core);
		atomic_set(&radio->is_doing, 0);
		atomic_set(&radio->is_rds_doing, 0);

#ifdef USE_AUDIO_PM
		if (radio->a_dev) {
			ret = pm_runtime_get_sync(radio->a_dev);
			if (ret < 0) {
				dev_err(radio->v4l2dev.dev,
					"audio get_sync failed with error %d\n", ret);
				goto err_open;
			}
			abox_request_power(radio->a_dev,
				ABOX_CPU_GEAR_CALL_KERNEL, "FM");
			dev_info(radio->v4l2dev.dev,
				"%s(), abox_request_power ok!\n", __func__);
		}
#endif /* USE_AUDIO_PM */

		ret = pm_runtime_get_sync(radio->dev);
		if (ret < 0) {
			dev_err(radio->v4l2dev.dev,
				"get_sync failed with err %d\n", ret);
			goto err_open;
		}

		fmspeedy_wakeup();
		fm_get_version_number();

		/* Initail fm low structure */
		ret = init_low_struc(radio);
		if (ret < 0) {
			dev_err(radio->v4l2dev.dev,
				"Failed to set for initial low struc\n");
			goto err_open;
		}

		/* Booting fm */
		ret = fm_boot(radio);
		if (ret < 0) {
			dev_err(radio->v4l2dev.dev,
				"Failed to set reg for FM boot\n");
			goto err_open;
		}

		fm_prepare(radio);

		ret = s621_core_set_power_state(radio,
				S621_POWER_ON_FM);
		if (ret < 0) {
			dev_err(radio->v4l2dev.dev,
				"Failed to set for power state FM on\n");
			goto err_open;
		}

		ret = s621_radio_pretune(radio);
		if (ret < 0) {
			dev_err(radio->v4l2dev.dev,
				"Failed to set for preturn\n");
			goto power_down;
		}

		radio->core->power_mode = S621_POWER_ON_FM;
		msleep(50);

		fm_speedy_m_int_disable();
		s621_core_unlock(radio->core);
		/*Must be done after s621_core_unlock to prevent a deadlock*/
		v4l2_ctrl_handler_setup(&radio->ctrl_handler);
	}

	FUNC_EXIT(radio);

	return ret;

power_down:
	s621_core_set_power_state(radio,
			S621_POWER_DOWN);

err_open:
	s621_core_unlock(radio->core);
	v4l2_fh_release(file);

	FUNC_EXIT(radio);

	return ret;

}

static int s621_radio_fops_release(struct file *file)
{
	int ret = 0;
	struct s621_radio *radio = video_drvdata(file);

	FUNC_ENTRY(radio);

	if (v4l2_fh_is_singular_file(file)) {
		s621_core_lock(radio->core);
		s621_core_set_power_state(radio, S621_POWER_DOWN);

		/* Speedy master interrupt disable */
		fm_speedy_m_int_disable();

		/* FM demod power off */
		fm_power_off();

		cancel_delayed_work_sync(&radio->dwork_sig2);
		cancel_delayed_work_sync(&radio->dwork_tune);

#ifdef	ENABLE_RDS_WORK_QUEUE
		cancel_work_sync(&radio->work);
#endif	/*ENABLE_RDS_WORK_QUEUE*/
#ifdef	IDLE_POLLING_ENABLE
		fm_idle_periodic_cancel((unsigned long) radio);
#endif	/*IDLE_POLLING_ENABLE*/
#ifdef	RDS_POLLING_ENABLE
		fm_rds_periodic_cancel((unsigned long) radio);
#endif	/*RDS_POLLING_ENABLE*/
/*#ifdef	ENABLE_IF_WORK_QUEUE*/
/*		cancel_work_sync(&radio->if_work);*/
/*#endif*/	/*ENABLE_IF_WORK_QUEUE*/

		pm_runtime_put_sync(radio->dev);
#ifdef USE_AUDIO_PM
		if (radio->a_dev) {
			abox_release_power(radio->a_dev,
				ABOX_CPU_GEAR_CALL_KERNEL, "FM");
			dev_info(radio->v4l2dev.dev,
				"%s(), abox_release_power ok!\n", __func__);

			pm_runtime_put_sync(radio->a_dev);
		}
#endif /* USE_AUDIO_PM */
		s621_core_unlock(radio->core);

#ifdef USE_FM_LNA_ENABLE
		if ((radio->without_elna != 1) && (radio->elna_gpio != -EINVAL)) {
			if (set_eLNA_gpio(radio, GPIO_LOW))
				dev_info(radio->v4l2dev.dev,
					"Failed to set gpio LOW for FM eLNA\n");
		}
#endif /* USE_FM_LNA_ENABLE */

#ifdef USE_FM_PIN_CONTROL
	fm_pinctrl_state_sleep(radio);
#endif /* USE_FM_PIN_CONTROL */

#ifdef USE_PMIC_RW
		if (radio->ldo_onoff) {
			if (radio->i2c_main || radio->i2c_sub) {
				ret = fm_ldo_disable(radio);
				if (ret < 0)
					dev_err(radio->v4l2dev.dev,
						"fm ldo disable failed with err %d\n", ret);
			}
		}
#endif /* USE_PMIC_RW */

#if IS_ENABLED(CONFIG_SCSC_FM)
		/* Stop FM/WLBT LDO */
		ret = mx250_fm_release();
		if (ret < 0)
			dev_err(radio->v4l2dev.dev,
				"mx250_fm_release() failed with err %d\n", ret);
#endif
	}

	if (wake_lock_active(&radio->wakelock))
		wake_unlock(&radio->wakelock);

	if (wake_lock_active(&radio->rdswakelock))
		wake_unlock(&radio->rdswakelock);

	ret = v4l2_fh_release(file);

	return ret;
}

static ssize_t s621_radio_fops_read(struct file *file, char __user *buf,
		size_t count, loff_t *ppos)
{
	int ret;
	size_t rsize, blocks;
	struct s621_radio *radio = video_drvdata(file);
	size_t rds_max_thresh;

	FUNC_ENTRY(radio);

	ret = wait_event_interruptible(radio->core->rds_read_queue,
			atomic_read(&radio->is_rds_new));
	if (ret < 0)
		return -EINTR;

	if (!atomic_read(&radio->is_rds_new)) {
		radio->rds_new_stat++;
		return -ERESTARTSYS;
	}

	if (s621_core_lock_interruptible(radio->core))
		return -ERESTARTSYS;

	if (radio->rds_parser_enable)
		rds_max_thresh = ringbuf_bytes_used(&radio->rds_rb);
	else
		rds_max_thresh = RDS_MEM_MAX_THRESH;

	/* Turn on RDS mode */
	memset(radio->rds_buf, 0, 480);

	rsize = rds_max_thresh;
	rsize = min(rsize, count);

	blocks = rsize/FM_RDS_BLK_SIZE;

	ret = fm_read_rds_data(radio, radio->rds_buf,
		(int)rsize, (u16 *)&blocks);
	if (ret == 0) {
		dev_err(radio->v4l2dev.dev, "%s(), Failed to read RDS mode\n", __func__);
		goto read_unlock;
	}

	/* always transfer rds complete blocks */
	if (copy_to_user(buf, &radio->rds_buf, rsize))
		ret = -EFAULT;

	if (radio->rds_parser_enable) {
		RDSEBUG(radio,
			"RDS RD done:%08d:%08d fifo err:%08d type(%02X) len(%02X)",
			radio->rds_read_cnt,
			radio->rds_n_count,
			radio->rds_fifo_err_cnt,
			radio->rds_buf[0],
			radio->rds_buf[1]);
	} else {
	if ((radio->rds_buf[2]&0x07) != RDS_BLKTYPE_A) {
		radio->rds_reset_cnt++;
		if (radio->rds_reset_cnt >
			radio->low->fm_state.rds_unsync_blk_cnt) {
			fm_set_rds_mode(radio, FM_RDS_DISABLE);
			usleep_range(10000, 10000);
			fm_set_rds_mode(radio, FM_RDS_ENABLE);
			radio->rds_reset_cnt = 0;
			RDSEBUG(radio,
				"RDS reset! cause of block type invalid");
		}
		RDSEBUG(radio, "RDS block type invalid! %02d, %08d",
			(radio->rds_buf[2]&0x07), radio->rds_reset_cnt);
	}

	RDSEBUG(radio,
		"RDS RD done:%08d:%08d fifo err:%08d block type0:%02X,%02X",
		radio->rds_read_cnt,
		radio->rds_n_count,
		radio->rds_fifo_err_cnt,
		(radio->rds_buf[2]&0x11)>>3,
		radio->rds_buf[2]&0x07);
	}

	radio->rds_read_cnt++;
	ret = rsize;
read_unlock:
	atomic_set(&radio->is_rds_new, 0);
	atomic_set(&gradio->is_doing, 0);
	atomic_set(&gradio->is_rds_doing, 0);

	s621_core_unlock(radio->core);

	FUNC_EXIT(radio);

	return ret;
}

static __poll_t s621_radio_fops_poll(struct file *file,
		struct poll_table_struct *pts)
{
	struct s621_radio *radio = video_drvdata(file);
	__poll_t req_events = poll_requested_events(pts);
	__poll_t ret = v4l2_ctrl_poll(file, pts);

	FUNC_ENTRY(radio);

	if (req_events & (EPOLLIN | EPOLLRDNORM)) {
		poll_wait(file, &radio->core->rds_read_queue, pts);

		if (atomic_read(&radio->is_rds_new))
			ret = EPOLLIN | EPOLLRDNORM;
	}

	FDEBUG(radio, "POLL RET: 0x%x pwmode: %d freq: %d",
		ret,
		radio->core->power_mode,
		radio->freq);
	FUNC_EXIT(radio);

	return ret;
}

static const struct v4l2_file_operations s621_fops = {
		.owner			= THIS_MODULE,
		.read			= s621_radio_fops_read,
		.poll			= s621_radio_fops_poll,
		.unlocked_ioctl		= video_ioctl2,
		.open			= s621_radio_fops_open,
		.release		= s621_radio_fops_release,
};

static const struct v4l2_ioctl_ops S621_ioctl_ops = {
		.vidioc_querycap		= s621_radio_querycap,
		.vidioc_g_tuner			= s621_radio_g_tuner,
		.vidioc_s_tuner			= s621_radio_s_tuner,

		.vidioc_g_frequency		= s621_radio_g_frequency,
		.vidioc_s_frequency		= s621_radio_s_frequency,
		.vidioc_s_hw_freq_seek		= s621_radio_s_hw_freq_seek,
		.vidioc_enum_freq_bands		= s621_radio_enum_freq_bands,

		.vidioc_subscribe_event		= v4l2_ctrl_subscribe_event,
		.vidioc_unsubscribe_event	= v4l2_event_unsubscribe,

#if IS_ENABLED(CONFIG_VIDEO_ADV_DEBUG)
		.vidioc_g_register		= s621_radio_g_register,
		.vidioc_s_register		= s621_radio_s_register,
#endif
};

static const struct video_device s621_viddev_template = {
		.fops			= &s621_fops,
		.name			= DRIVER_NAME,
		.release		= video_device_release_empty,
};

static int s621_radio_add_new_custom(struct s621_radio *radio,
		enum s621_ctrl_idx idx, unsigned long flag)
{
	int ret;
	struct v4l2_ctrl *ctrl;

	ctrl = v4l2_ctrl_new_custom(&radio->ctrl_handler,
			&s621_ctrls[idx],
			NULL);

	ret = radio->ctrl_handler.error;
	if (ctrl == NULL && ret) {
		dev_err(radio->v4l2dev.dev,
				"Could not initialize '%s' control %d\n",
				s621_ctrls[idx].name, ret);
		return -EINTR;
	}
	if (flag && (ctrl != NULL))
		ctrl->flags |= flag;

	return ret;
}

MODULE_ALIAS("platform:s621-radio");
#if IS_ENABLED(CONFIG_SOC_S5E8535)
static const struct of_device_id exynos_fm_of_match[] = {
		{
				.compatible = "samsung,exynos8535-fm",
		},
		{},
};
#endif /* CONFIG_SOC_S5E8535 */
#if IS_ENABLED(CONFIG_SOC_S5E8835)
static const struct of_device_id exynos_fm_of_match[] = {
		{
				.compatible = "samsung,exynos8835-fm",
		},
		{},
};
#endif /* CONFIG_SOC_S5E8835 */
#if IS_ENABLED(CONFIG_SOC_S5E8845)
static const struct of_device_id exynos_fm_of_match[] = {
		{
				.compatible = "samsung,exynos8845-fm",
		},
		{},
};
#endif /* CONFIG_SOC_S5E8845 */
MODULE_DEVICE_TABLE(of, exynos_fm_of_match);

#ifdef USE_AUDIO_PM
static struct device_node *exynos_audio_parse_dt(struct s621_radio *radio)
{
	struct platform_device *pdev = NULL;
	struct device_node *np = NULL;

	np = of_find_compatible_node(NULL, NULL, "samsung,abox");
	if (!np) {
		dev_err(radio->dev, "abox device is not available\n");
		return NULL;
	}

	pdev = of_find_device_by_node(np);
	if (!pdev) {
		dev_err(radio->dev,
			"%s: failed to get audio platform_device\n",
			__func__);
		return NULL;
	}
	radio->a_dev = &pdev->dev;

	return np;
}
#endif /* USE_AUDIO_PM */

#ifdef USE_PMIC_RW
/*
PMIC: S2MPU15X01(MAIN)
      S2MPU16X01(SUB)
Rice		            Quart		            Rose
L26M_VDD_WLBT_1P8	    L8S_VDD_WLBT_1P8	    L8S_AVDD_WLBT_1P8
L27M_VDD_WLBT_1P1	    L27M_VDD_WLBT_1P1	    L6S_AVDD_WLBT_1P1
*/
#if IS_ENABLED(CONFIG_SOC_S5E8535)
#define VDD_1P8_ADDR 	0xCE 	/* ADDR_LDO26M_CTRL */
#define VDD_1P1_ADDR  	0xCF	/* ADDR_LDO27M_CTRL */
#endif /* CONFIG_SOC_S5E8535 */
#if IS_ENABLED(CONFIG_SOC_S5E8835)
#define VDD_1P8_ADDR 	0xCF 	/* ADDR_LDO27M_CTRL */
#define VDD_1P1_ADDR  	0xBD	/* ADDR_LDO8S_CTRL */
#endif /* CONFIG_SOC_S5E8835 */
#if IS_ENABLED(CONFIG_SOC_S5E8845)
#define VDD_1P8_ADDR  	0xBD	/* ADDR_LDO8S_CTRL */
#define VDD_1P1_ADDR  	0xB9	/* ADDR_LDO6S_CTRL */
#endif /* CONFIG_SOC_S5E8845 */

static int pmic_get_i2c(struct s621_radio *radio);

#if IS_ENABLED(CONFIG_EXYNOS_AFM)
extern int main_pmic_read_reg(struct i2c_client *i2c, uint8_t reg, uint8_t *val);
extern int main_pmic_update_reg(struct i2c_client *i2c, uint8_t reg, uint8_t val, uint8_t mask);
extern int main_pmic_get_i2c(struct i2c_client **i2c);

extern int sub_pmic_read_reg(struct i2c_client *i2c, uint8_t reg, uint8_t *val);
extern int sub_pmic_update_reg(struct i2c_client *i2c, uint8_t reg, uint8_t val, uint8_t mask);
extern int sub_pmic_get_i2c(struct i2c_client **i2c);

#define MAIN_PMIC_GET_I2C	main_pmic_get_i2c
#define SUB_PMIC_GET_I2C	sub_pmic_get_i2c
#if IS_ENABLED(CONFIG_SOC_S5E8535)
#define VDD_1P8_PMIC_I2C 		i2c_main
#define VDD_1P8_PMIC_UPDATE		main_pmic_update_reg
#define VDD_1P8_PMIC_READ		main_pmic_read_reg

#define VDD_1P1_PMIC_I2C		i2c_main
#define VDD_1P1_PMIC_UPDATE		main_pmic_update_reg
#define VDD_1P1_PMIC_READ		main_pmic_read_reg
#endif /* CONFIG_SOC_S5E8535 */

#if IS_ENABLED(CONFIG_SOC_S5E8835)
#define VDD_1P8_PMIC_I2C 		i2c_main
#define VDD_1P8_PMIC_UPDATE		main_pmic_update_reg
#define VDD_1P8_PMIC_READ		main_pmic_read_reg

#define VDD_1P1_PMIC_I2C		i2c_sub
#define VDD_1P1_PMIC_UPDATE		sub_pmic_update_reg
#define VDD_1P1_PMIC_READ		sub_pmic_read_reg
#endif /* CONFIG_SOC_S5E8835 */

#if IS_ENABLED(CONFIG_SOC_S5E8845)
#define VDD_1P8_PMIC_I2C 		i2c_sub
#define VDD_1P8_PMIC_UPDATE		sub_pmic_update_reg
#define VDD_1P8_PMIC_READ		sub_pmic_read_reg

#define VDD_1P1_PMIC_I2C		i2c_sub
#define VDD_1P1_PMIC_UPDATE		sub_pmic_update_reg
#define VDD_1P1_PMIC_READ		sub_pmic_read_reg
#endif /* CONFIG_SOC_S5E8845 */
#else /* IS_ENABLED(CONFIG_EXYNOS_AFM) */
extern int s2mpu15_read_reg(struct i2c_client *i2c, uint8_t reg, uint8_t *val);
extern int s2mpu15_update_reg(struct i2c_client *i2c, uint8_t reg, uint8_t val, uint8_t mask);

extern int s2mpu16_read_reg(struct i2c_client *i2c, uint8_t reg, uint8_t *val);
extern int s2mpu16_update_reg(struct i2c_client *i2c, uint8_t reg, uint8_t val, uint8_t mask);

static int exynos_pmic_parse_dt_main(struct i2c_client **i2c);
static int exynos_pmic_parse_dt_sub(struct i2c_client **i2c);

#define MAIN_PMIC_GET_I2C	exynos_pmic_parse_dt_main
#define SUB_PMIC_GET_I2C	exynos_pmic_parse_dt_sub
#if IS_ENABLED(CONFIG_SOC_S5E8535)
#define VDD_1P8_PMIC_I2C 		i2c_main
#define VDD_1P8_PMIC_UPDATE		s2mpu15_update_reg
#define VDD_1P8_PMIC_READ		s2mpu15_read_reg

#define VDD_1P1_PMIC_I2C		i2c_main
#define VDD_1P1_PMIC_UPDATE		s2mpu15_update_reg
#define VDD_1P1_PMIC_READ		s2mpu15_read_reg
#endif /* CONFIG_SOC_S5E8535 */

#if IS_ENABLED(CONFIG_SOC_S5E8835)
#define VDD_1P8_PMIC_I2C 		i2c_main
#define VDD_1P8_PMIC_UPDATE		s2mpu15_update_reg
#define VDD_1P8_PMIC_READ		s2mpu15_read_reg

#define VDD_1P1_PMIC_I2C		i2c_sub
#define VDD_1P1_PMIC_UPDATE		s2mpu16_update_reg
#define VDD_1P1_PMIC_READ		s2mpu16_read_reg
#endif /* CONFIG_SOC_S5E8835 */

#if IS_ENABLED(CONFIG_SOC_S5E8845)
#define VDD_1P8_PMIC_I2C 		i2c_sub
#define VDD_1P8_PMIC_UPDATE		s2mpu16_update_reg
#define VDD_1P8_PMIC_READ		s2mpu16_read_reg

#define VDD_1P1_PMIC_I2C		i2c_sub
#define VDD_1P1_PMIC_UPDATE		s2mpu16_update_reg
#define VDD_1P1_PMIC_READ		s2mpu16_read_reg
#endif /* CONFIG_SOC_S5E8845 */

struct s2mpu_dev {
	struct device *dev;
	struct s2mpu16_platform_data *pdata;
	struct i2c_client *i2c;
	struct i2c_client *vgpio;
	struct i2c_client *pmic1;
	struct i2c_client *pmic2;
};

static struct i2c_client *exynos_pmic_parse_dt_str(char *str_pmic)
{
	struct device_node *np = NULL;
	struct i2c_client *client;
	struct s2mpu_dev *s2mpu;

	np = of_find_compatible_node(NULL, NULL, str_pmic);
	if (!np) {
		dev_err(gradio->dev, "%s PMIC device is not available\n", str_pmic);
		return NULL;
	}
	client = of_find_i2c_device_by_node(np);
	if (!client) {
		dev_err(gradio->dev, "%s PMIC i2s is not available\n", str_pmic);
		return NULL;
	}
	s2mpu = i2c_get_clientdata(client);

	return s2mpu->pmic1;
}

#define MAIN_PMIC_STR	"samsung,s2mpu15mfd"
#define SUB_PMIC_STR	"samsung,s2mpu16mfd"
static int exynos_pmic_parse_dt_main(struct i2c_client **i2c)
{
	*i2c = exynos_pmic_parse_dt_str(MAIN_PMIC_STR);
	if (!*i2c)
		return -ENODEV;

	return 0;
}

static int exynos_pmic_parse_dt_sub(struct i2c_client **i2c)
{
	*i2c = exynos_pmic_parse_dt_str(SUB_PMIC_STR);
	if (!*i2c)
		return -ENODEV;

	return 0;
}
#endif	/* IS_ENABLED(CONFIG_EXYNOS_AFM) */

static int pmic_get_i2c(struct s621_radio *radio)
{
	int ret = 0;

	ret = MAIN_PMIC_GET_I2C(&radio->i2c_main);
	if (ret < 0) {
		dev_err(radio->dev, "%s: failed to get PMIC main i2c\n", __func__);
	}
	dev_info(radio->dev, "%s: get PMIC main i2c\n", __func__);

	ret = SUB_PMIC_GET_I2C(&radio->i2c_sub);
	if (ret < 0) {
		dev_err(radio->dev, "%s: failed to get PMIC sub i2c\n", __func__);
	}
	dev_info(radio->dev, "%s: get PMIC sub i2c\n", __func__);

	return ret;
}

/* BUCK/LDO Enable control[7:6] */
#define S2MPU1X_ENABLE_SHIFT	0x06
#define S2MPU1X_ENABLE_MASK 	(0x03 << S2MPU1X_ENABLE_SHIFT)
#define S2MPU1X_SEL_VGPIO_ON	(0x01 << S2MPU1X_ENABLE_SHIFT)
#define S2MPU1X_SEL_ALWAYS_ON	(0x03 << S2MPU1X_ENABLE_SHIFT)

int fm_ldo_enable(struct s621_radio *radio)
{
	int ret = 0;

	VDD_1P8_PMIC_READ(radio->VDD_1P8_PMIC_I2C, VDD_1P8_ADDR, &radio->vdd_1p8);
	dev_info(radio->dev, "%s:read) read VDD_1P8_ADDR[%xh]: %xh\n", __func__, VDD_1P8_ADDR, radio->vdd_1p8);

	VDD_1P1_PMIC_READ(radio->VDD_1P1_PMIC_I2C, VDD_1P1_ADDR, &radio->vdd_1p1);
	dev_info(radio->dev, "%s:read) read VDD_1P1_ADDR[%xh]: %xh\n", __func__, VDD_1P1_ADDR, radio->vdd_1p1);

	/* VDD_1P8_PMIC VDD_1P8_ADDR Always On 0xD2 */
	ret = VDD_1P8_PMIC_UPDATE(radio->VDD_1P8_PMIC_I2C, VDD_1P8_ADDR, S2MPU1X_SEL_ALWAYS_ON, S2MPU1X_ENABLE_MASK);
	if (ret < 0) {
		dev_err(radio->dev, "%s: failed to update VDD_1P8_PMIC register\n", __func__);
		return -1;
	}

	/* VDD_1P1_PMIC VDD_1P1_ADDR Always On 0xD8 */
	ret = VDD_1P1_PMIC_UPDATE(radio->VDD_1P1_PMIC_I2C, VDD_1P1_ADDR, S2MPU1X_SEL_ALWAYS_ON, S2MPU1X_ENABLE_MASK);
	if (ret < 0) {
		dev_err(radio->dev, "%s: failed to update VDD_1P1_PMIC register\n", __func__);
		return -1;
	}

	VDD_1P8_PMIC_READ(radio->VDD_1P8_PMIC_I2C, VDD_1P8_ADDR, &radio->vdd_1p8_update);
	dev_info(radio->dev, "%s:update) read VDD_1P8_ADDR[%xh]: %xh\n", __func__, VDD_1P8_ADDR, radio->vdd_1p8_update);

	VDD_1P1_PMIC_READ(radio->VDD_1P1_PMIC_I2C, VDD_1P1_ADDR, &radio->vdd_1p1_update);
	dev_info(radio->dev, "%s:update) read VDD_1P1_ADDR[%xh]: %xh\n", __func__, VDD_1P1_ADDR, radio->vdd_1p1_update);

	return ret;
}

int fm_ldo_disable(struct s621_radio *radio)
{
	int ret = 0;

	VDD_1P8_PMIC_READ(radio->VDD_1P8_PMIC_I2C, VDD_1P8_ADDR, &radio->vdd_1p8);
	dev_info(radio->dev, "%s:read) read VDD_1P8_ADDR[%xh]: %xh\n", __func__, VDD_1P8_ADDR, radio->vdd_1p8);

	VDD_1P1_PMIC_READ(radio->VDD_1P1_PMIC_I2C, VDD_1P1_ADDR, &radio->vdd_1p1);
	dev_info(radio->dev, "%s:read) read VDD_1P1_ADDR[%xh]: %xh\n", __func__, VDD_1P1_ADDR, radio->vdd_1p1);

	/* VDD_1P8_PMIC VDD_1P8_ADDR On/off by SEL_VGPIO 0x52 */
	ret = VDD_1P8_PMIC_UPDATE(radio->VDD_1P8_PMIC_I2C, VDD_1P8_ADDR, S2MPU1X_SEL_VGPIO_ON, S2MPU1X_ENABLE_MASK);
	if (ret < 0) {
		dev_err(radio->dev, "%s: failed to update VDD_1P8_PMIC register\n", __func__);
		return -1;
	}

	/* VDD_1P1_PMIC VDD_1P1_ADDR On/off by SEL_VGPIO 0x58 */
	ret = VDD_1P1_PMIC_UPDATE(radio->VDD_1P1_PMIC_I2C, VDD_1P1_ADDR, S2MPU1X_SEL_VGPIO_ON, S2MPU1X_ENABLE_MASK);
	if (ret < 0) {
		dev_err(radio->dev, "%s: failed to update VDD_1P1_PMIC register\n", __func__);
		return -1;
	}

	VDD_1P8_PMIC_READ(radio->VDD_1P8_PMIC_I2C, VDD_1P8_ADDR, &radio->vdd_1p8_update);
	dev_info(radio->dev, "%s:update) read VDD_1P8_ADDR[%xh]: %xh\n", __func__, VDD_1P8_ADDR, radio->vdd_1p8_update);

	VDD_1P1_PMIC_READ(radio->VDD_1P1_PMIC_I2C, VDD_1P1_ADDR, &radio->vdd_1p1_update);
	dev_info(radio->dev, "%s:update) read VDD_1P1_ADDR[%xh]: %xh\n", __func__, VDD_1P1_ADDR, radio->vdd_1p1_update);

	return ret;
}
#endif /* USE_PMIC_RW */

#ifdef USE_FM_PIN_CONTROL
static int fm_pinctrl_state_active(struct s621_radio *radio)
{
	int ret = 0;

	ret = pinctrl_select_state(radio->pinctrl_fm, radio->pinctrl_state_active);
	if (ret)
		dev_err(radio->dev, "failed to init %s pinctrl state\n", __func__);

	dev_info(radio->dev, "%s: select pinctrl state active\n", __func__);

	return ret;
}

static int fm_pinctrl_state_sleep(struct s621_radio *radio)
{
	int ret = 0;

	ret = pinctrl_select_state(radio->pinctrl_fm, radio->pinctrl_state_sleep);
	if (ret)
		dev_err(radio->dev, "failed to init %s pinctrl state\n", __func__);

	dev_info(radio->dev, "%s: select pinctrl state sleep\n", __func__);

	return ret;
}

static int fm_pinctrl_state_elna(struct s621_radio *radio)
{
	int ret = 0;

	if (radio->without_elna != 1) {
		ret = pinctrl_select_state(radio->pinctrl_fm, radio->pinctrl_state_elna);
		if (ret)
			dev_err(radio->dev, "failed to init %s pinctrl state\n", __func__);

		dev_info(radio->dev, "%s: select pinctrl state elna\n", __func__);
	}

	return ret;
}

static int fm_init_pinctrl_info(struct s621_radio *radio)
{
	int ret = 0;

	radio->pinctrl_fm = devm_pinctrl_get(radio->dev);
	if (IS_ERR(radio->pinctrl_fm)) {
		dev_err(radio->dev, "No pinctrl info, err: %ld\n",
			PTR_ERR(radio->pinctrl_fm));
		return PTR_ERR(radio->pinctrl_fm);
	}
	dev_info(radio->dev, "%s: get pinctrl info\n", __func__);

	radio->pinctrl_state_active =
		pinctrl_lookup_state(radio->pinctrl_fm, "active");
	if (IS_ERR(radio->pinctrl_state_active)) {
		dev_err(radio->dev, "Missing active state, err: %ld\n",
			 PTR_ERR(radio->pinctrl_state_active));
		return PTR_ERR(radio->pinctrl_fm);
	}
	dev_info(radio->dev, "%s: get pinctrl state active\n", __func__);

	radio->pinctrl_state_sleep =
		pinctrl_lookup_state(radio->pinctrl_fm, "sleep");
	if (IS_ERR(radio->pinctrl_state_sleep)) {
		dev_err(radio->dev, "Missing active state, err: %ld\n",
			 PTR_ERR(radio->pinctrl_state_sleep));
		return PTR_ERR(radio->pinctrl_fm);
	}
	dev_info(radio->dev, "%s: get pinctrl state sleep\n", __func__);

	if (radio->without_elna != 1) {
		radio->pinctrl_state_elna =
			pinctrl_lookup_state(radio->pinctrl_fm, "eLNA");
		if (IS_ERR(radio->pinctrl_state_elna)) {
			dev_err(radio->dev, "Missing active state, err: %ld\n",
				 PTR_ERR(radio->pinctrl_state_elna));
			return PTR_ERR(radio->pinctrl_fm);
		}
		dev_info(radio->dev, "%s: get pinctrl state elna\n", __func__);
	}

	radio->pin_control_available = true;

	return ret;
}
#endif /* USE_FM_PIN_CONTROL */

static int s621_radio_probe(struct platform_device *pdev)
{
	unsigned long ret;
	struct s621_radio *radio;
	struct v4l2_ctrl *ctrl;
	const struct of_device_id *match;
	struct resource *resource;
	struct device *dev = &pdev->dev;
	static atomic_t instance = ATOMIC_INIT(0);
	struct device_node *dnode;

	dnode = dev->of_node;

	dev_info(&pdev->dev, ">>> start FM Radio probe.\n");

	radio = devm_kzalloc(&pdev->dev, sizeof(*radio), GFP_KERNEL);
	if (!radio)
		return -ENOMEM;

	radio->core = devm_kzalloc(&pdev->dev, sizeof(struct s621_core),
			GFP_KERNEL);
	if (!radio->core) {
		ret =  -ENOMEM;
		goto alloc_err0;
	}

	radio->low = devm_kzalloc(&pdev->dev, sizeof(struct s621_low),
			GFP_KERNEL);
	if (!radio->low) {
		ret =  -ENOMEM;
		goto alloc_err1;
	}

	radio->dev = dev;
	radio->pdev = pdev;
	gradio = radio;

	ret = fm_clk_get(radio);
	if (ret)
		goto alloc_err2;

	ret = fm_clk_prepare(radio);
	if (ret)
		goto alloc_err2;

	ret = fm_clk_enable(radio);
	if (ret) {
		fm_clk_unprepare(radio);
		goto alloc_err2;
	}

	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);

	spin_lock_init(&radio->slock);
	spin_lock_init(&radio->rds_lock);

	/* Init flags FR BL init_completion */
	init_completion(&radio->flags_set_fr_comp);
	init_completion(&radio->flags_seek_fr_comp);

	v4l2_device_set_name(&radio->v4l2dev, DRIVER_NAME, &instance);
	dev_info(&pdev->dev, "v4l2 device name:%s\n", radio->v4l2dev.name);

	ret = v4l2_device_register(&pdev->dev, &radio->v4l2dev);
	if (ret) {
		dev_err(&pdev->dev, "Cannot register v4l2_device.\n");
		goto alloc_err3;
	}

	match = of_match_node(exynos_fm_of_match, dev->of_node);
	if (!match) {
		dev_err(&pdev->dev, "failed to match node\n");
		ret =  -EINVAL;
		goto alloc_err4;
	}

	resource = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	radio->fmspeedy_base = devm_ioremap_resource(&pdev->dev, resource);
	if (IS_ERR(radio->fmspeedy_base)) {
		dev_err(&pdev->dev,
			"Failed to request memory region\n");
		ret = -EBUSY;
		goto alloc_err4;
	}

#ifdef USE_AUDIO_PM
	if (!exynos_audio_parse_dt(radio)) {
		ret = -EINVAL;
		goto alloc_err4;
	}
#endif /* USE_AUDIO_PM */

#ifdef USE_PMIC_RW
	ret = pmic_get_i2c(radio);
	if (ret < 0)
		dev_err(radio->dev, "%s: failed to get PMIC i2c\n", __func__);
#endif /* USE_PMIC_RW */

	memcpy(&radio->videodev, &s621_viddev_template,
			sizeof(struct video_device));

	radio->videodev.v4l2_dev  = &radio->v4l2dev;
	radio->videodev.ioctl_ops = &S621_ioctl_ops;
	radio->videodev.device_caps = V4L2_CAP_TUNER
				| V4L2_CAP_RADIO | V4L2_CAP_RDS_CAPTURE
				| V4L2_CAP_READWRITE | V4L2_CAP_HW_FREQ_SEEK;
	video_set_drvdata(&radio->videodev, radio);
	platform_set_drvdata(pdev, radio);

	radio->v4l2dev.ctrl_handler = &radio->ctrl_handler;

	v4l2_ctrl_handler_init(&radio->ctrl_handler,
			1 + ARRAY_SIZE(s621_ctrls));
	/* Set control */
	ret = s621_radio_add_new_custom(radio, S621_IDX_CH_SPACING,
			0); /*0x01*/
	if (ret < 0)
		goto exit;
	ret = s621_radio_add_new_custom(radio, S621_IDX_CH_BAND,
			0); /*0x02*/
	if (ret < 0)
		goto exit;
	ret = s621_radio_add_new_custom(radio, S621_IDX_SOFT_STEREO_BLEND,
			0); /*0x03*/
	if (ret < 0)
		goto exit;
	ret = s621_radio_add_new_custom(radio, S621_IDX_SOFT_STEREO_BLEND_COEFF,
			0); /*0x04*/
	if (ret < 0)
		goto exit;
	ret = s621_radio_add_new_custom(radio, S621_IDX_SOFT_MUTE_COEFF,
			0); /*0x05*/
	if (ret < 0)
		goto exit;
	ret = s621_radio_add_new_custom(radio, S621_IDX_RSSI_CURR,
			V4L2_CTRL_FLAG_VOLATILE); /*0x06*/
	if (ret < 0)
		goto exit;
	ret = s621_radio_add_new_custom(radio, S621_IDX_SNR_CURR,
			V4L2_CTRL_FLAG_VOLATILE); /*0x07*/
	if (ret < 0)
		goto exit;
	ret = s621_radio_add_new_custom(radio, S621_IDX_SEEK_CANCEL,
			V4L2_CTRL_FLAG_EXECUTE_ON_WRITE); /*0x08*/
	if (ret < 0)
		goto exit;
	ret = s621_radio_add_new_custom(radio, S621_IDX_SEEK_MODE,
			0); /*0x09*/
	if (ret < 0)
		goto exit;
	ret = s621_radio_add_new_custom(radio, S621_IDX_RDS_ON,
			0); /*0x0A*/
	if (ret < 0)
		goto exit;
	ret = s621_radio_add_new_custom(radio, S621_IDX_IF_COUNT1,
			0); /*0x0B*/
	if (ret < 0)
		goto exit;
	ret = s621_radio_add_new_custom(radio, S621_IDX_IF_COUNT2,
			0); /*0x0C*/
	if (ret < 0)
		goto exit;
	ret = s621_radio_add_new_custom(radio, S621_IDX_RSSI_TH,
			0); /*0x0D*/
	if (ret < 0)
		goto exit;
	ret = s621_radio_add_new_custom(radio, S621_IDX_KERNEL_VER,
			V4L2_CTRL_FLAG_VOLATILE); /*0x0E*/
	if (ret < 0)
		goto exit;
	ret = s621_radio_add_new_custom(radio, S621_IDX_SOFT_STEREO_BLEND_REF,
			V4L2_CTRL_FLAG_EXECUTE_ON_WRITE|V4L2_CTRL_FLAG_VOLATILE); /*0x0F*/
	if (ret < 0)
		goto exit;
	ret = s621_radio_add_new_custom(radio, S621_IDX_REG_RW_ADDR,
		V4L2_CTRL_FLAG_EXECUTE_ON_WRITE|V4L2_CTRL_FLAG_VOLATILE); /*0x10*/
	if (ret < 0)
		goto exit;
	ret = s621_radio_add_new_custom(radio, S621_IDX_REG_RW,
		V4L2_CTRL_FLAG_EXECUTE_ON_WRITE|V4L2_CTRL_FLAG_VOLATILE); /*0x11*/
	if (ret < 0)
		goto exit;

	ctrl = v4l2_ctrl_new_std(&radio->ctrl_handler, &s621_ctrl_ops,
			V4L2_CID_AUDIO_MUTE, 0, 4, 1, 1);
	ret = radio->ctrl_handler.error;
	if (ctrl == NULL && ret) {
		dev_err(&pdev->dev,
			"Could not initialize V4L2_CID_AUDIO_MUTE control %d\n",
			(int)ret);
		goto exit;
	}

	ctrl = v4l2_ctrl_new_std(&radio->ctrl_handler, &s621_ctrl_ops,
			V4L2_CID_AUDIO_VOLUME, 0, 15, 1, 1);
	ret = radio->ctrl_handler.error;
	if (ctrl == NULL && ret) {
		dev_err(&pdev->dev,
			"Could not initialize V4L2_CID_AUDIO_VOLUME control %d\n",
			(int)ret);
		goto exit;
	}

	ctrl = v4l2_ctrl_new_std_menu(&radio->ctrl_handler, &s621_ctrl_ops,
			V4L2_CID_TUNE_DEEMPHASIS,
			V4L2_DEEMPHASIS_75_uS, 0, V4L2_PREEMPHASIS_75_uS);
	ret = radio->ctrl_handler.error;
	if (ctrl == NULL && ret) {
		dev_err(&pdev->dev,
			"Could not initialize V4L2_CID_TUNE_DEEMPHASIS control %d\n",
			(int)ret);
		goto exit;
	}

	/* register video device */
	ret = video_register_device(&radio->videodev, VFL_TYPE_RADIO, -1);
	if (ret < 0) {
		dev_err(&pdev->dev, "Could not register video device\n");
		goto exit;
	}

	mutex_init(&radio->lock);
	s621_core_lock_init(radio->core);

	/*init_waitqueue_head(&radio->core->rds_read_queue);*/

	INIT_DELAYED_WORK(&radio->dwork_sig2, s621_sig2_work);
	INIT_DELAYED_WORK(&radio->dwork_tune, s621_tune_work);
#ifdef	RDS_POLLING_ENABLE
	INIT_DELAYED_WORK(&radio->dwork_rds_poll, s621_rds_poll_work);
#endif	/*RDS_POLLING_ENABLE*/
#ifdef	IDLE_POLLING_ENABLE
	INIT_DELAYED_WORK(&radio->dwork_idle_poll, s621_idle_poll_work);
#endif	/*IDLE_POLLING_ENABLE*/

#ifdef	ENABLE_RDS_WORK_QUEUE
	INIT_WORK(&radio->work, s621_rds_work);
#endif	/*ENABLE_RDS_WORK_QUEUE*/

	radio->rfchip_ver = S621_REV_0;
	dev_info(&pdev->dev, "Config RADIO_S621:[%08X] radio driver for RF Chip S621\n", radio->rfchip_ver);

	ret = of_property_read_u32(dnode, "fm_iclk_aux", &radio->iclkaux);
	if (ret)
		radio->iclkaux = 1;
	dev_info(radio->dev, "iClk Aux: %d\n", radio->iclkaux);

	ret = of_property_read_u32(dnode, "num-tcon-freq", &radio->tc_on);
	if (ret) {
		radio->tc_on = 0;
		goto skip_tc_off;
	}

	fm_spur_init = devm_kzalloc(&pdev->dev,
		radio->tc_on * sizeof(*fm_spur_init),
		GFP_KERNEL);
	if (!fm_spur_init) {
		dev_err(radio->dev, "Mem alloc failed for TC ON freq values, TC off\n");
		radio->tc_on = 0;
		goto skip_tc_off;
	}

	if (of_property_read_u32_array(dnode, "val-tcon-freq",
		fm_spur_init,
		radio->tc_on)) {
		dev_err(radio->dev, "Getting val-tcon-freq values faild, TC off\n");
		radio->tc_on = 0;
		goto skip_tc_off;
	}
	dev_info(radio->dev, "number TC On Freq: %d\n", radio->tc_on);
skip_tc_off:

	ret = of_property_read_u32(dnode, "num-trfon-freq", &radio->trf_on);
	if (ret) {
		radio->trf_on = 0;
		goto skip_trf_off;
	}

	fm_spur_trf_init = devm_kzalloc(&pdev->dev,
		radio->trf_on * sizeof(*fm_spur_trf_init),
					GFP_KERNEL);
	if (!fm_spur_trf_init) {
		dev_err(radio->dev, "Mem alloc failed for TRF ON freq values, TRF off\n");
		radio->trf_on = 0;
		goto skip_trf_off;
	}

	if (of_property_read_u32_array(dnode, "val-trfon-freq",
		fm_spur_trf_init,
		radio->trf_on)) {
		dev_err(radio->dev, "Getting val-trfon-freq values failed, TRF off\n");
		radio->trf_on = 0;
		goto skip_trf_off;
	}
	dev_info(radio->dev, "number TRF On Freq: %d\n", radio->trf_on);

	ret = of_property_read_u32(dnode, "spur-trfon-freq", &radio->trf_spur);
	if (ret)
		radio->trf_spur = 0;

skip_trf_off:

	ret = of_property_read_u32(dnode, "num-dual-clkon-freq",
			&radio->dual_clk_on);
	if (ret) {
		radio->dual_clk_on = 0;
		goto skip_dual_clk_off;
	}

	fm_dual_clk_init = devm_kzalloc(&pdev->dev,
		radio->dual_clk_on * sizeof(*fm_dual_clk_init),
					GFP_KERNEL);
	if (!fm_dual_clk_init) {
		dev_err(radio->dev, "Mem alloc failed for DUAL CLK ON freq values, DUAL CLK off\n");
		radio->dual_clk_on = 0;
		goto skip_dual_clk_off;
	}

	if (of_property_read_u32_array(dnode, "val-dual-clkon-freq",
		fm_dual_clk_init,
		radio->dual_clk_on)) {
		dev_err(radio->dev,
			"Getting val-dual-clkon-freq values failed, DUAL CLK off\n");
		radio->dual_clk_on = 0;
		goto skip_dual_clk_off;
	}
	dev_info(radio->dev, "number DUAL CLK On Freq: %d\n",
		radio->dual_clk_on);
skip_dual_clk_off:

	radio->vol_num = FM_RX_VOLUME_GAIN_STEP;
	radio->vol_level_mod = vol_level_init;

	ret = of_property_read_u32(dnode, "num-volume-level", &radio->vol_num);
	if (ret)
		goto skip_vol_sel;

	radio->vol_level_tmp = devm_kzalloc(&pdev->dev,
		radio->vol_num * sizeof(u32),
					GFP_KERNEL);
	if (!radio->vol_level_tmp) {
		dev_err(radio->dev, "Mem alloc failed for Volume level values, Volume Level default setting\n");
		goto skip_vol_sel;
	}

	if (of_property_read_u32_array(dnode, "val-vol-level",
		radio->vol_level_tmp, radio->vol_num)) {
		dev_err(radio->dev,
			"Getting val-vol-level values failed, Volume Level default set\n");
		kfree(radio->vol_level_tmp);
		radio->vol_num = FM_RX_VOLUME_GAIN_STEP;
		goto skip_vol_sel;
	}
	radio->vol_level_mod = radio->vol_level_tmp;

skip_vol_sel:
	dev_info(radio->dev, "volume select num: %d\n", radio->vol_num);

	ret = of_property_read_u32(dnode, "vol_3db_on", &radio->vol_3db_att);
	if (ret)
		radio->vol_3db_att = 0;
	dev_info(radio->dev, "volume -3dB: %d\n", radio->vol_3db_att);

	ret = of_property_read_u32(dnode, "rssi_est_on", &radio->rssi_est_on);
	if (ret)
		radio->rssi_est_on = 0;
	dev_info(radio->dev, "rssi_est_on: %d\n", radio->rssi_est_on);

	ret = of_property_read_u32(dnode, "sw_mute_weak", &radio->sw_mute_weak);
	if (ret)
		radio->sw_mute_weak = 0;
	dev_info(radio->dev, "sw_mute_weak: %d\n", radio->sw_mute_weak);

	ret = of_property_read_u32(dnode, "without_elna", &radio->without_elna);
	if (ret)
		radio->without_elna = 0;
	dev_info(radio->dev, "without eLNA: %d\n", radio->without_elna);

	ret = of_property_read_u16(dnode, "rssi_adjust", &radio->rssi_adjust);
	if (ret)
		radio->rssi_adjust = 0;
	dev_info(radio->dev, "rssi adjust: %d\n", radio->rssi_adjust);

	ret = of_property_read_u16(dnode, "ldo_onoff", &radio->ldo_onoff);
	if (ret)
		radio->ldo_onoff = 1;
	dev_info(radio->dev, "ldo_onoff: %d\n", radio->ldo_onoff);

#ifdef USE_FM_PIN_CONTROL
	fm_init_pinctrl_info(radio);
	fm_pinctrl_state_active(radio);
	fm_pinctrl_state_elna(radio);
#endif /* USE_FM_PIN_CONTROL */

#ifdef USE_FM_LNA_ENABLE
		if (radio->without_elna != 1) {
			radio->elna_gpio = of_get_named_gpio(dnode, "elna_gpio", 0);
			if (!gpio_is_valid(radio->elna_gpio)) {
				dev_err(dev, "(%s) elna_gpio invalid. Disable elna_gpio control\n",
					__func__);
				radio->elna_gpio = -EINVAL;
			} else {
				ret = gpio_request_one(radio->elna_gpio, GPIOF_OUT_INIT_LOW,
							"LNA_GPIO_EN");
				if (ret)
					gpio_set_value(radio->elna_gpio, GPIO_LOW);
				dev_info(dev, "(%s) Enable elna_gpio control :%d\n",
					__func__, gpio_get_value(radio->elna_gpio));
			}
		}
#endif /* USE_FM_LNA_ENABLE */

	wake_lock_init(radio->dev, &radio->wakelock.ws, "fm_wake");
	wake_lock_init(radio->dev, &radio->rdswakelock.ws, "fm_rdswake");

	dev_info(&pdev->dev, ">>> end FM Radio probe.\n");
	return 0;

exit:
	v4l2_ctrl_handler_free(radio->videodev.ctrl_handler);

alloc_err4:
#ifdef USE_FM_LNA_ENABLE
	if (radio->elna_gpio != -EINVAL)
		gpio_free(radio->elna_gpio);
#endif /*USE_FM_LNA_ENABLE*/

	v4l2_device_unregister(&radio->v4l2dev);

alloc_err3:
	pm_runtime_disable(&pdev->dev);
	pm_runtime_set_suspended(&pdev->dev);

	fm_clk_disable(radio);
	fm_clk_unprepare(radio);

alloc_err2:
	kfree(radio->low);

alloc_err1:
	kfree(radio->core);

alloc_err0:
	kfree(radio);

	return ret;
}

static int s621_radio_remove(struct platform_device *pdev)
{
	struct s621_radio *radio = platform_get_drvdata(pdev);

	if (radio) {
		fm_clk_disable(radio);
		fm_clk_unprepare(radio);
		fm_clk_put(radio);

		pm_runtime_disable(&pdev->dev);
		pm_runtime_set_suspended(&pdev->dev);

		v4l2_ctrl_handler_free(radio->videodev.ctrl_handler);
		video_unregister_device(&radio->videodev);
		v4l2_device_unregister(&radio->v4l2dev);
#ifdef USE_FM_LNA_ENABLE
	if (radio->elna_gpio != -EINVAL)
		gpio_free(radio->elna_gpio);
#endif /*USE_FM_LNA_ENABLE*/
#ifdef USE_FM_PIN_CONTROL
		devm_pinctrl_put(radio->pinctrl_fm);
#endif /* USE_FM_PIN_CONTROL */

		wake_lock_destroy(&radio->wakelock);
		wake_lock_destroy(&radio->rdswakelock);

		kfree(radio->vol_level_tmp);
		kfree(radio->low);
		kfree(radio->core);
		kfree(radio);
	}

	return 0;
}

static int fm_clk_get(struct s621_radio *radio)
{
	struct device *dev = radio->dev;
	struct clk *clk;
	int ret, i;

	radio->clk_count = of_property_count_strings(dev->of_node, "clock-names");
	if (IS_ERR_VALUE((unsigned long)radio->clk_count)) {
		dev_err(dev, "invalid clk list in %s node", dev->of_node->name);
		return -EINVAL;
	}
	dev_info(dev, "%s list: clk_count: %d\n", __func__, radio->clk_count);

	radio->clk_ids = devm_kmalloc(dev,
				(radio->clk_count) * sizeof(const char *),
				GFP_KERNEL);
	if (!radio->clk_ids) {
		dev_err(dev, "failed to alloc for clock ids");
		return -ENOMEM;
	}

	for (i = 0; i < radio->clk_count; i++) {
		ret = of_property_read_string_index(dev->of_node, "clock-names",
								i, &radio->clk_ids[i]);
		if (ret) {
			dev_err(dev, "failed to read clocks name %d from %s node\n",
					i, dev->of_node->name);
			return ret;
		}
	}

	radio->clocks = devm_kmalloc(dev,
			radio->clk_count * sizeof(struct clk *), GFP_KERNEL);
	if (!radio->clocks) {
		dev_err(dev, "%s: couldn't kmalloc for clock\n", __func__);
		return -ENOMEM;
	}

	for (i = 0; i < radio->clk_count; i++) {
		clk = devm_clk_get(dev, radio->clk_ids[i]);
		if (IS_ERR_OR_NULL(clk))
			goto err;

		radio->clocks[i] = clk;
	}

	return 0;

err:
	dev_err(dev, "couldn't get %s clock\n", radio->clk_ids[i]);
	return -EINVAL;
}

static int fm_clk_prepare(struct s621_radio *radio)
{
	int i;
	int ret;

	for (i = 0; i < radio->clk_count; i++) {
		ret = clk_prepare(radio->clocks[i]);
		if (ret)
			goto err;
	}

	return 0;

err:
	dev_err(radio->dev, "couldn't prepare clock[%d]\n", i);

	/* roll back */
	for (i = i - 1; i >= 0; i--)
		clk_unprepare(radio->clocks[i]);

	return ret;
}

static int fm_clk_enable(struct s621_radio *radio)
{
	int i;
	int ret;

	for (i = 0; i < radio->clk_count; i++) {
		ret = clk_enable(radio->clocks[i]);
		if (ret)
			goto err;

		dev_info(radio->dev, "clock %s: %lu\n", radio->clk_ids[i], clk_get_rate(radio->clocks[i]));
#ifdef DEBUG_AUDIO_FM_CLK
		if (!strcmp(radio->clk_ids[i], "clk_aud_fm")) {
			if (clk_get_rate(radio->clocks[i]) != 40000) {
				ret = clk_set_rate(radio->clocks[i], 40000);
				if (IS_ERR_VALUE((unsigned long)ret))
					dev_info(radio->dev,
					"setting clock clk_aud_fm to 40KHz is failed: %lu\n", ret);
				dev_info(radio->dev, "FM clock clk_aud_fm: %lu\n",
					clk_get_rate(radio->clocks[i]));
			}
		}
#endif
	}

	return 0;

err:
	dev_err(radio->dev, "couldn't enable clock[%d]\n", i);

	/* roll back */
	for (i = i - 1; i >= 0; i--)
		clk_disable(radio->clocks[i]);

	return ret;
}

static void fm_clk_unprepare(struct s621_radio *radio)
{
	int i;

	for (i = 0; i < radio->clk_count; i++)
		clk_unprepare(radio->clocks[i]);
}

static void fm_clk_disable(struct s621_radio *radio)
{
	int i;

	for (i = 0; i < radio->clk_count; i++)
		clk_disable(radio->clocks[i]);
}

static void fm_clk_put(struct s621_radio *radio)
{
	int i;

	for (i = 0; i < radio->clk_count; i++)
		clk_put(radio->clocks[i]);

}

#if IS_ENABLED(CONFIG_PM)
static int fm_radio_runtime_suspend(struct device *dev)
{
	struct s621_radio *radio = dev_get_drvdata(dev);

	FUNC_ENTRY(radio);

	fm_clk_disable(radio);

	FUNC_EXIT(radio);

	return 0;
}

static int fm_radio_runtime_resume(struct device *dev)
{
	struct s621_radio *radio = dev_get_drvdata(dev);

	FUNC_ENTRY(radio);

	fm_clk_enable(radio);

	FUNC_EXIT(radio);

	return 0;
}
#endif

#if IS_ENABLED(CONFIG_PM_SLEEP)
static int fm_radio_suspend(struct device *dev)
{
	dev_err(dev, "%s: FM suspend ..\n", __func__);

	return 0;
}

static int fm_radio_resume(struct device *dev)
{
	dev_err(dev, "%s: FM resume ..\n", __func__);

	return 0;
}
#endif /* CONFIG_PM_SLEEP */

static const struct dev_pm_ops fm_radio_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(fm_radio_suspend, fm_radio_resume)
	SET_RUNTIME_PM_OPS(fm_radio_runtime_suspend,
			fm_radio_runtime_resume, NULL)
};

#define DEV_PM_OPS	(&fm_radio_dev_pm_ops)

static struct platform_driver s621_radio_driver = {
		.driver		= {
				.name	= DRIVER_NAME,
				.owner	= THIS_MODULE,
				.of_match_table = of_match_ptr(exynos_fm_of_match),
				.pm	= DEV_PM_OPS,
		},
		.probe		= s621_radio_probe,
		.remove		= s621_radio_remove,
};

module_platform_driver(s621_radio_driver);
MODULE_AUTHOR("Youngjoon Chung, <young11@samsung.com>");
MODULE_DESCRIPTION("Driver for S621 FM Radio in Samsung Exynos chips");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.0.1");
