// SPDX-License-Identifier: GPL-2.0-only
/*
 * Lontium HDMI to CSI receiver management.
 */

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kref.h>
#include <linux/ktime.h>
#include <linux/math64.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of_device.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/property.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

#define LONTIUM_HDMI_PAGE_SELECT	0xff
#define LONTIUM_HDMI_POLL_MS	1000
#define LONTIUM_HDMI_SETTLE_MS	50
#define LONTIUM_HDMI_RETRIES	5
#define LONTIUM_HDMI_STATUS_SIZE	512

/* LT6911C / LT6911UXC register access. */
#define LT6911_I2C_CTRL			0x80ee

/* LT6911C. */
#define LT6911C_VIDEO_MEASURE		0xd283
#define LT6911C_VIDEO_MEASURE_START	0x11
#define LT6911C_HDMI_HACTIVE		0xd28b
#define LT6911C_HDMI_VACTIVE		0xd296
#define LT6911C_HDMI_HACTIVE_SCALE	2
#define LT6911C_CSI_HACTIVE		0xc238
#define LT6911C_CSI_VACTIVE		0xc206

/* LT6911UXC. */
#define LT6911UXC_VIDEO_CTRL		0x86ee
#define LT6911UXC_VIDEO_STATE		0x86a3
#define LT6911UXC_VIDEO_READY		0x55
#define LT6911UXC_VIDEO_OFF		0x88
#define LT6911UXC_CSI_HACTIVE		0x85ea
#define LT6911UXC_CSI_VACTIVE		0x85f0
#define LT6911UXC_CSI_HTOTAL		0xd426
#define LT6911UXC_CSI_VTOTAL		0xd432
#define LT6911UXC_CLOCK_MEASURE		0x8540
#define LT6911UXC_CLOCK_MEASURE_START	0x21
#define LT6911UXC_HALF_PIXELCLOCK	0x8548
#define LT6911UXC_HALF_PIXELCLOCK_MASK	0x0fffff

/* LT6911D. */
#define LT6911D_VIDEO_TIMINGS		0xe085
#define LT6911D_VIDEO_TIMINGS_SIZE	11
#define LT6911D_HALF_PIXELCLOCK_MASK	0x0fffff
#define LT6911D_HORIZONTAL_SCALE	2

/* LT7911EXS. */
#define LT7911EXS_INT_TYPE		0xe084
#define LT7911EXS_VIDEO_OFF		0x00
#define LT7911EXS_VIDEO_READY		0x01
#define LT7911EXS_AUDIO_OFF		0x02
#define LT7911EXS_AUDIO_READY		0x03
#define LT7911EXS_VIDEO_TIMINGS		0xe085
#define LT7911EXS_VIDEO_TIMINGS_SIZE	11

enum lontium_hdmi_signal {
	LONTIUM_HDMI_SIGNAL_UNKNOWN,
	LONTIUM_HDMI_SIGNAL_OFF,
	LONTIUM_HDMI_SIGNAL_ABSENT,
	LONTIUM_HDMI_SIGNAL_UNSTABLE,
	LONTIUM_HDMI_SIGNAL_VALID,
	LONTIUM_HDMI_SIGNAL_ERROR,
};

struct lontium_hdmi_timings {
	u32 width;
	u32 height;
	u32 htotal;
	u32 vtotal;
	u64 pixelclock_hz;
	u32 fps_millihz;
};

struct lontium_hdmi_state {
	bool powered;
	enum lontium_hdmi_signal signal;
	struct lontium_hdmi_timings input;
	struct lontium_hdmi_timings csi;
	int error;
};

struct lontium_hdmi;

enum lontium_hdmi_file {
	LONTIUM_HDMI_FILE_STATUS,
	LONTIUM_HDMI_FILE_CHIP,
	LONTIUM_HDMI_FILE_SIGNAL,
	LONTIUM_HDMI_FILE_INPUT,
	LONTIUM_HDMI_FILE_CSI,
	LONTIUM_HDMI_FILE_ERROR,
	LONTIUM_HDMI_FILE_POWER,
	LONTIUM_HDMI_FILE_COUNT,
};

struct lontium_hdmi_node {
	struct lontium_hdmi *hdmi;
	enum lontium_hdmi_file field;
};

struct lontium_hdmi_chip {
	const char *name;
	unsigned long irq_flags;
	bool i2c_control;
	/* Called with io_lock held for the entire access transaction. */
	int (*read_state)(struct lontium_hdmi *hdmi,
			  struct lontium_hdmi_state *state);
};

struct lontium_hdmi {
	struct kref ref;
	struct device *dev;
	struct regmap *regmap;
	const struct lontium_hdmi_chip *chip;
	struct gpio_desc *reset_gpio;
	int irq;
	bool irq_enabled;
	bool stopping;
	struct delayed_work work;

	/* Serializes manual power changes and device removal. */
	struct mutex control_lock;
	/* Includes register access enable, paged reads and disable. */
	struct mutex io_lock;
	/* Audio events must not overwrite the last EXS video event. */
	enum lontium_hdmi_signal exs_video;
	struct lontium_hdmi_state candidate;
	bool candidate_valid;
	unsigned long candidate_since;
	unsigned int retries;

	/* Protects one complete status snapshot, sequence and sample time. */
	struct mutex state_lock;
	struct lontium_hdmi_state state;
	u64 state_seq;
	u64 updated_ns;
	wait_queue_head_t wait;
	struct proc_dir_entry *proc_dir;
	struct lontium_hdmi_node nodes[LONTIUM_HDMI_FILE_COUNT];
};

