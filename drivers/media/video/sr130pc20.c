/* drivers/media/video/sr130pc20.c
 *
 * Copyright (c) 2010, Samsung Electronics. All rights reserved
 * Author: dongseong.lim
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * - change date: 2012.06.28
 */
#include "sr130pc20.h"
#include <linux/gpio.h>

#define i2c_read_nop() \
	(cam_err("error, not used read function, line %d\n", __LINE__))
#define i2c_write_nop() \
	(cam_err("error, not used write function, line %d\n", __LINE__))

#define isx012_readb(sd, addr, data)	i2c_read_nop()
#define isx012_writeb(sd, addr, data)	i2c_write_nop()

#define sr130pc20_readb(sd, addr, data) sr130pc20_i2c_read(sd, addr, data)
#define sr130pc20_readw(sd, addr, data)	i2c_read_nop()
#define sr130pc20_readl(sd, addr, data)	i2c_read_nop()
#define sr130pc20_writeb(sd, addr, data) sr130pc20_i2c_write(sd, addr, data)
#define sr130pc20_writew(sd, addr, data) i2c_write_nop()
#define sr130pc20_writel(sd, addr, data) i2c_write_nop()

static int dbg_level;

static const struct sr130pc20_fps sr130pc20_framerates[] = {
	{ I_FPS_0,	FRAME_RATE_AUTO },
	{ I_FPS_7,	FRAME_RATE_7},
	{ I_FPS_15,	FRAME_RATE_15 },
	{ I_FPS_25,	FRAME_RATE_25 },
	{ I_FPS_30,	FRAME_RATE_30 },
};

static const struct sr130pc20_framesize sr130pc20_preview_frmsizes[] = {
	/*{ PREVIEW_SZ_QCIF,	176,  144 },
	{ PREVIEW_SZ_CIF,	352,  288 },
	{ PREVIEW_SZ_528x432,	528,  432 }, */
	{ PREVIEW_SZ_VGA,	640,  480 },
};

static const struct sr130pc20_framesize sr130pc20_capture_frmsizes[] = {
	{ CAPTURE_SZ_VGA,	640,  480 },
	{ CAPTURE_SZ_960_720,	960,  720 },
	{ CAPTURE_SZ_1MP,	1280, 960 },
};

static struct sr130pc20_control sr130pc20_ctrls[] = {
	SR130PC20_INIT_CONTROL(V4L2_CID_CAMERA_FLASH_MODE, \
					FLASH_MODE_OFF),

	SR130PC20_INIT_CONTROL(V4L2_CID_CAMERA_BRIGHTNESS, \
					EV_DEFAULT),

	SR130PC20_INIT_CONTROL(V4L2_CID_CAMERA_METERING, \
					METERING_MATRIX),

	SR130PC20_INIT_CONTROL(V4L2_CID_CAMERA_WHITE_BALANCE, \
					WHITE_BALANCE_AUTO),

	SR130PC20_INIT_CONTROL(V4L2_CID_CAMERA_EFFECT, \
					IMAGE_EFFECT_NONE),
};

static const struct sr130pc20_regs reg_datas = {
	.ev = {
		SR130PC20_REGSET(GET_EV_INDEX(EV_MINUS_4),
				SR130PC20_ExpSetting_M4Step, 0),
		SR130PC20_REGSET(GET_EV_INDEX(EV_MINUS_3),
				SR130PC20_ExpSetting_M3Step, 0),
		SR130PC20_REGSET(GET_EV_INDEX(EV_MINUS_2),
				SR130PC20_ExpSetting_M2Step, 0),
		SR130PC20_REGSET(GET_EV_INDEX(EV_MINUS_1),
				SR130PC20_ExpSetting_M1Step, 0),
		SR130PC20_REGSET(GET_EV_INDEX(EV_DEFAULT),
				SR130PC20_ExpSetting_Default, 0),
		SR130PC20_REGSET(GET_EV_INDEX(EV_PLUS_1),
				SR130PC20_ExpSetting_P1Step, 0),
		SR130PC20_REGSET(GET_EV_INDEX(EV_PLUS_2),
				SR130PC20_ExpSetting_P2Step, 0),
		SR130PC20_REGSET(GET_EV_INDEX(EV_PLUS_3),
				SR130PC20_ExpSetting_P3Step, 0),
		SR130PC20_REGSET(GET_EV_INDEX(EV_PLUS_4),
				SR130PC20_ExpSetting_P4Step, 0),
	},
	.metering = {
		SR130PC20_REGSET(METERING_MATRIX, sr130pc20_Metering_Matrix, 0),
		SR130PC20_REGSET(METERING_CENTER, sr130pc20_Metering_Center, 0),
		SR130PC20_REGSET(METERING_SPOT, sr130pc20_Metering_Spot, 0),
	},
	.iso = {
		SR130PC20_REGSET(ISO_AUTO, sr130pc20_ISO_Auto, 0),
		SR130PC20_REGSET(ISO_50, sr130pc20_ISO_50, 0),
		SR130PC20_REGSET(ISO_100, sr130pc20_ISO_100, 0),
		SR130PC20_REGSET(ISO_200, sr130pc20_ISO_200, 0),
		SR130PC20_REGSET(ISO_400, sr130pc20_ISO_400, 0),
	},
	.effect = {
		SR130PC20_REGSET(IMAGE_EFFECT_NONE, sr130pc20_Effect_Normal, 0),
		SR130PC20_REGSET(IMAGE_EFFECT_BNW, sr130pc20_Effect_Black_White, 0),
		SR130PC20_REGSET(IMAGE_EFFECT_SEPIA, sr130pc20_Effect_Sepia, 0),
		SR130PC20_REGSET(IMAGE_EFFECT_NEGATIVE,
				SR130PC20_Effect_Negative, 0),
		SR130PC20_REGSET(IMAGE_EFFECT_SOLARIZE, sr130pc20_Effect_Solar, 0),
		SR130PC20_REGSET(IMAGE_EFFECT_SKETCH, sr130pc20_Effect_Sketch, 0),
		SR130PC20_REGSET(IMAGE_EFFECT_POINT_COLOR_3,
				sr130pc20_Effect_Pastel, 0),
	},
	.white_balance = {
		SR130PC20_REGSET(WHITE_BALANCE_AUTO, sr130pc20_WB_Auto, 0),
		SR130PC20_REGSET(WHITE_BALANCE_SUNNY, sr130pc20_WB_Sunny, 0),
		SR130PC20_REGSET(WHITE_BALANCE_CLOUDY, sr130pc20_WB_Cloudy, 0),
		SR130PC20_REGSET(WHITE_BALANCE_TUNGSTEN,
				sr130pc20_WB_Tungsten, 0),
		SR130PC20_REGSET(WHITE_BALANCE_FLUORESCENT,
				sr130pc20_WB_Fluorescent, 0),
	},
	.scene_mode = {
		SR130PC20_REGSET(SCENE_MODE_NONE, sr130pc20_Scene_Default, 0),
		SR130PC20_REGSET(SCENE_MODE_PORTRAIT, sr130pc20_Scene_Portrait, 0),
		SR130PC20_REGSET(SCENE_MODE_NIGHTSHOT, sr130pc20_Scene_Nightshot, 0),
		SR130PC20_REGSET(SCENE_MODE_BACK_LIGHT, sr130pc20_Scene_Backlight, 0),
		SR130PC20_REGSET(SCENE_MODE_LANDSCAPE, sr130pc20_Scene_Landscape, 0),
		SR130PC20_REGSET(SCENE_MODE_SPORTS, sr130pc20_Scene_Sports, 0),
		SR130PC20_REGSET(SCENE_MODE_PARTY_INDOOR,
				sr130pc20_Scene_Party_Indoor, 0),
		SR130PC20_REGSET(SCENE_MODE_BEACH_SNOW,
				sr130pc20_Scene_Beach_Snow, 0),
		SR130PC20_REGSET(SCENE_MODE_SUNSET, sr130pc20_Scene_Sunset, 0),
		SR130PC20_REGSET(SCENE_MODE_DUSK_DAWN, sr130pc20_Scene_Duskdawn, 0),
		SR130PC20_REGSET(SCENE_MODE_FALL_COLOR,
				sr130pc20_Scene_Fall_Color, 0),
		SR130PC20_REGSET(SCENE_MODE_FIREWORKS, sr130pc20_Scene_Fireworks, 0),
		SR130PC20_REGSET(SCENE_MODE_TEXT, sr130pc20_Scene_Text, 0),
		SR130PC20_REGSET(SCENE_MODE_CANDLE_LIGHT,
				sr130pc20_Scene_Candle_Light, 0),
	},
	.fps = {
		SR130PC20_REGSET(I_FPS_0, sr130pc20_fps_auto, 0),
		SR130PC20_REGSET(I_FPS_7, sr130pc20_fps_7fix, 0),
		SR130PC20_REGSET(I_FPS_15, sr130pc20_fps_15fix, 0),
		SR130PC20_REGSET(I_FPS_25, sr130pc20_fps_25fix, 0),
		SR130PC20_REGSET(I_FPS_30, sr130pc20_fps_30fix, 0),
	},
	.preview_size = {
		SR130PC20_REGSET(PREVIEW_SZ_VGA, sr130pc20_640_Preview, 0),
	},
	.capture_size = {
		SR130PC20_REGSET(CAPTURE_SZ_VGA, sr130pc20_VGA_Capture, 0),
		SR130PC20_REGSET(CAPTURE_SZ_960_720, sr130pc20_960_720_Capture, 0),
	},

	.init_reg = SR130PC20_REGSET_TABLE(SR130PC20_Init_Reg, 0),
	/* Camera mode */
	.preview_mode = SR130PC20_REGSET_TABLE(SR130PC20_Preview_Mode, 0),
	.capture_mode = SR130PC20_REGSET_TABLE(SR130PC20_Capture_Mode, 0),
	.capture_mode_night =
		SR130PC20_REGSET_TABLE(SR130PC20_Lowlux_Night_Capture_Mode, 0),
	.stream_stop = SR130PC20_REGSET_TABLE(sr130pc20_stop_stream, 0),
};

static const struct v4l2_mbus_framefmt capture_fmts[] = {
	{
		.code		= V4L2_MBUS_FMT_FIXED,
		.colorspace	= V4L2_COLORSPACE_JPEG,
	},
};

/**
 * msleep_debug: wrapper function calling proper sleep()
 * @msecs: time to be sleep (in milli-seconds unit)
 * @dbg_on: whether enable log or not.
 */
