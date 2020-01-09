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

#ifndef INC_QUEUE_UTIL_H_
#define INC_QUEUE_UTIL_H_

#include <stdbool.h>
#include <stdint.h>

struct queue {
	uint8_t 	*mem_space;
	uint16_t 	capacity;
	uint16_t 	head;
	uint16_t 	tail;
	uint16_t	inqueue;
	uint16_t	item_size;
};

int
init(struct queue *handle,
     uint16_t item_size,
     uint16_t num_items);

int
enqueue(struct queue *que, void *pkt);

int
enqueue_now(struct queue *que, void *pkt);


int
dequeue(struct queue *que, void *pkt);


int
reset_queue(struct queue *que);


uint16_t
size(struct queue *que);


bool
full(struct queue *que);


uint8_t *
front(struct queue *que);

uint8_t *
back(struct queue *que);

uint8_t *
get_element(struct queue *que, uint16_t pos);

#endif /* INC_QUEUE_UTIL_H_ */