struct lontium_hdmi_reader {
	struct lontium_hdmi *hdmi;
	enum lontium_hdmi_file field;
	/* Serializes the cached snapshot, file position and consumed sequence. */
	struct mutex lock;
	u64 seen_seq;
	u64 snapshot_seq;
	size_t length;
	char snapshot[LONTIUM_HDMI_STATUS_SIZE];
};

/* The board has one receiver, with alternative populated I2C addresses. */
static DEFINE_MUTEX(lontium_hdmi_proc_lock);
static struct lontium_hdmi *lontium_hdmi_proc_owner;

static const struct regmap_range_cfg lontium_hdmi_regmap_ranges[] = {
	{
		.range_min = 0,
		.range_max = 0xffff,
		.selector_reg = LONTIUM_HDMI_PAGE_SELECT,
		.selector_mask = 0xff,
		.window_start = 0,
		.window_len = 0x100,
	},
};

static const struct regmap_config lontium_hdmi_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 0xffff,
	.cache_type = REGCACHE_NONE,
	.ranges = lontium_hdmi_regmap_ranges,
	.num_ranges = ARRAY_SIZE(lontium_hdmi_regmap_ranges),
};

static u16 lontium_hdmi_be16(const u8 *data)
{
	return (data[0] << 8) | data[1];
}

static int lontium_hdmi_read16(struct lontium_hdmi *hdmi, unsigned int reg,
			       u32 *value)
{
	u8 data[2];
	int ret;

	ret = regmap_bulk_read(hdmi->regmap, reg, data, sizeof(data));
	if (!ret)
		*value = lontium_hdmi_be16(data);
	return ret;
}

static enum lontium_hdmi_signal
lontium_hdmi_size_state(const struct lontium_hdmi_timings *timings)
{
	if (!timings->width && !timings->height)
		return LONTIUM_HDMI_SIGNAL_ABSENT;
	if (!timings->width || !timings->height)
		return LONTIUM_HDMI_SIGNAL_UNSTABLE;
	return LONTIUM_HDMI_SIGNAL_VALID;
}

static int lontium_hdmi_calc_fps(struct lontium_hdmi_timings *timings)
{
	u64 fps;

	if (!timings->pixelclock_hz || !timings->htotal || !timings->vtotal)
		return 0;
	if (timings->htotal < timings->width ||
	    timings->vtotal < timings->height)
		return -EAGAIN;

	fps = div64_u64(timings->pixelclock_hz * 1000,
			(u64)timings->htotal * timings->vtotal);
	if (fps > U32_MAX)
		return -ERANGE;
	timings->fps_millihz = fps;
	return 0;
}

static int lt6911c_read_state(struct lontium_hdmi *hdmi,
			      struct lontium_hdmi_state *state)
{
	int ret;

	ret = regmap_write(hdmi->regmap, LT6911C_VIDEO_MEASURE,
			   LT6911C_VIDEO_MEASURE_START);
	if (ret)
		return ret;
	usleep_range(5000, 6000);
	ret = lontium_hdmi_read16(hdmi, LT6911C_HDMI_HACTIVE,
				  &state->input.width);
	if (ret)
		return ret;
	ret = lontium_hdmi_read16(hdmi, LT6911C_HDMI_VACTIVE,
				  &state->input.height);
	if (ret)
		return ret;
	state->input.width *= LT6911C_HDMI_HACTIVE_SCALE;
	/* Test complete 16-bit values, including sizes with a zero low byte. */
	state->signal = lontium_hdmi_size_state(&state->input);
	if (state->signal != LONTIUM_HDMI_SIGNAL_VALID)
		return 0;

	ret = lontium_hdmi_read16(hdmi, LT6911C_CSI_HACTIVE, &state->csi.width);
	if (ret)
		return ret;
	ret = lontium_hdmi_read16(hdmi, LT6911C_CSI_VACTIVE, &state->csi.height);
	if (ret)
		return ret;
	if (lontium_hdmi_size_state(&state->csi) != LONTIUM_HDMI_SIGNAL_VALID)
		state->signal = LONTIUM_HDMI_SIGNAL_UNSTABLE;
	/* The reference does not provide a reliable LT6911C frame rate. */
	return 0;
}

static int lt6911uxc_read_state(struct lontium_hdmi *hdmi,
				struct lontium_hdmi_state *state)
{
	unsigned int signal, check;
	u8 clock[3];
	u32 half_clock_khz;
	int ret;

	ret = regmap_write(hdmi->regmap, LT6911UXC_VIDEO_CTRL, 0);
	if (ret)
		return ret;
	ret = regmap_read(hdmi->regmap, LT6911UXC_VIDEO_STATE, &signal);
	if (ret)
		return ret;
	if (signal == LT6911UXC_VIDEO_OFF) {
		state->signal = LONTIUM_HDMI_SIGNAL_ABSENT;
		return 0;
	}
	if (signal != LT6911UXC_VIDEO_READY) {
		state->signal = LONTIUM_HDMI_SIGNAL_UNSTABLE;
		return 0;
	}

	ret = lontium_hdmi_read16(hdmi, LT6911UXC_CSI_HACTIVE, &state->csi.width);
	if (ret)
		return ret;
	ret = lontium_hdmi_read16(hdmi, LT6911UXC_CSI_VACTIVE, &state->csi.height);
	if (ret)
		return ret;
	state->signal = LONTIUM_HDMI_SIGNAL_UNSTABLE;
	if (lontium_hdmi_size_state(&state->csi) != LONTIUM_HDMI_SIGNAL_VALID)
		return 0;