static void msleep_debug(u32 msecs, bool dbg_on)
{
	u32 delta_halfrange; /* in us unit */

	if (unlikely(!msecs))
		return;

	if (dbg_on)
		cam_dbg("delay for %dms\n", msecs);

	if (msecs <= 7)
		delta_halfrange = 100;
	else
		delta_halfrange = 300;

	if (msecs <= 20)
		usleep_range((msecs * 1000 - delta_halfrange),
			(msecs * 1000 + delta_halfrange));
	else
		msleep(msecs);
}

#ifdef CONFIG_LOAD_FILE
#define TABLE_MAX_NUM 500
static char *sr130pc20_regs_table;
static int sr130pc20_regs_table_size;
static int gtable_buf[TABLE_MAX_NUM];
static int sr130pc20_i2c_write(struct v4l2_subdev *sd,
		u16 subaddr, u32 data, u32 len);

int sr130pc20_regs_table_init(void)
{
	struct file *filp;
	char *dp;
	long l;
	loff_t pos;
	int ret;
	mm_segment_t fs = get_fs();

	printk(KERN_DEBUG "%s %d\n", __func__, __LINE__);

	set_fs(get_ds());

	filp = filp_open("/mnt/sdcard/sr130pc20_regs.h", O_RDONLY, 0);

	if (IS_ERR_OR_NULL(filp)) {
		printk(KERN_DEBUG "file open error\n");
		return PTR_ERR(filp);
	}

	l = filp->f_path.dentry->d_inode->i_size;
	printk(KERN_DEBUG "l = %ld\n", l);
	//dp = kmalloc(l, GFP_KERNEL);
	dp = vmalloc(l);
	if (dp == NULL) {
		printk(KERN_DEBUG "Out of Memory\n");
		filp_close(filp, current->files);
	}

	pos = 0;
	memset(dp, 0, l);
	ret = vfs_read(filp, (char __user *)dp, l, &pos);

	if (ret != l) {
		printk(KERN_DEBUG "Failed to read file ret = %d\n", ret);
		/*kfree(dp);*/
		vfree(dp);
		filp_close(filp, current->files);
		return -EINVAL;
	}

	filp_close(filp, current->files);

	set_fs(fs);

	sr130pc20_regs_table = dp;

	sr130pc20_regs_table_size = l;

	*((sr130pc20_regs_table + sr130pc20_regs_table_size) - 1) = '\0';

	printk("sr130pc20_reg_table_init end\n");
	return 0;
}

void sr130pc20_regs_table_exit(void)
{
	printk(KERN_DEBUG "%s %d\n", __func__, __LINE__);

	if (sr130pc20_regs_table) {
		vfree(sr130pc20_regs_table);
		sr130pc20_regs_table = NULL;
	}
}

static int sr130pc20_define_table(void)
{
	char *start, *end, *reg;
	char *start_token, *reg_token, *temp;
	char reg_buf[61], temp2[61];
	char token_buf[5];
	int token_value = 0;
	int index_1 = 0, index_2 = 0, total_index;
	int len = 0, total_len = 0;

	*(reg_buf + 60) = '\0';
	*(temp2 + 60) = '\0';
	*(token_buf + 4) = '\0';
	memset(gtable_buf, 9999, TABLE_MAX_NUM);

	printk(KERN_DEBUG "sr130pc20_define_table start!\n");

	start = strstr(sr130pc20_regs_table, "aeoffset_table");
	if (!start) {
		cam_err("%s: can not find %s", __func__, "aeoffset_table");
		return -ENOENT;
	}
	end = strstr(start, "};");
	if (!end) {
		cam_err("%s: can not find %s end", __func__, "aeoffset_table");
		return -ENOENT;
	}

	/* Find table */
	index_2 = 0;
	while (1) {
		reg = strstr(start,"	");
		if ((reg == NULL) || (reg > end)) {
			printk(KERN_DEBUG "sr130pc20_define_table read end!\n");
			break;
		}

		/* length cal */
		index_1 = 0;
		total_len = 0;
		temp = reg;
		if (temp != NULL) {
			memcpy(temp2, (temp + 1), 60);
			//printk(KERN_DEBUG "temp2 : %s\n", temp2);
		}
		start_token = strstr(temp,",");
		while (index_1 < 10) {
			start_token = strstr(temp, ",");
			len = strcspn(temp, ",");
			//printk(KERN_DEBUG "len[%d]\n", len);	//Only debug
			total_len = total_len + len;
			temp = (temp + (len+2));
			index_1 ++;
		}
		total_len = total_len + 19;
		//printk(KERN_DEBUG "%d\n", total_len);	//Only debug

		/* read table */
		if (reg != NULL) {
			memcpy(reg_buf, (reg + 1), total_len);
			//printk(KERN_DEBUG "reg_buf : %s\n", reg_buf);	//Only debug
			start = (reg + total_len+1);
		}

		reg_token = reg_buf;

		index_1 = 0;
		start_token=strstr(reg_token,",");
		while (index_1 < 10) {
			start_token = strstr(reg_token, ",");
			len = strcspn(reg_token, ",");
			//printk(KERN_DEBUG "len[%d]\n", len);	//Only debug
			memcpy(token_buf, reg_token, len);
			//printk(KERN_DEBUG "[%d]%s ", index_1, token_buf);	//Only debug
			token_value = (unsigned short)simple_strtoul(token_buf, NULL, 10);
			total_index = index_2 * 10 + index_1;
			//printk(KERN_DEBUG "[%d]%d ", total_index, token_value);	//Only debug
			gtable_buf[total_index] = token_value;
			index_1 ++;
			reg_token = (reg_token + (len + 2));
		}
		index_2 ++;
	}

#if 0	//Only debug
	index_2 = 0;
	while ( index_2 < TABLE_MAX_NUM) {
		printk(KERN_DEBUG "[%d]%d ",index_2, gtable_buf[index_2]);
		index_2++;
	}
#endif
	printk(KERN_DEBUG "sr130pc20_define_table end!\n");

	return 0;
}

static int sr130pc20_define_read(char *name, int len_size)
{
	char *start, *end, *reg;
	char reg_7[7], reg_5[5];
	int define_value = 0;

	*(reg_7 + 6) = '\0';
	*(reg_5 + 4) = '\0';

	//printk(KERN_DEBUG "sr130pc20_define_read start!\n");

	start = strstr(sr130pc20_regs_table, name);
	if (!start) {
		cam_err("%s: can not find %s", __func__, name);
		return -ENOENT;
	}
	end = strstr(start, "tuning");
	if (!end) {
		cam_err("%s: can not find %s", __func__,  "tuning");
		return -ENOENT;
	}

	reg = strstr(start," ");

	if ((reg == NULL) || (reg > end)) {
		printk(KERN_DEBUG "sr130pc20_define_read error %s : ",name);
		return -1;
	}

	/* Write Value to Address */
	if (reg != NULL) {
		if (len_size == 6) {
			memcpy(reg_7, (reg + 1), len_size);
			define_value = (unsigned short)simple_strtoul(reg_7, NULL, 16);
		} else {
			memcpy(reg_5, (reg + 1), len_size);
			define_value = (unsigned short)simple_strtoul(reg_5, NULL, 10);
		}
	}
	//printk(KERN_DEBUG "sr130pc20_define_read end (0x%x)!\n", define_value);

	return define_value;
}

static int sr130pc20_write_regs_from_sd(struct v4l2_subdev *sd, const char *name)
{
	char *start, *end, *reg, *size;
	unsigned short addr;
	unsigned int len, value;
	char reg_buf[7], data_buf1[5], data_buf2[7], len_buf[5];

	*(reg_buf + 6) = '\0';
	*(data_buf1 + 4) = '\0';
	*(data_buf2 + 6) = '\0';
	*(len_buf + 4) = '\0';

	printk(KERN_DEBUG "sr130pc20_regs_table_write start!\n");
	printk(KERN_DEBUG "E string = %s\n", name);

	start = strstr(sr130pc20_regs_table, name);
	if (!start) {
		cam_err("%s: can not find %s", __func__, name);
		return -ENOENT;
	}
	end = strstr(start, "};");

	while (1) {
		/* Find Address */
		reg = strstr(start,"{0x");
		if ((reg == NULL) || (reg > end))
			break;

		/* Write Value to Address */
		if (reg != NULL) {
			memcpy(reg_buf, (reg + 1), 6);
			memcpy(data_buf2, (reg + 8), 6);
			size = strstr(data_buf2,",");
			if (size) { /* 1 byte write */
				memcpy(data_buf1, (reg + 8), 4);
				memcpy(len_buf, (reg + 13), 4);
				addr = (unsigned short)simple_strtoul(reg_buf, NULL, 16);
				value = (unsigned int)simple_strtoul(data_buf1, NULL, 16);
				len = (unsigned int)simple_strtoul(len_buf, NULL, 16);
				if (reg)
					start = (reg + 20);  //{0x000b,0x04,0x01},
			} else {/* 2 byte write */
				memcpy(len_buf, (reg + 15), 4);
				addr = (u16)simple_strtoul(reg_buf, NULL, 16);
				value = (u32)simple_strtoul(data_buf2, NULL, 16);
				len = (u32)simple_strtoul(len_buf, NULL, 16);
				if (reg)
					start = (reg + 22);  //{0x000b,0x0004,0x01},
			}
			size = NULL;

			if (addr == 0xFFFF)
				msleep_debug(value, true);
			else
				sr130pc20_i2c_write(sd, addr, value, len);
		}
	}

	printk(KERN_DEBUG "sr130pc20_regs_table_write end!\n");

	return 0;
}
#endif

/**
 * sr130pc20_i2c_write_twobyte: Write (I2C) multiple bytes to the camera sensor
 * @client: pointer to i2c_client
 * @cmd: command register
 * @w_data: data to be written
 * @w_len: length of data to be written
 *
 * Returns 0 on success, <0 on error
 */
