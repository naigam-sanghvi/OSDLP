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

#include "test.h"

void
test_tm(void **state)
{
	uint8_t tx_buf[300];
	uint8_t data[100];
	for (int i = 0; i < 100; i++)
		data[i] = rand() % 256;
	struct tm_transfer_frame tm_tx;
	struct tm_transfer_frame tm_rx;
	uint8_t cnt 		= 0;
	uint8_t vcid 		= 4;
	uint8_t mcid 		= 4;
	uint8_t scid 		= 30;
	tm_crc_flag_t crc 	= TM_CRC_PRESENT;
	tm_ocf_flag_t ocf 	= TM_OCF_NOTPRESENT;
	uint16_t frame_len 	= 300;
	uint8_t maxvcs 		= 2;
	uint16_t maxfifo 	= 10;
	int ret = tm_init(&tm_tx, scid,
	                  &cnt, vcid, mcid, ocf,
	                  0, 0, 0, NULL, 0, crc,
	                  frame_len, maxvcs, maxfifo,
	                  TM_STUFFING_OFF, util_tx);
	assert_int_equal(0, ret);

	ret = tm_init(&tm_rx, 0,
	              &cnt, 0, 0, 0, 0, 0,
	              0, NULL, 0, crc,
	              frame_len, maxvcs, maxfifo,
	              TM_STUFFING_OFF, util_tx);
	assert_int_equal(0, ret);


	tm_pack(&tm_tx, tx_buf, data, 100);

	tm_unpack(&tm_rx, tx_buf);

	assert_int_equal(tm_tx.primary_hdr.ocf, tm_rx.primary_hdr.ocf);
	assert_int_equal(tm_tx.primary_hdr.mcid.spacecraft_id,
	                 tm_rx.primary_hdr.mcid.spacecraft_id);
	assert_int_equal(tm_tx.primary_hdr.vcid, tm_rx.primary_hdr.vcid);
	assert_memory_equal(data, tm_rx.data, 100);
	assert_int_equal(tm_tx.crc, tm_rx.crc);
}

void
test_tc(void **state)
{
	uint8_t tx_buf[300];
	uint8_t data[100];
	uint8_t util[100];
	for (int i = 0; i < 100; i++)
		data[i] = rand() % 256;
	struct tc_transfer_frame tc_tx;
	struct tc_transfer_frame tc_rx;
	struct cop_config cop;
	uint8_t vcid 			= 4;
	uint8_t scid 			= 30;
	uint8_t mapid 			= 20;
	uint16_t max_sdu_size 	= 700;
	uint16_t max_fdu_size 	= 500;
	tc_crc_flag_t crc 		= TC_CRC_PRESENT;
	tc_seg_hdr_t seg 		= TC_SEG_HDR_PRESENT;
	tc_bypass_t bypass 		= TYPE_A;
	tc_ctrl_t ctrl_cmd 		= TC_DATA;
	uint16_t rx_max_fifo_size = 10;
	int ret = tc_init(&tc_tx,
	                  scid, max_sdu_size, max_fdu_size,
	                  rx_max_fifo_size,
	                  vcid, mapid, crc,
	                  seg, bypass, ctrl_cmd,
	                  util, cop);
	assert_int_equal(0, ret);

	ret = tc_init(&tc_rx,
	              0, max_sdu_size, max_fdu_size,
	              rx_max_fifo_size,
	              0, 0, crc,
	              seg, 0, 0,
	              util, cop);
	assert_int_equal(0, ret);


	tc_pack(&tc_tx, tx_buf, data, 100);

	tc_unpack(&tc_rx, tx_buf);

	assert_int_equal(tc_tx.primary_hdr.bypass, tc_rx.primary_hdr.bypass);
	assert_int_equal(tc_tx.primary_hdr.spacecraft_id,
	                 tc_rx.primary_hdr.spacecraft_id);
	assert_int_equal(tc_tx.primary_hdr.vcid, tc_rx.primary_hdr.vcid);
	assert_memory_equal(data, tc_rx.frame_data.data, 100);
	assert_int_equal(tc_tx.crc, tc_rx.crc);
}