	ret = lontium_hdmi_read16(hdmi, LT6911UXC_CSI_HTOTAL, &state->csi.htotal);
	if (ret)
		return ret;
	ret = lontium_hdmi_read16(hdmi, LT6911UXC_CSI_VTOTAL, &state->csi.vtotal);
	if (ret)
		return ret;
	msleep(20);
	ret = regmap_write(hdmi->regmap, LT6911UXC_CLOCK_MEASURE,
			   LT6911UXC_CLOCK_MEASURE_START);
	if (ret)
		return ret;
	usleep_range(10000, 11000);
	ret = regmap_bulk_read(hdmi->regmap, LT6911UXC_HALF_PIXELCLOCK,
			       clock, sizeof(clock));
	if (ret)
		return ret;
	half_clock_khz = ((clock[0] << 16) | (clock[1] << 8) | clock[2]) &
			 LT6911UXC_HALF_PIXELCLOCK_MASK;
	state->csi.pixelclock_hz = (u64)half_clock_khz * 2000;
	ret = lontium_hdmi_calc_fps(&state->csi);
	/* UXC FPS is known to disagree with hardware in the reference driver. */
	if (ret && ret != -EAGAIN)
		return ret;
	ret = regmap_read(hdmi->regmap, LT6911UXC_VIDEO_STATE, &check);
	if (ret)
		return ret;
	if (check != signal)
		return -EAGAIN;
	state->signal = LONTIUM_HDMI_SIGNAL_VALID;
	return 0;
}

static int lt6911d_read_state(struct lontium_hdmi *hdmi,
			      struct lontium_hdmi_state *state)
{
	u8 data[LT6911D_VIDEO_TIMINGS_SIZE];
	u32 half_clock_khz;
	int ret;

	ret = regmap_bulk_read(hdmi->regmap, LT6911D_VIDEO_TIMINGS,
			       data, sizeof(data));
	if (ret)
		return ret;
	state->csi.width = (u32)lontium_hdmi_be16(data + 7) * LT6911D_HORIZONTAL_SCALE;
	state->csi.height = lontium_hdmi_be16(data + 9);
	state->signal = lontium_hdmi_size_state(&state->csi);
	if (state->signal != LONTIUM_HDMI_SIGNAL_VALID)
		return 0;
	state->csi.htotal = (u32)lontium_hdmi_be16(data + 3) * LT6911D_HORIZONTAL_SCALE;
	state->csi.vtotal = lontium_hdmi_be16(data + 5);
	half_clock_khz = ((data[0] << 16) | (data[1] << 8) | data[2]) &
			 LT6911D_HALF_PIXELCLOCK_MASK;
	state->csi.pixelclock_hz = (u64)half_clock_khz * 2000;
	return lontium_hdmi_calc_fps(&state->csi);
}

static int lt7911exs_read_state(struct lontium_hdmi *hdmi,
				struct lontium_hdmi_state *state)
{
	unsigned int event, check;
	u32 pixelclock_khz;
	u8 data[LT7911EXS_VIDEO_TIMINGS_SIZE];
	int ret;

	ret = regmap_read(hdmi->regmap, LT7911EXS_INT_TYPE, &event);
	if (ret)
		return ret;
	switch (event) {
	case LT7911EXS_VIDEO_OFF:
		hdmi->exs_video = LONTIUM_HDMI_SIGNAL_ABSENT;
		break;
	case LT7911EXS_VIDEO_READY:
		hdmi->exs_video = LONTIUM_HDMI_SIGNAL_VALID;
		break;
	case LT7911EXS_AUDIO_OFF:
	case LT7911EXS_AUDIO_READY:
		/* These are audio events, not a current video presence flag. */
		break;
	default:
		return -EOPNOTSUPP;
	}
	if (hdmi->exs_video == LONTIUM_HDMI_SIGNAL_ABSENT) {
		state->signal = LONTIUM_HDMI_SIGNAL_ABSENT;
		return 0;
	}

	ret = regmap_bulk_read(hdmi->regmap, LT7911EXS_VIDEO_TIMINGS,
			       data, sizeof(data));
	if (ret)
		return ret;
	ret = regmap_read(hdmi->regmap, LT7911EXS_INT_TYPE, &check);
	if (ret)
		return ret;
	if (check != event)
		return -EAGAIN;

	/* Unlike LT6911D, EXS reports full pixel widths and clock in kHz. */
	state->csi.width = lontium_hdmi_be16(data + 7);
	state->csi.height = lontium_hdmi_be16(data + 9);
	state->csi.htotal = lontium_hdmi_be16(data + 3);
	state->csi.vtotal = lontium_hdmi_be16(data + 5);
	pixelclock_khz = (data[0] << 16) | (data[1] << 8) | data[2];
	state->csi.pixelclock_hz = (u64)pixelclock_khz * 1000;
	state->signal = LONTIUM_HDMI_SIGNAL_UNSTABLE;
	if (lontium_hdmi_size_state(&state->csi) != LONTIUM_HDMI_SIGNAL_VALID)
		return 0;
	ret = lontium_hdmi_calc_fps(&state->csi);
	if (ret)
		return ret;
	/* Startup may find an audio event after video was already enabled. */
	state->signal = LONTIUM_HDMI_SIGNAL_VALID;
	return 0;
}

static const struct lontium_hdmi_chip lt6911c_chip = {
	.name = "LT6911C",
	.irq_flags = IRQF_TRIGGER_RISING,
	.i2c_control = true,
	.read_state = lt6911c_read_state,
};

