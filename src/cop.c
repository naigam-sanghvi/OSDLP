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
prepare_fop(struct fop_config *fop, uint16_t slide_wnd,
            fop_state_t state, uint16_t t1_init, uint8_t timeout_type,
            uint8_t tx_lim)
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
prepare_farm(struct farm_config *farm, farm_state_t state, uint8_t window_width)
{
	farm->state = state;
	farm->pw = window_width / 2;
	farm->nw = window_width / 2;
	farm->w = window_width;
	return 0;
}

int
farm_1(struct tc_transfer_frame *tc_tf)
{
	if (tc_tf->primary_hdr.bypass == TYPE_A) {
		if (tc_tf->primary_hdr.frame_seq_num == tc_tf->cop_cfg.farm.vr) {
			/*E1*/
			if (!rx_queue_full(tc_tf->primary_hdr.vcid)) {
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
				}
			}
			/*E2*/
			else {
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
				}
			}
		}
		/*E3*/
		else if ((tc_tf->primary_hdr.frame_seq_num > tc_tf->cop_cfg.farm.vr) &&
		         (tc_tf->primary_hdr.frame_seq_num <= tc_tf->cop_cfg.farm.vr +
		          tc_tf->cop_cfg.farm.pw - 1)) {
			switch (tc_tf->cop_cfg.farm.state) {
				case FARM_STATE_OPEN:
					tc_tf->cop_cfg.farm.retransmit = 1;
					return COP_DISCARD;
				case FARM_STATE_WAIT:
					return COP_DISCARD;
				case FARM_STATE_LOCKOUT:
					return COP_DISCARD;
			}
		}
		/*E4*/
		else if ((tc_tf->primary_hdr.frame_seq_num < tc_tf->cop_cfg.farm.vr) &&
		         (tc_tf->primary_hdr.frame_seq_num >= tc_tf->cop_cfg.farm.vr -
		          tc_tf->cop_cfg.farm.nw)) {
			switch (tc_tf->cop_cfg.farm.state) {
				case FARM_STATE_OPEN:
					return COP_DISCARD;
				case FARM_STATE_WAIT:
					return COP_DISCARD;
				case FARM_STATE_LOCKOUT:
					return COP_DISCARD;
			}
		}
		/*E5*/
		else if ((tc_tf->primary_hdr.frame_seq_num > tc_tf->cop_cfg.farm.vr +
		          tc_tf->cop_cfg.farm.pw - 1) &&
		         (tc_tf->primary_hdr.frame_seq_num < tc_tf->cop_cfg.farm.vr -
		          tc_tf->cop_cfg.farm.nw)) {
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
			}
		}
		/*Undefined*/
		else {
			return COP_ERROR;
		}
	} else if (tc_tf->primary_hdr.bypass == TYPE_B) {
		/*Ε6*/
		if (tc_tf->primary_hdr.ctrl_cmd == TC_DATA) {
			switch (tc_tf->cop_cfg.farm.state) {
				case FARM_STATE_OPEN:
					tc_tf->cop_cfg.farm.farmb_cnt = (tc_tf->cop_cfg.farm.farmb_cnt + 1) % 4;
					return COP_PRIORITY_ENQ;
				case FARM_STATE_WAIT:
					tc_tf->cop_cfg.farm.farmb_cnt = (tc_tf->cop_cfg.farm.farmb_cnt + 1) % 4;
					return COP_PRIORITY_ENQ;
				case FARM_STATE_LOCKOUT:
					tc_tf->cop_cfg.farm.farmb_cnt = (tc_tf->cop_cfg.farm.farmb_cnt + 1) % 4;
					return COP_PRIORITY_ENQ;
			}
		} else if (tc_tf->primary_hdr.ctrl_cmd == TC_COMMAND) {
			/*E7*/
			if (tc_tf->frame_data.data[0] == 0) { /*UNLOCK*/
				switch (tc_tf->cop_cfg.farm.state) {
					case FARM_STATE_OPEN:
						tc_tf->cop_cfg.farm.farmb_cnt = (tc_tf->cop_cfg.farm.farmb_cnt + 1) % 4;
						tc_tf->cop_cfg.farm.retransmit = 0;
						return COP_OK;
					case FARM_STATE_WAIT:
						tc_tf->cop_cfg.farm.farmb_cnt = (tc_tf->cop_cfg.farm.farmb_cnt + 1) % 4;
						tc_tf->cop_cfg.farm.retransmit = 0;
						tc_tf->cop_cfg.farm.wait = 0;
						return COP_OK;
					case FARM_STATE_LOCKOUT:
						tc_tf->cop_cfg.farm.farmb_cnt = (tc_tf->cop_cfg.farm.farmb_cnt + 1) % 4;
						tc_tf->cop_cfg.farm.retransmit = 0;
						tc_tf->cop_cfg.farm.wait = 0;
						tc_tf->cop_cfg.farm.lockout = 0;
						return COP_OK;
				}
			}
			/*E8*/
			else if (tc_tf->frame_data.data[0] == SETVR_BYTE1 &&
			         tc_tf->frame_data.data[1] == SETVR_BYTE2) { /*SET V(R)*/
				switch (tc_tf->cop_cfg.farm.state) {
					case FARM_STATE_OPEN:
						tc_tf->cop_cfg.farm.vr = tc_tf->frame_data.data[2];
						tc_tf->cop_cfg.farm.farmb_cnt = (tc_tf->cop_cfg.farm.farmb_cnt + 1) % 4;
						tc_tf->cop_cfg.farm.retransmit = 0;
						return COP_OK;
					case FARM_STATE_WAIT:
						tc_tf->cop_cfg.farm.vr = tc_tf->frame_data.data[2];
						tc_tf->cop_cfg.farm.farmb_cnt = (tc_tf->cop_cfg.farm.farmb_cnt + 1) % 4;
						tc_tf->cop_cfg.farm.retransmit = 0;
						tc_tf->cop_cfg.farm.wait = 0;
						return COP_OK;
					case FARM_STATE_LOCKOUT:
						tc_tf->cop_cfg.farm.farmb_cnt = (tc_tf->cop_cfg.farm.farmb_cnt + 1) % 4;
						return COP_OK;
				}
			}
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
	notification_t notif;
	int ret;
	if (!sent_queue_empty(tc_tf->primary_hdr.vcid)) {
		struct queue_item item;
		ret = get_first_ad_rt_frame(&item, tc_tf->primary_hdr.vcid);
		if (!ret || item.rt_flag == RT_FLAG_ON) {
			item.rt_flag = RT_FLAG_OFF;
			ret = tx_queue_enqueue(item.fdu);
			uint8_t seq = item.fdu[4];
			if (ret) {
				notif = ad_reject(tc_tf);
				return notif;
			} else {
				ret = reset_rt_frame(&item, tc_tf->primary_hdr.vcid);
				notif = ad_accept(tc_tf);
				return notif;
			}
		}
	}
	/*Perform checks in case window wraps around*/

	if (!wait_queue_empty(tc_tf->primary_hdr.vcid)) {
		if (tc_tf->cop_cfg.fop.nnr + tc_tf->cop_cfg.fop.slide_wnd >= 256) {
			if (tc_tf->cop_cfg.fop.vs
			    > (tc_tf->cop_cfg.fop.nnr + tc_tf->cop_cfg.fop.slide_wnd)
			    % 256) {
				notif = transmit_type_ad(tc_tf);
				return notif;
			} else {
				return DELAY_RESP;
			}
		} else {
			if (tc_tf->cop_cfg.fop.vs
			    < tc_tf->cop_cfg.fop.nnr + tc_tf->cop_cfg.fop.slide_wnd) {
				notif = transmit_type_ad(tc_tf);
				return notif;
			} else {
				return DELAY_RESP;
			}
		}
	} else {
		return IGNORE;
	}
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
		ret = tx_queue_enqueue(item.fdu);
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

notification_t
transmit_type_ad(struct tc_transfer_frame *tc_tf)
{
	notification_t notif;
	struct queue_item item;
	struct tc_transfer_frame wait_item;
	int ret = wait_queue_dequeue(&wait_item, tc_tf->primary_hdr.vcid);

	ret = tc_pack(tc_tf, tc_tf->mission.util.buffer, wait_item.frame_data.data,
	              wait_item.frame_data.data_len);
	if (ret) {
		return UNDEF_ERROR;
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
		return UNDEF_ERROR;
	}

	ret = tx_queue_enqueue(tc_tf->mission.util.buffer);
	if (ret) {
		notif = ad_reject(tc_tf);
		return notif;
	} else {
		notif = ad_accept(tc_tf);
		return notif;
	}
}

notification_t
transmit_type_bc(struct tc_transfer_frame *tc_tf)
{
	notification_t notif;
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

	ret = tx_queue_enqueue(tc_tf->mission.util.buffer);
	if (ret) {
		notif = bc_reject(tc_tf);
		return notif;
	} else {
		notif = bc_accept(tc_tf);
		return notif;
	}
}

notification_t
transmit_type_bd(struct tc_transfer_frame *tc_tf)
{
	notification_t notif;
	int ret = tc_pack(tc_tf, tc_tf->mission.util.buffer, tc_tf->frame_data.data,
	                  tc_tf->frame_data.data_len);
	if (ret) {
		return 1;
	}
	//Set BD_Out not ready
	ret = tx_queue_enqueue(tc_tf->mission.util.buffer);
	if (ret) {
		notif = bd_reject(tc_tf);
		return notif;
	} else {
		notif = bd_accept(tc_tf);
		return notif;
	}
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
			return 1;
		}
		if (item.seq_num  != nr) {
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

int
handle_clcw(struct tc_transfer_frame *tc_tf, struct clcw_frame *clcw)
{
	int ret;
	notification_t notif;
	if (clcw->lockout == CLCW_NO_LOCKOUT) {
		if (tc_tf->cop_cfg.fop.vs == clcw->report_value) {
			if (clcw->rt == CLCW_NO_RETRANSMIT) {
				if (clcw->wait == CLCW_DO_NOT_WAIT) {
					if (clcw->report_value == tc_tf->cop_cfg.fop.nnr) {
						/*E1*/
						switch (tc_tf->cop_cfg.fop.state) {
							case FOP_STATE_ACTIVE:
								// Ignore
								return IGNORE;
							case FOP_STATE_RT_NO_WAIT:
								// Alert
								ret = alert(tc_tf);
								if (ret) {
									return UNDEF_ERROR;
								}
								tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
								return ALERT_SYNCH;
							case FOP_STATE_RT_WAIT:
								// Alert;
								ret = alert(tc_tf);
								if (ret) {
									return UNDEF_ERROR;
								}
								tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
								return ALERT_SYNCH;
							case FOP_STATE_INIT_NO_BC:
								tc_tf->cop_cfg.fop.state = FOP_STATE_ACTIVE;
								timer_cancel(tc_tf->primary_hdr.vcid);
								return POSITIVE_DIR;
							case FOP_STATE_INIT_BC:
								ret = release_copy_of_bc(tc_tf);
								if (ret) {
									return UNDEF_ERROR;
								}
								tc_tf->cop_cfg.fop.state = FOP_STATE_ACTIVE;
								timer_cancel(tc_tf->primary_hdr.vcid);
								return IGNORE;
							case FOP_STATE_INIT:
								//Ignore
								return IGNORE;
						}
					} else {
						/* E2 */
						switch (tc_tf->cop_cfg.fop.state) {
							case FOP_STATE_ACTIVE:
								ret = remove_acked_frames(tc_tf, clcw->report_value);
								if (ret) {
									return UNDEF_ERROR;
								}
								timer_cancel(tc_tf->primary_hdr.vcid);
								notif = look_for_fdu(tc_tf);
								tc_tf->cop_cfg.fop.signal = notif;
								return notif;
							case FOP_STATE_RT_NO_WAIT:
								ret = remove_acked_frames(tc_tf, clcw->report_value);
								if (ret) {
									return UNDEF_ERROR;
								}
								timer_cancel(tc_tf->primary_hdr.vcid);
								notif = look_for_fdu(tc_tf);
								tc_tf->cop_cfg.fop.signal = notif;
								return notif;
							case FOP_STATE_RT_WAIT:
								ret = remove_acked_frames(tc_tf, clcw->report_value);
								if (ret) {
									return UNDEF_ERROR;
								}
								timer_cancel(tc_tf->primary_hdr.vcid);
								notif = look_for_fdu(tc_tf);
								tc_tf->cop_cfg.fop.signal = notif;
								return notif;
							case FOP_STATE_INIT_NO_BC:
								// NA
								return NA;
							case FOP_STATE_INIT_BC:
								// NA
								return NA;
							case FOP_STATE_INIT:
								//Ignore
								return IGNORE;
						}
					}
				} else { // Wait flag  = 1
					/*E3*/
					switch (tc_tf->cop_cfg.fop.state) {
						case FOP_STATE_ACTIVE:
							// Alert
							ret = alert(tc_tf);
							if (ret) {
								return UNDEF_ERROR;
							}
							tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
							return ALERT_CLCW;
						case FOP_STATE_RT_NO_WAIT:
							// Alert
							ret = alert(tc_tf);
							if (ret) {
								return UNDEF_ERROR;
							}
							tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
							return ALERT_CLCW;
						case FOP_STATE_RT_WAIT:
							// Alert
							ret = alert(tc_tf);
							if (ret) {
								return UNDEF_ERROR;
							}
							tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
							return ALERT_CLCW;
						case FOP_STATE_INIT_NO_BC:
							// Alert
							ret = alert(tc_tf);
							if (ret) {
								return UNDEF_ERROR;
							}
							tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
							return ALERT_CLCW;
						case FOP_STATE_INIT_BC:
							// Alert
							ret = alert(tc_tf);
							if (ret) {
								return UNDEF_ERROR;
							}
							tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
							return ALERT_CLCW;
						case FOP_STATE_INIT:
							// Ignore
							return IGNORE;
					}
				}
			} else { // Retransmit flag = 1
				/*E4*/
				switch (tc_tf->cop_cfg.fop.state) {
					case FOP_STATE_ACTIVE:
						// Alert
						ret = alert(tc_tf);
						if (ret) {
							return UNDEF_ERROR;
						}
						tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
						return ALERT_SYNCH;
					case FOP_STATE_RT_NO_WAIT:
						// Alert
						ret = alert(tc_tf);
						if (ret) {
							return UNDEF_ERROR;
						}
						tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
						return ALERT_SYNCH;
					case FOP_STATE_RT_WAIT:
						// Alert
						ret = alert(tc_tf);
						if (ret) {
							return UNDEF_ERROR;
						}
						tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
						return ALERT_SYNCH;
					case FOP_STATE_INIT_NO_BC:
						// Alert
						ret = alert(tc_tf);
						if (ret) {
							return UNDEF_ERROR;
						}
						tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
						return ALERT_SYNCH;
					case FOP_STATE_INIT_BC:
						// Ignore
						return IGNORE;
					case FOP_STATE_INIT:
						// Ignore
						tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
						return IGNORE;
				}
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
						switch (tc_tf->cop_cfg.fop.state) {
							case FOP_STATE_ACTIVE:
								// Ignore
								return IGNORE;
							case FOP_STATE_RT_NO_WAIT:
								// Alert
								ret = alert(tc_tf);
								if (ret) {
									return UNDEF_ERROR;
								}
								tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
								return ALERT_SYNCH;
							case FOP_STATE_RT_WAIT:
								// Alert
								ret = alert(tc_tf);
								if (ret) {
									return UNDEF_ERROR;
								}
								tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
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
						}
					} else {
						/* E6 */
						switch (tc_tf->cop_cfg.fop.state) {
							case FOP_STATE_ACTIVE:
								ret = remove_acked_frames(tc_tf, clcw->report_value);
								if (ret) {
									return UNDEF_ERROR;
								}
								notif = look_for_fdu(tc_tf);
								tc_tf->cop_cfg.fop.signal = notif;
								return notif;
							case FOP_STATE_RT_NO_WAIT:
								ret = remove_acked_frames(tc_tf, clcw->report_value);
								if (ret) {
									return UNDEF_ERROR;
								}
								notif = look_for_fdu(tc_tf);
								tc_tf->cop_cfg.fop.signal = notif;
								tc_tf->cop_cfg.fop.state = FOP_STATE_ACTIVE;
								return notif;
							case FOP_STATE_RT_WAIT:
								ret = remove_acked_frames(tc_tf, clcw->report_value);
								if (ret) {
									return UNDEF_ERROR;
								}
								notif = look_for_fdu(tc_tf);
								tc_tf->cop_cfg.fop.signal = notif;
								tc_tf->cop_cfg.fop.state = FOP_STATE_ACTIVE;
								return notif;
							case FOP_STATE_INIT_NO_BC:
								// NA
								return NA;
							case FOP_STATE_INIT_BC:
								// NA
								return NA;
							case FOP_STATE_INIT:
								//Ignore
								return IGNORE;
						}
					}
				} else { // Wait flag  = 1
					/*E7*/
					switch (tc_tf->cop_cfg.fop.state) {
						case FOP_STATE_ACTIVE:
							// Alert
							ret = alert(tc_tf);
							if (ret) {
								return UNDEF_ERROR;
							}
							tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
							return ALERT_CLCW;
						case FOP_STATE_RT_NO_WAIT:
							// Alert
							ret = alert(tc_tf);
							if (ret) {
								return UNDEF_ERROR;
							}
							tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
							return ALERT_CLCW;
						case FOP_STATE_RT_WAIT:
							// Alert
							ret = alert(tc_tf);
							if (ret) {
								return UNDEF_ERROR;
							}
							tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
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
					}
				}
			} else { // Retransmit flag = 1
				if (tc_tf->cop_cfg.fop.tx_lim == 1) {
					/*E101*/
					if (clcw->report_value != tc_tf->cop_cfg.fop.nnr) {
						switch (tc_tf->cop_cfg.fop.state) {
							case FOP_STATE_ACTIVE:
								ret = remove_acked_frames(tc_tf, clcw->report_value);
								if (ret) {
									return UNDEF_ERROR;
								}
								// Alert
								ret = alert(tc_tf);
								if (ret) {
									return UNDEF_ERROR;
								}
								tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
								return ALERT_LIMIT;
							case FOP_STATE_RT_NO_WAIT:
								ret = remove_acked_frames(tc_tf, clcw->report_value);
								if (ret) {
									return UNDEF_ERROR;
								}
								// Alert
								ret = alert(tc_tf);
								if (ret) {
									return UNDEF_ERROR;
								}
								tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
								return ALERT_LIMIT;
							case FOP_STATE_RT_WAIT:
								remove_acked_frames(tc_tf, clcw->report_value);
								// Alert
								ret = alert(tc_tf);
								if (ret) {
									return UNDEF_ERROR;
								}
								tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
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
						}
					}
					/*E102*/
					else {
						switch (tc_tf->cop_cfg.fop.state) {
							case FOP_STATE_ACTIVE:
								// Alert
								ret = alert(tc_tf);
								if (ret) {
									return UNDEF_ERROR;
								}
								tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
								return ALERT_LIMIT;
							case FOP_STATE_RT_NO_WAIT:
								// Alert
								ret = alert(tc_tf);
								if (ret) {
									return UNDEF_ERROR;
								}
								tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
								return ALERT_LIMIT;
							case FOP_STATE_RT_WAIT:
								// Alert
								ret = alert(tc_tf);
								if (ret) {
									return UNDEF_ERROR;
								}
								tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
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
						}
					}
				} else if (tc_tf->cop_cfg.fop.tx_lim > 1) {
					if (clcw->report_value != tc_tf->cop_cfg.fop.nnr) {
						/*E8*/
						if (clcw->wait == CLCW_DO_NOT_WAIT) {
							switch (tc_tf->cop_cfg.fop.state) {
								case FOP_STATE_ACTIVE:
									ret = remove_acked_frames(tc_tf, clcw->report_value);
									if (ret) {
										return UNDEF_ERROR;
									}
									ret = initiate_ad_retransmission(tc_tf);
									if (ret) {
										return UNDEF_ERROR;
									}
									notif = look_for_fdu(tc_tf);
									tc_tf->cop_cfg.fop.state = FOP_STATE_RT_NO_WAIT;
									tc_tf->cop_cfg.fop.signal = notif;
									return notif;
								case FOP_STATE_RT_NO_WAIT:
									ret = remove_acked_frames(tc_tf, clcw->report_value);
									if (ret) {
										return UNDEF_ERROR;
									}
									ret = initiate_ad_retransmission(tc_tf);
									if (ret) {
										return UNDEF_ERROR;
									}
									notif = look_for_fdu(tc_tf);
									tc_tf->cop_cfg.fop.signal = notif;
									return notif;
								case FOP_STATE_RT_WAIT:
									ret = remove_acked_frames(tc_tf, clcw->report_value);
									if (ret) {
										return UNDEF_ERROR;
									}
									ret = initiate_ad_retransmission(tc_tf);
									if (ret) {
										return UNDEF_ERROR;
									}
									notif = look_for_fdu(tc_tf);
									tc_tf->cop_cfg.fop.state = FOP_STATE_RT_NO_WAIT;
									tc_tf->cop_cfg.fop.signal = notif;
									return notif;
								case FOP_STATE_INIT_NO_BC:
									// NA
									return NA;
								case FOP_STATE_INIT_BC:
									// NA
									return NA;
								case FOP_STATE_INIT:
									//Ignore
									return IGNORE;
							}
						}
						/*E9*/
						else { //Wait flag = 1
							switch (tc_tf->cop_cfg.fop.state) {
								case FOP_STATE_ACTIVE:
									ret = remove_acked_frames(tc_tf, clcw->report_value);
									if (ret) {
										return UNDEF_ERROR;
									}
									tc_tf->cop_cfg.fop.state = FOP_STATE_RT_WAIT;
									return IGNORE;
								case FOP_STATE_RT_NO_WAIT:
									ret = remove_acked_frames(tc_tf, clcw->report_value);
									if (ret) {
										return UNDEF_ERROR;
									}
									tc_tf->cop_cfg.fop.state = FOP_STATE_RT_WAIT;
									return IGNORE;
								case FOP_STATE_RT_WAIT:
									ret = remove_acked_frames(tc_tf, clcw->report_value);
									if (ret) {
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
							}
						}
					} else { //N(R == NN{R}
						if (tc_tf->cop_cfg.fop.tx_cnt
						    < tc_tf->cop_cfg.fop.tx_lim) {
							/*E10*/
							if (clcw->wait == CLCW_DO_NOT_WAIT) {
								switch (tc_tf->cop_cfg.fop.state) {
									case FOP_STATE_ACTIVE:
										ret = initiate_ad_retransmission(tc_tf);
										if (ret) {
											return UNDEF_ERROR;
										}
										notif = look_for_fdu(tc_tf);
										tc_tf->cop_cfg.fop.state = FOP_STATE_RT_NO_WAIT;
										tc_tf->cop_cfg.fop.signal = notif;
										return notif;
									case FOP_STATE_RT_NO_WAIT:
										// Ignore
										return IGNORE;
									case FOP_STATE_RT_WAIT:
										ret = initiate_ad_retransmission(tc_tf);
										if (ret) {
											return UNDEF_ERROR;
										}
										notif = look_for_fdu(tc_tf);
										tc_tf->cop_cfg.fop.state = FOP_STATE_RT_NO_WAIT;
										tc_tf->cop_cfg.fop.signal = notif;
										return notif;
									case FOP_STATE_INIT_NO_BC:
										// NA
										return NA;
									case FOP_STATE_INIT_BC:
										// NA
										return NA;
									case FOP_STATE_INIT:
										//Ignore
										return IGNORE;
								}
							}
							/*E11*/
							else { //Wait flag = 1
								switch (tc_tf->cop_cfg.fop.state) {
									case FOP_STATE_ACTIVE:
										// Ignore
										tc_tf->cop_cfg.fop.state = FOP_STATE_RT_WAIT;
										return IGNORE;
									case FOP_STATE_RT_NO_WAIT:
										// Ignore
										tc_tf->cop_cfg.fop.state = FOP_STATE_RT_WAIT;
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
								}
							}
						} else { // Tx cnt >= Tx limit
							/*E12*/
							if (clcw->wait == CLCW_DO_NOT_WAIT) {
								switch (tc_tf->cop_cfg.fop.state) {
									case FOP_STATE_ACTIVE:
										// ignore
										tc_tf->cop_cfg.fop.state = FOP_STATE_RT_NO_WAIT;
										return IGNORE;
									case FOP_STATE_RT_NO_WAIT:
										// Ignore
										return IGNORE;
									case FOP_STATE_RT_WAIT:
										// Ignore
										tc_tf->cop_cfg.fop.state = FOP_STATE_RT_NO_WAIT;
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
								}
							}
							/*E103*/
							else { //Wait flag = 1
								switch (tc_tf->cop_cfg.fop.state) {
									case FOP_STATE_ACTIVE:
										// Ignore
										tc_tf->cop_cfg.fop.state = FOP_STATE_RT_WAIT;
										return IGNORE;
									case FOP_STATE_RT_NO_WAIT:
										// Ignore
										tc_tf->cop_cfg.fop.state = FOP_STATE_RT_WAIT;
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
								}
							}
						}
					}
				}
			}
		} else { // N(R) not within bounds
			/*E13*/
			switch (tc_tf->cop_cfg.fop.state) {
				case FOP_STATE_ACTIVE:
					// Alert
					ret = alert(tc_tf);
					if (ret) {
						return UNDEF_ERROR;
					}
					tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
					return ALERT_NNR;
				case FOP_STATE_RT_NO_WAIT:
					// Alert
					ret = alert(tc_tf);
					if (ret) {
						return UNDEF_ERROR;
					}
					tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
					return ALERT_NNR;
				case FOP_STATE_RT_WAIT:
					// Alert
					ret = alert(tc_tf);
					if (ret) {
						return UNDEF_ERROR;
					}
					tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
					return ALERT_NNR;
				case FOP_STATE_INIT_NO_BC:
					// Alert
					ret = alert(tc_tf);
					if (ret) {
						return UNDEF_ERROR;
					}
					tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
					return ALERT_NNR;
				case FOP_STATE_INIT_BC:
					// Ignore
					return IGNORE;
				case FOP_STATE_INIT:
					//Ignore
					return IGNORE;
			}
		}
	} else { // Lockout = 1
		/*E14*/
		switch (tc_tf->cop_cfg.fop.state) {
			case FOP_STATE_ACTIVE:
				// Alert
				ret = alert(tc_tf);
				if (ret) {
					return UNDEF_ERROR;
				}
				tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
				return ALERT_LOCKOUT;
			case FOP_STATE_RT_NO_WAIT:
				// Alert
				ret = alert(tc_tf);
				if (ret) {
					return UNDEF_ERROR;
				}
				tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
				return ALERT_LOCKOUT;
			case FOP_STATE_RT_WAIT:
				// Alert
				ret = alert(tc_tf);
				if (ret) {
					return UNDEF_ERROR;
				}
				tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
				return ALERT_LOCKOUT;
			case FOP_STATE_INIT_NO_BC:
				// Alert
				ret = alert(tc_tf);
				if (ret) {
					return UNDEF_ERROR;
				}
				tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
				return ALERT_LOCKOUT;
			case FOP_STATE_INIT_BC:
				// Ignore
				return IGNORE;
			case FOP_STATE_INIT:
				//Ignore
				return IGNORE;
		}
	}
	return UNDEF_ERROR;
}

notification_t
handle_timer_expired(struct tc_transfer_frame *tc_tf)
{
	int ret;
	notification_t notif;
	if (tc_tf->cop_cfg.fop.tx_cnt < tc_tf->cop_cfg.fop.tx_lim) {
		if (tc_tf->cop_cfg.fop.tt == 0) {
			/*E16*/
			switch (tc_tf->cop_cfg.fop.state) {
				case FOP_STATE_ACTIVE:
					ret = initiate_ad_retransmission(tc_tf);
					if (ret) {
						return UNDEF_ERROR;
					}
					notif = look_for_fdu(tc_tf);
					tc_tf->cop_cfg.fop.signal = notif;
					return notif;
				case FOP_STATE_RT_NO_WAIT:
					ret = initiate_ad_retransmission(tc_tf);
					if (ret) {
						return UNDEF_ERROR;
					}
					notif = look_for_fdu(tc_tf);
					tc_tf->cop_cfg.fop.signal = notif;
					return notif;
				case FOP_STATE_RT_WAIT:
					// Ignore
					return IGNORE;
				case FOP_STATE_INIT_NO_BC:
					// Alert
					ret = alert(tc_tf);
					if (ret) {
						return UNDEF_ERROR;
					}
					tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
					return ALERT_T1;
				case FOP_STATE_INIT_BC:
					ret = initiate_bc_retransmission(tc_tf);
					if (ret) {
						return UNDEF_ERROR;
					}
					notif = look_for_directive(tc_tf);
					tc_tf->cop_cfg.fop.signal = notif;
					return notif;
				case FOP_STATE_INIT:
					// NA
					return NA;
			}
		} else if (tc_tf->cop_cfg.fop.tt == 1) {
			/*E104*/
			switch (tc_tf->cop_cfg.fop.state) {
				case FOP_STATE_ACTIVE:
					ret = initiate_ad_retransmission(tc_tf);
					if (ret) {
						return UNDEF_ERROR;
					}
					notif = look_for_fdu(tc_tf);
					tc_tf->cop_cfg.fop.signal = notif;
					return notif;
				case FOP_STATE_RT_NO_WAIT:
					ret = initiate_ad_retransmission(tc_tf);
					if (ret) {
						return UNDEF_ERROR;
					}
					notif = look_for_fdu(tc_tf);
					tc_tf->cop_cfg.fop.signal = notif;
					return notif;
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
						return UNDEF_ERROR;
					}
					notif = look_for_directive(tc_tf);
					tc_tf->cop_cfg.fop.signal = notif;
					return notif;
				case FOP_STATE_INIT:
					// NA
					return NA;
			}
		}
	} else { // tx_cnt >= tx_lim
		if (tc_tf->cop_cfg.fop.tt == 0) {
			/*E17*/
			switch (tc_tf->cop_cfg.fop.state) {
				case FOP_STATE_ACTIVE:
					// Alert
					ret = alert(tc_tf);
					if (ret) {
						return UNDEF_ERROR;
					}
					tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
					return ALERT_T1;
				case FOP_STATE_RT_NO_WAIT:
					// Alert
					ret = alert(tc_tf);
					if (ret) {
						return UNDEF_ERROR;
					}
					tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
					return ALERT_T1;
				case FOP_STATE_RT_WAIT:
					// Alert
					ret = alert(tc_tf);
					if (ret) {
						return UNDEF_ERROR;
					}
					tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
					return ALERT_T1;
				case FOP_STATE_INIT_NO_BC:
					// Alert
					ret = alert(tc_tf);
					if (ret) {
						return UNDEF_ERROR;
					}
					tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
					return ALERT_T1;
				case FOP_STATE_INIT_BC:
					// Alert
					ret = alert(tc_tf);
					if (ret) {
						return UNDEF_ERROR;
					}
					tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
					return ALERT_T1;
				case FOP_STATE_INIT:
					// NA
					return NA;
			}
		} else if (tc_tf->cop_cfg.fop.tt == 1) {
			/*E18*/
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
						return UNDEF_ERROR;
					}
					tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
					return ALERT_T1;
				case FOP_STATE_INIT:
					// NA
					return NA;
			}
		}
	}
	return UNDEF_ERROR;
}

