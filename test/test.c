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
uint8_t                    util_tx[TC_MAX_SDU_SIZE];
uint8_t                    util_rx[TC_MAX_SDU_SIZE];

uint8_t                    util_tx_unseg[TC_MAX_SDU_SIZE];
uint8_t                    util_rx_unseg[TC_MAX_SDU_SIZE];
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

struct tm_transfer_frame   tm_tx;
struct tm_transfer_frame   tm_rx;
struct queue  	           tx_queues[NUMVCS];     /* TM TX queues */

uint16_t
osdlp_tc_wait_queue_size(uint16_t vcid)
{
	return wait_queues[vcid].inqueue;
}

int
osdlp_tc_wait_queue_enqueue(void *tc_tf, uint16_t vcid)
{
	int ret = enqueue(&wait_queues[vcid], tc_tf);
	return ret;
}

int
osdlp_tc_wait_queue_dequeue(void *tc_tf, uint16_t vcid)
{
	int ret = dequeue(&wait_queues[vcid], tc_tf);
	return ret;
}

bool
osdlp_tc_wait_queue_empty(uint16_t vcid)
{
	return wait_queues[vcid].inqueue == 0 ? true : false;
}

int
osdlp_tc_wait_queue_clear(uint16_t vcid)
{
	int ret = reset_queue(&wait_queues[vcid]);
	return ret;
}

int
osdlp_tc_tx_queue_clear()
{
	int ret = reset_queue(&uplink_channel);
	return ret;
}

int
osdlp_tc_sent_queue_clear(uint16_t vcid)
{
	int ret = reset_queue(&sent_queues[vcid]);
	return ret;
}

uint16_t
tc_sent_queue_size(uint16_t vcid)
{
	return sent_queues[vcid].inqueue;
}

int
osdlp_tc_sent_queue_dequeue(struct queue_item *qi, uint16_t vcid)
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
osdlp_tc_sent_queue_enqueue(struct queue_item *qi, uint16_t vcid)
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
osdlp_tc_sent_queue_empty(uint16_t vcid)
{
	return sent_queues[vcid].inqueue == 0 ? true : false;
}

bool
osdlp_tc_sent_queue_full(uint16_t vcid)
{
	return sent_queues[vcid].inqueue == sent_queues[vcid].capacity ? true : false;
}

int
osdlp_tc_sent_queue_head(struct queue_item *qi, uint16_t vcid)
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
		return -1;
	}

}

struct tc_transfer_frame *
osdlp_tc_get_tx_config(uint16_t vcid)
{
	if (vcid == 1) {
		return &tc_tx;
	} else if (vcid == 0) {
		return &tc_tx_unseg;
	} else {
		return NULL;
	}
}


bool
osdlp_tc_rx_queue_full(uint16_t vcid)
{
	return rx_queues[vcid].inqueue == rx_queues[vcid].capacity ? true : false;
}

bool
osdlp_tc_tx_queue_full()
{
	return uplink_channel.inqueue == uplink_channel.capacity ? true : false;
}

int
osdlp_tc_tx_queue_enqueue(uint8_t *buffer, uint16_t vcid)
{
	struct tc_transfer_frame *tc;
	tc = (struct tc_transfer_frame *)osdlp_tc_get_tx_config(vcid);
	int ret = enqueue(&uplink_channel, buffer);
	if ((buffer[0] >> 5) & 0x01) {   // Type B
		if ((buffer[0] >> 4) & 0x01) { // Control
			if (ret < 0) {
				osdlp_bc_reject(tc);
			} else {
				osdlp_bc_accept(tc);
			}
		} else {						// Data
			if (ret < 0) {
				osdlp_bd_reject(tc);
			} else {
				osdlp_bd_accept(tc);
			}
		}
	} else { // Type A
		if (ret < 0) {
			osdlp_ad_reject(tc);
		} else {
			osdlp_ad_accept(tc);
		}
	}
	return ret;
}

int
osdlp_tc_rx_queue_enqueue(uint8_t *buffer, uint32_t length, uint16_t vcid)
{
	int ret = enqueue(&rx_queues[vcid], buffer);
	return ret;
}

int
osdlp_tc_rx_queue_enqueue_now(uint8_t *buffer, uint32_t length, uint8_t vcid)
{
	int ret = enqueue(&rx_queues[vcid], buffer);
	if (ret < 0) {
		if (osdlp_tc_rx_queue_full(vcid)) {
			ret = enqueue_now(&rx_queues[vcid], buffer);
			return 0;
		}
	} else {
		return 0;
	}
	return -1;
}

int
osdlp_tc_get_rx_config(struct tc_transfer_frame **tf, uint16_t vcid)
{
	if (vcid == 1) {
		*tf = &tc_rx;
		return 0;
	} else if (vcid == 0) {
		*tf = &tc_rx_unseg;
		return 0;
	} else {
		return -1;
	}
}

int
osdlp_cancel_lower_ops()
{
	int ret = osdlp_tc_tx_queue_clear();
	return ret;
}