static const struct lontium_hdmi_chip lt6911uxc_chip = {
	.name = "LT6911UXC",
	.irq_flags = IRQF_TRIGGER_FALLING,
	.i2c_control = true,
	.read_state = lt6911uxc_read_state,
};

static const struct lontium_hdmi_chip lt6911d_chip = {
	.name = "LT6911D",
	.irq_flags = IRQF_TRIGGER_FALLING,
	.read_state = lt6911d_read_state,
};

static const struct lontium_hdmi_chip lt7911exs_chip = {
	.name = "LT7911EXS",
	.irq_flags = IRQF_TRIGGER_FALLING,
	.read_state = lt7911exs_read_state,
};

static int lontium_hdmi_read_state(struct lontium_hdmi *hdmi,
				   struct lontium_hdmi_state *state)
{
	int ret, disable_ret;

	if (!hdmi->chip->i2c_control)
		return hdmi->chip->read_state(hdmi, state);

	ret = regmap_write(hdmi->regmap, LT6911_I2C_CTRL, 1);
	if (!ret)
		ret = hdmi->chip->read_state(hdmi, state);
	/* Always release register access, including partially failed enables. */
	disable_ret = regmap_write(hdmi->regmap, LT6911_I2C_CTRL, 0);
	return ret ? ret : disable_ret;
}

/* Receive one byte, as in i2cdetect -r, without writing control registers. */
static int lontium_hdmi_address_present(struct i2c_client *client, u16 address)
{
	struct i2c_client *probe = client;
	int ret;

	/* Reserve auxiliary addresses so another device cannot be probed. */
	if (address != client->addr) {
		probe = i2c_new_dummy_device(client->adapter, address);
		if (IS_ERR(probe))
			return PTR_ERR(probe);
	}
	ret = i2c_smbus_read_byte(probe);
	if (probe != client)
		i2c_unregister_device(probe);

	if (ret == -ENXIO || ret == -EREMOTEIO)
		return 0;
	return ret < 0 ? ret : 1;
}

static int lontium_hdmi_detect(struct i2c_client *client,
			       const struct lontium_hdmi_chip **chip)
{
	int ret, c, uxc;

	if (!*chip && client->addr != 0x2b)
		return -EINVAL;
	ret = lontium_hdmi_address_present(client, client->addr);
	if (ret <= 0)
		return ret ? ret : -ENODEV;
	if (*chip)
		return 0;

	/* NanoKVM firmware address signatures; all three use 0x2b for control. */
	c = lontium_hdmi_address_present(client, 0x2c);
	if (c < 0)
		return c;
	uxc = lontium_hdmi_address_present(client, 0x44);
	if (uxc < 0)
		return uxc;
	if (c && uxc)
		return dev_err_probe(&client->dev, -EINVAL,
				     "conflicting receiver address signatures\n");

	*chip = c ? &lt6911c_chip : uxc ? &lt6911uxc_chip : &lt6911d_chip;
	return 0;
}

static bool
lontium_hdmi_same_timings(const struct lontium_hdmi_timings *a,
			  const struct lontium_hdmi_timings *b)
{
	/* Clock measurement noise must not keep invalidating a fixed mode. */
	return a->width == b->width && a->height == b->height &&
		a->htotal == b->htotal && a->vtotal == b->vtotal &&
		DIV_ROUND_CLOSEST((u64)a->fps_millihz, 1000) ==
		DIV_ROUND_CLOSEST((u64)b->fps_millihz, 1000);
}

static bool lontium_hdmi_same_state(const struct lontium_hdmi_state *a,
				    const struct lontium_hdmi_state *b)
{
	return a->powered == b->powered &&
		a->signal == b->signal && a->error == b->error &&
		lontium_hdmi_same_timings(&a->input, &b->input) &&
		lontium_hdmi_same_timings(&a->csi, &b->csi);
}

static void lontium_hdmi_publish(struct lontium_hdmi *hdmi,
				 struct lontium_hdmi_state *state)
{
	bool changed;

	if (state->signal != LONTIUM_HDMI_SIGNAL_VALID) {
		memset(&state->input, 0, sizeof(state->input));
		memset(&state->csi, 0, sizeof(state->csi));
	}
	mutex_lock(&hdmi->state_lock);
	changed = !lontium_hdmi_same_state(&hdmi->state, state);
	hdmi->state = *state;
	hdmi->updated_ns = ktime_get_boottime_ns();
	if (changed)
		hdmi->state_seq++;
	mutex_unlock(&hdmi->state_lock);
	if (changed)
		wake_up_interruptible_poll(&hdmi->wait, EPOLLIN | EPOLLRDNORM);
}