static int sr130pc20_i2c_write_twobyte(struct i2c_client *client,
					 u16 addr, u16 w_data)
{
	int retry_count = 5;
	int ret = 0;
	u8 buf[4] = {0,};
	struct i2c_msg msg = {
		.addr	= client->addr,
		.flags	= 0,
		.len	= 4,
		.buf	= buf,
	};

	buf[0] = addr >> 8;
	buf[1] = addr;
	buf[2] = w_data >> 8;
	buf[3] = w_data & 0xff;

#if (0)
	sr130pc20_debug(SR130PC20_DEBUG_I2C, "%s : W(0x%02X%02X%02X%02X)\n",
		__func__, buf[0], buf[1], buf[2], buf[3]);
#else
	/* cam_dbg("I2C writing: 0x%02X%02X%02X%02X\n",
			buf[0], buf[1], buf[2], buf[3]); */
#endif

	do {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (likely(ret == 1))
			break;
		msleep(POLL_TIME_MS);
		cam_err("%s: error(%d), write (%04X, %04X), retry %d.\n",
				__func__, ret, addr, w_data, retry_count);
	} while (retry_count-- > 0);

	CHECK_ERR_COND_MSG(ret != 1, -EIO, "I2C does not working.\n\n");

	return 0;
}

#if 0
static int sr130pc20_i2c_write_block(struct v4l2_subdev *sd, u8 *buf, u32 size)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int retry_count = 5;
	int ret = 0;
	struct i2c_msg msg = {client->addr, 0, size, buf};

#ifdef CONFIG_VIDEO_SR130PC20_DEBUG
	if (sr130pc20_debug_mask & SR130PC20_DEBUG_I2C_BURSTS) {
		if ((buf[0] == 0x0F) && (buf[1] == 0x12))
			pr_info("%s : data[0,1] = 0x%02X%02X,"
				" total data size = %d\n",
				__func__, buf[2], buf[3], size-2);
		else
			pr_info("%s : 0x%02X%02X%02X%02X\n",
				__func__, buf[0], buf[1], buf[2], buf[3]);
	}
#endif

	do {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (likely(ret == 1))
			break;
		msleep(POLL_TIME_MS);
	} while (retry_count-- > 0);
	if (ret != 1) {
		dev_err(&client->dev, "%s: I2C is not working.\n", __func__);
		return -EIO;
	}

	return 0;
}
#endif

#define BURST_MODE_BUFFER_MAX_SIZE 2700
u8 sr130pc20_burstmode_buf[BURST_MODE_BUFFER_MAX_SIZE];

/* PX: */
static int sr130pc20_burst_write_regs(struct v4l2_subdev *sd,
			const u32 list[], u32 size, char *name)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = -EINVAL;
	int i = 0, idx = 0;
	u16 subaddr = 0, next_subaddr = 0, value = 0;
	struct i2c_msg msg = {
		.addr	= client->addr,
		.flags	= 0,
		.len	= 0,
		.buf	= sr130pc20_burstmode_buf,
	};

	cam_trace("E\n");

	for (i = 0; i < size; i++) {
		CHECK_ERR_COND_MSG((idx > (BURST_MODE_BUFFER_MAX_SIZE - 10)),
			err, "BURST MOD buffer overflow!\n")

		subaddr = (list[i] & 0xFFFF0000) >> 16;
		if (subaddr == 0x0F12)
			next_subaddr = (list[i+1] & 0xFFFF0000) >> 16;

		value = list[i] & 0x0000FFFF;

		switch (subaddr) {
		case 0x0F12:
			/* make and fill buffer for burst mode write. */
			if (idx == 0) {
				sr130pc20_burstmode_buf[idx++] = 0x0F;
				sr130pc20_burstmode_buf[idx++] = 0x12;
			}
			sr130pc20_burstmode_buf[idx++] = value >> 8;
			sr130pc20_burstmode_buf[idx++] = value & 0xFF;

			/* write in burstmode*/
			if (next_subaddr != 0x0F12) {
				msg.len = idx;
				err = i2c_transfer(client->adapter,
					&msg, 1) == 1 ? 0 : -EIO;
				CHECK_ERR_MSG(err, "i2c_transfer\n");
				/* cam_dbg("sr130pc20_sensor_burst_write,
						idx = %d\n", idx); */
				idx = 0;
			}
			break;

		case 0xFFFF:
			msleep_debug(value, true);
			break;

		default:
			idx = 0;
			err = sr130pc20_i2c_write_twobyte(client,
						subaddr, value);
			CHECK_ERR_MSG(err, "i2c_write_twobytes\n");
			break;
		}
	}

	return 0;
}

/**
 * sr130pc20_read: read data from sensor with I2C
 * Note the data-store way(Big or Little)
 */
static int sr130pc20_i2c_read(struct v4l2_subdev *sd,
			u8 subaddr, u8 *data)
{
	int err = -EIO;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_msg msg[2];
	u8 buf[16] = {0,};
	int retry = 5;


	CHECK_ERR_COND_MSG(!client->adapter, -ENODEV,
		"can't search i2c client adapter\n");

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = sizeof(subaddr);
	msg[0].buf = &subaddr;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = buf;

	while (retry-- > 0) {
		err = i2c_transfer(client->adapter, msg, 2);
		if (likely(err == 2))
			break;
		cam_err("i2c read: error, read register(0x%X). cnt %d\n",
			subaddr, retry);
		msleep_debug(POLL_TIME_MS, false);
	}

	CHECK_ERR_COND_MSG(err != 2, -EIO, "I2C does not work\n");

	*data = buf[0];

	return 0;
}

/**
 * sr130pc20_write: write data with I2C
 * Note the data-store way(Big or Little)
 */
static inline int  sr130pc20_i2c_write(struct v4l2_subdev *sd,
					u8 subaddr, u8 data)
{
	u8 buf[2];
	int err = 0, retry = 5;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_msg msg = {
		.addr	= client->addr,
		.flags	= 0,
		.buf	= buf,
		.len	= 2,
	};

	CHECK_ERR_COND_MSG(!client->adapter, -ENODEV,
		"can't search i2c client adapter\n");

	buf[0] = subaddr;
	buf[1] = data;

	while (retry-- > 0) {
		err = i2c_transfer(client->adapter, &msg, 1);
		if (likely(err == 1))
			break;
		cam_err("i2c write: error %d, write 0x%04X, retry %d\n",
			err, ((subaddr << 8) | data), retry);
		msleep_debug(POLL_TIME_MS, false);
	}

	CHECK_ERR_COND_MSG(err != 1, -EIO, "I2C does not work\n");
	return 0;
}

#define SONY_SR130PC20_BURST_DATA_LENGTH	1200
static unsigned char burst_buf[SONY_SR130PC20_BURST_DATA_LENGTH];
static int sr130pc20_i2c_burst_write_list(struct v4l2_subdev *sd,
		const sr130pc20_regset_t regs[], int size, const char *name)
{
#if 0 /* DSLIM */
	struct i2c_client *sr130pc20_client = v4l2_get_subdevdata(sd);
	int i = 0;
	int iTxDataIndex = 0;
	int retry_count = 5;
	int err = 0;
	u8 *buf = burst_buf;

	struct i2c_msg msg = {sr130pc20_client->addr, 0, 4, buf};

	cam_trace("%s\n", name);

	if (!sr130pc20_client->adapter) {
		printk(KERN_ERR "%s: %d can't search i2c client adapter\n", __func__, __LINE__);
		return -EIO;
	}

	while ( i < size )//0<1
	{
		if ( 0 == iTxDataIndex )
		{
	        //printk("11111111111 delay 0x%04x, value 0x%04x\n", regs[i].subaddr, regs[i].value);
			buf[iTxDataIndex++] = ( regs[i].subaddr & 0xFF00 ) >> 8;
			buf[iTxDataIndex++] = ( regs[i].subaddr & 0xFF );
		}

		if ( ( i < size - 1 ) && ( ( iTxDataIndex + regs[i].len ) <= ( SONY_SR130PC20_BURST_DATA_LENGTH - regs[i+1].len ) ) && ( regs[i].subaddr + regs[i].len == regs[i+1].subaddr ) )
		{
			if ( 1 == regs[i].len )
			{
				//printk("2222222 delay 0x%04x, value 0x%04x\n", regs[i].subaddr, regs[i].value);
				buf[iTxDataIndex++] = ( regs[i].value & 0xFF );
			}
			else
			{
				// Little Endian
				buf[iTxDataIndex++] = ( regs[i].value & 0x00FF );
				buf[iTxDataIndex++] = ( regs[i].value & 0xFF00 ) >> 8;
				//printk("3333333 delay 0x%04x, value 0x%04x\n", regs[i].subaddr, regs[i].value);
			}
		}
		else
		{
				if ( 1 == regs[i].len )
				{
					//printk("4444444 delay 0x%04x, value 0x%04x\n", regs[i].subaddr, regs[i].value);
					buf[iTxDataIndex++] = ( regs[i].value & 0xFF );
					//printk("burst_index:%d\n", iTxDataIndex);
					msg.len = iTxDataIndex;

				}
				else
				{
					//printk("555555 delay 0x%04x, value 0x%04x\n", regs[i].subaddr, regs[i].value);
					// Little Endian
					buf[iTxDataIndex++] = (regs[i].value & 0x00FF );
					buf[iTxDataIndex++] = (regs[i].value & 0xFF00 ) >> 8;
					//printk("burst_index:%d\n", iTxDataIndex);
					msg.len = iTxDataIndex;

				}

				while(retry_count--) {
				err  = i2c_transfer(sr130pc20_client->adapter, &msg, 1);
					if (likely(err == 1))
						break;

			}
			iTxDataIndex = 0;
		}
		i++;
	}
#else
	cam_err("burst write: not implemented\n");
#endif
	return 0;
}

static inline int sr130pc20_write_regs(struct v4l2_subdev *sd,
			const sr130pc20_regset_t regs[], int size)
{
	int err = 0, i;
	u8 subaddr, value;

	cam_trace("size %d\n", size);

	for (i = 0; i < size; i++) {
		subaddr = (u8)(regs[i] >> 8);
		value = (u8)(regs[i]);
		if (unlikely(DELAY_SEQ == subaddr))
			msleep_debug(value * 10, true);
		else {
			err = sr130pc20_writeb(sd, subaddr, value);
			CHECK_ERR_MSG(err, "register set failed\n")
                }
	}

	return 0;
}

