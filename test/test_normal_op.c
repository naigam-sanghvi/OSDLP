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
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include "test.h"
#include "queue_util.h"

uint8_t buf[TC_MAX_SDU_SIZE];
pthread_mutex_t lock;
pthread_mutex_t tlock;
bool has_ended;
bool kill_timer;
bool timer_running;
bool start_timer;
bool timer_expired;
bool req_for_timer;
int total_packets = 0;
bool end_process;
int packets_rxed = 0;
uint16_t active_vcid = 0;

int
tc_rx_queue_clear(uint16_t vcid)
{
	reset_queue(&rx_queues[vcid]);
	return 0;
}

void *
transmitter(void *vargp)
{
	notification_t notif;
	int tc_tx_ret;
	pthread_mutex_lock(&lock);
	notif = initiate_no_clcw(&tc_tx);
	assert_int_equal(notif, POSITIVE_DIR);
	pthread_mutex_unlock(&lock);

	pthread_mutex_lock(&lock);
	notif = initiate_no_clcw(&tc_tx_unseg);
	assert_int_equal(notif, POSITIVE_DIR);
	pthread_mutex_unlock(&lock);

	int tx_sleep;
	bool has_packets = false;
	int size = 150;
	int npackets;
	int packets_txed = 6;
	/* First transmit unsegmented packets */
	while (1) {
		if (packets_txed == 0) {
			if (!(sent_queues[0].inqueue != 0)) {
				break;
			}
		}
		if (tc_tx_unseg.cop_cfg.fop.signal == ACCEPT_TX ||
		    tc_tx_unseg.cop_cfg.fop.signal == ACCEPT_DIR ||
		    tc_tx_unseg.cop_cfg.fop.signal == POSITIVE_TX ||
		    tc_tx_unseg.cop_cfg.fop.signal == POSITIVE_DIR) {
			if (tc_tx_unseg.seg_status.flag == SEG_IN_PROGRESS) {
				tc_tx_ret = tc_transmit(&tc_tx_unseg, buf, size);
				assert_int_equal(tc_tx_ret, TC_TX_OK);
				assert_int_equal(tc_tx.cop_cfg.fop.signal, ACCEPT_TX);
			} else {
				if (packets_txed == 0) {
					continue;
				}
				packets_txed--;
				for (int i = 0; i < size; i++) {
					buf[i] = rand() % 256;
				}
				/*Insert sequence number in the
				 * first and last byte of the TX buffer,
				 * as well as the size in the 2nd and 3rd bytes*/
				buf[0] = (packets_txed & 0xff);
				buf[size - 1] = (packets_txed & 0xff);
				buf[1] = (size >> 8) & 0xff;
				buf[2] = (size) & 0xff;
				/* Prepare the frame */
				prepare_typea_data_frame(&tc_tx_unseg, buf, size);

				pthread_mutex_lock(&lock);
				/* Transmit the frame */
				tc_tx_ret = tc_transmit(&tc_tx_unseg, buf, size);

				pthread_mutex_unlock(&lock);
			}
			if (tc_tx_ret == -TC_TX_DELAY) {
				/* In this case, COP has informed us that although a frame was inserted
				 * in the wait queue, for some reason it hasn't been able to transmit it
				 * yet. For this, we'll have to check the 'signal' field in the transfer
				 * frame struct to inform us when the COP has accepted our frame */
				continue;
			}
		}
	}
	active_vcid = 1;
	while (1) {
		/* If some error occurs, break;*/
		if (tc_tx.cop_cfg.fop.signal == UNDEF_ERROR ||
		    ((tc_tx.cop_cfg.fop.signal >= ALERT_LIMIT)
		     && (tc_tx.cop_cfg.fop.signal <= ALERT_TERM))) {
			break;
		}
		if (end_process) {
			/* Wait for any unfinished segmentation to end before exiting*/
			if (!(sent_queues[1].inqueue != 0
			      || tc_tx.seg_status.flag == SEG_IN_PROGRESS)) {
				break;
			}
		}
		if (!has_packets && !end_process) {
			/* Randomly select parameters of transmission */
			npackets = (rand() % 5) + 1;
			packets_txed = npackets;
			has_packets = true;
			tx_sleep = (rand() % 3);
			if (total_packets == 0) {
				size = TC_MAX_SDU_SIZE;
			}
			size = (rand() % (TC_MAX_SDU_SIZE - 10)) + 4;
			sleep(tx_sleep);
		}
		/* Ensure that we have received the ACCEPT signal from the COP
		 * in order to proceed with a new transmission */
		if (tc_tx.cop_cfg.fop.signal == ACCEPT_TX ||
		    tc_tx.cop_cfg.fop.signal == ACCEPT_DIR ||
		    tc_tx.cop_cfg.fop.signal == POSITIVE_TX ||
		    tc_tx.cop_cfg.fop.signal == POSITIVE_DIR) {
			if (tc_tx.seg_status.flag == SEG_IN_PROGRESS) {
				tc_tx_ret = tc_transmit(&tc_tx, buf, size);
			} else {
				if (packets_txed == 0) {
					has_packets = false;
					continue;
				}
				packets_txed--;

				for (int i = 0; i < size; i++) {
					buf[i] = rand() % 256;
				}
				buf[0] = (total_packets & 0xff);
				buf[size - 1] = (total_packets & 0xff);
				buf[1] = (size >> 8) & 0xff;
				buf[2] = (size) & 0xff;
				prepare_typea_data_frame(&tc_tx, buf, size);

				pthread_mutex_lock(&lock);
				tc_tx_ret = tc_transmit(&tc_tx, buf, size);
				total_packets++;
				pthread_mutex_unlock(&lock);
			}
			if (tc_tx_ret == -TC_TX_DELAY) {
				continue;
			}
			if (packets_txed == 0) {
				has_packets = false;
			}
		}
		if (tc_tx.cop_cfg.fop.signal == NEGATIVE_TX ||
		    tc_tx.cop_cfg.fop.signal == UNDEF_ERROR) {
			assert_int_equal(1, 0);
			break;
		}
	}
	pthread_mutex_lock(&lock);
	has_ended = true;
	pthread_mutex_unlock(&lock);
	return NULL;
}

