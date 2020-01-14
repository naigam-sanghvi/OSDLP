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

#include "cop.h"
#include "osdlp.h"

int
prepare_fop(struct fop_config *fop, uint16_t slide_wnd, fop_state_t state,
            uint16_t t1_init, uint8_t timeout_type, uint8_t tx_lim)
{
	fop->nnr = 0;
	fop->slide_wnd = slide_wnd;
	fop->ss = 0;
	fop->state = state;
	fop->t1_init = t1_init;
	fop->tt = timeout_type;
	fop->tx_cnt = 0;
	fop->tx_lim = tx_lim;
	fop->vs = 0;
	fop->signal = IGNORE;
	return 0;
}

int
prepare_farm(struct farm_config *farm, farm_state_t state,
             uint8_t window_width)
{
	farm->state = state;
	farm->pw = window_width / 2;
	farm->nw = window_width / 2;
	farm->w = window_width;
	return 0;
}

int
buffer_release(struct tc_transfer_frame *tc_tf)
{
	reset_wait(tc_tf);
	return 0;
}

int
reset_wait(struct tc_transfer_frame *tc_tf)
{
	tc_tf->cop_cfg.farm.wait = 0;
	tc_tf->cop_cfg.farm.state = FARM_STATE_OPEN;
	return 0;
}

static farm_result_t
farm_e1(struct tc_transfer_frame *tc_tf)
{
	switch (tc_tf->cop_cfg.farm.state) {
		case FARM_STATE_OPEN:
			tc_tf->cop_cfg.farm.vr = (tc_tf->cop_cfg.farm.vr + 1) % 256;
			if (tc_tf->cop_cfg.farm.retransmit) {
				tc_tf->cop_cfg.farm.retransmit = 0;
			}
			return COP_ENQ;
		case FARM_STATE_WAIT:
			return COP_ERROR;
		case FARM_STATE_LOCKOUT:
			return COP_DISCARD;
		default:
			return COP_ERROR;
	}
}

static farm_result_t
farm_e2(struct tc_transfer_frame *tc_tf)
{
	switch (tc_tf->cop_cfg.farm.state) {
		case FARM_STATE_OPEN:
			tc_tf->cop_cfg.farm.retransmit = 1;
			tc_tf->cop_cfg.farm.wait = 1;
			tc_tf->cop_cfg.farm.state = FARM_STATE_WAIT;
			return COP_DISCARD;
		case FARM_STATE_WAIT:
			return COP_DISCARD;
		case FARM_STATE_LOCKOUT:
			return COP_DISCARD;
		default:
			return COP_ERROR;
	}
}

static farm_result_t
farm_e3(struct tc_transfer_frame *tc_tf)
{
	switch (tc_tf->cop_cfg.farm.state) {
		case FARM_STATE_OPEN:
			tc_tf->cop_cfg.farm.retransmit = 1;
			return COP_DISCARD;
		case FARM_STATE_WAIT:
			return COP_DISCARD;
		case FARM_STATE_LOCKOUT:
			return COP_DISCARD;
		default:
			return COP_ERROR;
	}
}

static farm_result_t
farm_e4(struct tc_transfer_frame *tc_tf)
{
	switch (tc_tf->cop_cfg.farm.state) {
		case FARM_STATE_OPEN:
			return COP_DISCARD;
		case FARM_STATE_WAIT:
			return COP_DISCARD;
		case FARM_STATE_LOCKOUT:
			return COP_DISCARD;
		default:
			return COP_ERROR;
	}
}

static farm_result_t
farm_e5(struct tc_transfer_frame *tc_tf)
{
	switch (tc_tf->cop_cfg.farm.state) {
		case FARM_STATE_OPEN:
			tc_tf->cop_cfg.farm.state = FARM_STATE_LOCKOUT;
			tc_tf->cop_cfg.farm.lockout = 1;
			return COP_DISCARD;
		case FARM_STATE_WAIT:
			tc_tf->cop_cfg.farm.state = FARM_STATE_LOCKOUT;
			tc_tf->cop_cfg.farm.lockout = 1;
			return COP_DISCARD;
		case FARM_STATE_LOCKOUT:
			return COP_DISCARD;
		default:
			return COP_ERROR;
	}
}

static farm_result_t
farm_e6(struct tc_transfer_frame *tc_tf)
{
	switch (tc_tf->cop_cfg.farm.state) {
		case FARM_STATE_OPEN:
			tc_tf->cop_cfg.farm.farmb_cnt = (tc_tf->cop_cfg.farm.farmb_cnt
			                                 + 1) % 4;
			return COP_PRIORITY_ENQ;
		case FARM_STATE_WAIT:
			tc_tf->cop_cfg.farm.farmb_cnt = (tc_tf->cop_cfg.farm.farmb_cnt
			                                 + 1) % 4;
			return COP_PRIORITY_ENQ;
		case FARM_STATE_LOCKOUT:
			tc_tf->cop_cfg.farm.farmb_cnt = (tc_tf->cop_cfg.farm.farmb_cnt
			                                 + 1) % 4;
			return COP_PRIORITY_ENQ;
		default:
			return COP_ERROR;
	}
}

static farm_result_t
farm_e7(struct tc_transfer_frame *tc_tf)
{
	switch (tc_tf->cop_cfg.farm.state) {
		case FARM_STATE_OPEN:
			tc_tf->cop_cfg.farm.farmb_cnt =
			        (tc_tf->cop_cfg.farm.farmb_cnt + 1) % 4;
			tc_tf->cop_cfg.farm.retransmit = 0;
			return COP_OK;
		case FARM_STATE_WAIT:
			tc_tf->cop_cfg.farm.farmb_cnt =
			        (tc_tf->cop_cfg.farm.farmb_cnt + 1) % 4;
			tc_tf->cop_cfg.farm.retransmit = 0;
			tc_tf->cop_cfg.farm.wait = 0;
			return COP_OK;
		case FARM_STATE_LOCKOUT:
			tc_tf->cop_cfg.farm.farmb_cnt =
			        (tc_tf->cop_cfg.farm.farmb_cnt + 1) % 4;
			tc_tf->cop_cfg.farm.retransmit = 0;
			tc_tf->cop_cfg.farm.wait = 0;
			tc_tf->cop_cfg.farm.lockout = 0;
			return COP_OK;
		default:
			return COP_ERROR;
	}
}

static farm_result_t
farm_e8(struct tc_transfer_frame *tc_tf)
{
	switch (tc_tf->cop_cfg.farm.state) {
		case FARM_STATE_OPEN:
			tc_tf->cop_cfg.farm.vr = tc_tf->frame_data.data[2];
			tc_tf->cop_cfg.farm.farmb_cnt =
			        (tc_tf->cop_cfg.farm.farmb_cnt + 1) % 4;
			tc_tf->cop_cfg.farm.retransmit = 0;
			return COP_OK;
		case FARM_STATE_WAIT:
			tc_tf->cop_cfg.farm.vr = tc_tf->frame_data.data[2];
			tc_tf->cop_cfg.farm.farmb_cnt =
			        (tc_tf->cop_cfg.farm.farmb_cnt + 1) % 4;
			tc_tf->cop_cfg.farm.retransmit = 0;
			tc_tf->cop_cfg.farm.wait = 0;
			return COP_OK;
		case FARM_STATE_LOCKOUT:
			tc_tf->cop_cfg.farm.farmb_cnt =
			        (tc_tf->cop_cfg.farm.farmb_cnt + 1) % 4;
			return COP_OK;
		default:
			return COP_ERROR;
	}
}

static farm_result_t
handle_farm_typea(struct tc_transfer_frame *tc_tf)
{
	farm_result_t ret;
	if (tc_tf->primary_hdr.frame_seq_num == tc_tf->cop_cfg.farm.vr) {
		/*E1*/
		if (!rx_queue_full(tc_tf->primary_hdr.vcid)) {
			ret = farm_e1(tc_tf);
			return ret;
		}
		/*E2*/
		else {
			ret = farm_e2(tc_tf);
			return ret;
		}
	}
	/*E3*/
	else if ((tc_tf->primary_hdr.frame_seq_num > tc_tf->cop_cfg.farm.vr)
	         && (tc_tf->primary_hdr.frame_seq_num
	             <= tc_tf->cop_cfg.farm.vr + tc_tf->cop_cfg.farm.pw - 1)) {
		ret = farm_e3(tc_tf);
		return ret;
	}
	/*E4*/
	else if ((tc_tf->primary_hdr.frame_seq_num < tc_tf->cop_cfg.farm.vr)
	         && (tc_tf->primary_hdr.frame_seq_num
	             >= tc_tf->cop_cfg.farm.vr - tc_tf->cop_cfg.farm.nw)) {
		ret = farm_e4(tc_tf);
		return ret;
	}
	/*E5*/
	else if ((tc_tf->primary_hdr.frame_seq_num
	          > tc_tf->cop_cfg.farm.vr + tc_tf->cop_cfg.farm.pw - 1)
	         && (tc_tf->primary_hdr.frame_seq_num
	             < tc_tf->cop_cfg.farm.vr - tc_tf->cop_cfg.farm.nw)) {
		ret = farm_e5(tc_tf);
		return ret;
	}
	/*Undefined*/
	else {
		return COP_ERROR;
	}
}



