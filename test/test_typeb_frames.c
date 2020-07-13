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

#include <string.h>
#include "test.h"
#include "queue_util.h"


extern struct queue
	uplink_channel;          /* Queue simulating uplink channel */
extern struct queue  	           rx_queues[NUMVCS];       /* Receiving queue */
/* Config structs for the first VC*/
extern struct tc_transfer_frame   tc_tx;
extern struct tc_transfer_frame   tc_rx;

extern uint8_t                    test_util[TC_MAX_FRAME_LEN];
extern uint8_t                    temp[TC_MAX_FRAME_LEN];

extern struct cop_config          cop_tx;
extern struct cop_config          cop_rx;
extern struct fop_config          fop;
extern struct farm_config         farm;

/**
 * Send two type BD frames and receive them in the RX queue
 */
void
test_simple_bd_frame(void **state)
{
	uint16_t      up_chann_item_size = TC_MAX_FRAME_LEN;
	uint16_t      up_chann_capacity = 10;
	uint16_t      down_chann_item_size = sizeof(struct clcw_frame);
	uint16_t      down_chann_capacity = 10;
	uint16_t      sent_item_size = sizeof(struct local_queue_item);
	uint16_t      sent_capacity = 10;
	uint16_t      wait_item_size = sizeof(struct tc_transfer_frame);
	uint16_t      rx_item_size = TC_MAX_FRAME_LEN;
	uint16_t      rx_capacity = 10;

	uint16_t      scid = 101;
	uint16_t      max_frame_size = TC_MAX_FRAME_LEN;
	uint16_t      rx_max_fifo_size = 10;
	uint8_t       vcid = 1;
	uint8_t       mapid = 1;
	tc_crc_flag_t crc = TC_CRC_PRESENT;
	tc_seg_hdr_t  seg_hdr = TC_SEG_HDR_PRESENT;
	tc_bypass_t   bypass = TYPE_B;
	tc_ctrl_t     ctrl = TC_DATA;
	uint16_t      fop_slide_wnd = 3;
	fop_state_t   fop_init_st = FOP_STATE_INIT;
	uint16_t      fop_t1_init = 100;
	uint16_t      fop_timeout_type = 0;
	uint8_t       fop_tx_limit = 3;
	farm_state_t  farm_init_st = FARM_STATE_OPEN;
	uint8_t       farm_wnd_width = 10;


	setup_queues(up_chann_item_size,
	             up_chann_capacity,
	             down_chann_item_size,
	             down_chann_capacity,
	             sent_item_size,
	             sent_capacity,
	             wait_item_size,
	             rx_item_size,
	             rx_capacity);                           /*Prepare queues*/

	setup_tc_configs(&tc_tx, &tc_rx,
	                 &cop_tx, &cop_rx,
	                 &fop, &farm,
	                 scid, max_frame_size,
	                 rx_max_fifo_size,
	                 vcid, mapid, crc,
	                 seg_hdr, bypass,
	                 ctrl, fop_slide_wnd,
	                 fop_init_st, fop_t1_init,
	                 fop_timeout_type, fop_tx_limit,
	                 farm_init_st, farm_wnd_width);         /*Prepare config structs*/

	notification_t notif;
	int tc_tx_ret;
	int tc_rx_ret;

	notif = initiate_no_clcw(&tc_tx);               /* Initiate service*/
	assert_int_equal(notif, POSITIVE_DIR);
	uint8_t buf[100];
	for (int i = 0; i < 100; i++) {
		buf[i] = i;
	}
	prepare_typeb_data_frame(&tc_tx, buf, 100);

	tc_tx_ret = tc_transmit(&tc_tx, buf, 100);            /* Transmit packet */
	assert_int_equal(tc_tx_ret, TC_TX_OK);
	assert_int_equal(tc_tx.cop_cfg.fop.signal, ACCEPT_TX);
	assert_int_equal(1, uplink_channel.inqueue);
	memcpy(temp, tc_tx.mission.util.buffer, 110);	/* Transmit 2nd packet */
	tc_tx_ret = tc_transmit(&tc_tx, buf, 100);
	assert_int_equal(tc_tx_ret, TC_TX_OK);
	assert_int_equal(tc_tx.cop_cfg.fop.signal, ACCEPT_TX);
	assert_int_equal(2, uplink_channel.inqueue);

	dequeue(&uplink_channel, test_util);
	tc_rx_ret = tc_receive(test_util,
	                       TC_MAX_FRAME_LEN);    /* Receive first packet*/
	assert_int_equal(tc_rx_ret, TC_RX_OK);

	dequeue(&uplink_channel, test_util);
	tc_rx_ret = tc_receive(test_util, TC_MAX_FRAME_LEN);    /* Receive 2nd packet*/
	assert_int_equal(tc_rx_ret, TC_RX_OK);
	assert_int_equal(2, rx_queues[1].inqueue);
}