notification_t
req_transfer_fdu(struct tc_transfer_frame *tc_tf)
{
	int ret;
	notification_t notif;
	if (tc_tf->primary_hdr.bypass == TYPE_A) {
		if (wait_queue_empty(tc_tf->primary_hdr.vcid)) {
			/*E19*/
			switch (tc_tf->cop_cfg.fop.state) {
				case FOP_STATE_ACTIVE:
					ret = wait_queue_enqueue(tc_tf, tc_tf->primary_hdr.vcid);
					if (ret) {
						return UNDEF_ERROR;
					}
					notif = look_for_fdu(tc_tf);
					tc_tf->cop_cfg.fop.signal = notif;
					return notif;
				case FOP_STATE_RT_NO_WAIT:
					ret = wait_queue_enqueue(tc_tf, tc_tf->primary_hdr.vcid);
					if (ret) {
						return UNDEF_ERROR;
					}
					notif = look_for_fdu(tc_tf);
					tc_tf->cop_cfg.fop.signal = notif;
					return notif;
				case FOP_STATE_RT_WAIT:
					ret = wait_queue_enqueue(tc_tf, tc_tf->primary_hdr.vcid);
					if (ret) {
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
			}
		} else { // Wait queue not empty
			/*E20*/
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
			}
		}
	} else if (tc_tf->primary_hdr.bypass == TYPE_B
	           && tc_tf->primary_hdr.ctrl_cmd == TC_DATA) { // Type BD
		/*E21*/
		switch (tc_tf->cop_cfg.fop.state) {
			case FOP_STATE_ACTIVE:
				notif = transmit_type_bd(tc_tf);
				tc_tf->cop_cfg.fop.signal = notif;
				return notif;
			case FOP_STATE_RT_NO_WAIT:
				notif = transmit_type_bd(tc_tf);
				tc_tf->cop_cfg.fop.signal = notif;
				return notif;
			case FOP_STATE_RT_WAIT:
				notif = transmit_type_bd(tc_tf);
				tc_tf->cop_cfg.fop.signal = notif;
				return notif;
			case FOP_STATE_INIT_NO_BC:
				notif = transmit_type_bd(tc_tf);
				tc_tf->cop_cfg.fop.signal = notif;
				return notif;
			case FOP_STATE_INIT_BC:
				notif = transmit_type_bd(tc_tf);
				tc_tf->cop_cfg.fop.signal = notif;
				return notif;
			case FOP_STATE_INIT:
				notif = transmit_type_bd(tc_tf);
				tc_tf->cop_cfg.fop.signal = notif;
				return notif;
		}

	}
	return UNDEF_ERROR;
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
				return UNDEF_ERROR;
			}
			// confirm();
			tc_tf->cop_cfg.fop.state = FOP_STATE_ACTIVE;
			return POSITIVE_TX;
	}
	return UNDEF_ERROR;
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
				return UNDEF_ERROR;
			}
			timer_start(tc_tf->primary_hdr.vcid);
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT_NO_BC;
			tc_tf->cop_cfg.fop.signal = ACCEPT_TX;
			return ACCEPT_TX;
	}
	return UNDEF_ERROR;
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
				return UNDEF_ERROR;
			}
			ret = prepare_typeb_unlock(tc_tf);
			if (ret) {
				return UNDEF_ERROR;
			}
			ret = transmit_type_bc(tc_tf); // Unlock
			if (ret) {
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT_BC;
			return ACCEPT_DIR;
	}
	return UNDEF_ERROR;
}