/* PX: */
static int sr130pc20_set_from_table(struct v4l2_subdev *sd,
				const char *setting_name,
				const struct regset_table *table,
				u32 table_size, s32 index)
{
	int err = 0;

	cam_trace("set %s index %d\n", setting_name, index);

	CHECK_ERR_COND_MSG(((index < 0) || (index >= table_size)),
		-EINVAL, "index(%d) out of range[0:%d] for table for %s\n",
		index, table_size, setting_name);

	table += index;

#ifdef CONFIG_LOAD_FILE
	cam_dbg("%s: \"%s\", reg_name=%s\n", __func__,
			setting_name, table->name);
	return sr130pc20_write_regs_from_sd(sd, table->name);

#else /* !CONFIG_LOAD_FILE */
	CHECK_ERR_COND_MSG(!table->reg, -EFAULT, \
		"table=%s, index=%d, reg = NULL\n", setting_name, index);
# ifdef DEBUG_WRITE_REGS
	cam_dbg("write_regtable: \"%s\", reg_name=%s\n", setting_name,
		table->name);
# endif /* DEBUG_WRITE_REGS */

	if (table->burst) {
		err = sr130pc20_i2c_burst_write_list(sd,
			table->reg, table->array_size, setting_name);
	} else
		err = sr130pc20_write_regs(sd, table->reg, table->array_size);

	CHECK_ERR_MSG(err, "write regs(%s), err=%d\n", setting_name, err);

	return 0;
#endif /* CONFIG_LOAD_FILE */
}

static inline int sr130pc20_hw_stby_on(struct v4l2_subdev *sd, bool on)
{
	struct sr130pc20_state *state = to_state(sd);

#if 0 /* dslim */
	return state->pdata->stby_on(on);
#else
	return 0;
#endif
}

static int sr130pc20_is_om_changed(struct v4l2_subdev *sd)
{
	u32 status = 0x3, val = 0;
	int err, cnt1, cnt2;

	cam_dbg("om changed: int cnt=%d, clr cnt=%d\n", cnt1, cnt2);

	return 0;
}

static inline int sr130pc20_transit_preview_mode(struct v4l2_subdev *sd)
{
	struct sr130pc20_state *state = to_state(sd);
	int err = 0;

	if (state->exposure.ae_lock || state->wb.awb_lock)
		cam_info("Restore user ae(awb)-lock...\n");

	err = sr130pc20_set_from_table(sd, "preview_mode",
		&state->regs->preview_mode, 1, 0);

	return err;
}

static inline int sr130pc20_transit_capture_mode(struct v4l2_subdev *sd)
{
	struct sr130pc20_state *state = to_state(sd);
	int err = -EIO;

	if (state->capture.lowlux_night) {
		cam_info("capture_mode: night lowlux\n");
		err = sr130pc20_set_from_table(sd, "capture_mode_night",
			&state->regs->capture_mode_night, 1, 0);
	} else
		err = sr130pc20_set_from_table(sd, "capture_mode",
			&state->regs->capture_mode, 1, 0);

	return err;
}

/**
 * sr130pc20_transit_movie_mode: switch camera mode if needed.
 * Note that this fuction should be called from start_preview().
 */
static inline int sr130pc20_transit_movie_mode(struct v4l2_subdev *sd)
{
	struct sr130pc20_state *state = to_state(sd);

	/* we'll go from the below modes to RUNNING or RECORDING */
	switch (state->runmode) {
	case RUNMODE_INIT:
		/* case of entering camcorder firstly */
		break;
	case RUNMODE_RUNNING_STOP:
		/* case of switching from camera to camcorder */
		break;
	case RUNMODE_RECORDING_STOP:
		/* case of switching from camcorder to camera */
		break;

	default:
		break;
	}

	return 0;
}

/**
 * sr130pc20_is_hwflash_on - check whether flash device is on
 *
 * Refer to state->flash.on to check whether flash is in use in driver.
 */
static inline int sr130pc20_is_hwflash_on(struct v4l2_subdev *sd)
{
	struct sr130pc20_state *state = to_state(sd);

#ifdef SR130PC20_SUPPORT_FLASH
	return state->pdata->is_flash_on();
#else
	return 0;
#endif
}

/**
 * sr130pc20_flash_en - contro Flash LED
 * @mode: SR130PC20_FLASH_MODE_NORMAL or SR130PC20_FLASH_MODE_MOVIE
 * @onoff: SR130PC20_FLASH_ON or SR130PC20_FLASH_OFF
 */
static int sr130pc20_flash_en(struct v4l2_subdev *sd, s32 mode, s32 onoff)
{
	struct sr130pc20_state *state = to_state(sd);

	if (unlikely(state->flash.ignore_flash)) {
		cam_warn("WARNING, we ignore flash command.\n");
		return 0;
	}

#ifdef SR130PC20_SUPPORT_FLASH
	return state->pdata->flash_en(mode, onoff);
#else
	return 0;
#endif
}

/**
 * sr130pc20_flash_torch - turn flash on/off as torch for preflash, recording
 * @onoff: SR130PC20_FLASH_ON or SR130PC20_FLASH_OFF
 *
 * This func set state->flash.on properly.
 */
static inline int sr130pc20_flash_torch(struct v4l2_subdev *sd, s32 onoff)
{
	struct sr130pc20_state *state = to_state(sd);
	int err = 0;

	err = sr130pc20_flash_en(sd, SR130PC20_FLASH_MODE_MOVIE, onoff);
	state->flash.on = (onoff == SR130PC20_FLASH_ON) ? 1 : 0;

	return err;
}

/**
 * sr130pc20_flash_oneshot - turn main flash on for capture
 * @onoff: SR130PC20_FLASH_ON or SR130PC20_FLASH_OFF
 *
 * Main flash is turn off automatically in some milliseconds.
 */
static inline int sr130pc20_flash_oneshot(struct v4l2_subdev *sd, s32 onoff)
{
	struct sr130pc20_state *state = to_state(sd);
	int err = 0;

	err = sr130pc20_flash_en(sd, SR130PC20_FLASH_MODE_NORMAL, onoff);
	state->flash.on = (onoff == SR130PC20_FLASH_ON) ? 1 : 0;

	return err;
}

static const struct sr130pc20_framesize *sr130pc20_get_framesize
	(const struct sr130pc20_framesize *frmsizes,
	u32 frmsize_count, u32 index)
{
	int i = 0;

	for (i = 0; i < frmsize_count; i++) {
		if (frmsizes[i].index == index)
			return &frmsizes[i];
	}

	return NULL;
}

/* This function is called from the g_ctrl api
 *
 * This function should be called only after the s_fmt call,
 * which sets the required width/height value.
 *
 * It checks a list of available frame sizes and sets the
 * most appropriate frame size.
 *
 * The list is stored in an increasing order (as far as possible).
 * Hence the first entry (searching from the beginning) where both the
 * width and height is more than the required value is returned.
 * In case of no perfect match, we set the last entry (which is supposed
 * to be the largest resolution supported.)
 */
static void sr130pc20_set_framesize(struct v4l2_subdev *sd,
				const struct sr130pc20_framesize *frmsizes,
				u32 num_frmsize, bool preview)
{
	struct sr130pc20_state *state = to_state(sd);
	const struct sr130pc20_framesize **found_frmsize = NULL;
	u32 width = state->req_fmt.width;
	u32 height = state->req_fmt.height;
	int i = 0;

	cam_dbg("%s: Requested Res %dx%d\n", __func__,
			width, height);

	found_frmsize = preview ?
		&state->preview.frmsize : &state->capture.frmsize;

	for (i = 0; i < num_frmsize; i++) {
		if ((frmsizes[i].width == width) &&
			(frmsizes[i].height == height)) {
			*found_frmsize = &frmsizes[i];
			break;
		}
	}

	if (*found_frmsize == NULL) {
		cam_err("%s: error, invalid frame size %dx%d\n",
			__func__, width, height);
		*found_frmsize = preview ?
			sr130pc20_get_framesize(frmsizes, num_frmsize,
					PREVIEW_SZ_VGA) :
			sr130pc20_get_framesize(frmsizes, num_frmsize,
					CAPTURE_SZ_1MP);
		BUG_ON(!(*found_frmsize));
	}

	if (preview)
		cam_info("Preview Res Set: %dx%d, index %d\n",
			(*found_frmsize)->width, (*found_frmsize)->height,
			(*found_frmsize)->index);
	else
		cam_info("Capture Res Set: %dx%d, index %d\n",
			(*found_frmsize)->width, (*found_frmsize)->height,
			(*found_frmsize)->index);
}

/* PX: Set scene mode */
static int sr130pc20_set_scene_mode(struct v4l2_subdev *sd, s32 val)
{
	struct sr130pc20_state *state = to_state(sd);

	cam_trace("E, value %d\n", val);

retry:
	switch (val) {
	case SCENE_MODE_NONE:
	case SCENE_MODE_PORTRAIT:
	case SCENE_MODE_NIGHTSHOT:
	case SCENE_MODE_BACK_LIGHT:
	case SCENE_MODE_LANDSCAPE:
	case SCENE_MODE_SPORTS:
	case SCENE_MODE_PARTY_INDOOR:
	case SCENE_MODE_BEACH_SNOW:
	case SCENE_MODE_SUNSET:
	case SCENE_MODE_DUSK_DAWN:
	case SCENE_MODE_FALL_COLOR:
	case SCENE_MODE_FIREWORKS:
	case SCENE_MODE_TEXT:
	case SCENE_MODE_CANDLE_LIGHT:
		sr130pc20_set_from_table(sd, "scene_mode",
			state->regs->scene_mode,
			ARRAY_SIZE(state->regs->scene_mode), val);
		break;

	default:
		cam_err("set_scene: error, not supported (%d)\n", val);
		val = SCENE_MODE_NONE;
		goto retry;
	}

	state->scene_mode = val;

	cam_trace("X\n");
	return 0;
}

/* PX: Set brightness */
static int sr130pc20_set_exposure(struct v4l2_subdev *sd, s32 val)
{
	struct sr130pc20_state *state = to_state(sd);
	static const u8 gain_level[GET_EV_INDEX(EV_MAX_V4L2)] = {
		0x14, 0x1C, 0x21, 0x26, 0x2B, 0x2F, 0x33, 0x37, 0x3B};
	int err = 0;

	if ((val < EV_MINUS_4) || (val > EV_PLUS_4)) {
		cam_err("%s: error, invalid value(%d)\n", __func__, val);
		return -EINVAL;
	}

	state->lux_level_flash = gain_level[GET_EV_INDEX(val)];

	sr130pc20_set_from_table(sd, "brightness", state->regs->ev,
		ARRAY_SIZE(state->regs->ev), GET_EV_INDEX(val));

	state->exposure.val = val;

	return err;
}

static inline u32 sr130pc20_get_light_level(struct v4l2_subdev *sd,
					u32 *light_level)
{
	struct sr130pc20_state *state = to_state(sd);
	u32 val_lsb = 0, val_msb = 0;

/* To Do */

	cam_trace("X, iso = %d, light level = 0x%X", state->iso, *light_level);

	return 0;
}

