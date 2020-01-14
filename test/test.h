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
#include <cmocka.h>

#include "queue_util.h"

#define NUMVCS	           3
#define MAX_SDU_SIZE	   1024

struct queue  	           wait_queues[NUMVCS];     /* Wait queue */
struct queue	           sent_queues[NUMVCS];     /* Sent queue */
struct queue
	downlink_channel;        /* Queue simulating downlink channel */
struct queue
	uplink_channel;          /* Queue simulating uplink channel */
struct queue  	           rx_queues[NUMVCS];       /* Receiving queue */
/* Config structs for the first VC*/
struct tc_transfer_frame   tc_tx;
struct tc_transfer_frame   tc_rx;

/* Config structs for the second VC*/
struct tc_transfer_frame   tc_tx_unseg;
struct tc_transfer_frame   tc_rx_unseg;

/* Utility buffers */
uint8_t                    util_tx[MAX_SDU_SIZE];
uint8_t                    util_rx[MAX_SDU_SIZE];

uint8_t                    util_tx_unseg[MAX_SDU_SIZE];
uint8_t                    util_rx_unseg[MAX_SDU_SIZE];
uint8_t                    test_util[TC_MAX_FRAME_LEN];
uint8_t                    temp[TC_MAX_FRAME_LEN];

struct cop_config          cop_tx;
struct cop_config          cop_rx;
struct fop_config          fop;
struct farm_config         farm;

struct cop_config          cop_tx_unseg;
struct cop_config          cop_rx_unseg;
struct fop_config          fop_unseg;
struct farm_config         farm_unseg;

struct local_queue_item {
	uint8_t 	fdu[MAX_SDU_SIZE];
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
setup_configs(struct tc_transfer_frame *tc_tx,
              struct tc_transfer_frame *tc_rx,
              struct cop_config *fop_conf, struct cop_config *farm_conf,
              struct fop_config *fop, struct farm_config *farm,
              uint16_t scid, uint16_t max_frame_size,
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

#endif /* TEST_TEST_H_ */