void
test_unlock_cmd(void **state)
{
	uint16_t      up_chann_item_size = TC_MAX_FRAME_LEN;
	uint16_t      up_chann_capacity = 10;
	uint16_t      down_chann_item_size = sizeof(struct clcw_frame);
	uint16_t      down_chann_capacity = 10;
	uint16_t      sent_item_size = sizeof(struct local_queue_item);
	uint16_t      sent_capacity = 10;
	uint16_t      wait_item_size = sizeof(struct tc_transfer_frame);
	uint16_t      rx_item_size = TC_MAX_FRAME_LEN;
	uint16_t      rx_capacity = 10;

	uint16_t      scid = 101;
	uint16_t      max_frame_size = TC_MAX_FRAME_LEN;
	uint8_t       vcid = 1;
	uint8_t       mapid = 1;
	tc_crc_flag_t crc = TC_CRC_PRESENT;
	tc_seg_hdr_t  seg_hdr = TC_SEG_HDR_PRESENT;
	tc_bypass_t   bypass = TYPE_B;
	tc_ctrl_t     ctrl = TC_DATA;
	uint16_t      fop_slide_wnd = 3;
	fop_state_t   fop_init_st = FOP_STATE_INIT;
	uint16_t      fop_t1_init = 100;
	uint16_t      fop_timeout_type = 0;
	uint8_t       fop_tx_limit = 3;
	farm_state_t  farm_init_st = FARM_STATE_OPEN;
	uint8_t       farm_wnd_width = 10;
	uint16_t      rx_max_fifo_size = 10;


	setup_queues(up_chann_item_size,
	             up_chann_capacity,
	             down_chann_item_size,
	             down_chann_capacity,
	             sent_item_size,
	             sent_capacity,
	             wait_item_size,
	             rx_item_size,
	             rx_capacity);                           /*Prepare queues*/

	setup_tc_configs(&tc_tx, &tc_rx,
	                 &cop_tx, &cop_rx,
	                 &fop, &farm,
	                 scid, max_frame_size,
	                 rx_max_fifo_size,
	                 vcid, mapid, crc,
	                 seg_hdr, bypass,
	                 ctrl, fop_slide_wnd,
	                 fop_init_st, fop_t1_init,
	                 fop_timeout_type, fop_tx_limit,
	                 farm_init_st, farm_wnd_width);         /*Prepare config structs*/

	int ret;
	notification_t notif;
	notif = initiate_no_clcw(&tc_tx);
	assert_int_equal(notif, POSITIVE_DIR);

	prepare_typeb_unlock(&tc_tx);
	ret = transmit_type_bc(&tc_tx);
	assert_int_equal(0, ret);
	assert_int_equal(1, uplink_channel.inqueue);

	prepare_typeb_setvr(&tc_tx, 66);
	ret = transmit_type_bc(&tc_tx);
	assert_int_equal(0, ret);

	ret = dequeue(&uplink_channel, test_util);
	ret = tc_receive(test_util, TC_MAX_FRAME_LEN);
	assert_int_equal(0, ret);
	assert_int_equal(1, tc_rx.cop_cfg.farm.farmb_cnt);

	ret = dequeue(&uplink_channel, test_util);
	ret = tc_receive(test_util, TC_MAX_FRAME_LEN);
	assert_int_equal(0, ret);
	assert_int_equal(2, tc_rx.cop_cfg.farm.farmb_cnt);

}