static farm_result_t
handle_farm_typeb_cmd(struct tc_transfer_frame *tc_tf)
{
	farm_result_t ret;
	/*E7*/
	if (tc_tf->frame_data.data[0] == 0) { /*UNLOCK*/
		ret = farm_e7(tc_tf);
		return ret;
	}
	/*E8*/
	else if (tc_tf->frame_data.data[0] == SETVR_BYTE1
	         && tc_tf->frame_data.data[1] == SETVR_BYTE2) { /*SET V(R)*/
		ret = farm_e8(tc_tf);
		return ret;
	} else { /* Undefined */
		return COP_ERROR;
	}
}

static farm_result_t
handle_farm_typeb_data(struct tc_transfer_frame *tc_tf)
{
	/*Ε6*/
	farm_result_t ret = farm_e6(tc_tf);
	return ret;
}

farm_result_t
farm_1(struct tc_transfer_frame *tc_tf)
{
	farm_result_t ret;
	if (tc_tf->primary_hdr.bypass == TYPE_A) {
		ret = handle_farm_typea(tc_tf);
		return ret;
	} else if (tc_tf->primary_hdr.bypass == TYPE_B) {

		if (tc_tf->primary_hdr.ctrl_cmd == TC_DATA) {
			ret = handle_farm_typeb_data(tc_tf);
			return ret;
		} else if (tc_tf->primary_hdr.ctrl_cmd == TC_COMMAND) {
			ret = handle_farm_typeb_cmd(tc_tf);
			return ret;
		}
	}
	/*E9*/
	else {
		return COP_DISCARD;
	}
	return COP_ERROR;
}

int
initialize_cop(struct tc_transfer_frame *tc_tf)
{
	int ret;
	ret = purge_wait_queue(tc_tf);
	if (ret) {
		return 1;
	}
	ret = purge_sent_queue(tc_tf);
	if (ret) {
		return 1;
	}
	tc_tf->cop_cfg.fop.tx_cnt = 1;
	tc_tf->cop_cfg.fop.ss = 0;
	return 0;
}

int
alert(struct tc_transfer_frame *tc_tf)
{
	int ret;
	ret = timer_cancel(tc_tf->primary_hdr.vcid);
	if (ret) {
		return 1;
	}
	ret = purge_wait_queue(tc_tf);
	if (ret) {
		return 1;
	}
	ret = purge_sent_queue(tc_tf);
	if (ret) {
		return 1;
	}
	// Return negative confirm directive
	tc_tf->cop_cfg.fop.signal = NEGATIVE_TX;
	return 0;
}

int
resume(struct tc_transfer_frame *tc_tf)
{
	int ret = timer_start(tc_tf->primary_hdr.vcid);
	if (ret) {
		return 1;
	}
	tc_tf->cop_cfg.fop.ss = 0;
	return 0;
}

notification_t
look_for_fdu(struct tc_transfer_frame *tc_tf)
{
	int ret;
	if (!sent_queue_empty(tc_tf->primary_hdr.vcid)) {
		struct queue_item item;
		ret = get_first_ad_rt_frame(&item, tc_tf->primary_hdr.vcid);
		if (!ret) {
			ret = reset_rt_frame(&item, tc_tf->primary_hdr.vcid);
			ret = tx_queue_enqueue(item.fdu, tc_tf->primary_hdr.vcid);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
		}
	}
	/*Perform checks in case window wraps around*/

	if (!wait_queue_empty(tc_tf->primary_hdr.vcid)) {
		if (tc_tf->cop_cfg.fop.nnr + tc_tf->cop_cfg.fop.slide_wnd >= 256) {
			if (tc_tf->cop_cfg.fop.vs
			    > (tc_tf->cop_cfg.fop.nnr + tc_tf->cop_cfg.fop.slide_wnd)
			    % 256) {
				ret = transmit_type_ad(tc_tf);
				if (ret) {
					tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
					return UNDEF_ERROR;
				}
				tc_tf->cop_cfg.fop.signal = ACCEPT_TX;
				return ACCEPT_TX;
			} else {
				tc_tf->cop_cfg.fop.signal = DELAY_RESP;
				return DELAY_RESP;
			}
		} else {
			if (tc_tf->cop_cfg.fop.vs
			    < tc_tf->cop_cfg.fop.nnr + tc_tf->cop_cfg.fop.slide_wnd) {
				ret = transmit_type_ad(tc_tf);
				if (ret) {
					tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
					return UNDEF_ERROR;
				}
				tc_tf->cop_cfg.fop.signal = ACCEPT_TX;
				return ACCEPT_TX;
			} else {
				tc_tf->cop_cfg.fop.signal = DELAY_RESP;
				return DELAY_RESP;
			}
		}
	} else {
		return IGNORE;
	}
	tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
	return UNDEF_ERROR; // Should not get here
}

notification_t
look_for_directive(struct tc_transfer_frame *tc_tf)
{
	notification_t notif;
	int ret;
	struct queue_item item;
	ret = sent_queue_head(&item,
	                      tc_tf->primary_hdr.vcid); //TODO Are we sure the first item is a BC type?
	if (item.rt_flag == RT_FLAG_ON && item.type == TYPE_B) {
		item.rt_flag = RT_FLAG_OFF;
		ret = tx_queue_enqueue(item.fdu, tc_tf->primary_hdr.vcid);
		if (ret) {
			notif = bc_reject(tc_tf);
			return notif;
		} else {
			notif = bc_accept(tc_tf);
			return notif;
		}
	}
	return ACCEPT_DIR;
}

int
transmit_type_ad(struct tc_transfer_frame *tc_tf)
{
	struct queue_item item;
	struct tc_transfer_frame wait_item;
	int ret = wait_queue_dequeue(&wait_item, tc_tf->primary_hdr.vcid);

	ret = tc_pack(tc_tf, tc_tf->mission.util.buffer, wait_item.frame_data.data,
	              wait_item.frame_data.data_len);
	if (ret) {
		return 1;
	}

	if (sent_queue_empty(tc_tf->primary_hdr.vcid)) {
		tc_tf->cop_cfg.fop.tx_cnt = 1;
	}
	timer_start(tc_tf->primary_hdr.vcid);

	item.type = TYPE_A;
	item.fdu = tc_tf->mission.util.buffer;
	item.rt_flag = RT_FLAG_OFF;
	item.seq_num = tc_tf->cop_cfg.fop.vs;
	ret = sent_queue_enqueue(&item, tc_tf->primary_hdr.vcid);
	tc_tf->cop_cfg.fop.vs = (tc_tf->cop_cfg.fop.vs + 1) % 256;

	if (ret) {
		return 1;
	}

	ret = tx_queue_enqueue(tc_tf->mission.util.buffer, tc_tf->primary_hdr.vcid);
	if (ret) {
		return 1;
	}
	return 0;
}

int
transmit_type_bc(struct tc_transfer_frame *tc_tf)
{
	struct queue_item item;
	int ret = tc_pack(tc_tf, tc_tf->mission.util.buffer, tc_tf->frame_data.data,
	                  tc_tf->frame_data.data_len);
	if (ret) {
		return 1;
	}
	tc_tf->cop_cfg.fop.tx_cnt = 1;
	timer_start(tc_tf->primary_hdr.vcid);
	item.type = TYPE_B;
	item.fdu = tc_tf->mission.util.buffer;
	item.rt_flag = RT_FLAG_OFF;
	item.seq_num = tc_tf->cop_cfg.fop.vs;
	ret = sent_queue_enqueue(&item, tc_tf->primary_hdr.vcid);
	if (ret) {
		return 1;
	}

	ret = tx_queue_enqueue(tc_tf->mission.util.buffer, tc_tf->primary_hdr.vcid);
	if (ret) {
		return 1;
	}
	return 0;
}

int
transmit_type_bd(struct tc_transfer_frame *tc_tf)
{
	int ret = tc_pack(tc_tf, tc_tf->mission.util.buffer, tc_tf->frame_data.data,
	                  tc_tf->frame_data.data_len);
	if (ret) {
		return 1;
	}
	//Set BD_Out not ready
	ret = tx_queue_enqueue(tc_tf->mission.util.buffer, tc_tf->primary_hdr.vcid);
	if (ret) {
		return 1;
	}
	return 0;
}

int
initiate_ad_retransmission(struct tc_transfer_frame *tc_tf)
{
	int ret;
	cancel_lower_ops();
	tc_tf->cop_cfg.fop.tx_cnt = (tc_tf->cop_cfg.fop.tx_cnt + 1) % 256;
	ret = timer_start(tc_tf->primary_hdr.vcid);
	if (ret) {
		return 1;
	}
	ret = mark_ad_as_rt(tc_tf->primary_hdr.vcid);
	if (ret) {
		return 1;
	}
	return 0;
}