void *
receiver(void *vargp)
{
	int ret;
	int cnt = 10;
	int rx_sleep;
	int packets = 0;
	srand(time(0));
	while (1) {
		/* Sleep some random time to simulate delays*/
		rx_sleep = rand() % 3;
		sleep(rx_sleep);
		pthread_mutex_lock(&lock);
		if (has_ended) {
			pthread_mutex_unlock(&lock);
			cnt--;
			if (cnt == 0) {
				break;
			}
		}
		pthread_mutex_unlock(&lock);
		pthread_mutex_lock(&lock);
		if (uplink_channel.inqueue != 0) {
			/* Ignore one in five packets to simulate packet loss*/
			if ((rand() % 5) == 0) {
				ret = dequeue(&uplink_channel, test_util);
				pthread_mutex_unlock(&lock);
				continue;
			}
			packets++;
			/* Dequeue the packet from the receiving queue*/
			ret = dequeue(&uplink_channel, test_util);
			assert_int_equal(ret, 0);
			/* Pass it to the receive function of the TC*/
			tc_receive(test_util, TC_MAX_FRAME_LEN);
			/*Respond with the clcw */
			if (((test_util[2] >> 2) & 0x3f) == 1)
				prepare_clcw(&tc_rx, test_util);
			else if (((test_util[2] >> 2) & 0x3f) == 0)
				prepare_clcw(&tc_rx_unseg, test_util);
			assert_int_equal(ret, 0);
			//clcw_pack(&clcw, test_util);

			ret = enqueue(&downlink_channel, test_util);
			assert_int_equal(ret, 0);
			if (rx_queues[1].inqueue == rx_queues[1].capacity) {
				/* Handle the case where the RX queue is full*/
				uint8_t *p;
				uint16_t size;
				for (int i = 0; i < rx_queues[1].inqueue; i++) {
					p = (uint8_t *)get_element(&rx_queues[1], i);
					size = ((p[1] << 8) | p[2]);
					assert_int_equal(p[0], p[size - 1]);
				}
				packets_rxed += rx_queues[1].inqueue;
				tc_rx_queue_clear(1);
				/*This function must be called after an rx_queue_full event,
				 * to notify COP that it can resume operations and reset the
				 * wait flag */
				buffer_release(&tc_rx);
			}
			pthread_mutex_unlock(&lock);
		} else {
			pthread_mutex_unlock(&lock);
		}
	}
	return NULL;
}

/**
 * Listens for clcw returns
 */
void *
clcw_listener(void *vargp)
{
	int cnt = 10;
	struct clcw_frame clcw;
	uint8_t clcw_buf[100];
	while (1) {
		if (has_ended) {
			cnt--;
			if (cnt == 0) {
				break;
			}
		}
		pthread_mutex_lock(&lock);
		if (downlink_channel.inqueue != 0) {

			struct queue_item it;
			int ret = get_first_ad_rt_frame(&it, 1);
			ret = dequeue(&downlink_channel, clcw_buf);
			assert_int_equal(ret, 0);
			clcw_unpack(&clcw, clcw_buf);
			if (clcw.vcid == 0) {
				/* This is the function that must be called whenever
				 * a new clcw is received */
				handle_clcw(&tc_tx_unseg, clcw_buf);
			} else if (clcw.vcid == 1) {
				handle_clcw(&tc_tx, clcw_buf);
			}

			pthread_mutex_unlock(&lock);
		} else {
			pthread_mutex_unlock(&lock);
		}
	}
	return NULL;
}

