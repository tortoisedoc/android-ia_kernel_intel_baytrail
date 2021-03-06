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

#include "ia_css_acc_types.h"
#define __INLINE_SP__
#include "sp.h"

#ifdef HRT_CSIM
#include <hive_isp_css_sp_hrt.h>
#endif

#include "memory_access.h"
#include "assert_support.h"
#include "ia_css_spctrl.h"

typedef struct {
	struct ia_css_sp_init_dmem_cfg dmem_config;
	uint32_t        spctrl_config_dmem_addr; /** location of dmem_cfg  in SP dmem */
	uint32_t        spctrl_state_dmem_addr;
	unsigned int    sp_entry;           /* entry function ptr on SP */
	hrt_vaddress    code_addr;          /* sp firmware location in host mem-DDR*/
	uint32_t        code_size;
	char           *program_name;       /* used in case of PLATFORM_SIM */
} spctrl_context_info;

static spctrl_context_info spctrl_cofig_info[N_SP_ID];
static bool spctrl_loaded[N_SP_ID] = {0};

/* Load firmware */
enum ia_css_err ia_css_spctrl_load_fw(sp_ID_t sp_id,
				ia_css_spctrl_cfg *spctrl_cfg)
{
	hrt_vaddress code_addr = mmgr_NULL;
	struct ia_css_sp_init_dmem_cfg *init_dmem_cfg;

	if ((sp_id >= N_SP_ID) || (spctrl_cfg == 0))
		return IA_CSS_ERR_INVALID_ARGUMENTS;

	spctrl_cofig_info[sp_id].code_addr = mmgr_NULL;

#if defined(C_RUN) || defined(HRT_UNSCHED)
	(void)init_dmem_cfg;
	code_addr = mmgr_malloc(1);
	if (code_addr == mmgr_NULL)
		return IA_CSS_ERR_CANNOT_ALLOCATE_MEMORY;
#else
	init_dmem_cfg = &spctrl_cofig_info[sp_id].dmem_config;
	init_dmem_cfg->dmem_data_addr = spctrl_cfg->dmem_data_addr;
	init_dmem_cfg->dmem_bss_addr  = spctrl_cfg->dmem_bss_addr;
	init_dmem_cfg->data_size      = spctrl_cfg->data_size;
	init_dmem_cfg->bss_size       = spctrl_cfg->bss_size;
	init_dmem_cfg->sp_id          = sp_id;

	spctrl_cofig_info[sp_id].spctrl_config_dmem_addr = spctrl_cfg->spctrl_config_dmem_addr;
	spctrl_cofig_info[sp_id].spctrl_state_dmem_addr = spctrl_cfg->spctrl_state_dmem_addr;

	/* store code (text + icache) and data to DDR
	 *
	 * Data used to be stored separately, because of access alignment constraints,
	 * fix the FW generation instead
	 */
	code_addr = mmgr_malloc(spctrl_cfg->code_size);
	if (code_addr == mmgr_NULL)
		return IA_CSS_ERR_CANNOT_ALLOCATE_MEMORY;
	mmgr_store(code_addr, spctrl_cfg->code, spctrl_cfg->code_size);

	assert(sizeof(hrt_vaddress) <= sizeof(hrt_data));

	init_dmem_cfg->ddr_data_addr  = code_addr + spctrl_cfg->ddr_data_offset;
	assert((init_dmem_cfg->ddr_data_addr % HIVE_ISP_DDR_WORD_BYTES) == 0);
#endif
	spctrl_cofig_info[sp_id].sp_entry = spctrl_cfg->sp_entry;
	spctrl_cofig_info[sp_id].code_addr = code_addr;
	spctrl_cofig_info[sp_id].program_name = spctrl_cfg->program_name;

#ifdef HRT_CSIM
	hrt_cell_set_icache_base_address(SP, spctrl_cofig_info[sp_id].code_addr);
	hrt_cell_invalidate_icache(SP);
	hrt_cell_load_program(SP, spctrl_cofig_info[sp_id].program_name);
#else
	/* now we program the base address into the icache and
	 * invalidate the cache.
	 */
	sp_ctrl_store(sp_id, SP_ICACHE_ADDR_REG, (hrt_data)spctrl_cofig_info[sp_id].code_addr);
	sp_ctrl_setbit(sp_id, SP_ICACHE_INV_REG, SP_ICACHE_INV_BIT);
#endif
	spctrl_loaded[sp_id] = true;
	return IA_CSS_SUCCESS;
}

