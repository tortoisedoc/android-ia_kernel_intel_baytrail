/*
 * Support for Intel Camera Imaging ISP subsystem.
 *
 * Copyright (c) 2010 - 2014 Intel Corporation. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#ifndef __ISYS_STREAM2MMIO_PRIVATE_H_INCLUDED__
#define __ISYS_STREAM2MMIO_PRIVATE_H_INCLUDED__

#include "isys_stream2mmio_public.h"

#include "device_access.h"	/* ia_css_device_load_uint32 */

#include "assert_support.h" /* assert */

#define STREAM2MMIO_COMMAND_REG_ID             0
#define STREAM2MMIO_ACKNOWLEDGE_REG_ID         1
#define STREAM2MMIO_PIX_WIDTH_ID_REG_ID        2
#define STREAM2MMIO_START_ADDR_REG_ID          3      /* master port address,NOT Byte */
#define STREAM2MMIO_END_ADDR_REG_ID            4      /* master port address,NOT Byte */
#define STREAM2MMIO_STRIDE_REG_ID              5      /* stride in master port words, increment is per packet for long sids, stride is not used for short sid's*/
#define STREAM2MMIO_NUM_ITEMS_REG_ID           6      /* number of packets for store packets cmd, number of words for store_words cmd */
#define STREAM2MMIO_BLOCK_WHEN_NO_CMD_REG_ID   7      /* if this register is 1, input will be stalled if there is no pending command for this sid */
#define STREAM2MMIO_REGS_PER_SID               8

/*****************************************************
 *
 * Native command interface (NCI).
 *
 *****************************************************/
/**
 * @brief Get the stream2mmio-controller state.
 * Refer to "stream2mmio_public.h" for details.
 */
STORAGE_CLASS_STREAM2MMIO_C void stream2mmio_get_state(
		const stream2mmio_ID_t ID,
		stream2mmio_state_t *state)
{
	uint32_t i;

	/*
	 * Get the values of the register-set per
	 * stream2mmio-controller sids.
	 */
	for (i = 0; i < N_STREAM2MMIO_SID_PROCS[ID]; i++) {
		stream2mmio_get_sid_state(
				ID,
				i,
				&(state->sid_state[i]));
	}
}

/**
 * @brief Get the state of the stream2mmio-controller sidess.
 * Refer to "stream2mmio_public.h" for details.
 */
STORAGE_CLASS_STREAM2MMIO_C void stream2mmio_get_sid_state(
		const stream2mmio_ID_t ID,
		const uint32_t sid_id,
		stream2mmio_sid_state_t	*state)
{

	state->rcv_ack =
		stream2mmio_reg_load(ID, sid_id, STREAM2MMIO_ACKNOWLEDGE_REG_ID);

	state->pix_width_id =
		stream2mmio_reg_load(ID, sid_id, STREAM2MMIO_PIX_WIDTH_ID_REG_ID);

	state->start_addr =
		stream2mmio_reg_load(ID, sid_id, STREAM2MMIO_START_ADDR_REG_ID);

	state->end_addr =
		stream2mmio_reg_load(ID, sid_id, STREAM2MMIO_END_ADDR_REG_ID);

	state->strides =
		stream2mmio_reg_load(ID, sid_id, STREAM2MMIO_STRIDE_REG_ID);

	state->num_items =
		stream2mmio_reg_load(ID, sid_id, STREAM2MMIO_NUM_ITEMS_REG_ID);

	state->block_when_no_cmd =
		stream2mmio_reg_load(ID, sid_id, STREAM2MMIO_BLOCK_WHEN_NO_CMD_REG_ID);

}
/** end of NCI */

/*****************************************************
 *
 * Device level interface (DLI).
 *
 *****************************************************/
/**
 * @brief Load the register value.
 * Refer to "stream2mmio_public.h" for details.
 */
STORAGE_CLASS_STREAM2MMIO_C hrt_data stream2mmio_reg_load(
	const stream2mmio_ID_t ID,
	const uint32_t sid_id,
	const uint32_t reg_idx)
{
	uint32_t reg_bank_offset;
	assert(ID < N_STREAM2MMIO_ID);

	reg_bank_offset =
		STREAM2MMIO_REGS_PER_SID * sid_id;
	return ia_css_device_load_uint32(STREAM2MMIO_CTRL_BASE[ID] +
						(reg_bank_offset + reg_idx)*sizeof(hrt_data));
}


/**
 * @brief Store a value to the register.
 * Refer to "stream2mmio_public.h" for details.
 */
STORAGE_CLASS_STREAM2MMIO_C void stream2mmio_reg_store(
	const stream2mmio_ID_t ID,
	const hrt_address reg,
	const hrt_data value)
{
	assert(ID < N_STREAM2MMIO_ID);
	assert(STREAM2MMIO_CTRL_BASE[ID] != (hrt_address)-1);

	ia_css_device_store_uint32(STREAM2MMIO_CTRL_BASE[ID] + reg*sizeof(hrt_data), value);
}
/** end of DLI */

#endif /* __ISYS_STREAM2MMIO_PRIVATE_H_INCLUDED__ */