/* Both the IRQ thread and delayed work use the same serialized sampling path. */
static void lontium_hdmi_refresh(struct lontium_hdmi *hdmi, bool from_irq)
{
	struct lontium_hdmi_state state = { .powered = true };
	unsigned int delay = LONTIUM_HDMI_POLL_MS;
	int ret;

	mutex_lock(&hdmi->io_lock);
	if (READ_ONCE(hdmi->stopping) || !hdmi->state.powered)
		goto out;
	if (from_irq)
		hdmi->retries = 0;

	ret = lontium_hdmi_read_state(hdmi, &state);
	if (ret) {
		state.signal = ret == -EAGAIN ? LONTIUM_HDMI_SIGNAL_UNSTABLE :
					      LONTIUM_HDMI_SIGNAL_ERROR;
		state.error = ret;
	}

	if (state.signal == LONTIUM_HDMI_SIGNAL_VALID) {
		if (lontium_hdmi_same_state(&hdmi->state, &state)) {
			hdmi->candidate_valid = false;
			hdmi->retries = 0;
		} else if (hdmi->candidate_valid &&
			   lontium_hdmi_same_state(&hdmi->candidate, &state)) {
			if (time_after_eq(jiffies, hdmi->candidate_since +
					  msecs_to_jiffies(LONTIUM_HDMI_SETTLE_MS))) {
				hdmi->candidate_valid = false;
				hdmi->retries = 0;
			} else {
				state.signal = LONTIUM_HDMI_SIGNAL_UNSTABLE;
			}
		} else {
			hdmi->candidate = state;
			hdmi->candidate_valid = true;
			hdmi->candidate_since = jiffies;
			state.signal = LONTIUM_HDMI_SIGNAL_UNSTABLE;
		}
	} else {
		hdmi->candidate_valid = false;
	}
	lontium_hdmi_publish(hdmi, &state);

	if (state.signal == LONTIUM_HDMI_SIGNAL_UNSTABLE ||
	    state.signal == LONTIUM_HDMI_SIGNAL_ERROR) {
		if (hdmi->retries < LONTIUM_HDMI_RETRIES) {
			hdmi->retries++;
			delay = LONTIUM_HDMI_SETTLE_MS;
		}
	} else {
		hdmi->retries = 0;
	}
	if (!READ_ONCE(hdmi->stopping))
		mod_delayed_work(system_wq, &hdmi->work, msecs_to_jiffies(delay));
out:
	mutex_unlock(&hdmi->io_lock);
}

static int lontium_hdmi_set_power(struct lontium_hdmi *hdmi, bool on)
{
	struct lontium_hdmi_state state = {
		.powered = on,
		.signal = on ? LONTIUM_HDMI_SIGNAL_UNKNOWN : LONTIUM_HDMI_SIGNAL_OFF,
	};
	int ret = 0;

	mutex_lock(&hdmi->control_lock);
	if (READ_ONCE(hdmi->stopping)) {
		ret = -ENODEV;
		goto out;
	}
	if (!hdmi->reset_gpio) {
		ret = -EOPNOTSUPP;
		goto out;
	}
	/* Workers preserve this flag; control_lock serializes its changes. */
	if (READ_ONCE(hdmi->state.powered) == on)
		goto out;

	/* Drain both sampling paths before touching reset, without io_lock held. */
	if (hdmi->irq_enabled) {
		disable_irq(hdmi->irq);
		hdmi->irq_enabled = false;
	}
	cancel_delayed_work_sync(&hdmi->work);
	mutex_lock(&hdmi->io_lock);
	/* The legacy HDMI switch holds reset asserted while disabled. */
	gpiod_set_value_cansleep(hdmi->reset_gpio, !on);
	msleep(on ? 400 : 20);
	hdmi->candidate_valid = false;
	hdmi->retries = 0;
	hdmi->exs_video = LONTIUM_HDMI_SIGNAL_UNKNOWN;
	lontium_hdmi_publish(hdmi, &state);
	if (on)
		mod_delayed_work(system_wq, &hdmi->work, 0);
	mutex_unlock(&hdmi->io_lock);
	if (on && hdmi->irq) {
		enable_irq(hdmi->irq);
		hdmi->irq_enabled = true;
	}
out:
	mutex_unlock(&hdmi->control_lock);
	return ret;
}

static void lontium_hdmi_work(struct work_struct *work)
{
	struct lontium_hdmi *hdmi;

	hdmi = container_of(to_delayed_work(work), struct lontium_hdmi, work);
	lontium_hdmi_refresh(hdmi, false);
}

static irqreturn_t lontium_hdmi_irq_thread(int irq, void *data)
{
	struct lontium_hdmi *hdmi = data;

	lontium_hdmi_refresh(hdmi, true);
	return IRQ_HANDLED;
}

static const char *lontium_hdmi_signal_name(enum lontium_hdmi_signal signal)
{
	switch (signal) {
	case LONTIUM_HDMI_SIGNAL_OFF:
		return "off";
	case LONTIUM_HDMI_SIGNAL_ABSENT:
		return "absent";
	case LONTIUM_HDMI_SIGNAL_UNSTABLE:
		return "unstable";
	case LONTIUM_HDMI_SIGNAL_VALID:
		return "valid";
	case LONTIUM_HDMI_SIGNAL_ERROR:
		return "error";
	default:
		return "unknown";
	}
}