enum ia_css_err ia_css_spctrl_unload_fw(sp_ID_t sp_id)
{
	if ((sp_id >= N_SP_ID) || ((sp_id < N_SP_ID) && (!spctrl_loaded[sp_id])))
		return IA_CSS_ERR_INVALID_ARGUMENTS;

	/*  freeup the resource */
	if (spctrl_cofig_info[sp_id].code_addr)
		mmgr_free(spctrl_cofig_info[sp_id].code_addr);
	spctrl_loaded[sp_id] = false;
	return IA_CSS_SUCCESS;
}

#ifdef HRT_CSIM
enum ia_css_err ia_css_spctrl_start(sp_ID_t sp_id)
{
	unsigned int HIVE_ADDR_sp_start_isp_entry;
	if ((sp_id >= N_SP_ID) || ((sp_id < N_SP_ID) && (!spctrl_loaded[sp_id])))
		return IA_CSS_ERR_INVALID_ARGUMENTS;

	HIVE_ADDR_sp_start_isp_entry = spctrl_cofig_info[sp_id].sp_entry;

#if !defined(C_RUN) && !defined(HRT_UNSCHED)
	sp_dmem_store(sp_id,
		spctrl_cofig_info[sp_id].spctrl_config_dmem_addr,
		&spctrl_cofig_info[sp_id].dmem_config,
		sizeof(spctrl_cofig_info[sp_id].dmem_config));
#endif
	hrt_cell_start_function(SP, sp_start_isp);
	return IA_CSS_SUCCESS;
}

#else

/* Initialize dmem_cfg in SP dmem  and  start SP program*/
enum ia_css_err ia_css_spctrl_start(sp_ID_t sp_id)
{
	if ((sp_id >= N_SP_ID) || ((sp_id < N_SP_ID) && (!spctrl_loaded[sp_id])))
		return IA_CSS_ERR_INVALID_ARGUMENTS;

	/* Set descr in the SP to initialize the SP DMEM */
	/*
	 * The FW stores user-space pointers to the FW, the ISP pointer
	 * is only available here
	 *
	 */
	assert(sizeof(unsigned int) <= sizeof(hrt_data));

	sp_dmem_store(sp_id,
		spctrl_cofig_info[sp_id].spctrl_config_dmem_addr,
		&spctrl_cofig_info[sp_id].dmem_config,
		sizeof(spctrl_cofig_info[sp_id].dmem_config));
	/* set the start address */
	sp_ctrl_store(sp_id, SP_START_ADDR_REG, (hrt_data)spctrl_cofig_info[sp_id].sp_entry);
	sp_ctrl_setbit(sp_id, SP_SC_REG, SP_RUN_BIT);
	sp_ctrl_setbit(sp_id, SP_SC_REG, SP_START_BIT);
	return IA_CSS_SUCCESS;
}

#endif

/* Query the state of SP */
ia_css_spctrl_sp_sw_state ia_css_spctrl_get_state(sp_ID_t sp_id)
{
	ia_css_spctrl_sp_sw_state state;
	unsigned int HIVE_ADDR_sp_sw_state;

	if (sp_id >= N_SP_ID)
		return IA_CSS_SP_SW_TERMINATED;

	HIVE_ADDR_sp_sw_state = spctrl_cofig_info[sp_id].spctrl_state_dmem_addr;
	(void)HIVE_ADDR_sp_sw_state; /* Suppres warnings in CRUN */
	state = sp_dmem_load_uint32(sp_id, (unsigned)sp_address_of(sp_sw_state));

	return state;
}