/* PX(NEW) */
static int sr130pc20_set_capture_size(struct v4l2_subdev *sd)
{
	struct sr130pc20_state *state = to_state(sd);
	u32 width, height;

	if (unlikely(!state->capture.frmsize)) {
		cam_warn("warning, capture resolution not set\n");
		state->capture.frmsize = sr130pc20_get_framesize(
					sr130pc20_capture_frmsizes,
					ARRAY_SIZE(sr130pc20_capture_frmsizes),
					CAPTURE_SZ_1MP);
	}

	width = state->capture.frmsize->width;
	height = state->capture.frmsize->height;

	cam_dbg("set capture size(%dx%d)\n", width, height);

	return 0;
}

/* PX: Set sensor mode */
static int sr130pc20_set_sensor_mode(struct v4l2_subdev *sd, s32 val)
{
	struct sr130pc20_state *state = to_state(sd);

	cam_trace("mode=%d\n", val);

	switch (val) {
	case SENSOR_MOVIE:
		/* We does not support movie mode when in VT. */
		if (state->vt_mode) {
			state->sensor_mode = SENSOR_CAMERA;
			cam_err("%s: error, Not support movie\n", __func__);
			break;
		}
		/* We do not break. */

	case SENSOR_CAMERA:
		state->sensor_mode = val;
		break;

	default:
		cam_err("%s: error, Not support.(%d)\n", __func__, val);
		state->sensor_mode = SENSOR_CAMERA;
		WARN_ON(1);
		break;
	}

	return 0;
}

/* PX: Set framerate */
static int sr130pc20_set_frame_rate(struct v4l2_subdev *sd, s32 fps)
{
	struct sr130pc20_state *state = to_state(sd);
	int err = -EIO;
	int i = 0, fps_index = -1;

#if 0 /* DSLIM */
	if (!state->initialized || (state->req_fps < 0))
		return 0;

	cam_info("set frame rate %d\n", fps);

	for (i = 0; i < ARRAY_SIZE(sr130pc20_framerates); i++) {
		if (fps == sr130pc20_framerates[i].fps) {
			fps_index = sr130pc20_framerates[i].index;
			state->fps = fps;
			state->req_fps = -1;
			break;
		}
	}

	if (unlikely(fps_index < 0)) {
		cam_err("set_fps: warning, not supported fps %d\n", fps);
		return 0;
	}

	err = sr130pc20_set_from_table(sd, "fps", state->regs->fps,
			ARRAY_SIZE(state->regs->fps), fps_index);
	CHECK_ERR_MSG(err, "fail to set framerate\n");
#endif
	return 0;
}

#if 0 /* DSLIM */
static int sr130pc20_init_param(struct v4l2_subdev *sd)
{
	struct v4l2_control ctrl;
	int i;

	for (i = 0; i < ARRAY_SIZE(sr130pc20_ctrls); i++) {
		if (sr130pc20_ctrls[i].value !=
				sr130pc20_ctrls[i].default_value) {
			ctrl.id = sr130pc20_ctrls[i].id;
			ctrl.value = sr130pc20_ctrls[i].value;
			sr130pc20_s_ctrl(sd, &ctrl);
		}
	}

	return 0;
}
#endif

#if 0 /* dslim */
static int sr130pc20_init_regs(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct sr130pc20_state *state = to_state(sd);
	u16 read_value = 0;
	int err = -ENODEV;


	/* we'd prefer to do this in probe, but the framework hasn't
	 * turned on the camera yet so our i2c operations would fail
	 * if we tried to do it in probe, so we have to do it here
	 * and keep track if we succeeded or not.
	 */

	/* enter read mode */
	err = sr130pc20_i2c_write_twobyte(client, 0x002C, 0x7000);
	if (unlikely(err < 0))
		return -ENODEV;

	sr130pc20_i2c_write_twobyte(client, 0x002E, 0x0150);
	sr130pc20_i2c_read_twobyte(client, 0x0F12, &read_value);
	if (likely(read_value == SR130PC20_CHIP_ID))
		cam_info("Sensor ChipID: 0x%04X\n", SR130PC20_CHIP_ID);
	else
		cam_info("Sensor ChipID: 0x%04X, unknown ChipID\n", read_value);

	sr130pc20_i2c_write_twobyte(client, 0x002C, 0x7000);
	sr130pc20_i2c_write_twobyte(client, 0x002E, 0x0152);
	sr130pc20_i2c_read_twobyte(client, 0x0F12, &read_value);
	if (likely(read_value == SR130PC20_CHIP_REV))
		cam_info("Sensor revision: 0x%04X\n", SR130PC20_CHIP_REV);
	else
		cam_info("Sensor revision: 0x%04X, unknown revision\n",
				read_value);

	/* restore write mode */
	err = sr130pc20_i2c_write_twobyte(client, 0x0028, 0x7000);
	CHECK_ERR_COND(err < 0, -ENODEV);

	state->regs = &reg_datas;

	return 0;
}
#endif

static inline int sr130pc20_do_wait_steamoff(struct v4l2_subdev *sd)
{
	struct sr130pc20_state *state = to_state(sd);
	struct sr130pc20_stream_time *stream_time = &state->stream_time;
	s32 elapsed_msec = 0;
	u32 val = 0;
	int err = 0, count;

	cam_trace("E\n");

	do_gettimeofday(&stream_time->curr_time);

	elapsed_msec = GET_ELAPSED_TIME(stream_time->curr_time, \
				stream_time->before_time) / 1000;

	for (count = 0; count < SR130PC20_CNT_STREAMOFF; count++) {
		err = isx012_readb(sd, 0x00DC, &val);
		CHECK_ERR_MSG(err, "wait_steamoff: error, readb\n")

		if ((val & 0x04) == 0) {
			cam_dbg("wait_steamoff: %dms + cnt(%d)\n",
				elapsed_msec, count);
			break;
		}

		/* cam_info("wait_steamoff: val = 0x%X\n", val);*/
		msleep_debug(2, false);
	}

	if (unlikely(count >= SR130PC20_CNT_STREAMOFF))
		cam_info("wait_steamoff: time-out!\n\n");

	state->need_wait_streamoff = 0;

	return 0;
}

static inline int sr130pc20_fast_capture_switch(struct v4l2_subdev *sd)
{
	u32 val = 0;
	int err = 0, count;

	cam_trace("EX\n");

	for (count = 0; count < SR130PC20_CNT_STREAMOFF; count++) {
		err = isx012_readb(sd, 0x00DC, &val);
		CHECK_ERR_MSG(err, "fast_capture_switch: error, readb\n")

		if ((val & 0x03) == 0x02) {
			cam_dbg("fast_capture_switch: cnt(%d)\n", count);
			break;
		}

		/* cam_info("wait_steamoff: val = 0x%X\n", val);*/
		msleep_debug(2, false);
	}

	if (unlikely(count >= SR130PC20_CNT_STREAMOFF))
		cam_info("fast_capture_switch: time-out!\n\n");

	return 0;
}

static int sr130pc20_wait_steamoff(struct v4l2_subdev *sd)
{
	struct sr130pc20_state *state = to_state(sd);

	if (unlikely(!state->pdata->is_mipi))
		return 0;

	if (state->need_wait_streamoff) {
		sr130pc20_do_wait_steamoff(sd);
		state->need_wait_streamoff = 0;
	} else if (state->format_mode == V4L2_PIX_FMT_MODE_CAPTURE)
		sr130pc20_fast_capture_switch(sd);

	cam_trace("EX\n");

	return 0;
}

static int sr130pc20_control_stream(struct v4l2_subdev *sd, u32 cmd)
{
	struct sr130pc20_state *state = to_state(sd);
	int err = -EINVAL;

	if (cmd == STREAM_STOP) {
		cam_info("STREAM STOP!!\n");
		err = sr130pc20_set_from_table(sd, "stream_stop",
				&state->regs->stream_stop, 1, 0);
		CHECK_ERR_MSG(err, "failed to stop stream\n");
	} else {
		cam_info("STREAM START\n");
		return 0;
	}

	switch (state->runmode) {
	case RUNMODE_CAPTURING:
		cam_dbg("Capture Stop!\n");
		state->runmode = RUNMODE_CAPTURING_STOP;
		state->capture.ready = 0;
		state->capture.lowlux_night = 0;

	case RUNMODE_RUNNING:
		cam_dbg("Preview Stop!\n");
		state->runmode = RUNMODE_RUNNING_STOP;

		/*if (state->capture.pre_req) {
			sr130pc20_prepare_fast_capture(sd);
			state->capture.pre_req = 0;
		}*/
		break;

	case RUNMODE_RECORDING:
		state->runmode = RUNMODE_RECORDING_STOP;
		break;

	default:
		break;
	}

	msleep_debug(state->pdata->streamoff_delay, true);

	return 0;
}

/* PX: Set flash mode */
static int sr130pc20_set_flash_mode(struct v4l2_subdev *sd, s32 val)
{
	struct sr130pc20_state *state = to_state(sd);

	/* movie flash mode should be set when recording is started */
/*	if (state->sensor_mode == SENSOR_MOVIE && !state->recording)
		return 0;*/

	if (state->flash.mode == val) {
		cam_dbg("the same flash mode=%d\n", val);
		return 0;
	}

	if (val == FLASH_MODE_TORCH)
		sr130pc20_flash_torch(sd, SR130PC20_FLASH_ON);

	if ((state->flash.mode == FLASH_MODE_TORCH)
	    && (val == FLASH_MODE_OFF))
		sr130pc20_flash_torch(sd, SR130PC20_FLASH_OFF);

	state->flash.mode = val;
	cam_dbg("Flash mode = %d\n", val);
	return 0;
}

static int sr130pc20_check_esd(struct v4l2_subdev *sd, s32 val)
{
	u32 data = 0, size_h = 0, size_v = 0;

/* To do */
	return 0;

esd_out:
	cam_err("Check ESD(%d): ESD Shock detected! val=0x%X\n\n", data, val);
	return -ERESTART;
}

/* returns the real iso currently used by sensor due to lighting
 * conditions, not the requested iso we sent using s_ctrl.
 */
static inline int sr130pc20_get_exif_iso(struct v4l2_subdev *sd, u16 *iso)
{
	static const u16 iso_table[] = { 0, 25, 32, 40, 50, 64, 80, 100,
		125, 160, 200, 250, 320, 400, 500, 640, 800,
		1000, 1250, 1600};
	u32 val = 0;

    /* To do */
	*iso = iso_table[val];

	cam_dbg("reg=%d, ISO=%d\n", val, *iso);
	return 0;
}

