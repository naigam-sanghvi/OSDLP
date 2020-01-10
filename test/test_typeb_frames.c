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

#define NUMVCS	           3
#define MAX_SDU_SIZE	   600

struct queue  	           wait_queues[NUMVCS];     /* Wait queue */
struct queue	           sent_queues[NUMVCS];     /* Sent queue */
struct queue
	downlink_channel;        /* Queue simulating downlink channel */
struct queue
	uplink_channel;          /* Queue simulating uplink channel */
struct queue  	           rx_queues[NUMVCS];       /* Receiving queue */
struct tc_transfer_frame   tc_tx;
struct tc_transfer_frame   tc_rx;
/* Utility buffers */
uint8_t                    util_tx[MAX_SDU_SIZE];
uint8_t                    util_rx[MAX_SDU_SIZE];
uint8_t                    test_util[TC_MAX_FRAME_LEN];
uint8_t                    temp[TC_MAX_FRAME_LEN];

struct cop_config          cop_tx;
struct cop_config          cop_rx;
struct fop_config          fop;
struct farm_config         farm;

struct local_queue_item {
	uint8_t 	fdu[MAX_SDU_SIZE];
	uint8_t 	rt_flag;
	uint8_t		seq_num;
	tc_bypass_t type;
};

uint16_t
wait_queue_size(uint16_t vcid)
{
	return wait_queues[vcid].inqueue;
}

int
wait_queue_enqueue(void *tc_tf, uint16_t vcid)
{
	int ret = enqueue(&wait_queues[vcid], tc_tf);
	return ret;
}

int
wait_queue_dequeue(void *tc_tf, uint16_t vcid)
{
	int ret = dequeue(&wait_queues[vcid], tc_tf);
	return ret;
}

bool
wait_queue_empty(uint16_t vcid)
{
	return wait_queues[vcid].inqueue == 0 ? true : false;
}

int
wait_queue_clear(uint16_t vcid)
{
	int ret = reset_queue(&wait_queues[vcid]);
	return ret;
}

int
tx_queue_clear()
{
	int ret = reset_queue(&uplink_channel);
	return ret;
}

int
sent_queue_clear(uint16_t vcid)
{
	int ret = reset_queue(&sent_queues[vcid]);
	return ret;
}

uint16_t
sent_queue_size(uint16_t vcid)
{
	return sent_queues[vcid].inqueue;
}

int
sent_queue_dequeue(struct queue_item *qi, uint16_t vcid)
{
	struct local_queue_item new_item;
	int ret = dequeue(&sent_queues[vcid], &new_item);
	qi->fdu = new_item.fdu;
	qi->rt_flag = new_item.rt_flag;
	qi->seq_num = new_item.seq_num;
	qi->type = new_item.type;
	return ret;
}

int
sent_queue_enqueue(struct queue_item *qi, uint16_t vcid)
{
	uint16_t frame_len = (((qi->fdu[2] & 0x03) << 8) | qi->fdu[3]);
	struct local_queue_item new_item;
	memcpy(new_item.fdu, qi->fdu, (frame_len + 1) * sizeof(uint8_t));
	new_item.rt_flag = qi->rt_flag;
	new_item.seq_num = qi->seq_num;
	new_item.type = qi->type;
	int ret = enqueue(&sent_queues[vcid], &new_item);
	return ret;
}

bool
sent_queue_empty(uint16_t vcid)
{
	return sent_queues[vcid].inqueue == 0 ? true : false;
}

bool
sent_queue_full(uint16_t vcid)
{
	return sent_queues[vcid].inqueue == sent_queues[vcid].capacity ? true : false;
}

int
sent_queue_head(struct queue_item *qi, uint16_t vcid)
{
	struct local_queue_item *item;
	if (sent_queues[vcid].inqueue > 0) {
		item = (struct local_queue_item *) front(&sent_queues[vcid]);
		qi->fdu = item->fdu;
		qi->rt_flag = item->rt_flag;
		qi->seq_num = item->seq_num;
		qi->type = item->type;
		return 0;
	} else {
		return 1;
	}

}


bool
rx_queue_full(uint8_t vcid)
{
	return rx_queues[vcid].inqueue == rx_queues[vcid].capacity ? true : false;
}

bool
tx_queue_full()
{
	return uplink_channel.inqueue == uplink_channel.capacity ? true : false;
}

int
tx_queue_enqueue(uint8_t *buffer)
{
	int ret = enqueue(&uplink_channel, buffer);
	return ret;
}

int
rx_queue_enqueue(uint8_t *buffer, uint16_t vcid)
{
	int ret = enqueue(&rx_queues[vcid], buffer);
	return ret;
}

int
rx_queue_dequeue(uint8_t *buffer, uint16_t vcid)
{
	int ret = dequeue(&rx_queues[vcid], buffer);
	return ret;
}

int
rx_queue_enqueue_now(uint8_t *buffer, uint8_t vcid)
{
	int ret = enqueue(&rx_queues[vcid], buffer);
	if (ret == 1) {
		if (rx_queue_full(vcid)) {
			ret = enqueue_now(&rx_queues[vcid], buffer);
			return 0;
		}
	} else {
		return 0;
	}
	return 1;
}

struct tc_transfer_frame *
get_config(uint16_t vcid)
{
	if (vcid == 1) {
		return &tc_rx;
	} else {
		return NULL;
	}
}

int
cancel_lower_ops()
{
	int ret = tx_queue_clear();
	return ret;
}

int
timer_start(uint16_t vcid)
{
	return 0;
}