notification_t
initiate_with_setvr(struct tc_transfer_frame *tc_tf, uint8_t new_vr)
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
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.vs = new_vr;
			tc_tf->cop_cfg.fop.nnr = new_vr;
			ret = prepare_typeb_setvr(tc_tf, new_vr);
			if (ret) {
				return UNDEF_ERROR;
			}
			ret = transmit_type_bc(tc_tf); // Set V(R)
			if (ret) {
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT_BC;
			return ACCEPT_DIR;
	}

	return UNDEF_ERROR;
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
				return UNDEF_ERROR;
			}
			//confirm();
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			return POSITIVE_DIR;
		case FOP_STATE_RT_NO_WAIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				return UNDEF_ERROR;
			}
			//confirm();
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			return POSITIVE_DIR;
		case FOP_STATE_RT_WAIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				return UNDEF_ERROR;
			}
			//confirm();
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			return POSITIVE_DIR;
		case FOP_STATE_INIT_NO_BC:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				return UNDEF_ERROR;
			}
			//confirm();
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			return POSITIVE_DIR;
		case FOP_STATE_INIT_BC:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				return UNDEF_ERROR;
			}
			//confirm();
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			return POSITIVE_DIR;
		case FOP_STATE_INIT:
			//confirm();
			return POSITIVE_DIR;
	}
	return UNDEF_ERROR;
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
					return UNDEF_ERROR;
				}
				//confirm();
				tc_tf->cop_cfg.fop.state = FOP_STATE_ACTIVE;
				return POSITIVE_DIR;
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
					return UNDEF_ERROR;
				}
				//confirm();
				tc_tf->cop_cfg.fop.state = FOP_STATE_RT_NO_WAIT;
				return POSITIVE_DIR;
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
					return UNDEF_ERROR;
				}
				//confirm();
				tc_tf->cop_cfg.fop.state = FOP_STATE_RT_WAIT;
				return POSITIVE_DIR;
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
					return UNDEF_ERROR;
				}
				//confirm();
				tc_tf->cop_cfg.fop.state = FOP_STATE_INIT_NO_BC;
				return POSITIVE_DIR;
		}
	}
	return UNDEF_ERROR;
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
				return POSITIVE_DIR;
			} else {
				// Reject
				return REJECT_DIR;
			}
	}
	return UNDEF_ERROR;
}