/* reader->lock keeps short reads on the same complete status snapshot. */
static void lontium_hdmi_snapshot(struct lontium_hdmi_reader *reader)
{
	static const char format[] =
		"abi=1 sequence=%llu updated_ns=%llu chip=%s power=%s power_control=%u\n"
		"signal=%s error=%d\n"
		"input_valid=%u input_width=%u input_height=%u\n"
		"csi_valid=%u csi_width=%u csi_height=%u\n"
		"csi_htotal=%u csi_vtotal=%u csi_pixelclock_hz=%llu\n"
		"csi_fps_millihz=%u\n";
	struct lontium_hdmi *hdmi = reader->hdmi;
	const struct lontium_hdmi_timings *timings;
	struct lontium_hdmi_state state;
	char *buf = reader->snapshot;
	size_t size = sizeof(reader->snapshot);
	u64 updated_ns;

	mutex_lock(&hdmi->state_lock);
	state = hdmi->state;
	reader->snapshot_seq = hdmi->state_seq;
	updated_ns = hdmi->updated_ns;
	mutex_unlock(&hdmi->state_lock);

	switch (reader->field) {
	case LONTIUM_HDMI_FILE_CHIP:
		reader->length = scnprintf(buf, size, "%s\n", hdmi->chip->name);
		break;
	case LONTIUM_HDMI_FILE_SIGNAL:
		reader->length = scnprintf(buf, size, "%s\n",
					   lontium_hdmi_signal_name(state.signal));
		break;
	case LONTIUM_HDMI_FILE_ERROR:
		reader->length = scnprintf(buf, size, "%d\n", state.error);
		break;
	case LONTIUM_HDMI_FILE_POWER:
		reader->length = scnprintf(buf, size, "%s\n",
					   state.powered ? "on" : "off");
		break;
	case LONTIUM_HDMI_FILE_INPUT:
	case LONTIUM_HDMI_FILE_CSI:
		timings = reader->field == LONTIUM_HDMI_FILE_INPUT ?
			  &state.input : &state.csi;
		reader->length = scnprintf(buf, size,
					   "valid=%u width=%u height=%u htotal=%u vtotal=%u pixelclock_hz=%llu fps_millihz=%u\n",
					   !!timings->width && !!timings->height,
					   timings->width, timings->height,
					   timings->htotal, timings->vtotal,
					   timings->pixelclock_hz, timings->fps_millihz);
		break;
	default:
		reader->length = scnprintf(buf, size, format,
					   reader->snapshot_seq, updated_ns, hdmi->chip->name,
					   state.powered ? "on" : "off", !!hdmi->reset_gpio,
					   lontium_hdmi_signal_name(state.signal), state.error,
					   !!state.input.width && !!state.input.height,
					   state.input.width, state.input.height,
					   !!state.csi.width && !!state.csi.height,
					   state.csi.width, state.csi.height,
					   state.csi.htotal, state.csi.vtotal,
					   state.csi.pixelclock_hz, state.csi.fps_millihz);
		break;
	}
}

static void lontium_hdmi_free(struct kref *ref)
{
	struct lontium_hdmi *hdmi = container_of(ref, struct lontium_hdmi, ref);

	kfree(hdmi);
}

static void lontium_hdmi_put(void *data)
{
	struct lontium_hdmi *hdmi = data;

	kref_put(&hdmi->ref, lontium_hdmi_free);
}

static const struct file_operations lontium_hdmi_file_ops;
static const struct file_operations lontium_hdmi_power_ops;

static int lontium_hdmi_proc_open(struct inode *inode, struct file *file)
{
	struct lontium_hdmi_node *node = PDE_DATA(inode);
	struct lontium_hdmi *hdmi = node->hdmi;
	struct lontium_hdmi_reader *reader;
	const struct file_operations *fops = node->field == LONTIUM_HDMI_FILE_POWER ?
					   &lontium_hdmi_power_ops :
					   &lontium_hdmi_file_ops;

	if (READ_ONCE(hdmi->stopping))
		return -ENODEV;
	reader = kzalloc(sizeof(*reader), GFP_KERNEL);
	if (!reader)
		return -ENOMEM;
	fops = fops_get(fops);
	if (!fops) {
		kfree(reader);
		return -ENODEV;
	}
	kref_get(&hdmi->ref);
	reader->hdmi = hdmi;
	reader->field = node->field;
	mutex_init(&reader->lock);
	file->private_data = reader;
	/*
	 * proc_remove() releases proc_ops readers before their files are closed.
	 * Use file lifetime for the queue: poll/select may still hold wait entries
	 * after proc_remove(). Each open file pins both hdmi and this module.
	 */
	replace_fops(file, fops);
	return 0;
}

static ssize_t lontium_hdmi_proc_read(struct file *file, char __user *buf,
				      size_t count, loff_t *pos)
{
	struct lontium_hdmi_reader *reader = file->private_data;
	ssize_t ret;

	if (!count)
		return 0;
	mutex_lock(&reader->lock);
	if (READ_ONCE(reader->hdmi->stopping)) {
		ret = -ENODEV;
		goto out;
	}
	if (!*pos || !reader->length)
		lontium_hdmi_snapshot(reader);
	ret = simple_read_from_buffer(buf, count, pos, reader->snapshot,
				      reader->length);
	/* A newer event arriving during the copy must remain pending. */
	if (ret > 0 && *pos >= (loff_t)reader->length)
		reader->seen_seq = reader->snapshot_seq;
out:
	mutex_unlock(&reader->lock);
	return ret;
}

static loff_t lontium_hdmi_proc_lseek(struct file *file, loff_t offset,
				      int whence)
{
	struct lontium_hdmi_reader *reader = file->private_data;
	loff_t ret;

	mutex_lock(&reader->lock);
	if (READ_ONCE(reader->hdmi->stopping)) {
		ret = -ENODEV;
		goto out;
	}
	if (!reader->length)
		lontium_hdmi_snapshot(reader);
	ret = fixed_size_llseek(file, offset, whence, reader->length);
out:
	mutex_unlock(&reader->lock);
	return ret;
}

static __poll_t lontium_hdmi_proc_poll(struct file *file, poll_table *wait)
{
	struct lontium_hdmi_reader *reader = file->private_data;
	struct lontium_hdmi *hdmi = reader->hdmi;
	__poll_t mask = 0;

	poll_wait(file, &hdmi->wait, wait);
	mutex_lock(&reader->lock);
	mutex_lock(&hdmi->state_lock);
	if (READ_ONCE(hdmi->stopping))
		mask = EPOLLERR | EPOLLHUP;
	else if (reader->seen_seq != hdmi->state_seq)
		mask = EPOLLIN | EPOLLRDNORM;
	mutex_unlock(&hdmi->state_lock);
	mutex_unlock(&reader->lock);
	return mask;
}