/* PX: Set ISO */
static int __used sr130pc20_set_iso(struct v4l2_subdev *sd, s32 val)
{
	struct sr130pc20_state *state = to_state(sd);

	sr130pc20_set_from_table(sd, "iso", state->regs->iso,
		ARRAY_SIZE(state->regs->iso), val);

	state->iso = val;

	cam_trace("X\n");
	return 0;
}

/* PX: Return exposure time (ms) */
static inline int sr130pc20_get_exif_exptime(struct v4l2_subdev *sd,
						u32 *exp_time)
{
	u32 val_lsb = 0, val_msb = 0;

    /* To Do */
	*exp_time = (val_msb << 16) | (val_lsb & 0xFFFF);
	cam_dbg("exposure time %dus\n", *exp_time);
	return 0;
}

static inline void sr130pc20_get_exif_flash(struct v4l2_subdev *sd,
					u16 *flash)
{
	struct sr130pc20_state *state = to_state(sd);

	*flash = 0;

	switch (state->flash.mode) {
	case FLASH_MODE_OFF:
		*flash |= EXIF_FLASH_MODE_SUPPRESSION;
		break;

	case FLASH_MODE_AUTO:
		*flash |= EXIF_FLASH_MODE_AUTO;
		break;

	case FLASH_MODE_ON:
	case FLASH_MODE_TORCH:
		*flash |= EXIF_FLASH_MODE_FIRING;
		break;

	default:
		break;
	}

	if (state->flash.on)
		*flash |= EXIF_FLASH_FIRED;
}

/* PX: */
static int sr130pc20_get_exif(struct v4l2_subdev *sd)
{
	struct sr130pc20_state *state = to_state(sd);
	u32 exposure_time = 0;
	u32 int_dec, integer;

	/* exposure time */
	state->exif.exp_time_den = 0;
	sr130pc20_get_exif_exptime(sd, &exposure_time);
	if (exposure_time) {
		state->exif.exp_time_den = 1000 * 1000 / exposure_time;

		int_dec = (1000 * 1000) * 10 / exposure_time;
		integer = state->exif.exp_time_den * 10;

		/* Round off */
		if ((int_dec - integer) > 5)
			state->exif.exp_time_den += 1;
	} else
		state->exif.exp_time_den = 0;

	/* iso */
	state->exif.iso = 0;
	sr130pc20_get_exif_iso(sd, &state->exif.iso);

	/* flash */
	sr130pc20_get_exif_flash(sd, &state->exif.flash);

	cam_dbg("EXIF: ex_time_den=%d, iso=%d, flash=0x%02X\n",
		state->exif.exp_time_den, state->exif.iso, state->exif.flash);

	return 0;
}

static int sr130pc20_set_preview_size(struct v4l2_subdev *sd)
{
	struct sr130pc20_state *state = to_state(sd);
	u32 width, height;

	if (!state->preview.update_frmsize)
		return 0;

	if (unlikely(!state->preview.frmsize)) {
		cam_warn("warning, preview resolution not set\n");
		state->preview.frmsize = sr130pc20_get_framesize(
					sr130pc20_preview_frmsizes,
					ARRAY_SIZE(sr130pc20_preview_frmsizes),
					PREVIEW_SZ_XGA);
	}

	width = state->preview.frmsize->width;
	height = state->preview.frmsize->height;

	cam_dbg("set preview size(%dx%d)\n", width, height);

	state->preview.update_frmsize = 0;

	return 0;
}

static int sr130pc20_start_preview(struct v4l2_subdev *sd)
{
	struct sr130pc20_state *state = to_state(sd);
	int err = -EINVAL;

	cam_info("Camera Preview start, runmode = %d\n", state->runmode);

	if ((state->runmode == RUNMODE_NOTREADY) ||
	    (state->runmode == RUNMODE_CAPTURING)) {
		cam_err("%s: error - Invalid runmode\n", __func__);
		return -EPERM;
	}

	state->focus.status = AF_RESULT_NONE;
	state->flash.preflash = PREFLASH_NONE;
	state->focus.touch = 0;

#if 1
	err = sr130pc20_set_from_table(sd, "init_reg",
			       &state->regs->init_reg, 1, 0);
	CHECK_ERR_MSG(err, "failed to initialize camera device\n");
#else
	/* Transit preview mode */
	err = sr130pc20_transit_preview_mode(sd);
	CHECK_ERR_MSG(err, "preview_mode(%d)\n", err);
	sr130pc20_set_preview_size(sd);
#endif
	sr130pc20_control_stream(sd, STREAM_START);

	state->runmode = (state->sensor_mode == SENSOR_CAMERA) ?
			RUNMODE_RUNNING : RUNMODE_RECORDING;
	return 0;
}

static int sr130pc20_set_capture(struct v4l2_subdev *sd)
{
	struct sr130pc20_state *state = to_state(sd);
	int err = 0;

	cam_info("set_capture\n");
	/* Transit to capture mode */
	err = sr130pc20_transit_capture_mode(sd);
	CHECK_ERR_MSG(err, "fail to capture_mode (%d)\n", err);
	return 0;
}

static int sr130pc20_prepare_fast_capture(struct v4l2_subdev *sd)
{
	struct sr130pc20_state *state = to_state(sd);
	int err = 0;

	cam_info("prepare_fast_capture\n");

	state->req_fmt.width = (state->capture.pre_req >> 16);
	state->req_fmt.height = (state->capture.pre_req & 0xFFFF);
	sr130pc20_set_framesize(sd, sr130pc20_capture_frmsizes,
		ARRAY_SIZE(sr130pc20_capture_frmsizes), false);

	err = sr130pc20_set_capture(sd);
	CHECK_ERR(err);

	state->capture.ready = 1;

	return 0;
}

static int sr130pc20_start_capture(struct v4l2_subdev *sd)
{
	struct sr130pc20_state *state = to_state(sd);
	int err = -ENODEV, count;
	u32 val = 0;
	u32 night_delay;

	cam_info("start_capture\n");

	if (!state->capture.ready) {
		err = sr130pc20_set_capture(sd);
		CHECK_ERR(err);

		sr130pc20_control_stream(sd, STREAM_START);
		night_delay = 500;
	} else
		night_delay = 700; /* for completely skipping 1 frame. */

	state->runmode = RUNMODE_CAPTURING;

	if (state->capture.lowlux_night)
		msleep_debug(night_delay, true);

	/* Get EXIF */
	sr130pc20_get_exif(sd);

	return 0;
}

/**
 * sr200pc20_init_regs: Indentify chip and get pointer to reg table
 * @
 */
static int sr130pc20_init_regs(struct v4l2_subdev *sd)
{
	struct sr130pc20_state *state = to_state(sd);
	int err = -ENODEV;
	u8 read_value = 0;

	err = sr130pc20_writeb(sd, 0x03, 0x00);
	CHECK_ERR_COND(err < 0, -ENODEV);

	sr130pc20_readb(sd, 0x04, &read_value);
	if (SR130PC20_CHIP_ID == read_value)
		cam_info("Sensor ChipID: 0x%02X\n", SR130PC20_CHIP_ID);
	else
		cam_info("Sensor ChipID: 0x%02X, unknown chipID\n", read_value);

	state->regs = &reg_datas;

	return 0;
}


/* PX(NEW) */
static int sr130pc20_s_mbus_fmt(struct v4l2_subdev *sd,
				struct v4l2_mbus_framefmt *fmt)
{
	struct sr130pc20_state *state = to_state(sd);
	s32 previous_index = 0;

	cam_dbg("%s: pixelformat = 0x%x, colorspace = 0x%x, width = %d, height = %d\n",
		__func__, fmt->code, fmt->colorspace, fmt->width, fmt->height);

	v4l2_fill_pix_format(&state->req_fmt, fmt);
	if (fmt->field < IS_MODE_CAPTURE_STILL)
		state->format_mode = V4L2_PIX_FMT_MODE_PREVIEW;
	else
		state->format_mode = V4L2_PIX_FMT_MODE_CAPTURE;

	if (state->format_mode != V4L2_PIX_FMT_MODE_CAPTURE) {
		previous_index = state->preview.frmsize ?
				state->preview.frmsize->index : -1;
		sr130pc20_set_framesize(sd, sr130pc20_preview_frmsizes,
			ARRAY_SIZE(sr130pc20_preview_frmsizes), true);

		if (previous_index != state->preview.frmsize->index)
			state->preview.update_frmsize = 1;
	} else {
		/*
		 * In case of image capture mode,
		 * if the given image resolution is not supported,
		 * use the next higher image resolution. */
		sr130pc20_set_framesize(sd, sr130pc20_capture_frmsizes,
			ARRAY_SIZE(sr130pc20_capture_frmsizes), false);

		/* for maket app.
		 * Samsung camera app does not use unmatched ratio.*/
		if (unlikely(NULL == state->preview.frmsize)) {
			cam_warn("warning, capture without preview resolution\n");
		} else if (unlikely(FRM_RATIO(state->preview.frmsize)
		    != FRM_RATIO(state->capture.frmsize))) {
			cam_warn("warning, capture ratio " \
				"is different with preview ratio\n");
		}
	}

	return 0;
}

static int sr130pc20_enum_mbus_fmt(struct v4l2_subdev *sd, unsigned int index,
					enum v4l2_mbus_pixelcode *code)
{
	cam_dbg("%s: index = %d\n", __func__, index);

	if (index >= ARRAY_SIZE(capture_fmts))
		return -EINVAL;

	*code = capture_fmts[index].code;

	return 0;
}

static int sr130pc20_try_mbus_fmt(struct v4l2_subdev *sd,
				struct v4l2_mbus_framefmt *fmt)
{
	int num_entries;
	int i;

	num_entries = ARRAY_SIZE(capture_fmts);

	cam_dbg("%s: code = 0x%x , colorspace = 0x%x, num_entries = %d\n",
		__func__, fmt->code, fmt->colorspace, num_entries);

	for (i = 0; i < num_entries; i++) {
		if (capture_fmts[i].code == fmt->code &&
		    capture_fmts[i].colorspace == fmt->colorspace) {
			cam_info("%s: match found, returning 0\n", __func__);
			return 0;
		}
	}

	cam_err("%s: no match found, returning -EINVAL\n", __func__);
	return -EINVAL;
}


static int sr130pc20_enum_framesizes(struct v4l2_subdev *sd,
				  struct v4l2_frmsizeenum *fsize)
{
	struct sr130pc20_state *state = to_state(sd);

