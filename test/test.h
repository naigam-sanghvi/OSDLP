/*
 *  Open Space Data Link Protocol
 *
 *  Copyright (C) 2020 Libre Space Foundation (https://libre.space)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TEST_TEST_H_
#define TEST_TEST_H_

#include "osdlp.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include "cmocka.h"

#include "queue_util.h"

#define NUMVCS	           3
#define TC_MAX_SDU_SIZE	   1024
#define TM_TX_CAPACITY     10
#define TC_MAX_FRAME_LEN   256
#define TM_MAX_SDU_LEN     1024
#define TM_FRAME_LEN       256

struct local_queue_item {
	uint8_t 	fdu[TC_MAX_SDU_SIZE];
	uint8_t 	rt_flag;
	uint8_t		seq_num;
	tc_bypass_t type;
};

int
setup_queues(uint16_t up_chann_item_size,
             uint16_t up_chann_capacity,
             uint16_t down_chann_item_size,
             uint16_t down_chann_capacity,
             uint16_t sent_item_size,
             uint16_t sent_capacity,
             uint16_t wait_item_size,
             uint16_t rx_item_size,
             uint16_t rx_capacity);

int
setup_tc_configs(struct tc_transfer_frame *tc_tx,
                 struct tc_transfer_frame *tc_rx,
                 struct cop_config *fop_conf, struct cop_config *farm_conf,
                 struct fop_config *fop, struct farm_config *farm,
                 uint16_t scid, uint16_t max_frame_size, uint16_t max_fifo_size,
                 uint8_t vcid, uint8_t mapid, tc_crc_flag_t crc,
                 tc_seg_hdr_t seg_hdr, tc_bypass_t bypass, tc_ctrl_t ctrl,
                 uint8_t fop_slide_wnd, fop_state_t fop_init_st,
                 uint16_t fop_t1_init, uint16_t fop_timeout_type,
                 uint8_t fop_tx_limit, farm_state_t farm_init_st,
                 uint8_t farm_wnd_width);



void
test_tm(void **state);

void
test_tc(void **state);

void
test_simple_bd_frame(void **state);

void
test_simple_ad_frame(void **state);

void
test_spp_hdr_only(void **state);

void
test_spp_hdr_with_data(void **state);

void
test_spp_invalid(void **state);

void
test_unlock_cmd(void **state);

void
test_segmentation(void **state);

void
test_one_rt(void **state);

void
test_segmentation_two_way(void **state);

void
test_timer(void **state);

void
test_operation(void **state);

void
test_vr(void **state);

void
test_tm_no_stuffing(void **state);

void
test_tm_with_stuffing(void **state);

#endif /* TEST_TEST_H_ */