int
initiate_bc_retransmission(struct tc_transfer_frame *tc_tf)
{
	int ret;
	cancel_lower_ops();
	tc_tf->cop_cfg.fop.tx_cnt = (tc_tf->cop_cfg.fop.tx_cnt + 1) % 256;
	ret = timer_start(tc_tf->primary_hdr.vcid);
	if (ret) {
		return 1;
	}
	ret = mark_bc_as_rt(tc_tf->primary_hdr.vcid);
	if (ret) {
		return 1;
	}
	return 0;
}

int
purge_sent_queue(struct tc_transfer_frame *tc_tf)
{
	int ret = sent_queue_clear(tc_tf->primary_hdr.vcid);
	if (ret) {
		return 1;
	}
	return 0;
}

int
purge_wait_queue(struct tc_transfer_frame *tc_tf)
{
	int ret = wait_queue_clear(tc_tf->primary_hdr.vcid);
	if (ret) {
		return 1;
	}
	return 0;
}

int
remove_acked_frames(struct tc_transfer_frame *tc_tf, uint8_t nr)
{
	struct queue_item item;
	int ret;
	uint16_t counter = 0;
	while (1) {
		ret = sent_queue_head(&item, tc_tf->primary_hdr.vcid);
		if (ret) {
			break;
		}
		if (item.seq_num != nr) {
			ret = sent_queue_dequeue(&item, tc_tf->primary_hdr.vcid);
			if (ret) {
				return 1;
			}
			tc_tf->cop_cfg.fop.nnr = nr;
			tc_tf->cop_cfg.fop.tx_cnt = 1;
		} else {
			break;
		}
		counter++;
		if (counter > tc_tf->cop_cfg.fop.slide_wnd) {
			return 1;
		}
	}
	return 0;
}

int
release_copy_of_bc(struct tc_transfer_frame *tc_tf)
{
	int ret;
	struct queue_item item;
	ret = sent_queue_head(&item, tc_tf->primary_hdr.vcid);
	/* Check that item in head is TYPE B*/
	if ((item.type == TYPE_B)) {
		ret = sent_queue_dequeue(&item, tc_tf->primary_hdr.vcid);
		if (ret) {
			return 1;
		}
	}
	return 1;
}