static int lontium_hdmi_proc_release(struct inode *inode, struct file *file)
{
	struct lontium_hdmi_reader *reader = file->private_data;

	lontium_hdmi_put(reader->hdmi);
	kfree(reader);
	return 0;
}

static ssize_t lontium_hdmi_power_write(struct file *file,
					const char __user *buf, size_t count,
					loff_t *pos)
{
	struct lontium_hdmi_reader *reader = file->private_data;
	char command[16], *value;
	bool on;
	int ret;

	if (!count)
		return 0;
	if (count >= sizeof(command))
		return -EINVAL;
	if (copy_from_user(command, buf, count))
		return -EFAULT;
	if (memchr(command, '\0', count))
		return -EINVAL;
	command[count] = '\0';
	value = strim(command);
	if (!strcmp(value, "on") || !strcmp(value, "1"))
		on = true;
	else if (!strcmp(value, "off") || !strcmp(value, "0"))
		on = false;
	else
		return -EINVAL;

	mutex_lock(&reader->lock);
	ret = lontium_hdmi_set_power(reader->hdmi, on);
	if (!ret) {
		reader->length = 0;
		*pos = 0;
	}
	mutex_unlock(&reader->lock);
	if (ret)
		return ret;
	return count;
}

static const struct file_operations lontium_hdmi_file_ops = {
	.owner = THIS_MODULE,
	.read = lontium_hdmi_proc_read,
	.llseek = lontium_hdmi_proc_lseek,
	.poll = lontium_hdmi_proc_poll,
	.release = lontium_hdmi_proc_release,
};

static const struct file_operations lontium_hdmi_power_ops = {
	.owner = THIS_MODULE,
	.read = lontium_hdmi_proc_read,
	.write = lontium_hdmi_power_write,
	.llseek = lontium_hdmi_proc_lseek,
	.poll = lontium_hdmi_proc_poll,
	.release = lontium_hdmi_proc_release,
};

static const struct proc_ops lontium_hdmi_proc_ops = {
	.proc_open = lontium_hdmi_proc_open,
};

static int lontium_hdmi_register_proc(struct lontium_hdmi *hdmi)
{
	static const char * const names[] = {
		[LONTIUM_HDMI_FILE_STATUS] = "status",
		[LONTIUM_HDMI_FILE_CHIP] = "chip",
		[LONTIUM_HDMI_FILE_SIGNAL] = "signal",
		[LONTIUM_HDMI_FILE_INPUT] = "input",
		[LONTIUM_HDMI_FILE_CSI] = "csi",
		[LONTIUM_HDMI_FILE_ERROR] = "error",
		[LONTIUM_HDMI_FILE_POWER] = "power",
	};
	unsigned int i;
	umode_t mode;
	int ret = 0;

	mutex_lock(&lontium_hdmi_proc_lock);
	if (lontium_hdmi_proc_owner) {
		ret = -EBUSY;
		goto out;
	}
	hdmi->proc_dir = proc_mkdir("lontium", NULL);
	if (!hdmi->proc_dir) {
		ret = -ENOMEM;
		goto out;
	}
	for (i = 0; i < ARRAY_SIZE(names); i++) {
		hdmi->nodes[i].hdmi = hdmi;
		hdmi->nodes[i].field = i;
		mode = i == LONTIUM_HDMI_FILE_POWER ? 0644 : 0444;
		if (!proc_create_data(names[i], mode, hdmi->proc_dir,
				      &lontium_hdmi_proc_ops, &hdmi->nodes[i])) {
			proc_remove(hdmi->proc_dir);
			hdmi->proc_dir = NULL;
			ret = -ENOMEM;
			goto out;
		}
	}
	lontium_hdmi_proc_owner = hdmi;
out:
	mutex_unlock(&lontium_hdmi_proc_lock);
	return ret;
}

static int lontium_hdmi_get_resources(struct lontium_hdmi *hdmi,
				      struct i2c_client *client)
{
	struct device *dev = hdmi->dev;
	struct gpio_desc *irq_gpio;

	if (client->irq && device_property_present(dev, "irq-gpios"))
		return dev_err_probe(dev, -EINVAL,
				     "specify interrupts or irq-gpios, not both\n");

	/* Keep reset deasserted; only the manual power switch asserts it. */
	hdmi->reset_gpio = devm_gpiod_get_optional(dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(hdmi->reset_gpio))
		return dev_err_probe(dev, PTR_ERR(hdmi->reset_gpio),
				     "failed to acquire reset GPIO\n");

	hdmi->irq = client->irq;
	if (hdmi->irq)
		return hdmi->irq < 0 ? hdmi->irq : 0;
	irq_gpio = devm_gpiod_get_optional(dev, "irq", GPIOD_IN);
	if (IS_ERR(irq_gpio))
		return dev_err_probe(dev, PTR_ERR(irq_gpio),
				     "failed to acquire interrupt GPIO\n");
	if (irq_gpio) {
		hdmi->irq = gpiod_to_irq(irq_gpio);
		if (hdmi->irq < 0)
			return dev_err_probe(dev, hdmi->irq,
					     "failed to map interrupt GPIO\n");
	}
	return 0;
}