void *
timer_thread(void *vargp)
{
	int trigger = tc_tx.cop_cfg.fop.t1_init;
	time_t begin, end;
	begin = time(NULL);
	do {
		if (req_for_timer) {                           // Restart
			begin = time(NULL);
			req_for_timer = false;
		}
		end = time(NULL);
	} while ((difftime(end, begin) < trigger) && !kill_timer);
	if (kill_timer) {
		kill_timer = false;
		pthread_mutex_lock(&tlock);
		timer_running = false;
		pthread_mutex_unlock(&tlock);
		return NULL;
	} else {
		pthread_mutex_lock(&tlock);
		timer_running = false;
		timer_expired = true;
		kill_timer = false;
		pthread_mutex_unlock(&tlock);
	}

	return NULL;
}
int
timer_start(uint16_t vcid)
{
	pthread_mutex_lock(&tlock);
	if (timer_running) {
		req_for_timer = true;
		pthread_mutex_unlock(&tlock);
		return 0;
	}
	start_timer = true;
	pthread_mutex_unlock(&tlock);
	return 0;
}

int
timer_cancel(uint16_t vcid)
{
	if (timer_running) {
		kill_timer = true;
	}
	return 0;
}
void *
timer_handler(void *vargp)
{
	while (1) {
		if (has_ended) {
			break;
		}
		//usleep(100);
		if (start_timer && !timer_running) {
			pthread_t thread4;
			pthread_create(&thread4, NULL, timer_thread, NULL);
			pthread_mutex_lock(&tlock);
			start_timer = false;
			timer_running = true;
			pthread_mutex_unlock(&tlock);
			pthread_join(thread4, NULL);
		}
		if (timer_expired) {
			/* This function must be called whenever the timer expires */
			if (active_vcid == 0)
				handle_timer_expired(&tc_tx_unseg);
			else
				handle_timer_expired(&tc_tx);
			pthread_mutex_lock(&tlock);
			timer_expired = false;
			pthread_mutex_unlock(&tlock);
		}
	}
	return NULL;
}

void *
process_timer(void *var)
{
	time_t begin, end;
	begin = time(NULL);
	int trigger = 30;
	do {
		end = time(NULL);
	} while (difftime(end, begin) < trigger);
	end_process = true;
	return NULL;
}
/**
 * Receiver drops one packet
 */
void
test_operation(void **state)
{
	uint16_t      up_chann_item_size = TC_MAX_FRAME_LEN;
	uint16_t      up_chann_capacity = 10;
	uint16_t      down_chann_item_size = sizeof(struct clcw_frame);
	uint16_t      down_chann_capacity = 10;
	uint16_t      sent_item_size = sizeof(struct local_queue_item);
	uint16_t      sent_capacity = 10;
	uint16_t      wait_item_size = sizeof(struct tc_transfer_frame);
	uint16_t      rx_item_size = TC_MAX_SDU_SIZE;
	uint16_t      rx_capacity = 10;

	uint16_t      scid = 101;
	uint16_t      max_frame_size = TC_MAX_FRAME_LEN;
	uint8_t       vcid = 1;
	uint8_t       mapid = 1;
	tc_crc_flag_t crc = TC_CRC_PRESENT;
	tc_seg_hdr_t  seg_hdr = TC_SEG_HDR_PRESENT;
	tc_bypass_t   bypass = TYPE_B;
	tc_ctrl_t     ctrl = TC_DATA;
	uint16_t      fop_slide_wnd = 5;
	fop_state_t   fop_init_st = FOP_STATE_INIT;
	uint16_t      fop_t1_init = 1;
	uint16_t      fop_timeout_type = 0;
	uint8_t       fop_tx_limit = 10;
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

	setup_tc_configs(&tc_tx_unseg, &tc_rx_unseg,
	                 &cop_tx_unseg, &cop_rx_unseg,
	                 &fop_unseg, &farm_unseg,
	                 scid, max_frame_size,
	                 rx_max_fifo_size,
	                 0, 2, crc,
	                 TC_SEG_HDR_NOTPRESENT, bypass,
	                 ctrl, fop_slide_wnd,
	                 fop_init_st, fop_t1_init,
	                 fop_timeout_type, fop_tx_limit,
	                 farm_init_st, farm_wnd_width);         /*Prepare config structs*/
	has_ended = false;
	kill_timer = false;
	timer_running = false;
	req_for_timer = false;
	timer_expired = false;
	start_timer = false;
	end_process = false;
	pthread_t thread1;
	pthread_t thread2;
	pthread_t thread3;
	pthread_t thread5;
	pthread_t thread6;
	pthread_create(&thread1, NULL, transmitter, NULL);
	pthread_create(&thread2, NULL, receiver, NULL);
	pthread_create(&thread3, NULL, clcw_listener, NULL);
	pthread_create(&thread5, NULL, timer_handler, NULL);
	pthread_create(&thread6, NULL, process_timer, NULL);
	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);
	pthread_join(thread3, NULL);
	pthread_join(thread5, NULL);
	pthread_join(thread6, NULL);
	pthread_mutex_destroy(&lock);
	pthread_mutex_destroy(&tlock);
	uint8_t *p;
	uint16_t size = 0;
	for (int i = 0; i < rx_queues[1].inqueue; i++) {
		size = 0;
		p = (uint8_t *)get_element(&rx_queues[1], i);
		size = ((p[1] << 8) | p[2]);
		assert_int_equal(p[0], p[size - 1]);
	}
	assert_int_equal(packets_rxed + rx_queues[1].inqueue,
	                 total_packets);
	assert_int_equal(rx_queues[0].inqueue,
	                 6);
}





