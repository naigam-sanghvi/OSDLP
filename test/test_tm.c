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

bool
tm_tx_queue_empty(uint8_t vcid)
{
	return (tx_queues[vcid].inqueue == 0);
}

int
tm_tx_queue_back(uint8_t **pkt, uint8_t vcid)
{
	*pkt = back(&tx_queues[vcid]);
	if (*pkt == NULL) {
		return -1;
	} else {
		return 0;
	}
}

int
tm_tx_queue_enqueue(uint8_t *pkt, uint8_t vcid)
{
	int ret = enqueue(&tx_queues[vcid], pkt);
	return ret;
}

int
tm_rx_queue_enqueue(uint8_t *pkt, uint8_t vcid)
{
	int ret = enqueue(&rx_queues[vcid], pkt);
	return ret;
}

/**
 * Returns the length of the packet pointed to by
 * the pointer
 * @param pointer to the variable where the length
 * will be stored
 * @param the pointer to the packet
 * @param the length of the memory space pointed
 * to by the pointer where the user is allowed to
 * search for the packet length field
 *
 * @return error code. Negative for error, zero or positive for success
 */

int
tm_get_packet_len(uint16_t *length, uint8_t *pkt, uint16_t mem_len)
{
	if (mem_len >= 5) {
		if (((pkt[3] << 8) | pkt[4]) <= TM_MAX_SDU_LEN) {
			*length = ((pkt[3] << 8) | pkt[4]);
			return 0;
		} else {
			return -1;
		}
	} else {
		return -1;
	}
}

void
tm_tx_commit_back(uint8_t vcid)
{
	return;
}

