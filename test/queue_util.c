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

#include <queue_util.h>
#include <stdlib.h>
#include <string.h>
#include <tc.h>
#include <cop.h>

int
init(struct queue *handle,
     uint16_t item_size,
     uint16_t num_items)
{
	handle->capacity = num_items;
	handle->head = 0;
	handle->inqueue = 0;
	handle->tail = 0;
	handle->item_size = item_size;
	handle->mem_space = (uint8_t *)malloc(num_items * item_size * sizeof(uint8_t));
	if (handle->mem_space == NULL) {
		return 1;
	}
	return 0;
}

int
enqueue(struct queue *que, void *pkt)
{
	if (que->inqueue < que->capacity) {
		memcpy(que->mem_space + (que->tail * que->item_size), (uint8_t *)pkt,
		       que->item_size * sizeof(uint8_t));
		que->tail++;
		if (que->tail >= que->capacity) {
			que->tail = 0;
		}
		que->inqueue++;
		return 0;
	} else {
		return 1;
	}
}

int
enqueue_now(struct queue *que, void *pkt)
{
	if (que->inqueue < que->capacity) {
		memcpy(que->mem_space + (que->tail * que->item_size), (uint8_t *)pkt,
		       que->item_size * sizeof(uint8_t));
		que->tail++;
		if (que->tail >= que->capacity) {
			que->tail = 0;
		}
		que->inqueue++;
		return 0;
	} else { // Overwrite back
		memcpy(que->mem_space + (que->tail * que->item_size), (uint8_t *)pkt,
		       que->item_size * sizeof(uint8_t));
		return 0;
	}
}

int
dequeue(struct queue *que, void *pkt)
{
	if (que->inqueue > 0) {
		memcpy((uint8_t *)pkt, que->mem_space + (que->head * que->item_size),
		       que->item_size * sizeof(uint8_t));
		que->head++;
		if (que->head >= que->capacity) {
			que->head = 0;
		}
		que->inqueue--;
		return 0;
	} else {
		return 1;
	}
}


int
reset_queue(struct queue *que)
{
	que->tail = 0;
	que->head = 0;
	que->inqueue = 0;
	return 0;
}


uint16_t
size(struct queue *que)
{
	return que->inqueue;
}


bool
full(struct queue *que)
{
	return (que->capacity - que->inqueue) ? false : true;
}


uint8_t *
front(struct queue *que)
{
	if (que->tail == que->head) {
		return NULL;
	}
	return que->mem_space + (que->head * que->item_size);
}


uint8_t *
back(struct queue *que)
{
	if (que->tail == que->head) {
		return NULL;
	}
	if (que->tail > 0) {
		return que->mem_space + ((que->tail - 1) * que->item_size);
	} else {
		return que->mem_space + ((que->capacity - 1) * que->item_size);
	}
}

uint8_t *
get_element(struct queue *que, uint16_t pos)
{
	if (pos >= que->capacity || pos >= que->inqueue) {
		return NULL;
	}
	if (que->head + pos < que->capacity) {
		return que->mem_space + (que->head + pos * que->item_size);
	} else {
		return que->mem_space + ((que->head + pos - que->capacity) * que->item_size);
	}
}