notification_t
fop_e1(struct tc_transfer_frame *tc_tf)
{
	notification_t ret;
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			// Ignore
			return IGNORE;
		case FOP_STATE_RT_NO_WAIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_SYNCH;
			return ALERT_SYNCH;
		case FOP_STATE_RT_WAIT:
			// Alert;
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_SYNCH;
			return ALERT_SYNCH;
		case FOP_STATE_INIT_NO_BC:
			tc_tf->cop_cfg.fop.state = FOP_STATE_ACTIVE;
			timer_cancel(tc_tf->primary_hdr.vcid);
			tc_tf->cop_cfg.fop.signal = POSITIVE_DIR;
			return POSITIVE_DIR;
		case FOP_STATE_INIT_BC:
			ret = release_copy_of_bc(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_ACTIVE;
			timer_cancel(tc_tf->primary_hdr.vcid);
			return IGNORE;
		case FOP_STATE_INIT:
			//Ignore
			return IGNORE;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
fop_e2(struct tc_transfer_frame *tc_tf,
       struct clcw_frame *clcw)
{
	notification_t notif;
	int ret;
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			ret = remove_acked_frames(tc_tf,
			                          clcw->report_value);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			timer_cancel(tc_tf->primary_hdr.vcid);
			notif = look_for_fdu(tc_tf);
			if (!(notif == IGNORE)) {
				tc_tf->cop_cfg.fop.signal = notif;
				return notif;
			} else {
				return tc_tf->cop_cfg.fop.signal;
			}
		case FOP_STATE_RT_NO_WAIT:
			ret = remove_acked_frames(tc_tf,
			                          clcw->report_value);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			timer_cancel(tc_tf->primary_hdr.vcid);
			notif = look_for_fdu(tc_tf);
			if (!(notif == IGNORE)) {
				tc_tf->cop_cfg.fop.signal = notif;
				return notif;
			} else {
				return tc_tf->cop_cfg.fop.signal;
			}
		case FOP_STATE_RT_WAIT:
			ret = remove_acked_frames(tc_tf,
			                          clcw->report_value);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			timer_cancel(tc_tf->primary_hdr.vcid);
			notif = look_for_fdu(tc_tf);
			if (!(notif == IGNORE)) {
				tc_tf->cop_cfg.fop.signal = notif;
				return notif;
			} else {
				return tc_tf->cop_cfg.fop.signal;
			}
		case FOP_STATE_INIT_NO_BC:
			// NA
			return NA;
		case FOP_STATE_INIT_BC:
			// NA
			return NA;
		case FOP_STATE_INIT:
			//Ignore
			return IGNORE;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
fop_e3(struct tc_transfer_frame *tc_tf,
       struct clcw_frame *clcw)
{
	int ret;
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_CLCW;
			return ALERT_CLCW;
		case FOP_STATE_RT_NO_WAIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_CLCW;
			return ALERT_CLCW;
		case FOP_STATE_RT_WAIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_CLCW;
			return ALERT_CLCW;
		case FOP_STATE_INIT_NO_BC:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_CLCW;
			return ALERT_CLCW;
		case FOP_STATE_INIT_BC:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_CLCW;
			return ALERT_CLCW;
		case FOP_STATE_INIT:
			// Ignore
			return IGNORE;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
fop_e4(struct tc_transfer_frame *tc_tf,
       struct clcw_frame *clcw)
{
	int ret;
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_SYNCH;
			return ALERT_SYNCH;
		case FOP_STATE_RT_NO_WAIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_SYNCH;
			return ALERT_SYNCH;
		case FOP_STATE_RT_WAIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_SYNCH;
			return ALERT_SYNCH;
		case FOP_STATE_INIT_NO_BC:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_SYNCH;
			return ALERT_SYNCH;
		case FOP_STATE_INIT_BC:
			// Ignore
			return IGNORE;
		case FOP_STATE_INIT:
			// Ignore
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			return IGNORE;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
fop_e5(struct tc_transfer_frame *tc_tf,
       struct clcw_frame *clcw)
{
	int ret;
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			// Ignore
			return IGNORE;
		case FOP_STATE_RT_NO_WAIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_SYNCH;
			return ALERT_SYNCH;
		case FOP_STATE_RT_WAIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_SYNCH;
			return ALERT_SYNCH;
		case FOP_STATE_INIT_NO_BC:
			// NA
			return NA;
		case FOP_STATE_INIT_BC:
			// NA
			return NA;
		case FOP_STATE_INIT:
			//Ignore
			return IGNORE;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
fop_e6(struct tc_transfer_frame *tc_tf,
       struct clcw_frame *clcw)
{
	notification_t notif;
	int ret;
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			ret = remove_acked_frames(tc_tf,
			                          clcw->report_value);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			notif = look_for_fdu(tc_tf);
			if (!(notif == IGNORE)) {
				tc_tf->cop_cfg.fop.signal = notif;
				return notif;
			} else {
				return tc_tf->cop_cfg.fop.signal;
			}
		case FOP_STATE_RT_NO_WAIT:
			ret = remove_acked_frames(tc_tf,
			                          clcw->report_value);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_ACTIVE;
			notif = look_for_fdu(tc_tf);
			if (!(notif == IGNORE)) {
				tc_tf->cop_cfg.fop.signal = notif;
				return notif;
			} else {
				return tc_tf->cop_cfg.fop.signal;
			}
		case FOP_STATE_RT_WAIT:
			ret = remove_acked_frames(tc_tf,
			                          clcw->report_value);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			notif = look_for_fdu(tc_tf);
			tc_tf->cop_cfg.fop.state = FOP_STATE_ACTIVE;
			if (!(notif == IGNORE)) {
				tc_tf->cop_cfg.fop.signal = notif;
				return notif;
			} else {
				return tc_tf->cop_cfg.fop.signal;
			}
		case FOP_STATE_INIT_NO_BC:
			// NA
			return NA;
		case FOP_STATE_INIT_BC:
			// NA
			return NA;
		case FOP_STATE_INIT:
			//Ignore
			return IGNORE;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
fop_e7(struct tc_transfer_frame *tc_tf,
       struct clcw_frame *clcw)
{
	int ret;
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_CLCW;
			return ALERT_CLCW;
		case FOP_STATE_RT_NO_WAIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_CLCW;
			return ALERT_CLCW;
		case FOP_STATE_RT_WAIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_CLCW;
			return ALERT_CLCW;
		case FOP_STATE_INIT_NO_BC:
			// NA
			return NA;
		case FOP_STATE_INIT_BC:
			// NA
			return NA;
		case FOP_STATE_INIT:
			// Ignore
			return IGNORE;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
fop_e101(struct tc_transfer_frame *tc_tf,
         struct clcw_frame *clcw)
{
	int ret;
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			ret = remove_acked_frames(tc_tf,
			                          clcw->report_value);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_LIMIT;
			return ALERT_LIMIT;
		case FOP_STATE_RT_NO_WAIT:
			ret = remove_acked_frames(tc_tf,
			                          clcw->report_value);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_LIMIT;
			return ALERT_LIMIT;
		case FOP_STATE_RT_WAIT:
			remove_acked_frames(tc_tf, clcw->report_value);
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_LIMIT;
			return ALERT_LIMIT;
		case FOP_STATE_INIT_NO_BC:
			// NA
			return NA;
		case FOP_STATE_INIT_BC:
			// NA
			return 0;
		case FOP_STATE_INIT:
			//Ignore
			return IGNORE;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
fop_e102(struct tc_transfer_frame *tc_tf,
         struct clcw_frame *clcw)
{
	int ret;
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_LIMIT;
			return ALERT_LIMIT;
		case FOP_STATE_RT_NO_WAIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_LIMIT;
			return ALERT_LIMIT;
		case FOP_STATE_RT_WAIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_LIMIT;
			return ALERT_LIMIT;
		case FOP_STATE_INIT_NO_BC:
			// NA
			return NA;
		case FOP_STATE_INIT_BC:
			// NA
			return NA;
		case FOP_STATE_INIT:
			//Ignore
			return IGNORE;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
fop_e8(struct tc_transfer_frame *tc_tf,
       struct clcw_frame *clcw)
{
	notification_t notif;
	int ret;
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			ret = remove_acked_frames(tc_tf,
			                          clcw->report_value);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			ret = initiate_ad_retransmission(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			notif = look_for_fdu(tc_tf);
			tc_tf->cop_cfg.fop.state = FOP_STATE_RT_NO_WAIT;
			if (!(notif == IGNORE)) {
				tc_tf->cop_cfg.fop.signal = notif;
				return notif;
			} else {
				return tc_tf->cop_cfg.fop.signal;
			}
		case FOP_STATE_RT_NO_WAIT:
			ret = remove_acked_frames(tc_tf,
			                          clcw->report_value);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			ret = initiate_ad_retransmission(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			notif = look_for_fdu(tc_tf);
			if (!(notif == IGNORE)) {
				tc_tf->cop_cfg.fop.signal = notif;
				return notif;
			} else {
				return tc_tf->cop_cfg.fop.signal;
			}
		case FOP_STATE_RT_WAIT:
			ret = remove_acked_frames(tc_tf,
			                          clcw->report_value);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			ret = initiate_ad_retransmission(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			notif = look_for_fdu(tc_tf);
			tc_tf->cop_cfg.fop.state = FOP_STATE_RT_NO_WAIT;
			if (!(notif == IGNORE)) {
				tc_tf->cop_cfg.fop.signal = notif;
				return notif;
			} else {
				return tc_tf->cop_cfg.fop.signal;
			}
		case FOP_STATE_INIT_NO_BC:
			// NA
			return NA;
		case FOP_STATE_INIT_BC:
			// NA
			return NA;
		case FOP_STATE_INIT:
			//Ignore
			return IGNORE;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
fop_e9(struct tc_transfer_frame *tc_tf,
       struct clcw_frame *clcw)
{
	int ret;
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			ret = remove_acked_frames(tc_tf,
			                          clcw->report_value);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_RT_WAIT;
			return IGNORE;
		case FOP_STATE_RT_NO_WAIT:
			ret = remove_acked_frames(tc_tf,
			                          clcw->report_value);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_RT_WAIT;
			return IGNORE;
		case FOP_STATE_RT_WAIT:
			ret = remove_acked_frames(tc_tf,
			                          clcw->report_value);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_RT_WAIT;
			return IGNORE;
		case FOP_STATE_INIT_NO_BC:
			// NA
			return NA;
		case FOP_STATE_INIT_BC:
			// NA
			return NA;
		case FOP_STATE_INIT:
			//Ignore
			return IGNORE;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
fop_e10(struct tc_transfer_frame *tc_tf,
        struct clcw_frame *clcw)
{
	notification_t notif;
	int ret;
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			ret = initiate_ad_retransmission(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			notif = look_for_fdu(tc_tf);
			tc_tf->cop_cfg.fop.state =
			        FOP_STATE_RT_NO_WAIT;
			if (!(notif == IGNORE)) {
				tc_tf->cop_cfg.fop.signal = notif;
				return notif;
			} else {
				return tc_tf->cop_cfg.fop.signal;
			}
		case FOP_STATE_RT_NO_WAIT:
			// Ignore
			return IGNORE;
		case FOP_STATE_RT_WAIT:
			ret = initiate_ad_retransmission(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			notif = look_for_fdu(tc_tf);
			tc_tf->cop_cfg.fop.state =
			        FOP_STATE_RT_NO_WAIT;
			if (!(notif == IGNORE)) {
				tc_tf->cop_cfg.fop.signal = notif;
				return notif;
			} else {
				return tc_tf->cop_cfg.fop.signal;
			}
		case FOP_STATE_INIT_NO_BC:
			// NA
			return NA;
		case FOP_STATE_INIT_BC:
			// NA
			return NA;
		case FOP_STATE_INIT:
			//Ignore
			return IGNORE;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
fop_e11(struct tc_transfer_frame *tc_tf,
        struct clcw_frame *clcw)
{
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			// Ignore
			tc_tf->cop_cfg.fop.state =
			        FOP_STATE_RT_WAIT;
			return IGNORE;
		case FOP_STATE_RT_NO_WAIT:
			// Ignore
			tc_tf->cop_cfg.fop.state =
			        FOP_STATE_RT_WAIT;
			return IGNORE;
		case FOP_STATE_RT_WAIT:
			// Ignore
			return IGNORE;
		case FOP_STATE_INIT_NO_BC:
			// NA
			return IGNORE;
		case FOP_STATE_INIT_BC:
			// NA
			return IGNORE;
		case FOP_STATE_INIT:
			//Ignore
			return IGNORE;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
fop_e12(struct tc_transfer_frame *tc_tf,
        struct clcw_frame *clcw)
{
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			// ignore
			tc_tf->cop_cfg.fop.state =
			        FOP_STATE_RT_NO_WAIT;
			return IGNORE;
		case FOP_STATE_RT_NO_WAIT:
			// Ignore
			return IGNORE;
		case FOP_STATE_RT_WAIT:
			// Ignore
			tc_tf->cop_cfg.fop.state =
			        FOP_STATE_RT_NO_WAIT;
			return IGNORE;
		case FOP_STATE_INIT_NO_BC:
			// NA
			return NA;
		case FOP_STATE_INIT_BC:
			// NA
			return NA;
		case FOP_STATE_INIT:
			//Ignore
			return IGNORE;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
fop_e103(struct tc_transfer_frame *tc_tf,
         struct clcw_frame *clcw)
{
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			// Ignore
			tc_tf->cop_cfg.fop.state =
			        FOP_STATE_RT_WAIT;
			return IGNORE;
		case FOP_STATE_RT_NO_WAIT:
			// Ignore
			tc_tf->cop_cfg.fop.state =
			        FOP_STATE_RT_WAIT;
			return IGNORE;
		case FOP_STATE_RT_WAIT:
			// Ignore
			return IGNORE;
		case FOP_STATE_INIT_NO_BC:
			// NA
			return NA;
		case FOP_STATE_INIT_BC:
			// NA
			return NA;
		case FOP_STATE_INIT:
			//Ignore
			return IGNORE;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
fop_e13(struct tc_transfer_frame *tc_tf,
        struct clcw_frame *clcw)
{
	int ret;
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_NNR;
			return ALERT_NNR;
		case FOP_STATE_RT_NO_WAIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_NNR;
			return ALERT_NNR;
		case FOP_STATE_RT_WAIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_NNR;
			return ALERT_NNR;
		case FOP_STATE_INIT_NO_BC:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_NNR;
			return ALERT_NNR;
		case FOP_STATE_INIT_BC:
			// Ignore
			return IGNORE;
		case FOP_STATE_INIT:
			//Ignore
			return IGNORE;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
fop_e14(struct tc_transfer_frame *tc_tf,
        struct clcw_frame *clcw)
{
	int ret;
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_LOCKOUT;
			return ALERT_LOCKOUT;
		case FOP_STATE_RT_NO_WAIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_LOCKOUT;
			return ALERT_LOCKOUT;
		case FOP_STATE_RT_WAIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_LOCKOUT;
			return ALERT_LOCKOUT;
		case FOP_STATE_INIT_NO_BC:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_LOCKOUT;
			return ALERT_LOCKOUT;
		case FOP_STATE_INIT_BC:
			// Ignore
			return IGNORE;
		case FOP_STATE_INIT:
			//Ignore
			return IGNORE;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
fop_e16(struct tc_transfer_frame *tc_tf)
{
	notification_t notif;
	int ret;
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			ret = initiate_ad_retransmission(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			notif = look_for_fdu(tc_tf);
			if (!(notif == IGNORE)) {
				tc_tf->cop_cfg.fop.signal = notif;
				return notif;
			} else {
				return tc_tf->cop_cfg.fop.signal;
			}
		case FOP_STATE_RT_NO_WAIT:
			ret = initiate_ad_retransmission(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			notif = look_for_fdu(tc_tf);
			if (!(notif == IGNORE)) {
				tc_tf->cop_cfg.fop.signal = notif;
				return notif;
			} else {
				return tc_tf->cop_cfg.fop.signal;
			}
		case FOP_STATE_RT_WAIT:
			// Ignore
			return IGNORE;
		case FOP_STATE_INIT_NO_BC:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_T1;
			return ALERT_T1;
		case FOP_STATE_INIT_BC:
			ret = initiate_bc_retransmission(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			notif = look_for_directive(tc_tf);
			if (!(notif == IGNORE)) {
				tc_tf->cop_cfg.fop.signal = notif;
				return notif;
			} else {
				return tc_tf->cop_cfg.fop.signal;
			}
		case FOP_STATE_INIT:
			// NA
			return NA;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
fop_e104(struct tc_transfer_frame *tc_tf)
{
	notification_t notif;
	int ret;
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			ret = initiate_ad_retransmission(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			notif = look_for_fdu(tc_tf);
			if (!(notif == IGNORE)) {
				tc_tf->cop_cfg.fop.signal = notif;
				return notif;
			} else {
				return tc_tf->cop_cfg.fop.signal;
			}
		case FOP_STATE_RT_NO_WAIT:
			ret = initiate_ad_retransmission(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			notif = look_for_fdu(tc_tf);
			if (!(notif == IGNORE)) {
				tc_tf->cop_cfg.fop.signal = notif;
				return notif;
			} else {
				return tc_tf->cop_cfg.fop.signal;
			}
		case FOP_STATE_RT_WAIT:
			// Ignore
			return IGNORE;
		case FOP_STATE_INIT_NO_BC:
			// Suspend
			tc_tf->cop_cfg.fop.ss = 4;
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			return SUSPEND;
		case FOP_STATE_INIT_BC:
			ret = initiate_bc_retransmission(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			notif = look_for_directive(tc_tf);
			if (!(notif == IGNORE)) {
				tc_tf->cop_cfg.fop.signal = notif;
				return notif;
			} else {
				return tc_tf->cop_cfg.fop.signal;
			}
		case FOP_STATE_INIT:
			// NA
			return NA;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
fop_e17(struct tc_transfer_frame *tc_tf)
{
	int ret;
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_T1;
			return ALERT_T1;
		case FOP_STATE_RT_NO_WAIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_T1;
			return ALERT_T1;
		case FOP_STATE_RT_WAIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_T1;
			return ALERT_T1;
		case FOP_STATE_INIT_NO_BC:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_T1;
			return ALERT_T1;
		case FOP_STATE_INIT_BC:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_T1;
			return ALERT_T1;
		case FOP_STATE_INIT:
			// NA
			return NA;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
fop_e18(struct tc_transfer_frame *tc_tf)
{
	int ret;
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			// Suspend
			tc_tf->cop_cfg.fop.ss = 1;
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			return SUSPEND;
		case FOP_STATE_RT_NO_WAIT:
			// Suspend
			tc_tf->cop_cfg.fop.ss = 2;
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			return SUSPEND;
		case FOP_STATE_RT_WAIT:
			// Suspend
			tc_tf->cop_cfg.fop.ss = 3;
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			return SUSPEND;
		case FOP_STATE_INIT_NO_BC:
			// Suspend
			tc_tf->cop_cfg.fop.ss = 4;
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			return SUSPEND;
		case FOP_STATE_INIT_BC:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_T1;
			return ALERT_T1;
		case FOP_STATE_INIT:
			// NA
			return NA;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
fop_e19(struct tc_transfer_frame *tc_tf)
{
	notification_t notif;
	int ret;
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			ret = wait_queue_enqueue(tc_tf, tc_tf->primary_hdr.vcid);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			notif = look_for_fdu(tc_tf);
			if (!(notif == IGNORE)) {
				tc_tf->cop_cfg.fop.signal = notif;
				return notif;
			} else {
				return tc_tf->cop_cfg.fop.signal;
			}
		case FOP_STATE_RT_NO_WAIT:
			ret = wait_queue_enqueue(tc_tf, tc_tf->primary_hdr.vcid);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			notif = look_for_fdu(tc_tf);
			if (!(notif == IGNORE)) {
				tc_tf->cop_cfg.fop.signal = notif;
				return notif;
			} else {
				return tc_tf->cop_cfg.fop.signal;
			}
		case FOP_STATE_RT_WAIT:
			ret = wait_queue_enqueue(tc_tf, tc_tf->primary_hdr.vcid);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.signal = DELAY_RESP;
			return DELAY_RESP;
		case FOP_STATE_INIT_NO_BC:
			// Reject
			tc_tf->cop_cfg.fop.signal = REJECT_TX;
			return REJECT_TX;
		case FOP_STATE_INIT_BC:
			// Reject
			tc_tf->cop_cfg.fop.signal = REJECT_TX;
			return REJECT_TX;
		case FOP_STATE_INIT:
			// Reject
			tc_tf->cop_cfg.fop.signal = REJECT_TX;
			return REJECT_TX;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
fop_e20(struct tc_transfer_frame *tc_tf)
{
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			// Reject
			tc_tf->cop_cfg.fop.signal = REJECT_TX;
			return REJECT_TX;
		case FOP_STATE_RT_NO_WAIT:
			// Reject
			tc_tf->cop_cfg.fop.signal = REJECT_TX;
			return REJECT_TX;
		case FOP_STATE_RT_WAIT:
			// Reject
			tc_tf->cop_cfg.fop.signal = REJECT_TX;
			return REJECT_TX;
		case FOP_STATE_INIT_NO_BC:
			// Reject
			tc_tf->cop_cfg.fop.signal = REJECT_TX;
			return REJECT_TX;
		case FOP_STATE_INIT_BC:
			// Reject
			tc_tf->cop_cfg.fop.signal = REJECT_TX;
			return REJECT_TX;
		case FOP_STATE_INIT:
			// Reject
			tc_tf->cop_cfg.fop.signal = REJECT_TX;
			return REJECT_TX;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
fop_e21(struct tc_transfer_frame *tc_tf)
{
	int ret;
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			ret = transmit_type_bd(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = REJECT_TX;
				return REJECT_TX;
			} else {
				tc_tf->cop_cfg.fop.signal = ACCEPT_TX;
				return ACCEPT_TX;
			}
		case FOP_STATE_RT_NO_WAIT:
			ret = transmit_type_bd(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = REJECT_TX;
				return REJECT_TX;
			} else {
				tc_tf->cop_cfg.fop.signal = ACCEPT_TX;
				return ACCEPT_TX;
			}
		case FOP_STATE_RT_WAIT:
			ret = transmit_type_bd(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = REJECT_TX;
				return REJECT_TX;
			} else {
				tc_tf->cop_cfg.fop.signal = ACCEPT_TX;
				return ACCEPT_TX;
			}
		case FOP_STATE_INIT_NO_BC:
			ret = transmit_type_bd(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = REJECT_TX;
				return REJECT_TX;
			} else {
				tc_tf->cop_cfg.fop.signal = ACCEPT_TX;
				return ACCEPT_TX;
			}
		case FOP_STATE_INIT_BC:
			ret = transmit_type_bd(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = REJECT_TX;
				return REJECT_TX;
			} else {
				tc_tf->cop_cfg.fop.signal = ACCEPT_TX;
				return ACCEPT_TX;
			}
		case FOP_STATE_INIT:
			ret = transmit_type_bd(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = REJECT_TX;
				return REJECT_TX;
			} else {
				tc_tf->cop_cfg.fop.signal = ACCEPT_TX;
				return ACCEPT_TX;
			}
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
handle_clcw(struct tc_transfer_frame *tc_tf,
            struct clcw_frame *clcw)
{
	notification_t notif;
	if (clcw->lockout == CLCW_NO_LOCKOUT) {
		if (tc_tf->cop_cfg.fop.vs == clcw->report_value) {
			if (clcw->rt == CLCW_NO_RETRANSMIT) {
				if (clcw->wait == CLCW_DO_NOT_WAIT) {
					if (clcw->report_value == tc_tf->cop_cfg.fop.nnr) {
						/*E1*/
						notif = fop_e1(tc_tf);
						return notif;
					} else {
						/* E2 */
						notif = fop_e2(tc_tf, clcw);
						return notif;
					}
				} else { // Wait flag  = 1
					/*E3*/
					notif = fop_e3(tc_tf, clcw);
					return notif;
				}
			} else { // Retransmit flag = 1
				/*E4*/
				notif = fop_e4(tc_tf, clcw);
				return notif;
			}
		}
		/* Make sure the N(R) falls within bounds even if the counter has wrapped around */
		else if (((tc_tf->cop_cfg.fop.vs > clcw->report_value)
		          && (clcw->report_value >= tc_tf->cop_cfg.fop.nnr))
		         || ((tc_tf->cop_cfg.fop.vs < clcw->report_value)
		             && (clcw->report_value > tc_tf->cop_cfg.fop.nnr))
		         || ((tc_tf->cop_cfg.fop.vs > clcw->report_value)
		             && (clcw->report_value < tc_tf->cop_cfg.fop.nnr))) {
			if (clcw->rt == CLCW_NO_RETRANSMIT) {
				if (clcw->wait == CLCW_DO_NOT_WAIT) {
					if (clcw->report_value == tc_tf->cop_cfg.fop.nnr) {
						/*E5*/
						notif = fop_e5(tc_tf, clcw);
						return notif;
					} else {
						/* E6 */
						notif = fop_e6(tc_tf, clcw);
						return notif;
					}
				} else { // Wait flag  = 1
					/*E7*/
					notif = fop_e7(tc_tf, clcw);
					return notif;
				}
			} else { // Retransmit flag = 1
				if (tc_tf->cop_cfg.fop.tx_lim == 1) {
					/*E101*/
					if (clcw->report_value != tc_tf->cop_cfg.fop.nnr) {
						notif = fop_e101(tc_tf, clcw);
						return notif;
					}
					/*E102*/
					else {
						notif = fop_e102(tc_tf, clcw);
						return notif;
					}
				} else if (tc_tf->cop_cfg.fop.tx_lim > 1) {
					if (clcw->report_value != tc_tf->cop_cfg.fop.nnr) {
						/*E8*/
						if (clcw->wait == CLCW_DO_NOT_WAIT) {
							notif = fop_e8(tc_tf, clcw);
							return notif;
						}
						/*E9*/
						else { //Wait flag = 1
							notif = fop_e9(tc_tf, clcw);
							return notif;
						}
					} else { //N(R == NN{R}
						if (tc_tf->cop_cfg.fop.tx_cnt
						    < tc_tf->cop_cfg.fop.tx_lim) {
							/*E10*/
							if (clcw->wait == CLCW_DO_NOT_WAIT) {
								notif = fop_e10(tc_tf, clcw);
								return notif;
							}
							/*E11*/
							else { //Wait flag = 1
								notif = fop_e11(tc_tf, clcw);
								return notif;
							}
						} else { // Tx cnt >= Tx limit
							/*E12*/
							if (clcw->wait == CLCW_DO_NOT_WAIT) {
								notif = fop_e12(tc_tf, clcw);
								return notif;
							}
							/*E103*/
							else { //Wait flag = 1
								notif = fop_e103(tc_tf, clcw);
								return notif;
							}
						}
					}
				} else {
					tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
					return UNDEF_ERROR;
				}
			}
		} else { // N(R) not within bounds
			/*E13*/
			notif = fop_e13(tc_tf, clcw);
			return notif;
		}
	} else { // Lockout = 1
		/*E14*/
		notif = fop_e14(tc_tf, clcw);
		return notif;
	}
}

notification_t
handle_timer_expired(struct tc_transfer_frame *tc_tf)
{
	notification_t notif;
	if (tc_tf->cop_cfg.fop.tx_cnt < tc_tf->cop_cfg.fop.tx_lim) {
		if (tc_tf->cop_cfg.fop.tt == 0) {
			/*E16*/
			notif = fop_e16(tc_tf);
			return notif;
		} else if (tc_tf->cop_cfg.fop.tt == 1) {
			/*E104*/
			notif = fop_e104(tc_tf);
			return notif;
		} else {
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
		}
	} else { // tx_cnt >= tx_lim
		if (tc_tf->cop_cfg.fop.tt == 0) {
			/*E17*/
			notif = fop_e17(tc_tf);
			return notif;
		} else if (tc_tf->cop_cfg.fop.tt == 1) {
			/*E18*/
			notif = fop_e18(tc_tf);
			return notif;
		} else {
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
		}
	}
}

notification_t
req_transfer_fdu(struct tc_transfer_frame *tc_tf)
{
	notification_t notif;
	if (tc_tf->primary_hdr.bypass == TYPE_A) {
		if (wait_queue_empty(tc_tf->primary_hdr.vcid)) {
			/*E19*/
			notif = fop_e19(tc_tf);
			return notif;
		} else { // Wait queue not empty
			/*E20*/
			notif = fop_e20(tc_tf);
			return notif;
		}
	} else if (tc_tf->primary_hdr.bypass == TYPE_B
	           && tc_tf->primary_hdr.ctrl_cmd == TC_DATA) { // Type BD
		/*E21*/
		notif = fop_e21(tc_tf);
		return notif;
	} else {
		tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
		return UNDEF_ERROR;
	}
}

notification_t
initiate_no_clcw(struct tc_transfer_frame *tc_tf)
{
	int ret;
	/*E23*/
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			// Reject
			tc_tf->cop_cfg.fop.signal = REJECT_TX;
			return REJECT_TX;
		case FOP_STATE_RT_NO_WAIT:
			// Reject
			tc_tf->cop_cfg.fop.signal = REJECT_TX;
			return REJECT_TX;
		case FOP_STATE_RT_WAIT:
			// Reject
			tc_tf->cop_cfg.fop.signal = REJECT_TX;
			return REJECT_TX;
		case FOP_STATE_INIT_NO_BC:
			// Reject
			tc_tf->cop_cfg.fop.signal = REJECT_TX;
			return REJECT_TX;
		case FOP_STATE_INIT_BC:
			// Reject
			tc_tf->cop_cfg.fop.signal = REJECT_TX;
			return REJECT_TX;
		case FOP_STATE_INIT:
			// accept();
			ret = initialize_cop(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			// confirm();
			tc_tf->cop_cfg.fop.state = FOP_STATE_ACTIVE;
			tc_tf->cop_cfg.fop.signal = POSITIVE_TX;
			return POSITIVE_TX;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
initiate_with_clcw(struct tc_transfer_frame *tc_tf)
{
	int ret;
	/*E24*/
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			// Reject
			tc_tf->cop_cfg.fop.signal = REJECT_TX;
			return REJECT_TX;
		case FOP_STATE_RT_NO_WAIT:
			// Reject
			tc_tf->cop_cfg.fop.signal = REJECT_TX;
			return REJECT_TX;
		case FOP_STATE_RT_WAIT:
			// Reject
			tc_tf->cop_cfg.fop.signal = REJECT_TX;
			return REJECT_TX;
		case FOP_STATE_INIT_NO_BC:
			// Reject
			tc_tf->cop_cfg.fop.signal = REJECT_TX;
			return REJECT_TX;
		case FOP_STATE_INIT_BC:
			// Reject
			tc_tf->cop_cfg.fop.signal = REJECT_TX;
			return REJECT_TX;
		case FOP_STATE_INIT:
			ret = initialize_cop(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			timer_start(tc_tf->primary_hdr.vcid);
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT_NO_BC;
			tc_tf->cop_cfg.fop.signal = ACCEPT_TX;
			return ACCEPT_TX;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
initiate_with_unlock(struct tc_transfer_frame *tc_tf)
{
	int ret;
	/*E25*/
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			// Reject
			return REJECT_DIR;
		case FOP_STATE_RT_NO_WAIT:
			// Reject
			return REJECT_DIR;
		case FOP_STATE_RT_WAIT:
			// Reject
			return REJECT_DIR;
		case FOP_STATE_INIT_NO_BC:
			// Reject
			return REJECT_DIR;
		case FOP_STATE_INIT_BC:
			// Reject
			return REJECT_DIR;
		case FOP_STATE_INIT:
			ret = initialize_cop(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			ret = prepare_typeb_unlock(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			ret = transmit_type_bc(tc_tf); // Unlock
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT_BC;
			tc_tf->cop_cfg.fop.signal = ACCEPT_DIR;
			return ACCEPT_DIR;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
initiate_with_setvr(struct tc_transfer_frame *tc_tf,
                    uint8_t new_vr)
{
	int ret;
	/*E27*/
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			// Reject
			return REJECT_DIR;
		case FOP_STATE_RT_NO_WAIT:
			// Reject
			return REJECT_DIR;
		case FOP_STATE_RT_WAIT:
			// Reject
			return REJECT_DIR;
		case FOP_STATE_INIT_NO_BC:
			// Reject
			return REJECT_DIR;
		case FOP_STATE_INIT_BC:
			// Reject
			return REJECT_DIR;
		case FOP_STATE_INIT:
			ret = initialize_cop(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.vs = new_vr;
			tc_tf->cop_cfg.fop.nnr = new_vr;
			ret = prepare_typeb_setvr(tc_tf, new_vr);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			ret = transmit_type_bc(tc_tf); // Set V(R)
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT_BC;
			tc_tf->cop_cfg.fop.signal = ACCEPT_DIR;
			return ACCEPT_DIR;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
terminate_ad(struct tc_transfer_frame *tc_tf)
{
	int ret;
	/*E29*/
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			//confirm();
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = POSITIVE_DIR;
			return POSITIVE_DIR;
		case FOP_STATE_RT_NO_WAIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			//confirm();
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = POSITIVE_DIR;
			return POSITIVE_DIR;
		case FOP_STATE_RT_WAIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			//confirm();
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = POSITIVE_DIR;
			return POSITIVE_DIR;
		case FOP_STATE_INIT_NO_BC:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			//confirm();
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = POSITIVE_DIR;
			return POSITIVE_DIR;
		case FOP_STATE_INIT_BC:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			//confirm();
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = POSITIVE_DIR;
			return POSITIVE_DIR;
		case FOP_STATE_INIT:
			//confirm();
			tc_tf->cop_cfg.fop.signal = POSITIVE_DIR;
			return POSITIVE_DIR;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
resume_ad(struct tc_transfer_frame *tc_tf)
{
	int ret;
	/*E30*/
	if (tc_tf->cop_cfg.fop.ss == 0) {
		switch (tc_tf->cop_cfg.fop.state) {
			case FOP_STATE_ACTIVE:
				// Reject
				return REJECT_DIR;
			case FOP_STATE_RT_NO_WAIT:
				// Reject
				return REJECT_DIR;
			case FOP_STATE_RT_WAIT:
				// Reject
				return REJECT_DIR;
			case FOP_STATE_INIT_NO_BC:
				// Reject
				return REJECT_DIR;
			case FOP_STATE_INIT_BC:
				// Reject
				return REJECT_DIR;
			case FOP_STATE_INIT:
				// Reject
				return REJECT_DIR;
			default:
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
		}
	} else if (tc_tf->cop_cfg.fop.ss == 1) {
		switch (tc_tf->cop_cfg.fop.state) {
			case FOP_STATE_ACTIVE:
				// NA
				return NA;
			case FOP_STATE_RT_NO_WAIT:
				// NA
				return NA;
			case FOP_STATE_RT_WAIT:
				// NA
				return NA;
			case FOP_STATE_INIT_NO_BC:
				// NA
				return NA;
			case FOP_STATE_INIT_BC:
				// NA
				return NA;
			case FOP_STATE_INIT:
				ret = resume(tc_tf);
				if (ret) {
					tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
					return UNDEF_ERROR;
				}
				//confirm();
				tc_tf->cop_cfg.fop.state = FOP_STATE_ACTIVE;
				tc_tf->cop_cfg.fop.signal = POSITIVE_DIR;
				return POSITIVE_DIR;
			default:
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
		}
	} else if (tc_tf->cop_cfg.fop.ss == 2) {
		/*E32*/
		switch (tc_tf->cop_cfg.fop.state) {
			case FOP_STATE_ACTIVE:
				// NA
				return NA;
			case FOP_STATE_RT_NO_WAIT:
				// NA
				return NA;
			case FOP_STATE_RT_WAIT:
				// NA
				return NA;
			case FOP_STATE_INIT_NO_BC:
				// NA
				return NA;
			case FOP_STATE_INIT_BC:
				// NA
				return NA;
			case FOP_STATE_INIT:
				ret = resume(tc_tf);
				if (ret) {
					tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
					return UNDEF_ERROR;
				}
				//confirm();
				tc_tf->cop_cfg.fop.state = FOP_STATE_RT_NO_WAIT;
				tc_tf->cop_cfg.fop.signal = POSITIVE_DIR;
				return POSITIVE_DIR;
			default:
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
		}
	} else if (tc_tf->cop_cfg.fop.ss == 3) {
		/*E33*/
		switch (tc_tf->cop_cfg.fop.state) {
			case FOP_STATE_ACTIVE:
				// NA
				return NA;
			case FOP_STATE_RT_NO_WAIT:
				// NA
				return NA;
			case FOP_STATE_RT_WAIT:
				// NA
				return NA;
			case FOP_STATE_INIT_NO_BC:
				// NA
				return NA;
			case FOP_STATE_INIT_BC:
				// NA
				return NA;
			case FOP_STATE_INIT:
				ret = resume(tc_tf);
				if (ret) {
					tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
					return UNDEF_ERROR;
				}
				//confirm();
				tc_tf->cop_cfg.fop.state = FOP_STATE_RT_WAIT;
				tc_tf->cop_cfg.fop.signal = POSITIVE_DIR;
				return POSITIVE_DIR;
			default:
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
		}
	} else if (tc_tf->cop_cfg.fop.ss == 4) {
		/*E34*/
		switch (tc_tf->cop_cfg.fop.state) {
			case FOP_STATE_ACTIVE:
				// NA
				return NA;
			case FOP_STATE_RT_NO_WAIT:
				// NA
				return NA;
			case FOP_STATE_RT_WAIT:
				// NA
				return NA;
			case FOP_STATE_INIT_NO_BC:
				// NA
				return NA;
			case FOP_STATE_INIT_BC:
				// NA
				return NA;
			case FOP_STATE_INIT:
				ret = resume(tc_tf);
				if (ret) {
					tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
					return UNDEF_ERROR;
				}
				//confirm();
				tc_tf->cop_cfg.fop.state = FOP_STATE_INIT_NO_BC;
				tc_tf->cop_cfg.fop.signal = POSITIVE_DIR;
				return POSITIVE_DIR;
			default:
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
		}
	} else {
		tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
		return UNDEF_ERROR;
	}
}

notification_t
set_new_vs(struct tc_transfer_frame *tc_tf, uint8_t new_vs)
{
	/*E35*/
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			// Reject
			return REJECT_DIR;
		case FOP_STATE_RT_NO_WAIT:
			// Reject
			return REJECT_DIR;
		case FOP_STATE_RT_WAIT:
			// Reject
			return REJECT_DIR;
		case FOP_STATE_INIT_NO_BC:
			// Reject
			return REJECT_DIR;
		case FOP_STATE_INIT_BC:
			// Reject
			return REJECT_DIR;
		case FOP_STATE_INIT:
			if (tc_tf->cop_cfg.fop.ss == 0) {
				tc_tf->cop_cfg.fop.vs = new_vs;
				tc_tf->cop_cfg.fop.nnr = new_vs;
				//confirm();
				tc_tf->cop_cfg.fop.signal = POSITIVE_DIR;
				return POSITIVE_DIR;
			} else {
				// Reject
				return REJECT_DIR;
			}
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
set_sliding_window(struct tc_transfer_frame *tc_tf,
                   uint8_t new_wnd)
{
	/*E36*/
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			tc_tf->cop_cfg.fop.slide_wnd = new_wnd;
			//confirm();
			tc_tf->cop_cfg.fop.signal = POSITIVE_DIR;
			return POSITIVE_DIR;
		case FOP_STATE_RT_NO_WAIT:
			tc_tf->cop_cfg.fop.slide_wnd = new_wnd;
			//confirm();
			tc_tf->cop_cfg.fop.signal = POSITIVE_DIR;
			return POSITIVE_DIR;
		case FOP_STATE_RT_WAIT:
			tc_tf->cop_cfg.fop.slide_wnd = new_wnd;
			//confirm();
			tc_tf->cop_cfg.fop.signal = POSITIVE_DIR;
			return POSITIVE_DIR;
		case FOP_STATE_INIT_NO_BC:
			tc_tf->cop_cfg.fop.slide_wnd = new_wnd;
			//confirm();
			tc_tf->cop_cfg.fop.signal = POSITIVE_DIR;
			return POSITIVE_DIR;
		case FOP_STATE_INIT_BC:
			tc_tf->cop_cfg.fop.slide_wnd = new_wnd;
			//confirm();
			tc_tf->cop_cfg.fop.signal = POSITIVE_DIR;
			return POSITIVE_DIR;
		case FOP_STATE_INIT:
			tc_tf->cop_cfg.fop.slide_wnd = new_wnd;
			//confirm();
			tc_tf->cop_cfg.fop.signal = POSITIVE_DIR;
			return POSITIVE_DIR;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
set_timer(struct tc_transfer_frame *tc_tf, uint16_t new_t1)
{
	/*E37*/
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			tc_tf->cop_cfg.fop.t1_init = new_t1;
			//confirm();
			tc_tf->cop_cfg.fop.signal = POSITIVE_DIR;
			return POSITIVE_DIR;
		case FOP_STATE_RT_NO_WAIT:
			tc_tf->cop_cfg.fop.t1_init = new_t1;
			//confirm();
			tc_tf->cop_cfg.fop.signal = POSITIVE_DIR;
			return POSITIVE_DIR;
		case FOP_STATE_RT_WAIT:
			tc_tf->cop_cfg.fop.t1_init = new_t1;
			//confirm();
			tc_tf->cop_cfg.fop.signal = POSITIVE_DIR;
			return POSITIVE_DIR;
		case FOP_STATE_INIT_NO_BC:
			tc_tf->cop_cfg.fop.t1_init = new_t1;
			//confirm();
			tc_tf->cop_cfg.fop.signal = POSITIVE_DIR;
			return POSITIVE_DIR;
		case FOP_STATE_INIT_BC:
			tc_tf->cop_cfg.fop.t1_init = new_t1;
			//confirm();
			tc_tf->cop_cfg.fop.signal = POSITIVE_DIR;
			return POSITIVE_DIR;
		case FOP_STATE_INIT:
			tc_tf->cop_cfg.fop.t1_init = new_t1;
			//confirm();
			tc_tf->cop_cfg.fop.signal = POSITIVE_DIR;
			return POSITIVE_DIR;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
set_tx_limit(struct tc_transfer_frame *tc_tf, uint8_t new_lim)
{
	/*E38*/
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			tc_tf->cop_cfg.fop.tx_lim = new_lim;
			//confirm();
			tc_tf->cop_cfg.fop.signal = POSITIVE_DIR;
			return POSITIVE_DIR;
		case FOP_STATE_RT_NO_WAIT:
			tc_tf->cop_cfg.fop.tx_lim = new_lim;
			//confirm();
			tc_tf->cop_cfg.fop.signal = POSITIVE_DIR;
			return POSITIVE_DIR;
		case FOP_STATE_RT_WAIT:
			tc_tf->cop_cfg.fop.tx_lim = new_lim;
			//confirm();
			tc_tf->cop_cfg.fop.signal = POSITIVE_DIR;
			return POSITIVE_DIR;
		case FOP_STATE_INIT_NO_BC:
			tc_tf->cop_cfg.fop.tx_lim = new_lim;
			//confirm();
			tc_tf->cop_cfg.fop.signal = POSITIVE_DIR;
			return POSITIVE_DIR;
		case FOP_STATE_INIT_BC:
			tc_tf->cop_cfg.fop.tx_lim = new_lim;
			//confirm();
			tc_tf->cop_cfg.fop.signal = POSITIVE_DIR;
			return POSITIVE_DIR;
		case FOP_STATE_INIT:
			tc_tf->cop_cfg.fop.tx_lim = new_lim;
			//confirm();
			tc_tf->cop_cfg.fop.signal = POSITIVE_DIR;
			return POSITIVE_DIR;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
set_tt(struct tc_transfer_frame *tc_tf, uint8_t new_tt)
{
	/*E38*/
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			tc_tf->cop_cfg.fop.tt = new_tt;
			//confirm();
			tc_tf->cop_cfg.fop.signal = POSITIVE_DIR;
			return POSITIVE_DIR;
		case FOP_STATE_RT_NO_WAIT:
			tc_tf->cop_cfg.fop.tt = new_tt;
			//confirm();
			tc_tf->cop_cfg.fop.signal = POSITIVE_DIR;
			return POSITIVE_DIR;
		case FOP_STATE_RT_WAIT:
			tc_tf->cop_cfg.fop.tt = new_tt;
			//confirm();
			tc_tf->cop_cfg.fop.signal = POSITIVE_DIR;
			return POSITIVE_DIR;
		case FOP_STATE_INIT_NO_BC:
			tc_tf->cop_cfg.fop.tt = new_tt;
			//confirm();
			tc_tf->cop_cfg.fop.signal = POSITIVE_DIR;
			return POSITIVE_DIR;
		case FOP_STATE_INIT_BC:
			tc_tf->cop_cfg.fop.tt = new_tt;
			//confirm();
			tc_tf->cop_cfg.fop.signal = POSITIVE_DIR;
			return POSITIVE_DIR;
		case FOP_STATE_INIT:
			tc_tf->cop_cfg.fop.tt = new_tt;
			//confirm();
			tc_tf->cop_cfg.fop.signal = POSITIVE_DIR;
			return POSITIVE_DIR;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
ad_accept(struct tc_transfer_frame *tc_tf)
{
	notification_t notif;
	/*E41*/
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			notif = look_for_fdu(tc_tf);
			if (!(notif == IGNORE)) {
				tc_tf->cop_cfg.fop.signal = notif;
				return notif;
			} else {
				return tc_tf->cop_cfg.fop.signal;
			}
		case FOP_STATE_RT_NO_WAIT:
			notif = look_for_fdu(tc_tf);
			if (!(notif == IGNORE)) {
				tc_tf->cop_cfg.fop.signal = notif;
				return notif;
			} else {
				return tc_tf->cop_cfg.fop.signal;
			}
		case FOP_STATE_RT_WAIT:
			return IGNORE;
		case FOP_STATE_INIT_NO_BC:
			return IGNORE;
		case FOP_STATE_INIT_BC:
			return IGNORE;
		case FOP_STATE_INIT:
			return IGNORE;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
ad_reject(struct tc_transfer_frame *tc_tf)
{
	int ret;
	/*E42*/
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_LLIF;
			return ALERT_LLIF;
		case FOP_STATE_RT_NO_WAIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_LLIF;
			return ALERT_LLIF;
		case FOP_STATE_RT_WAIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_LLIF;
			return ALERT_LLIF;
		case FOP_STATE_INIT_NO_BC:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_LLIF;
			return ALERT_LLIF;
		case FOP_STATE_INIT_BC:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_LLIF;
			return ALERT_LLIF;
		case FOP_STATE_INIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.signal = ALERT_LLIF;
			return ALERT_LLIF;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
bc_accept(struct tc_transfer_frame *tc_tf)
{
	notification_t notif;
	/*E43*/
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			return IGNORE;
		case FOP_STATE_RT_NO_WAIT:
			return IGNORE;
		case FOP_STATE_RT_WAIT:
			return IGNORE;
		case FOP_STATE_INIT_NO_BC:
			return IGNORE;
		case FOP_STATE_INIT_BC:
			notif = look_for_directive(tc_tf);
			if (!(notif == IGNORE)) {
				tc_tf->cop_cfg.fop.signal = notif;
				return notif;
			} else {
				return tc_tf->cop_cfg.fop.signal;
			}
		case FOP_STATE_INIT:
			return IGNORE;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
bc_reject(struct tc_transfer_frame *tc_tf)
{
	int ret;
	/*E44*/
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_LLIF;
			return ALERT_LLIF;
		case FOP_STATE_RT_NO_WAIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_LLIF;
			return ALERT_LLIF;
		case FOP_STATE_RT_WAIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_LLIF;
			return ALERT_LLIF;
		case FOP_STATE_INIT_NO_BC:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_LLIF;
			return ALERT_LLIF;
		case FOP_STATE_INIT_BC:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_LLIF;
			return ALERT_LLIF;
		case FOP_STATE_INIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.signal = ALERT_LLIF;
			return ALERT_LLIF;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
bd_accept(struct tc_transfer_frame *tc_tf)
{
	/*E45*/
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			tc_tf->cop_cfg.fop.signal = ACCEPT_TX;
			return ACCEPT_TX;
		case FOP_STATE_RT_NO_WAIT:
			tc_tf->cop_cfg.fop.signal = ACCEPT_TX;
			return ACCEPT_TX;
		case FOP_STATE_RT_WAIT:
			tc_tf->cop_cfg.fop.signal = ACCEPT_TX;
			return ACCEPT_TX;
		case FOP_STATE_INIT_NO_BC:
			tc_tf->cop_cfg.fop.signal = ACCEPT_TX;
			return ACCEPT_TX;
		case FOP_STATE_INIT_BC:
			tc_tf->cop_cfg.fop.signal = ACCEPT_TX;
			return ACCEPT_TX;
		case FOP_STATE_INIT:
			tc_tf->cop_cfg.fop.signal = ACCEPT_TX;
			return ACCEPT_TX;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}
}

notification_t
bd_reject(struct tc_transfer_frame *tc_tf)
{
	int ret;
	/*E46*/
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_LLIF;
			return ALERT_LLIF;
		case FOP_STATE_RT_NO_WAIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_LLIF;
			return ALERT_LLIF;
		case FOP_STATE_RT_WAIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_LLIF;
			return ALERT_LLIF;
		case FOP_STATE_INIT_NO_BC:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_LLIF;
			return ALERT_LLIF;
		case FOP_STATE_INIT_BC:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			tc_tf->cop_cfg.fop.signal = ALERT_LLIF;
			return ALERT_LLIF;
		case FOP_STATE_INIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.signal = ALERT_LLIF;
			return ALERT_LLIF;
		default:
			tc_tf->cop_cfg.fop.signal = UNDEF_ERROR;
			return UNDEF_ERROR;
	}

}