void
test_tm_no_stuffing(void **state)
{
	uint8_t rx_frame[TM_MAX_SDU_LEN];
	struct tm_transfer_frame tm_tx;
	struct tm_transfer_frame tm_rx;
	uint8_t cnt 		= 0;
	uint8_t vcid 		= 1;
	uint8_t scid 		= 30;
	tm_crc_flag_t crc 	= TM_CRC_PRESENT;
	tm_ocf_flag_t ocf 	= TM_OCF_NOTPRESENT;
	uint16_t frame_len 	= TM_FRAME_LEN;
	uint8_t maxvcs 		= 2;
	uint16_t maxfifo 	= 10;
	int ret = tm_init(&tm_tx, scid,
	                  &cnt, vcid, ocf,
	                  0, 0, 0, NULL, 0, crc,
	                  frame_len, TM_MAX_SDU_LEN, maxvcs, maxfifo,
	                  TM_STUFFING_OFF, util_tx);
	assert_int_equal(0, ret);

	ret = tm_init(&tm_rx, 0,
	              &cnt, vcid, ocf,
	              0, 0,
	              0, NULL, 0, crc,
	              frame_len, TM_MAX_SDU_LEN, maxvcs, maxfifo,
	              TM_STUFFING_OFF, util_rx);
	assert_int_equal(0, ret);

	uint16_t      up_chann_item_size = TC_MAX_FRAME_LEN;
	uint16_t      up_chann_capacity = 10;
	uint16_t      down_chann_item_size = 1;
	uint16_t      down_chann_capacity = 10;
	uint16_t      sent_item_size = 1;
	uint16_t      sent_capacity = 10;
	uint16_t      wait_item_size = 1;
	uint16_t      rx_item_size = TM_MAX_SDU_LEN;
	uint16_t      rx_capacity = 10;

	setup_queues(up_chann_item_size,
	             up_chann_capacity,
	             down_chann_item_size,
	             down_chann_capacity,
	             sent_item_size,
	             sent_capacity,
	             wait_item_size,
	             rx_item_size,
	             rx_capacity);                           /*Prepare queues*/

	uint8_t data[TM_MAX_SDU_LEN];
	uint16_t length = 100;
	for (int i = 0; i < length; i++)
		data[i] = i % 256;

	data[3] = (length >> 8) & 0xff;
	data[4] = length & 0xff;

	tm_transmit(&tm_tx, data, length);
	assert_int_equal(tx_queues[vcid].inqueue, 1);

	tm_transmit(&tm_tx, data, length);
	assert_int_equal(tx_queues[vcid].inqueue, 2);

	tm_transmit(&tm_tx, data, length);
	assert_int_equal(tx_queues[vcid].inqueue, 3);

	length = 600;
	for (int i = 0; i < length; i++)
		data[i] = i % 256;
	data[3] = (length >> 8) & 0xff;
	data[4] = length & 0xff;
	tm_transmit(&tm_tx, data, length);
	assert_int_equal(tx_queues[vcid].inqueue, 6);

	dequeue(&tx_queues[vcid], rx_frame);
	tm_receive(&tm_rx, rx_frame);
	assert_int_equal(rx_queues[vcid].inqueue, 1);

	dequeue(&tx_queues[vcid], rx_frame);
	tm_receive(&tm_rx, rx_frame);
	assert_int_equal(rx_queues[vcid].inqueue, 2);

	dequeue(&tx_queues[vcid], rx_frame);
	tm_receive(&tm_rx, rx_frame);
	assert_int_equal(rx_queues[vcid].inqueue, 3);

	memset(util_rx, 0, TM_MAX_SDU_LEN);

	dequeue(&tx_queues[vcid], rx_frame);
	tm_receive(&tm_rx, rx_frame);

	dequeue(&tx_queues[vcid], rx_frame);
	tm_receive(&tm_rx, rx_frame);

	dequeue(&tx_queues[vcid], rx_frame);
	tm_receive(&tm_rx, rx_frame);

	assert_int_equal(rx_queues[vcid].inqueue, 4);



	tm_transmit(&tm_tx, data, length);
	assert_int_equal(tx_queues[vcid].inqueue, 3);

	tm_transmit(&tm_tx, data, length);
	assert_int_equal(tx_queues[vcid].inqueue, 6);

	length = 300;
	for (int i = 0; i < length; i++)
		data[i] = i % 256;

	data[3] = (length >> 8) & 0xff;
	data[4] = length & 0xff;

	tm_transmit(&tm_tx, data, length);
	assert_int_equal(tx_queues[vcid].inqueue, 8);

	tm_transmit(&tm_tx, data, length);
	assert_int_equal(tx_queues[vcid].inqueue, 10);

	dequeue(&tx_queues[vcid], rx_frame);
	tm_receive(&tm_rx, rx_frame);

	dequeue(&tx_queues[vcid], rx_frame);
	tm_receive(&tm_rx, rx_frame);

	dequeue(&tx_queues[vcid], rx_frame);
	tm_receive(&tm_rx, rx_frame);

	assert_int_equal(rx_queues[vcid].inqueue, 5);

	dequeue(&tx_queues[vcid], rx_frame);
	tm_receive(&tm_rx, rx_frame);

	dequeue(&tx_queues[vcid], rx_frame);
	tm_receive(&tm_rx, rx_frame);

	dequeue(&tx_queues[vcid], rx_frame);
	tm_receive(&tm_rx, rx_frame);

	assert_int_equal(rx_queues[vcid].inqueue, 6);

	dequeue(&tx_queues[vcid], rx_frame);
	tm_receive(&tm_rx, rx_frame);

	dequeue(&tx_queues[vcid], rx_frame);
	tm_receive(&tm_rx, rx_frame);

	assert_int_equal(rx_queues[vcid].inqueue, 7);

	dequeue(&tx_queues[vcid], rx_frame);
	tm_receive(&tm_rx, rx_frame);

	dequeue(&tx_queues[vcid], rx_frame);
	tm_receive(&tm_rx, rx_frame);

	assert_int_equal(rx_queues[vcid].inqueue, 8);
}