notification_t
set_sliding_window(struct tc_transfer_frame *tc_tf, uint8_t new_wnd)
{
	/*E36*/
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			tc_tf->cop_cfg.fop.slide_wnd = new_wnd;
			//confirm();
			return POSITIVE_DIR;
		case FOP_STATE_RT_NO_WAIT:
			tc_tf->cop_cfg.fop.slide_wnd = new_wnd;
			//confirm();
			return POSITIVE_DIR;
		case FOP_STATE_RT_WAIT:
			tc_tf->cop_cfg.fop.slide_wnd = new_wnd;
			//confirm();
			return POSITIVE_DIR;
		case FOP_STATE_INIT_NO_BC:
			tc_tf->cop_cfg.fop.slide_wnd = new_wnd;
			//confirm();
			return POSITIVE_DIR;
		case FOP_STATE_INIT_BC:
			tc_tf->cop_cfg.fop.slide_wnd = new_wnd;
			//confirm();
			return POSITIVE_DIR;
		case FOP_STATE_INIT:
			tc_tf->cop_cfg.fop.slide_wnd = new_wnd;
			//confirm();
			return POSITIVE_DIR;
	}
	return UNDEF_ERROR;
}

notification_t
set_timer(struct tc_transfer_frame *tc_tf, uint16_t new_t1)
{
	/*E37*/
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			tc_tf->cop_cfg.fop.t1_init = new_t1;
			//confirm();
			return POSITIVE_DIR;
		case FOP_STATE_RT_NO_WAIT:
			tc_tf->cop_cfg.fop.t1_init = new_t1;
			//confirm();
			return POSITIVE_DIR;
		case FOP_STATE_RT_WAIT:
			tc_tf->cop_cfg.fop.t1_init = new_t1;
			//confirm();
			return POSITIVE_DIR;
		case FOP_STATE_INIT_NO_BC:
			tc_tf->cop_cfg.fop.t1_init = new_t1;
			//confirm();
			return POSITIVE_DIR;
		case FOP_STATE_INIT_BC:
			tc_tf->cop_cfg.fop.t1_init = new_t1;
			//confirm();
			return POSITIVE_DIR;
		case FOP_STATE_INIT:
			tc_tf->cop_cfg.fop.t1_init = new_t1;
			//confirm();
			return POSITIVE_DIR;
	}
	return UNDEF_ERROR;
}

