/*
 * Samsung Exynos SoC series Actuator driver
 *
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/i2c.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/videodev2.h>
#include <videodev2_exynos_camera.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <dt-bindings/camera/exynos_is_dt.h>

#include "is-aperture-ak7372.h"
#include "is-device-sensor.h"
#include "is-device-sensor-peri.h"
#include "is-helper-ixc.h"
#include "interface/is-interface-library.h"
#include "is-vendor-private.h"

#define APERTURE_NAME		"APERTURE_AK7372"

struct i2c_client *is_aperture_i2c_get_client(struct is_core *core)
{
	struct i2c_client *client = NULL;
	struct is_vendor_private *vendor_priv = core->vendor.private_data;
	u32 sensor_idx = vendor_priv->aperture_sensor_index;

	if (core->sensor[sensor_idx].aperture != NULL)
		client = core->sensor[sensor_idx].aperture->client;

	return client;
};

int sensor_ak7372_aperture_init(struct v4l2_subdev *subdev, u32 val)
{
	int ret = 0;
	int pos = 0;
	u8 data[2] = {0, };
	struct is_aperture *aperture;
#ifdef USE_CAMERA_HW_BIG_DATA
	struct cam_hw_param *hw_param = NULL;
	struct is_device_sensor *device = NULL;
#endif

	WARN_ON(!subdev);

	dbg_aperture("%s\n", __func__);

	aperture = (struct is_aperture *)v4l2_get_subdevdata(subdev);
	WARN_ON(!aperture);

	if (unlikely(!aperture->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	IXC_MUTEX_LOCK(aperture->ixc_lock);

	aperture->cur_value = 0;
	aperture->new_value = 0;
	aperture->start_value = F2_4;
	aperture->step = APERTURE_STEP_STATIONARY;

	ret = cis->ixc_ops->addr8_write8(aperture->client, 0xAE, 0x3B); /* setting mode */
	pos = 511;
	data[0] = pos >> 2 & 0xFF;
	data[1] = pos << 6 & 0xC0;
	ret |= cis->ixc_ops->addr8_write8(aperture->client, 0x00, data[0]); /* position code */
	ret |= cis->ixc_ops->addr8_write8(aperture->client, 0x01, data[1]);
	ret |= cis->ixc_ops->addr8_write8(aperture->client, 0x02, 0x00); /* active mode */

	if (ret < 0) {
#ifdef USE_CAMERA_HW_BIG_DATA
		device = v4l2_get_subdev_hostdata(subdev);
		if (device)
			is_sec_get_hw_param(&hw_param, device->position);
		if (hw_param)
			hw_param->i2c_aperture_err_cnt++;
#endif
		goto p_err;
	}

p_err:
	IXC_MUTEX_UNLOCK(aperture->ixc_lock);

	return ret;
}