void
test_tm_with_stuffing(void **state)
{
	tm_rx_result_t rx_status;
	uint8_t rx_frame[TM_MAX_SDU_LEN];
	struct tm_transfer_frame tm_tx;
	struct tm_transfer_frame tm_rx;
	uint8_t cnt 		= 0;
	uint8_t vcid 		= 1;
	uint8_t scid 		= 30;
	tm_crc_flag_t crc 	= TM_CRC_PRESENT;
	tm_ocf_flag_t ocf 	= TM_OCF_NOTPRESENT;
	uint16_t frame_len 	= TM_FRAME_LEN;
	uint8_t maxvcs 		= 2;
	uint16_t maxfifo 	= 10;
	int ret = tm_init(&tm_tx, scid,
	                  &cnt, vcid, ocf,
	                  0, 0, 0, NULL, 0, crc,
	                  frame_len, TM_MAX_SDU_LEN, maxvcs, maxfifo,
	                  TM_STUFFING_ON, util_tx);
	assert_int_equal(0, ret);

	ret = tm_init(&tm_rx, 0,
	              &cnt, vcid, ocf,
	              0, 0,
	              0, NULL, 0, crc,
	              frame_len, TM_MAX_SDU_LEN, maxvcs, maxfifo,
	              TM_STUFFING_ON, util_rx);
	assert_int_equal(0, ret);

	uint16_t      up_chann_item_size = TC_MAX_FRAME_LEN;
	uint16_t      up_chann_capacity = 10;
	uint16_t      down_chann_item_size = 1;
	uint16_t      down_chann_capacity = 10;
	uint16_t      sent_item_size = 1;
	uint16_t      sent_capacity = 10;
	uint16_t      wait_item_size = 1;
	uint16_t      rx_item_size = TM_MAX_SDU_LEN;
	uint16_t      rx_capacity = 10;

	setup_queues(up_chann_item_size,
	             up_chann_capacity,
	             down_chann_item_size,
	             down_chann_capacity,
	             sent_item_size,
	             sent_capacity,
	             wait_item_size,
	             rx_item_size,
	             rx_capacity);                           /*Prepare queues*/

	uint8_t data[TM_MAX_SDU_LEN];
	uint16_t length = 100;
	for (int i = 0; i < length; i++)
		data[i] = i % 256;

	data[3] = (length >> 8) & 0xff;
	data[4] = length & 0xff;

	tm_transmit(&tm_tx, data, length);
	assert_int_equal(tx_queues[vcid].inqueue, 1);

	tm_transmit(&tm_tx, data, length);
	assert_int_equal(tx_queues[vcid].inqueue, 1);

	tm_transmit(&tm_tx, data, length);
	assert_int_equal(tx_queues[vcid].inqueue, 2);

	tm_transmit(&tm_tx, data, length);
	assert_int_equal(tx_queues[vcid].inqueue, 2);

	length = 700;
	for (int i = 0; i < length; i++)
		data[i] = i % 256;

	data[3] = (length >> 8) & 0xff;
	data[4] = length & 0xff;

	tm_transmit(&tm_tx, data, length);

	assert_int_equal(tx_queues[vcid].inqueue, 5);

	length = 200;
	for (int i = 0; i < length; i++)
		data[i] = i % 256;

	data[3] = (length >> 8) & 0xff;
	data[4] = length & 0xff;

	tm_transmit(&tm_tx, data, length);

	assert_int_equal(tx_queues[vcid].inqueue, 6);


	dequeue(&tx_queues[vcid], rx_frame);
	rx_status = tm_receive(&tm_rx, rx_frame);
	assert_int_equal(rx_status, TM_RX_PENDING);
	assert_int_equal(rx_queues[vcid].inqueue, 2);

	dequeue(&tx_queues[vcid], rx_frame);
	tm_receive(&tm_rx, rx_frame);

	assert_int_equal(rx_queues[vcid].inqueue, 4);

	dequeue(&tx_queues[vcid], rx_frame);
	tm_receive(&tm_rx, rx_frame);

	dequeue(&tx_queues[vcid], rx_frame);
	tm_receive(&tm_rx, rx_frame);

	dequeue(&tx_queues[vcid], rx_frame);
	rx_status = tm_receive(&tm_rx, rx_frame);
	assert_int_equal(rx_status, TM_RX_PENDING);

	dequeue(&tx_queues[vcid], rx_frame);
	rx_status = tm_receive(&tm_rx, rx_frame);

	assert_int_equal(rx_status, TM_RX_OK);
	assert_int_equal(rx_queues[vcid].inqueue, 6);


	dequeue(&rx_queues[vcid], rx_frame);
	dequeue(&rx_queues[vcid], rx_frame);
	dequeue(&rx_queues[vcid], rx_frame);
	dequeue(&rx_queues[vcid], rx_frame);
	dequeue(&rx_queues[vcid], rx_frame);
	dequeue(&rx_queues[vcid], rx_frame);

	length = 246;
	for (int i = 0; i < length; i++)
		data[i] = i % 256;

	data[3] = (length >> 8) & 0xff;
	data[4] = length & 0xff;

	tm_transmit(&tm_tx, data, length);

	assert_int_equal(tx_queues[vcid].inqueue, 1);

	length = 250;
	for (int i = 0; i < length; i++)
		data[i] = i % 256;

	data[3] = (length >> 8) & 0xff;
	data[4] = length & 0xff;

	tm_transmit(&tm_tx, data, length);

	assert_int_equal(tx_queues[vcid].inqueue, 2);

	dequeue(&tx_queues[vcid], rx_frame);
	rx_status = tm_receive(&tm_rx, rx_frame);
	assert_int_equal(rx_status, TM_RX_PENDING);

	dequeue(&tx_queues[vcid], rx_frame);
	rx_status = tm_receive(&tm_rx, rx_frame);
	assert_int_equal(rx_status, TM_RX_OK);

	assert_int_equal(rx_queues[vcid].inqueue, 2);

	dequeue(&rx_queues[vcid], rx_frame);
	dequeue(&rx_queues[vcid], rx_frame);

	length = 246;
	for (int i = 0; i < length; i++)
		data[i] = i % 256;

	data[3] = (length >> 8) & 0xff;
	data[4] = length & 0xff;

	tm_transmit(&tm_tx, data, length);

	assert_int_equal(tx_queues[vcid].inqueue, 1);

	length = 100;
	for (int i = 0; i < length; i++)
		data[i] = i % 256;

	data[3] = (length >> 8) & 0xff;
	data[4] = length & 0xff;

	tm_transmit(&tm_tx, data, length);

	length = 50;
	for (int i = 0; i < length; i++)
		data[i] = i % 256;

	data[3] = (length >> 8) & 0xff;
	data[4] = length & 0xff;

	tm_transmit(&tm_tx, data, length);

	assert_int_equal(tx_queues[vcid].inqueue, 2);

	dequeue(&tx_queues[vcid], rx_frame);
	tm_receive(&tm_rx, rx_frame);

	dequeue(&tx_queues[vcid], rx_frame);
	tm_receive(&tm_rx, rx_frame);

	assert_int_equal(rx_queues[vcid].inqueue, 3);

	dequeue(&rx_queues[vcid], rx_frame);
	dequeue(&rx_queues[vcid], rx_frame);
	dequeue(&rx_queues[vcid], rx_frame);

	dequeue(&tx_queues[vcid], rx_frame);
	dequeue(&tx_queues[vcid], rx_frame);

	length = TM_MAX_SDU_LEN;
	for (int i = 0; i < length; i++)
		data[i] = i % 256;

	data[3] = (length >> 8) & 0xff;
	data[4] = length & 0xff;

	tm_transmit(&tm_tx, data, length);

	assert_int_equal(tx_queues[vcid].inqueue, 5);

	dequeue(&tx_queues[vcid], rx_frame);
	tm_receive(&tm_rx, rx_frame);
	dequeue(&tx_queues[vcid], rx_frame);
	tm_receive(&tm_rx, rx_frame);
	dequeue(&tx_queues[vcid], rx_frame);
	tm_receive(&tm_rx, rx_frame);
	dequeue(&tx_queues[vcid], rx_frame);
	tm_receive(&tm_rx, rx_frame);
	dequeue(&tx_queues[vcid], rx_frame);
	tm_receive(&tm_rx, rx_frame);

	assert_int_equal(rx_queues[vcid].inqueue, 1);
	dequeue(&rx_queues[vcid], rx_frame);

	tm_transmit_idle_fdu(&tm_tx, vcid);
	assert_int_equal(tx_queues[vcid].inqueue, 1);

	length = 2 * tm_tx.mission.max_data_len;
	for (int i = 0; i < length; i++)
		data[i] = i % 256;

	data[3] = (length >> 8) & 0xff;
	data[4] = length & 0xff;

	tm_transmit(&tm_tx, data, length);
	assert_int_equal(tx_queues[vcid].inqueue, 2);
}