notification_t
set_tx_limit(struct tc_transfer_frame *tc_tf, uint8_t new_lim)
{
	/*E38*/
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			tc_tf->cop_cfg.fop.tx_lim = new_lim;
			//confirm();
			return POSITIVE_DIR;
		case FOP_STATE_RT_NO_WAIT:
			tc_tf->cop_cfg.fop.tx_lim = new_lim;
			//confirm();
			return POSITIVE_DIR;
		case FOP_STATE_RT_WAIT:
			tc_tf->cop_cfg.fop.tx_lim = new_lim;
			//confirm();
			return POSITIVE_DIR;
		case FOP_STATE_INIT_NO_BC:
			tc_tf->cop_cfg.fop.tx_lim = new_lim;
			//confirm();
			return POSITIVE_DIR;
		case FOP_STATE_INIT_BC:
			tc_tf->cop_cfg.fop.tx_lim = new_lim;
			//confirm();
			return POSITIVE_DIR;
		case FOP_STATE_INIT:
			tc_tf->cop_cfg.fop.tx_lim = new_lim;
			//confirm();
			return POSITIVE_DIR;
	}
	return UNDEF_ERROR;
}

notification_t
set_tt(struct tc_transfer_frame *tc_tf, uint8_t new_tt)
{
	/*E38*/
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			tc_tf->cop_cfg.fop.tt = new_tt;
			//confirm();
			return POSITIVE_DIR;
		case FOP_STATE_RT_NO_WAIT:
			tc_tf->cop_cfg.fop.tt = new_tt;
			//confirm();
			return POSITIVE_DIR;
		case FOP_STATE_RT_WAIT:
			tc_tf->cop_cfg.fop.tt = new_tt;
			//confirm();
			return POSITIVE_DIR;
		case FOP_STATE_INIT_NO_BC:
			tc_tf->cop_cfg.fop.tt = new_tt;
			//confirm();
			return POSITIVE_DIR;
		case FOP_STATE_INIT_BC:
			tc_tf->cop_cfg.fop.tt = new_tt;
			//confirm();
			return POSITIVE_DIR;
		case FOP_STATE_INIT:
			tc_tf->cop_cfg.fop.tt = new_tt;
			//confirm();
			return POSITIVE_DIR;
	}
	return UNDEF_ERROR;
}

