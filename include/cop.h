/*
 *  Open Space Data Link Protocol
 *
 *  Copyright (C) 2019 Libre Space Foundation (https://libre.space)
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

#ifndef OSDLP_INC_COP_H_
#define OSDLP_INC_COP_H_

#include <stdint.h>
#include <stdbool.h>

#include "tc.h"

#define		UNLOCK_CMD		0
#define		SETVR_BYTE1		0x82
#define		SETVR_BYTE2		0

typedef enum {
	STATE_OPEN 		= 0,
	STATE_WAIT 		= 1,
	STATE_LOCKOUT 	= 2
} farm_state_t;

typedef enum {
	COP_OK 				= 0,
	COP_ENQ				= 1,
	COP_PRIORITY_ENQ 	= 2,
	COP_DISCARD 		= 3,
	COP_ERROR			= 4
} farm_result_t;

typedef enum {
	RT_FLAG_OFF = 0,
	RT_FLAG_ON  = 1
} rt_flag_t;

typedef enum {
	ACCEPT_DIR		= 0,
	REJECT_DIR		= 1,
	POSITIVE_DIR	= 2,
	NEGATIVE_DIR	= 3,
	SUSPEND			= 4,
	ALERT_LIMIT		= 5,
	ALERT_T1		= 6,
	ALERT_LOCKOUT	= 7,
	ALERT_SYNCH		= 8,
	ALERT_NNR		= 9,
	ALERT_CLCW		= 10,
	ALERT_LLIF		= 11,
	ALERT_TERM		= 12,
	ACCEPT_TX		= 13,
	REJECT_TX		= 14,
	POSITIVE_TX		= 15,
	NEGATIVE_TX		= 16,
	DELAY_RESP		= 17,
	UNDEF_ERROR		= 18,
	IGNORE			= 19,
	NA				= 20
} notification_t;

struct farm_vars {
	uint8_t		state;
	uint8_t		lockout;
	uint8_t		wait;
	uint8_t		retransmit;
	uint8_t		vr;
	uint8_t		farmb_cnt;
	uint8_t		w;
	uint8_t		pw;
	uint8_t		nw;
};

typedef enum {
	STATE_ACTIVE 		= 0,
	STATE_RT_NO_WAIT 	= 1,
	STATE_RT_WAIT		= 2,
	STATE_INIT_NO_BC	= 3,
	STATE_INIT_BC		= 4,
	STATE_INIT			= 5
} fop_state_t;

typedef enum {
	OUT_READY		= 0,
	OUT_NOT_READY	= 1
} out_flag_t;

struct fop_vars {
	uint8_t		state;
	uint8_t		vs;
	uint8_t		tobe_rt;
	uint8_t		out_flag;
	uint8_t		nnr;
	uint16_t	t1_init;
	uint8_t		tx_lim;
	uint8_t		tx_cnt;
	uint8_t		slide_wnd;
	uint8_t		tt;
	uint8_t		ss;
	uint8_t		accept_flag;
};

struct cop_params {
	struct fop_vars		fop;
//	union{
//		struct fop_vars		fop;
//		struct farm_vars 	farm;
//	};
};


#endif /* OSDLP_INC_COP_H_ */