int
osdlp_mark_ad_as_rt(uint16_t vcid)
{
	struct local_queue_item *item;
	for (int i = 0; i < sent_queues[vcid].inqueue; i++) {
		item = (struct local_queue_item *)get_element(&sent_queues[vcid], i);
		if (!item) {
			return -1;
		}
		if (item->type == TYPE_A) {
			item->rt_flag = RT_FLAG_ON;
		}
	}
	return 0;
}

int
osdlp_get_first_ad_rt_frame(struct queue_item *qi, uint16_t vcid)
{
	struct local_queue_item *item;
	for (int i = 0; i < sent_queues[vcid].inqueue; i++) {
		item = (struct local_queue_item *)get_element(&sent_queues[vcid], i);
		if (!item) {
			return -1;
		}
		if (item->type == TYPE_A && item->rt_flag == RT_FLAG_ON) {
			qi->fdu = item->fdu;
			qi->rt_flag = item->rt_flag;
			qi->seq_num = item->seq_num;
			qi->type = item->type;
			return 0;
		}
	}
	return -1;
}

int
osdlp_reset_rt_frame(struct queue_item *qi, uint16_t vcid)
{
	struct local_queue_item *item;
	for (int i = 0; i < sent_queues[vcid].inqueue; i++) {
		item = (struct local_queue_item *)get_element(&sent_queues[vcid], i);
		if (!item) {
			return -1;
		}
		if (item->seq_num == qi->seq_num) {
			item->rt_flag = RT_FLAG_OFF;
			return 0;
		}
	}
	return -1;
}

int
osdlp_mark_bc_as_rt(uint16_t vcid)
{
	struct queue_item *item;
	item = (struct queue_item *)front(&sent_queues[vcid]);
	if (!item) {
		return -1;
	}
	if (item->type == TYPE_B) {
		item->rt_flag = RT_FLAG_ON;
	}
	return 0;
}

/**
 * Setts the parameters for the various queues
 * @param up_chann_item_size uplink channel queue item size
 * @param up_chann_capacity uplink channel queue capacity
 * @param down_chann_item_size downlink channel queue item size
 * @param down_chann_capacity downlink channel queue capacity
 * @param sent_item_size sent queue item size
 * @param sent_capacity sent queue capacity
 * @param wait_item_size wait queue item size
 * @param rx_item_size rx queue item size
 * @param rx_capacity rx queue capacity
 */
int
setup_queues(uint16_t up_chann_item_size,
             uint16_t up_chann_capacity,
             uint16_t down_chann_item_size,
             uint16_t down_chann_capacity,
             uint16_t sent_item_size,
             uint16_t sent_capacity,
             uint16_t wait_item_size,
             uint16_t rx_item_size,
             uint16_t rx_capacity)
{
	int ret = init(&uplink_channel,
	               up_chann_item_size,
	               up_chann_capacity);
	assert_int_equal(ret, 0);
	ret = init(&downlink_channel,
	           down_chann_item_size,
	           down_chann_capacity);
	assert_int_equal(ret, 0);
	for (int i = 0; i < NUMVCS; i++) {
		ret = init(&sent_queues[i],
		           sent_item_size,
		           sent_capacity);
		assert_int_equal(ret, 0);

		ret = init(&wait_queues[i],
		           wait_item_size,
		           1);
		assert_int_equal(ret, 0);

		ret = init(&rx_queues[i],
		           rx_item_size,
		           rx_capacity);
		assert_int_equal(ret, 0);

		ret = init(&tx_queues[i],
		           TM_FRAME_LEN,
		           TM_TX_CAPACITY);
		assert_int_equal(ret, 0);
	}
	return 0;
}

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
                 uint8_t farm_wnd_width)
{
	int ret;
	osdlp_prepare_fop(fop, fop_slide_wnd,
	                  fop_init_st, fop_t1_init,
	                  fop_timeout_type, fop_tx_limit);
	memcpy(&fop_conf->fop, fop, sizeof(struct fop_config));
	//cop_tx.fop = fop;
	ret = osdlp_tc_init(tc_tx, scid, TC_MAX_SDU_SIZE,
	                    max_frame_size, max_fifo_size, vcid, mapid, crc,
	                    seg_hdr, bypass, ctrl, util_tx, *fop_conf);
	assert_int_not_equal(ret, -1);
	osdlp_prepare_farm(farm, farm_init_st, farm_wnd_width);
	memcpy(&farm_conf->farm, farm, sizeof(struct farm_config));
	//farm_conf.farm = farm;
	ret = osdlp_tc_init(tc_rx, scid, TC_MAX_SDU_SIZE,
	                    max_frame_size, max_fifo_size, vcid, mapid, crc,
	                    seg_hdr, bypass, ctrl, util_rx, *farm_conf);
	assert_int_not_equal(ret, -1);
	return 0;
}

int
main()
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_tm),
		cmocka_unit_test(test_tc),
		cmocka_unit_test(test_simple_bd_frame),
		cmocka_unit_test(test_spp_hdr_only),
		cmocka_unit_test(test_spp_hdr_with_data),
		cmocka_unit_test(test_spp_invalid),
		cmocka_unit_test(test_unlock_cmd),
		cmocka_unit_test(test_vr),
		cmocka_unit_test(test_operation),
		cmocka_unit_test(test_tm_no_stuffing),
		cmocka_unit_test(test_tm_with_stuffing)
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