static int lontium_hdmi_probe(struct i2c_client *client,
			      const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	const struct lontium_hdmi_chip *chip;
	struct lontium_hdmi *hdmi;
	unsigned long irq_flags;
	int ret;

	chip = of_device_get_match_data(dev);
	if (!chip && id)
		chip = (const struct lontium_hdmi_chip *)id->driver_data;
	if (!i2c_check_functionality(client->adapter,
				     I2C_FUNC_I2C | I2C_FUNC_SMBUS_READ_BYTE))
		return -EOPNOTSUPP;

	/* Unpopulated alternative addresses must not claim or toggle GPIOs. */
	ret = lontium_hdmi_detect(client, &chip);
	if (ret == -ENODEV)
		return ret;
	if (ret)
		return dev_err_probe(dev, ret, "failed to detect receiver\n");

	hdmi = kzalloc(sizeof(*hdmi), GFP_KERNEL);
	if (!hdmi)
		return -ENOMEM;
	kref_init(&hdmi->ref);
	ret = devm_add_action_or_reset(dev, lontium_hdmi_put, hdmi);
	if (ret)
		return ret;
	hdmi->dev = dev;
	hdmi->chip = chip;
	hdmi->stopping = true;
	hdmi->exs_video = LONTIUM_HDMI_SIGNAL_UNKNOWN;
	hdmi->state.powered = true;
	hdmi->state.signal = LONTIUM_HDMI_SIGNAL_UNKNOWN;
	hdmi->state.error = -ENODATA;
	hdmi->state_seq = 1;
	mutex_init(&hdmi->control_lock);
	mutex_init(&hdmi->io_lock);
	mutex_init(&hdmi->state_lock);
	init_waitqueue_head(&hdmi->wait);
	INIT_DELAYED_WORK(&hdmi->work, lontium_hdmi_work);
	i2c_set_clientdata(client, hdmi);

	hdmi->regmap = devm_regmap_init_i2c(client, &lontium_hdmi_regmap_config);
	if (IS_ERR(hdmi->regmap))
		return dev_err_probe(dev, PTR_ERR(hdmi->regmap),
				     "failed to initialize register access\n");
	ret = lontium_hdmi_get_resources(hdmi, client);
	if (ret)
		return ret;

	if (hdmi->irq) {
		irq_flags = irq_get_trigger_type(hdmi->irq);
		if (!irq_flags)
			irq_flags = chip->irq_flags;
		/* The firmware signals events with edges, without a host ACK. */
		if (irq_flags != IRQF_TRIGGER_RISING &&
		    irq_flags != IRQF_TRIGGER_FALLING)
			return dev_err_probe(dev, -EINVAL,
					     "interrupt must use one edge\n");
		ret = devm_request_threaded_irq(dev, hdmi->irq, NULL,
						lontium_hdmi_irq_thread,
						IRQF_ONESHOT | irq_flags,
						dev_name(dev), hdmi);
		if (ret)
			return dev_err_probe(dev, ret, "failed to request interrupt\n");
		hdmi->irq_enabled = true;
	}

	ret = lontium_hdmi_register_proc(hdmi);
	if (ret)
		return ret;
	WRITE_ONCE(hdmi->stopping, false);
	/* Includes an already connected source and events before IRQ setup. */
	lontium_hdmi_refresh(hdmi, false);
	dev_info(dev, "%s detected, using %s\n", chip->name,
		 hdmi->irq ? "irq and polling" : "polling");
	return 0;
}

static int lontium_hdmi_remove(struct i2c_client *client)
{
	struct lontium_hdmi *hdmi = i2c_get_clientdata(client);

	mutex_lock(&hdmi->control_lock);
	WRITE_ONCE(hdmi->stopping, true);
	if (hdmi->irq_enabled) {
		disable_irq(hdmi->irq);
		hdmi->irq_enabled = false;
	}
	cancel_delayed_work_sync(&hdmi->work);
	wake_up_interruptible_poll(&hdmi->wait, EPOLLERR | EPOLLHUP);
	mutex_lock(&lontium_hdmi_proc_lock);
	/* Drain opens; existing files retain hdmi until their final close. */
	proc_remove(hdmi->proc_dir);
	lontium_hdmi_proc_owner = NULL;
	mutex_unlock(&lontium_hdmi_proc_lock);
	/* Leave the receiver accessible for the next probe or legacy owner. */
	if (hdmi->reset_gpio && !hdmi->state.powered) {
		gpiod_set_value_cansleep(hdmi->reset_gpio, 0);
		msleep(400);
	}
	mutex_unlock(&hdmi->control_lock);
	return 0;
}

static const struct of_device_id lontium_hdmi_of_match[] = {
	{ .compatible = "sipeed,nanokvm-lt6911" },
	{ .compatible = "lontium,lt7911exs", .data = &lt7911exs_chip },
	{ }
};
MODULE_DEVICE_TABLE(of, lontium_hdmi_of_match);

static const struct i2c_device_id lontium_hdmi_i2c_ids[] = {
	{ "nanokvm-lt6911", 0 },
	{ "lt7911exs", (kernel_ulong_t)&lt7911exs_chip },
	{ }
};
MODULE_DEVICE_TABLE(i2c, lontium_hdmi_i2c_ids);

static struct i2c_driver lontium_hdmi_driver = {
	.driver = {
		.name = "lontium_hdmi",
		.of_match_table = lontium_hdmi_of_match,
	},
	.probe = lontium_hdmi_probe,
	.remove = lontium_hdmi_remove,
	.id_table = lontium_hdmi_i2c_ids,
};
module_i2c_driver(lontium_hdmi_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Lontium LT6911C/UXC/D and LT7911EXS HDMI receiver management");
