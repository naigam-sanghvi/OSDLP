
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
