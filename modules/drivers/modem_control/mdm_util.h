/*
 * linux/drivers/modem_control/mdm_util.h
 *
 * mdm_util.h
 * Utilities functions header.
 *
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
 *
 * Contact: Faouaz Tenoutit <faouazx.tenoutit@intel.com>
 *          Frederic Berat <fredericx.berat@intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/acpi.h>
#include <linux/acpi_gpio.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/delay.h>
#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#endif
#include <linux/mdm_ctrl.h>
#include <asm/intel-mid.h>
#include <asm/intel_scu_pmic.h>
#include <linux/mdm_ctrl_board.h>

#ifndef _MDM_UTIL_H
#define _MDM_UTIL_H

/**
 * struct mdm_ctrl - Modem boot driver
 *
 * @lock: spinlock to serialise access to the driver information
 * @major: char device major number
 * @tdev: char device type dev
 * @dev: char device
 * @cdev: char device
 * @class: char device class
 * @opened: This flag is used to allow only ONE instance of this driver
 * @wait_wq: Read/Poll/Select wait event
 * @gpio_rst_out: Modem RESET_OUT GPIO
 * @gpio_pwr_on: Modem PWR_ON GPIO
 * @gpio_rst_bbn: Modem RESET_BBN GPIO
 * @gpio_cdump: Modem coredump GPIO
 * @irq_cdump: the modem core dump interrupt line
 * @irq_reset: the modem reset interrupt line
 * @rst_ongoing: Stating that a reset is ongoing
 * @wq: Modem Reset/Coredump worqueue
 * @hangup_work: Modem Reset/Coredump work
 */
struct mdm_ctrl {
	/* Char device registration */
	int major;
	dev_t tdev;
	struct device *dev;
	struct cdev cdev;
	struct class *class;

	/* Device infos */
	struct mcd_base_info *pdata;

	/* Used to prevent multiple access to device */
	unsigned int opened;

	/* A waitqueue for poll/read operations */
	wait_queue_head_t wait_wq;
	unsigned int polled_states;
	bool polled_state_reached;

	/* modem status */
	atomic_t rst_ongoing;
	int hangup_causes;

	struct mutex lock;
	atomic_t modem_state;
#ifdef CONFIG_HAS_WAKELOCK
	struct wake_lock	 stay_awake;
#endif

	struct work_struct change_state_work;

	struct workqueue_struct *hu_wq;
	struct work_struct hangup_work;

	struct timer_list flashing_timer;

	bool is_mdm_ctrl_disabled;

};

/* List of states */
struct next_state {
	struct list_head link;
	int state;
};

/* Modem control driver instance */
extern struct mdm_ctrl *mdm_drv;

inline void mdm_ctrl_set_opened(struct mdm_ctrl *drv, int value);
inline int mdm_ctrl_get_opened(struct mdm_ctrl *drv);

void mdm_ctrl_enable_flashing(unsigned long int param);
void mdm_ctrl_disable_flashing(unsigned long int param);

void mdm_ctrl_launch_timer(struct timer_list *timer, int delay,
			   unsigned int timer_type);

inline void mdm_ctrl_set_reset_ongoing(struct mdm_ctrl *drv, int ongoing);
inline int mdm_ctrl_get_reset_ongoing(struct mdm_ctrl *drv);

inline void mdm_ctrl_set_state(struct mdm_ctrl *drv, int state);
inline int mdm_ctrl_get_state(struct mdm_ctrl *drv);

void mdm_ctrl_get_device_info(struct mdm_ctrl *drv,
			      struct platform_device *pdev);
#endif				/* _MDM_UTIL_H */