int sensor_ak7372_aperture_set_start_value_step1(struct v4l2_subdev *subdev, int value)
{
	int ret = 0;
	int pos = 0;
	u8 data[2] = {0, };
	struct is_aperture *aperture;

	WARN_ON(!subdev);

	dbg_aperture("%s value = %d\n", __func__, value);

	aperture = (struct is_aperture *)v4l2_get_subdevdata(subdev);
	WARN_ON(!aperture);

	if (unlikely(!aperture->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto exit;
	}

#ifdef USE_OIS_SHIFT_FOR_APERTURE
	if (aperture->sensor_peri->subdev_ois) {
		ret = CALL_OISOPS(aperture->sensor_peri->ois, ois_center_shift, aperture->sensor_peri->subdev_ois);
		if (ret < 0)
			err("v4l2_subdev_call(ois_center_shift) is fail(%d)", ret);
	}
	usleep_range(15000, 16000);
#endif

	switch (value) {
	case F1_5:
		pos = 0;
		break;
	case F2_4:
		pos = 1023;
		break;
	default:
		dbg_aperture("%s: mode is not set.(mode = %d)\n", __func__, value);
		goto exit;
	}

	IXC_MUTEX_LOCK(aperture->ixc_lock);

	ret = cis->ixc_ops->addr8_write8(aperture->client, 0xA6, 0x7B); /* open mode */
	data[0] = pos >> 2 & 0xFF;
	data[1] = pos << 6 & 0xC0;
	ret |= cis->ixc_ops->addr8_write8(aperture->client, 0x00, data[0]); /* position code */
	ret |= cis->ixc_ops->addr8_write8(aperture->client, 0x01, data[1]);

	if (ret < 0)
		err("i2c fail occurred.");

	IXC_MUTEX_UNLOCK(aperture->ixc_lock);

exit:
	return ret;
}

int sensor_ak7372_aperture_set_start_value_step2(struct v4l2_subdev *subdev, int value)
{
	int ret = 0;
	int pos = 511;
	u8 data[2] = {0, };
	struct is_aperture *aperture;

	WARN_ON(!subdev);

	dbg_aperture("%s value = %d\n", __func__, value);

	aperture = (struct is_aperture *)v4l2_get_subdevdata(subdev);
	WARN_ON(!aperture);

	if (unlikely(!aperture->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto exit;
	}

	IXC_MUTEX_LOCK(aperture->ixc_lock);

	ret |= cis->ixc_ops->addr8_write8(aperture->client, 0xA6, 0x00); /* close code */
	data[0] = pos >> 2 & 0xFF;
	data[1] = pos << 6 & 0xC0;
	ret |= cis->ixc_ops->addr8_write8(aperture->client, 0x00, data[0]); /* position code */
	ret |= cis->ixc_ops->addr8_write8(aperture->client, 0x01, data[1]);
	aperture->cur_value = value;
	info("current aperture value = %d", value);

	if (ret < 0)
		err("i2c fail occurred.");

	aperture->step = APERTURE_STEP_STATIONARY;

	IXC_MUTEX_UNLOCK(aperture->ixc_lock);

exit:
	return ret;
}

int sensor_ak7372_aperture_set_value(struct v4l2_subdev *subdev, int value)
{
	int ret = 0;
	struct is_aperture *aperture;

	WARN_ON(!subdev);

	dbg_aperture("%s value = %d\n", __func__, value);

	aperture = (struct is_aperture *)v4l2_get_subdevdata(subdev);
	WARN_ON(!aperture);

	if (unlikely(!aperture->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto exit;
	}

	if (value == aperture->cur_value) {
		aperture->step = APERTURE_STEP_STATIONARY;
		dbg_aperture("%s: new value is same with cur value.(cur value = %d, new value = %d)\n",
			__func__, aperture->cur_value, value);
		return ret;
	}

	sensor_ak7372_aperture_set_start_value_step1(subdev, value);
	usleep_range(10000, 11000);
	sensor_ak7372_aperture_set_start_value_step2(subdev, value);
	usleep_range(15000, 16000);

exit:
	return ret;
}

int sensor_ak7372_aperture_prepare_ois_autotest(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct is_aperture *aperture;

	WARN_ON(!subdev);

	aperture = (struct is_aperture *)v4l2_get_subdevdata(subdev);
	WARN_ON(!aperture);

	if (unlikely(!aperture->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto exit;
	}

	ret = cis->ixc_ops->addr8_write8(aperture->client, 0xAE, 0x3B);
	ret |= cis->ixc_ops->addr8_write8(aperture->client, 0x00, 0x7F);
	ret |= cis->ixc_ops->addr8_write8(aperture->client, 0x01, 0xC0);
	ret |= cis->ixc_ops->addr8_write8(aperture->client, 0x02, 0x00);
	ret |= cis->ixc_ops->addr8_write8(aperture->client, 0xA6, 0x7B);
	ret |= cis->ixc_ops->addr8_write8(aperture->client, 0x00, 0xFF);
	ret |= cis->ixc_ops->addr8_write8(aperture->client, 0x01, 0xC0);
	usleep_range(10000, 11000);

	ret |= cis->ixc_ops->addr8_write8(aperture->client, 0xA6, 0x00);
	ret |= cis->ixc_ops->addr8_write8(aperture->client, 0x00, 0x7F);
	ret |= cis->ixc_ops->addr8_write8(aperture->client, 0x01, 0xC0);
	usleep_range(15000, 16000);

	if (ret < 0)
		err("i2c fail occurred.");

exit:
	return ret;
}

int sensor_ak7372_aperture_deinit(struct v4l2_subdev *subdev, int value)
{
	int ret = 0;
	struct is_aperture *aperture;

	WARN_ON(!subdev);

	info("[%s] start\n", __func__);

	aperture = (struct is_aperture *)v4l2_get_subdevdata(subdev);
	WARN_ON(!aperture);

	if (unlikely(!aperture->client)) {
		err("client is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	if (aperture->sensor_peri->subdev_ois) {
		ret = CALL_OISOPS(aperture->sensor_peri->ois, ois_set_mode, aperture->sensor_peri->subdev_ois,
			OPTICAL_STABILIZATION_MODE_CENTERING);
		if (ret < 0)
			err("v4l2_subdev_call(ois_set_mode) is fail(%d)", ret);

		usleep_range(10000, 11000);
	}

	sensor_ak7372_aperture_set_start_value_step1(subdev, value);
	usleep_range(5000, 6000);

p_err:
	info("[%s] end\n", __func__);

	return ret;
}

static struct is_aperture_ops aperture_ops = {
	.set_aperture_value = sensor_ak7372_aperture_set_value,
	.set_aperture_start_value_step1 = sensor_ak7372_aperture_set_start_value_step1,
	.set_aperture_start_value_step2 = sensor_ak7372_aperture_set_start_value_step2,
	.prepare_ois_autotest = sensor_ak7372_aperture_prepare_ois_autotest,
	.aperture_deinit = sensor_ak7372_aperture_deinit,
};

static const struct v4l2_subdev_core_ops core_ops = {
	.init = sensor_ak7372_aperture_init,
};

static const struct v4l2_subdev_ops subdev_ops = {
	.core = &core_ops,
};

int sensor_ak7372_aperture_probe_i2c(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int ret = 0;
	struct is_core *core = NULL;
	struct v4l2_subdev *subdev_aperture = NULL;
	struct is_device_sensor *device;
	struct is_aperture *aperture = NULL;
	struct device *dev;
	struct device_node *dnode;
	u32 sensor_id[IS_SENSOR_COUNT] = {0, };
	const u32 *sensor_id_spec;
	u32 sensor_id_len;
	int i = 0;

	WARN_ON(!is_dev);
	WARN_ON(!client);

	core = (struct is_core *)dev_get_drvdata(is_dev);
	if (!core) {
		err("core device is not yet probed");
		ret = -EPROBE_DEFER;
		goto p_err;
	}

	dev = &client->dev;
	dnode = dev->of_node;

	sensor_id_spec = of_get_property(dnode, "id", &sensor_id_len);
	if (!sensor_id_spec) {
		err("sensor_id num read is fail(%d)", ret);
		goto p_err;
	}

	sensor_id_len /= (unsigned int)sizeof(*sensor_id_spec);

	ret = of_property_read_u32_array(dnode, "id", sensor_id, sensor_id_len);
	if (ret) {
		err("sensor_id read is fail(%d)", ret);
		goto p_err;
	}

	for (i = 0; i < sensor_id_len; i++) {
		device = &core->sensor[sensor_id[i]];
		if (!device) {
			err("sensor device is NULL");
			ret = -EPROBE_DEFER;
			goto p_err;
		}

		aperture = kzalloc(sizeof(struct is_aperture), GFP_KERNEL);
		if (!aperture) {
			err("aperture is NULL");
			ret = -ENOMEM;
			goto p_err;
		}

		subdev_aperture = kzalloc(sizeof(struct v4l2_subdev), GFP_KERNEL);
		if (!subdev_aperture) {
			err("subdev_aperture is NULL");
			ret = -ENOMEM;
			kfree(aperture);
			goto p_err;
		}

		aperture->id = APERTURE_NAME_AK7372;
		aperture->subdev = subdev_aperture;
		aperture->device = sensor_id[i];
		aperture->client = client;
		aperture->ixc_lock = NULL;
		aperture->aperture_ops = &aperture_ops;
		aperture->ixc_ops = pablo_get_i2c();

		mutex_init(&aperture->control_lock);

		device->subdev_aperture = subdev_aperture;
		device->aperture = aperture;

		v4l2_i2c_subdev_init(subdev_aperture, client, &subdev_ops);
		v4l2_set_subdevdata(subdev_aperture, aperture);
		v4l2_set_subdev_hostdata(subdev_aperture, device);
	}

p_err:
	probe_info("%s done\n", __func__);
	return ret;
}

static int sensor_ak7372_aperture_remove_i2c(struct i2c_client *client)
{
	int ret = 0;

	return ret;
}

static const struct of_device_id exynos_is_aperture_ak7372_match[] = {
	{
		.compatible = "samsung,exynos5-fimc-is-aperture-ak7372",
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_is_aperture_ak7372_match);

static const struct i2c_device_id aperture_ak7372_idt[] = {
	{ APERTURE_NAME, 0 },
	{},
};

static struct i2c_driver aperture_ak7372_driver = {
	.driver = {
		.name	= APERTURE_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = exynos_is_aperture_ak7372_match
	},
	.probe	= sensor_ak7372_aperture_probe_i2c,
	.remove	= sensor_ak7372_aperture_remove_i2c,
	.id_table = aperture_ak7372_idt
};
module_i2c_driver(aperture_ak7372_driver);