notification_t
ad_accept(struct tc_transfer_frame *tc_tf)
{
	notification_t notif;
	/*E41*/
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			notif = look_for_fdu(tc_tf);
			tc_tf->cop_cfg.fop.signal = notif;
			return notif;
		case FOP_STATE_RT_NO_WAIT:
			notif = look_for_fdu(tc_tf);
			tc_tf->cop_cfg.fop.signal = notif;
			return notif;
		case FOP_STATE_RT_WAIT:
			tc_tf->cop_cfg.fop.signal = IGNORE;
			return IGNORE;
		case FOP_STATE_INIT_NO_BC:
			tc_tf->cop_cfg.fop.signal = IGNORE;
			return IGNORE;
		case FOP_STATE_INIT_BC:
			tc_tf->cop_cfg.fop.signal = IGNORE;
			return IGNORE;
		case FOP_STATE_INIT:
			tc_tf->cop_cfg.fop.signal = IGNORE;
			return IGNORE;
	}
	return UNDEF_ERROR;
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
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			return ALERT_LLIF;
		case FOP_STATE_RT_NO_WAIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			return ALERT_LLIF;
		case FOP_STATE_RT_WAIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			return ALERT_LLIF;
		case FOP_STATE_INIT_NO_BC:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			return ALERT_LLIF;
		case FOP_STATE_INIT_BC:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			return ALERT_LLIF;
		case FOP_STATE_INIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				return UNDEF_ERROR;
			}
			return ALERT_LLIF;
	}
	return UNDEF_ERROR;
}

