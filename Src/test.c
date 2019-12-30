/*
 * tm_test.c
 *
 *  Created on: Dec 24, 2019
 *      Author: sleepwalker
 */


#include "tm.h"
#include "tc.h"
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdio.h>
#include <stdlib.h>

static void
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
	uint8_t maxfifo 	= 10;
	int ret = tm_init(&tm_tx, scid,
	                  &cnt, vcid, mcid, ocf,
	                  0, 0, 0, NULL, 0, crc,
	                  frame_len, maxvcs, maxfifo);
	assert_int_equal(0, ret);

	ret = tm_init(&tm_rx, 0,
	              &cnt, 0, 0, 0, 0, 0,
	              0, NULL, 0, crc,
	              frame_len, maxvcs, maxfifo);
	assert_int_equal(0, ret);


	ret = tm_pack(&tm_tx, tx_buf, data, 100);
	assert_int_equal(0, ret);

	ret = tm_unpack(&tm_rx, tx_buf);
	assert_int_equal(0, ret);

	assert_int_equal(tm_tx.primary_hdr.ocf, tm_rx.primary_hdr.ocf);
	assert_int_equal(tm_tx.primary_hdr.mcid.spacecraft_id,
	                 tm_rx.primary_hdr.mcid.spacecraft_id);
	assert_int_equal(tm_tx.primary_hdr.vcid, tm_rx.primary_hdr.vcid);
	assert_memory_equal(data, tm_rx.data, 100);
	assert_int_equal(tm_tx.crc, tm_rx.crc);
}

static void
test_tc(void **state)
{
	uint8_t tx_buf[300];
	uint8_t data[100];
	uint8_t util[100];
	for (int i = 0; i < 100; i++)
		data[i] = rand() % 256;
	struct tc_transfer_frame tc_tx;
	struct tc_transfer_frame tc_rx;
	struct cop_params cop;
	uint8_t vcid 			= 4;
	uint8_t scid 			= 30;
	uint8_t mapid 			= 20;
	uint16_t max_fdu_size 	= 500;
	tc_crc_flag_t crc 		= TC_CRC_PRESENT;
	tc_seg_hdr_t seg 		= TC_SEG_HDR_PRESENT;
	tc_bypass_t bypass 		= TYPE_A;
	tc_ctrl_t ctrl_cmd 		= TC_DATA;
	int ret = tc_init(&tc_tx,
	                  scid, max_fdu_size,
	                  vcid, mapid, crc,
	                  seg, bypass, ctrl_cmd,
	                  util, cop);
	assert_int_equal(0, ret);

	ret = tc_init(&tc_rx,
	              0, max_fdu_size,
	              0, 0, crc,
	              seg, 0, 0,
	              util, cop);
	assert_int_equal(0, ret);


	ret = tc_pack(&tc_tx, tx_buf, data, 100);
	assert_int_equal(0, ret);

	ret = tc_unpack(&tc_rx, tx_buf);
	assert_int_equal(0, ret);

	assert_int_equal(tc_tx.primary_hdr.bypass, tc_rx.primary_hdr.bypass);
	assert_int_equal(tc_tx.primary_hdr.spacecraft_id,
	                 tc_rx.primary_hdr.spacecraft_id);
	assert_int_equal(tc_tx.primary_hdr.vcid, tc_rx.primary_hdr.vcid);
	assert_memory_equal(data, tc_rx.frame_data.data, 100);
	assert_int_equal(tc_tx.crc, tc_rx.crc);
}

int
main()
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_tm),
		cmocka_unit_test(test_tc),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