	/*
	* The camera interface should read this value, this is the resolution
	* at which the sensor would provide framedata to the camera i/f
	* In case of image capture,
	* this returns the default camera resolution (VGA)
	*/
	if (state->format_mode != V4L2_PIX_FMT_MODE_CAPTURE) {
		if (unlikely(state->preview.frmsize == NULL)) {
			cam_err("%s: error\n", __func__);
			return -EFAULT;
		}

		fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
		fsize->discrete.width = state->preview.frmsize->width;
		fsize->discrete.height = state->preview.frmsize->height;
	} else {
		if (unlikely(state->capture.frmsize == NULL)) {
			cam_err("%s: error\n", __func__);
			return -EFAULT;
		}

		fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
		fsize->discrete.width = state->capture.frmsize->width;
		fsize->discrete.height = state->capture.frmsize->height;
	}

	return 0;
}

static int sr130pc20_g_parm(struct v4l2_subdev *sd,
			struct v4l2_streamparm *param)
{
	return 0;
}

static int sr130pc20_s_parm(struct v4l2_subdev *sd,
			struct v4l2_streamparm *param)
{
	struct sr130pc20_state *state = to_state(sd);

	state->req_fps = param->parm.capture.timeperframe.denominator /
			param->parm.capture.timeperframe.numerator;

	cam_dbg("s_parm state->fps=%d, state->req_fps=%d\n",
		state->fps, state->req_fps);

	if ((state->req_fps < 0) || (state->req_fps > 30)) {
		cam_err("%s: error, invalid frame rate %d. we'll set to 30\n",
				__func__, state->req_fps);
		state->req_fps = 0;
	}

	return sr130pc20_set_frame_rate(sd, state->req_fps);
}

static int sr130pc20_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct sr130pc20_state *state = to_state(sd);
	int err = 0;

	if (!state->initialized) {
		cam_err("%s: WARNING, camera not initialized\n", __func__);
		return 0;
	}

	mutex_lock(&state->ctrl_lock);

	switch (ctrl->id) {
	case V4L2_CID_CAMERA_EXIF_EXPTIME:
		if (state->sensor_mode == SENSOR_CAMERA)
			ctrl->value = state->exif.exp_time_den;
		else
			ctrl->value = 24;
		break;

	case V4L2_CID_CAMERA_EXIF_ISO:
		if (state->sensor_mode == SENSOR_CAMERA)
			ctrl->value = state->exif.iso;
		else
			ctrl->value = 100;
		break;

	case V4L2_CID_CAMERA_EXIF_FLASH:
		if (state->sensor_mode == SENSOR_CAMERA)
			ctrl->value = state->exif.flash;
		else
			sr130pc20_get_exif_flash(sd, (u16 *)ctrl->value);
		break;

#if !defined(CONFIG_CAM_YUV_CAPTURE)
	case V4L2_CID_CAM_JPEG_MAIN_SIZE:
		ctrl->value = state->jpeg.main_size;
		break;

	case V4L2_CID_CAM_JPEG_MAIN_OFFSET:
		ctrl->value = state->jpeg.main_offset;
		break;

	case V4L2_CID_CAM_JPEG_THUMB_SIZE:
		ctrl->value = state->jpeg.thumb_size;
		break;

	case V4L2_CID_CAM_JPEG_THUMB_OFFSET:
		ctrl->value = state->jpeg.thumb_offset;
		break;

	case V4L2_CID_CAM_JPEG_QUALITY:
		ctrl->value = state->jpeg.quality;
		break;

	case V4L2_CID_CAM_JPEG_MEMSIZE:
		ctrl->value = SENSOR_JPEG_SNAPSHOT_MEMSIZE;
		break;
#endif

	case V4L2_CID_CAMERA_AUTO_FOCUS_RESULT:
		ctrl->value = state->focus.status;
		break;

	case V4L2_CID_CAMERA_WHITE_BALANCE:
	case V4L2_CID_CAMERA_EFFECT:
	case V4L2_CID_CAMERA_CONTRAST:
	case V4L2_CID_CAMERA_SATURATION:
	case V4L2_CID_CAMERA_SHARPNESS:
	case V4L2_CID_CAMERA_OBJ_TRACKING_STATUS:
	case V4L2_CID_CAMERA_SMART_AUTO_STATUS:
	default:
		cam_err("%s: WARNING, unknown Ctrl-ID 0x%x\n",
					__func__, ctrl->id);
		err = 0; /* we return no error. */
		break;
	}

	mutex_unlock(&state->ctrl_lock);

	return err;
}