notification_t
bc_accept(struct tc_transfer_frame *tc_tf)
{
	notification_t notif;
	/*E43*/
	switch (tc_tf->cop_cfg.fop.state) {
		case FOP_STATE_ACTIVE:
			tc_tf->cop_cfg.fop.signal = IGNORE;
			return IGNORE;
		case FOP_STATE_RT_NO_WAIT:
			tc_tf->cop_cfg.fop.signal = IGNORE;
			return IGNORE;
		case FOP_STATE_RT_WAIT:
			tc_tf->cop_cfg.fop.signal = IGNORE;
			return IGNORE;
		case FOP_STATE_INIT_NO_BC:
			tc_tf->cop_cfg.fop.signal = IGNORE;
			return IGNORE;
		case FOP_STATE_INIT_BC:
			notif = look_for_directive(tc_tf);
			tc_tf->cop_cfg.fop.signal = notif;
			return notif;
		case FOP_STATE_INIT:
			tc_tf->cop_cfg.fop.signal = IGNORE;
			return IGNORE;
	}
	return UNDEF_ERROR;
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
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			return ALERT_LLIF;
		case FOP_STATE_RT_NO_WAIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			return ALERT_LLIF;
		case FOP_STATE_RT_WAIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			return ALERT_LLIF;
		case FOP_STATE_INIT_NO_BC:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			return ALERT_LLIF;
		case FOP_STATE_INIT_BC:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			return ALERT_LLIF;
		case FOP_STATE_INIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				return UNDEF_ERROR;
			}
			return ALERT_LLIF;
	}
	return UNDEF_ERROR;
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
	}
	return UNDEF_ERROR;
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
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			return ALERT_LLIF;
		case FOP_STATE_RT_NO_WAIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			return ALERT_LLIF;
		case FOP_STATE_RT_WAIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			return ALERT_LLIF;
		case FOP_STATE_INIT_NO_BC:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			return ALERT_LLIF;
		case FOP_STATE_INIT_BC:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				return UNDEF_ERROR;
			}
			tc_tf->cop_cfg.fop.state = FOP_STATE_INIT;
			return ALERT_LLIF;
		case FOP_STATE_INIT:
			// Alert
			ret = alert(tc_tf);
			if (ret) {
				return UNDEF_ERROR;
			}
			return ALERT_LLIF;
	}
	return UNDEF_ERROR;
}