int
mark_ad_as_rt(uint16_t vcid)
{
	struct local_queue_item *item;
	for (int i = 0; i < sent_queues[vcid].inqueue; i++) {
		item = (struct local_queue_item *)get_element(&sent_queues[vcid], i);
		if (!item) {
			return 1;
		}
		if (item->type == TYPE_A) {
			item->rt_flag = RT_FLAG_ON;
		}
	}
	return 0;
}

int
get_first_ad_rt_frame(struct queue_item *qi, uint16_t vcid)
{
	struct local_queue_item *item;
	for (int i = 0; i < sent_queues[vcid].inqueue; i++) {
		item = (struct local_queue_item *)get_element(&sent_queues[vcid], i);
		if (!item) {
			return 1;
		}
		if (item->type == TYPE_A && item->rt_flag == RT_FLAG_ON) {
			qi->fdu = item->fdu;
			qi->rt_flag = item->rt_flag;
			qi->seq_num = item->seq_num;
			qi->type = item->type;
			return 0;
		}
	}
	return 1;
}

int
reset_rt_frame(struct queue_item *qi, uint16_t vcid)
{
	struct local_queue_item *item;
	for (int i = 0; i < sent_queues[vcid].inqueue; i++) {
		item = (struct local_queue_item *)get_element(&sent_queues[vcid], i);
		if (!item) {
			return 1;
		}
		if (item->seq_num == qi->seq_num) {
			item->rt_flag = RT_FLAG_OFF;
			return 0;
		}
	}
	return 1;
}

int
mark_bc_as_rt(uint16_t vcid)
{
	struct queue_item *item;
	item = (struct queue_item *)front(&sent_queues[vcid]);
	if (!item) {
		return 1;
	}
	if (item->type == TYPE_B) {
		item->rt_flag = RT_FLAG_ON;
	}
	return 0;
}

int
setup_queues()
{
	int ret = init(&uplink_channel,
	               TC_MAX_FRAME_LEN,
	               10);
	assert_int_equal(ret, 0);
	ret = init(&downlink_channel,
	           TC_MAX_FRAME_LEN,
	           10);
	assert_int_equal(ret, 0);
	for (int i = 0; i < NUMVCS; i++) {
		ret = init(&sent_queues[i],
		           sizeof(struct queue_item),
		           10);
		assert_int_equal(ret, 0);

		ret = init(&wait_queues[i],
		           sizeof(struct tc_transfer_frame),
		           10);
		assert_int_equal(ret, 0);

		ret = init(&rx_queues[i],
		           TC_MAX_FRAME_LEN,
		           10);
		assert_int_equal(ret, 0);
	}
	return 0;
}

int
setup_configs()
{
	int ret;
	uint16_t      scid = 101;
	uint16_t      max_frame_size = TC_MAX_FRAME_LEN;
	uint8_t       vcid = 1;
	uint8_t       mapid = 1;
	tc_crc_flag_t crc = TC_CRC_PRESENT;
	tc_seg_hdr_t  seg_hdr = TC_SEG_HDR_PRESENT;
	tc_bypass_t   bypass = TYPE_B;
	tc_ctrl_t     ctrl = TC_DATA;


	ret = prepare_fop(&fop, 5, FOP_STATE_INIT, 100, 0, 3);
	cop_tx.fop = fop;
	ret = tc_init(&tc_tx, scid, MAX_SDU_SIZE,
	              max_frame_size, vcid, mapid, crc,
	              seg_hdr, bypass, ctrl, util_tx, cop_tx);
	assert_int_not_equal(ret, 1);
	ret = prepare_farm(&farm, FARM_STATE_OPEN, 10);
	cop_rx.farm = farm;
	ret = tc_init(&tc_rx, scid, MAX_SDU_SIZE,
	              max_frame_size, vcid, mapid, crc,
	              seg_hdr, bypass, ctrl, util_rx, cop_rx);
	assert_int_not_equal(ret, 1);
	return 0;
}

/**
 * Send two type BD frames and receive them in the RX queue
 */
void
test_simple_bd_frame(void **state)
{
	setup_queues();                                 /*Prepare queues*/
	setup_configs();                                /*Prepare config structs*/

	int ret;
	notification_t notif;

	notif = initiate_no_clcw(&tc_tx);               /* Initiate service*/
	assert_int_equal(notif, POSITIVE_TX);
	uint8_t buf[100];
	for (int i = 0; i < 100; i++) {
		buf[i] = i;
	}
	ret = prepare_typeb_data_frame(&tc_tx, buf, 100);
	assert_int_equal(ret, 0);
	notif = tc_transmit(&tc_tx, buf, 100);            /* Transmit packet */
	assert_int_equal(notif, ACCEPT_TX);
	assert_int_equal(1, uplink_channel.inqueue);
	memcpy(temp, tc_tx.mission.util.buffer, 110);	/* Transmit 2nd packet */
	notif = tc_transmit(&tc_tx, buf, 100);
	assert_int_equal(notif, ACCEPT_TX);
	assert_int_equal(2, uplink_channel.inqueue);

	ret = dequeue(&uplink_channel, test_util);
	ret = tc_receive(test_util, TC_MAX_FRAME_LEN);    /* Receive first packet*/
	assert_int_equal(0, ret);

	ret = dequeue(&uplink_channel, test_util);
	ret = tc_receive(test_util, TC_MAX_FRAME_LEN);    /* Receive 2nd packet*/
	assert_int_equal(0, ret);
	assert_int_equal(2, rx_queues[1].inqueue);
}