static int sr130pc20_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct sr130pc20_state *state = to_state(sd);
	int err = 0;

	if (!state->initialized && ctrl->id != V4L2_CID_CAMERA_SENSOR_MODE) {
		cam_warn("%s: WARNING, camera not initialized. ID = %d(0x%X)\n",
			__func__, ctrl->id - V4L2_CID_PRIVATE_BASE,
			ctrl->id - V4L2_CID_PRIVATE_BASE);
		return 0;
	}

	cam_dbg("%s: ID =%d, val = %d\n",
		__func__, ctrl->id - V4L2_CID_PRIVATE_BASE, ctrl->value);

	mutex_lock(&state->ctrl_lock);

	switch (ctrl->id) {
	case V4L2_CID_CAMERA_SENSOR_MODE:
		err = sr130pc20_set_sensor_mode(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_OBJECT_POSITION_X:
		state->focus.pos_x = ctrl->value;
		err = 0;
		break;

	case V4L2_CID_CAMERA_OBJECT_POSITION_Y:
		state->focus.pos_y = ctrl->value;
		err = 0;
		break;

	case V4L2_CID_CAMERA_TOUCH_AF_START_STOP:
		break;

	case V4L2_CID_CAMERA_FOCUS_MODE:
		break;

	case V4L2_CID_CAMERA_SET_AUTO_FOCUS:
		break;

	case V4L2_CID_CAMERA_FLASH_MODE:
		err = sr130pc20_set_flash_mode(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_BRIGHTNESS:
		err = sr130pc20_set_exposure(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_WHITE_BALANCE:
		err = sr130pc20_set_from_table(sd, "white balance",
			state->regs->white_balance,
			ARRAY_SIZE(state->regs->white_balance), ctrl->value);
		state->wb.mode = ctrl->value;
		break;

	case V4L2_CID_CAMERA_EFFECT:
		err = sr130pc20_set_from_table(sd, "effects",
			state->regs->effect,
			ARRAY_SIZE(state->regs->effect), ctrl->value);
		break;

	case V4L2_CID_CAMERA_METERING:
		err = sr130pc20_set_from_table(sd, "metering",
			state->regs->metering,
			ARRAY_SIZE(state->regs->metering), ctrl->value);
		break;

	case V4L2_CID_CAMERA_CONTRAST:
		err = sr130pc20_set_from_table(sd, "contrast",
			state->regs->contrast,
			ARRAY_SIZE(state->regs->contrast), ctrl->value);
		break;

	case V4L2_CID_CAMERA_SCENE_MODE:
		err = sr130pc20_set_scene_mode(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_CHECK_ESD:
		err = sr130pc20_check_esd(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_ISO:
		err = sr130pc20_set_iso(sd, ctrl->value);
		break;

	case V4L2_CID_CAMERA_CAPTURE_MODE:
		if (RUNMODE_RUNNING == state->runmode)
			state->capture.pre_req = ctrl->value;

		break;

	case V4L2_CID_CAMERA_ANTI_BANDING:
		break;

	case V4L2_CID_CAMERA_SATURATION:
	case V4L2_CID_CAMERA_SHARPNESS:
	case V4L2_CID_CAMERA_FRAME_RATE:
	case V4L2_CID_CAMERA_AE_LOCK_UNLOCK:
	case V4L2_CID_CAMERA_AWB_LOCK_UNLOCK:
	default:
		cam_err("%s: WARNING, unknown Ctrl-ID 0x%x\n",
			__func__, ctrl->id);
		/* we return no error. */
		break;
	}

	mutex_unlock(&state->ctrl_lock);
	CHECK_ERR_MSG(err, "s_ctrl failed %d\n", err)

	return 0;
}

static int sr130pc20_s_ext_ctrl(struct v4l2_subdev *sd,
			      struct v4l2_ext_control *ctrl)
{
	return 0;
}

static int sr130pc20_s_ext_ctrls(struct v4l2_subdev *sd,
				struct v4l2_ext_controls *ctrls)
{
	struct v4l2_ext_control *ctrl = ctrls->controls;
	int ret;
	int i;

	for (i = 0; i < ctrls->count; i++, ctrl++) {
		ret = sr130pc20_s_ext_ctrl(sd, ctrl);

		if (ret) {
			ctrls->error_idx = i;
			break;
		}
	}

	return ret;
}

static int sr130pc20_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct sr130pc20_state *state = to_state(sd);
	int err = 0;

	cam_info("stream mode = %d\n", enable);

	BUG_ON(!state->initialized);

	switch (enable) {
	case STREAM_MODE_CAM_OFF:
		if (state->pdata->is_mipi)
			err = sr130pc20_control_stream(sd, STREAM_STOP);
		break;

	case STREAM_MODE_CAM_ON:
		switch (state->sensor_mode) {
		case SENSOR_CAMERA:
			if (state->format_mode == V4L2_PIX_FMT_MODE_CAPTURE)
				err = sr130pc20_start_capture(sd);
			else
				err = sr130pc20_start_preview(sd);
			break;

		case SENSOR_MOVIE:
			err = sr130pc20_start_preview(sd);
			break;

		default:
			break;
		}
		break;

	case STREAM_MODE_MOVIE_OFF:
		cam_info("movie off");
		state->recording = 0;
		if (state->flash.on)
			sr130pc20_flash_torch(sd, SR130PC20_FLASH_OFF);
		break;

	case STREAM_MODE_MOVIE_ON:
		cam_info("movie on");
		state->recording = 1;
		if (state->flash.mode != FLASH_MODE_OFF)
			sr130pc20_flash_torch(sd, SR130PC20_FLASH_ON);
		break;

	default:
		cam_err("%s: error - Invalid stream mode\n", __func__);
		break;
	}

	CHECK_ERR_MSG(err, "failed\n");

	return 0;
}

static inline int sr130pc20_check_i2c(struct v4l2_subdev *sd, u16 data)
{
	int err;
	u32 val;

	err = sr130pc20_readw(sd, 0x0000, &val);
	if (unlikely(err))
		return err;

	cam_dbg("version: 0x%04X is 0x6017?\n", val);
	return 0;
}

static void sr130pc20_init_parameter(struct v4l2_subdev *sd)
{
	struct sr130pc20_state *state = to_state(sd);

	state->runmode = RUNMODE_INIT;

	/* Default state values */
	state->scene_mode = SCENE_MODE_NONE;
	state->wb.mode = WHITE_BALANCE_AUTO;
	state->light_level = LUX_LEVEL_MAX;

	/* Set update_frmsize to 1 for case of power reset */
	state->preview.update_frmsize = 1;

	/* Initialize focus field for case of init after power reset. */
	memset(&state->focus, 0, sizeof(state->focus));

	state->lux_level_flash = LUX_LEVEL_FLASH_ON;
	state->shutter_level_flash = 0x0;

}

static int sr130pc20_init(struct v4l2_subdev *sd, u32 val)
{
	struct sr130pc20_state *state = to_state(sd);
	int err = -EINVAL;

	cam_info("init: start v08(%s)\n", __DATE__);

#ifdef CONFIG_LOAD_FILE
	err = sr130pc20_regs_table_init();
	CHECK_ERR_MSG(err, "loading setfile fail!\n");
#endif

	err = sr130pc20_init_regs(sd);
	CHECK_ERR_MSG(err, "failed to indentify sensor chip\n");

	/* err = sr130pc20_set_from_table(sd, "init_reg",
			       &state->regs->init_reg, 1, 0);
	CHECK_ERR_MSG(err, "failed to initialize camera device\n");*//*Temp*/

	sr130pc20_init_parameter(sd);
	state->initialized = 1;

	/* Call after setting state->initialized to 1 */
	err = sr130pc20_set_frame_rate(sd, state->req_fps);
	CHECK_ERR(err);

	return 0;
}

/*
 * s_config subdev ops
 * With camera device, we need to re-initialize
 * every single opening time therefor,
 * it is not necessary to be initialized on probe time.
 * except for version checking
 * NOTE: version checking is optional
 */
static int sr130pc20_s_config(struct v4l2_subdev *sd,
			int irq, void *platform_data)
{
	struct sr130pc20_state *state = to_state(sd);
	int i;
#ifdef CONFIG_LOAD_FILE
	int err = 0;
#endif

	if (!platform_data) {
		cam_err("%s: error, no platform data\n", __func__);
		return -ENODEV;
	}
	state->pdata = platform_data;
	state->dbg_level = &state->pdata->dbg_level;

	/*
	 * Assign default format and resolution
	 * Use configured default information in platform data
	 * or without them, use default information in driver
	 */
	state->req_fmt.width = state->pdata->default_width;
	state->req_fmt.height = state->pdata->default_height;

	if (!state->pdata->pixelformat)
		state->req_fmt.pixelformat = DEFAULT_PIX_FMT;
	else
		state->req_fmt.pixelformat = state->pdata->pixelformat;

	if (!state->pdata->freq)
		state->freq = DEFAULT_MCLK;	/* 24MHz default */
	else
		state->freq = state->pdata->freq;

	state->preview.frmsize = state->capture.frmsize = NULL;
	state->sensor_mode = SENSOR_CAMERA;
	state->format_mode = V4L2_PIX_FMT_MODE_PREVIEW;
	state->fps = 0;
	state->req_fps = -1;

	/* Initialize the independant HW module like flash here */
	state->flash.mode = FLASH_MODE_OFF;
	state->flash.on = 0;

	for (i = 0; i < ARRAY_SIZE(sr130pc20_ctrls); i++)
		sr130pc20_ctrls[i].value = sr130pc20_ctrls[i].default_value;

#ifdef SR130PC20_SUPPORT_FLASH
	if (sr130pc20_is_hwflash_on(sd))
		state->flash.ignore_flash = 1;
#endif

	state->regs = &reg_datas;

	return 0;
}

static const struct v4l2_subdev_core_ops sr130pc20_core_ops = {
	.init = sr130pc20_init,	/* initializing API */
	.g_ctrl = sr130pc20_g_ctrl,
	.s_ctrl = sr130pc20_s_ctrl,
	.s_ext_ctrls = sr130pc20_s_ext_ctrls,
	/*eset = sr130pc20_reset, */
};

static const struct v4l2_subdev_video_ops sr130pc20_video_ops = {
	.s_mbus_fmt = sr130pc20_s_mbus_fmt,
	.enum_framesizes = sr130pc20_enum_framesizes,
	.enum_mbus_fmt = sr130pc20_enum_mbus_fmt,
	.try_mbus_fmt = sr130pc20_try_mbus_fmt,
	.g_parm = sr130pc20_g_parm,
	.s_parm = sr130pc20_s_parm,
	.s_stream = sr130pc20_s_stream,
};

static const struct v4l2_subdev_ops sr130pc20_ops = {
	.core = &sr130pc20_core_ops,
	.video = &sr130pc20_video_ops,
};


/*
 * sr130pc20_probe
 * Fetching platform data is being done with s_config subdev call.
 * In probe routine, we just register subdev device
 */
static int sr130pc20_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct sr130pc20_state *state;
	int err = -EINVAL;

	state = kzalloc(sizeof(struct sr130pc20_state), GFP_KERNEL);
	if (unlikely(!state)) {
		dev_err(&client->dev, "probe, fail to get memory\n");
		return -ENOMEM;
	}

	mutex_init(&state->ctrl_lock);

	state->runmode = RUNMODE_NOTREADY;
	sd = &state->sd;
	strcpy(sd->name, SR130PC20_DRIVER_NAME);

	/* Registering subdev */
	v4l2_i2c_subdev_init(sd, client, &sr130pc20_ops);

	state->workqueue = create_workqueue("cam_workqueue");
	if (unlikely(!state->workqueue)) {
		dev_err(&client->dev, "probe, fail to create workqueue\n");
		goto err_out;
	}

	err = sr130pc20_s_config(sd, 0, client->dev.platform_data);
	CHECK_ERR_MSG(err, "fail to s_config\n");

	printk(KERN_DEBUG "%s %s: driver probed!!\n",
		dev_driver_string(&client->dev), dev_name(&client->dev));

	return 0;

err_out:
	kfree(state);
	return -ENOMEM;
}

static int sr130pc20_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct sr130pc20_state *state = to_state(sd);

	destroy_workqueue(state->workqueue);

	/* Check whether flash is on when unlolading driver,
	 * to preventing Market App from controlling improperly flash.
	 * It isn't necessary in case that you power flash down
	 * in power routine to turn camera off.*/
	if (unlikely(state->flash.on && !state->flash.ignore_flash))
		sr130pc20_flash_torch(sd, SR130PC20_FLASH_OFF);

	v4l2_device_unregister_subdev(sd);
	mutex_destroy(&state->ctrl_lock);
	kfree(state);

	printk(KERN_DEBUG "%s %s: driver removed!!\n",
		dev_driver_string(&client->dev), dev_name(&client->dev));
	return 0;
}

static int is_sysdev(struct device *dev, void *str)
{
	return !strcmp(dev_name(dev), (char *)str) ? 1 : 0;
}

static ssize_t cam_loglevel_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	char temp_buf[60] = {0,};

	sprintf(buf, "Log Level: ");
	if (dbg_level & CAMDBG_LEVEL_TRACE) {
		sprintf(temp_buf, "trace ");
		strcat(buf, temp_buf);
	}

	if (dbg_level & CAMDBG_LEVEL_DEBUG) {
		sprintf(temp_buf, "debug ");
		strcat(buf, temp_buf);
	}

	if (dbg_level & CAMDBG_LEVEL_INFO) {
		sprintf(temp_buf, "info ");
		strcat(buf, temp_buf);
	}

	sprintf(temp_buf, "\n - warn and error level is always on\n\n");
	strcat(buf, temp_buf);

	return strlen(buf);
}

static ssize_t cam_loglevel_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	printk(KERN_DEBUG "CAM buf=%s, count=%d\n", buf, count);

	if (strstr(buf, "trace"))
		dbg_level |= CAMDBG_LEVEL_TRACE;
	else
		dbg_level &= ~CAMDBG_LEVEL_TRACE;

	if (strstr(buf, "debug"))
		dbg_level |= CAMDBG_LEVEL_DEBUG;
	else
		dbg_level &= ~CAMDBG_LEVEL_DEBUG;

	if (strstr(buf, "info"))
		dbg_level |= CAMDBG_LEVEL_INFO;

	return count;
}

static DEVICE_ATTR(loglevel, 0664, cam_loglevel_show, cam_loglevel_store);

static int sr130pc20_create_dbglogfile(struct class *cls)
{
	struct device *dev;
	int err;

	dbg_level |= CAMDBG_LEVEL_DEFAULT;

	dev = class_find_device(cls, NULL, "front", is_sysdev);
	if (unlikely(!dev)) {
		pr_info("[SR130PC20] can not find front device\n");
		return 0;
	}

	err = device_create_file(dev, &dev_attr_loglevel);
	if (unlikely(err < 0)) {
		pr_err("cam_init: failed to create device file, %s\n",
			dev_attr_loglevel.attr.name);
	}

	return 0;
}

static const struct i2c_device_id sr130pc20_id[] = {
	{ SR130PC20_DRIVER_NAME, 0 },
	{}
};

MODULE_DEVICE_TABLE(i2c, sr130pc20_id);

static struct i2c_driver v4l2_i2c_driver = {
	.driver.name	= SR130PC20_DRIVER_NAME,
	.probe		= sr130pc20_probe,
	.remove		= sr130pc20_remove,
	.id_table	= sr130pc20_id,
};

static int __init v4l2_i2c_drv_init(void)
{
	pr_info("%s: %s called\n", __func__, SR130PC20_DRIVER_NAME); /* dslim*/
	/* sr130pc20_create_file(camera_class); */ /*dslim */
	sr130pc20_create_dbglogfile(camera_class);
	return i2c_add_driver(&v4l2_i2c_driver);
}

static void __exit v4l2_i2c_drv_cleanup(void)
{
	pr_info("%s: %s called\n", __func__, SR130PC20_DRIVER_NAME); /* dslim*/
	i2c_del_driver(&v4l2_i2c_driver);
}

module_init(v4l2_i2c_drv_init);
module_exit(v4l2_i2c_drv_cleanup);

MODULE_DESCRIPTION("SILICONFILE SR130PC20 1.3MP SOC camera driver");
MODULE_AUTHOR("Dong-Seong Lim <dongseong.lim@samsung.com>");
MODULE_LICENSE("GPL");
