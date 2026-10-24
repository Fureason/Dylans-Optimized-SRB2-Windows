// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2003      by James Haley
// Copyright (C) 2003-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#include <stdlib.h>

#include "z_zone.h"
#include "m_queue.h"
#include "m_misc.h"







void M_QueueInit(mqueue_t *queue)
{
	queue->head.next = NULL;
	queue->tail = &(queue->head);
	queue->rover = &(queue->head);
}






void M_QueueInsert(mqueueitem_t *item, mqueue_t *queue)
{

	queue->tail = queue->tail->next = item;
}








mqueueitem_t *M_QueueIterator(mqueue_t *queue)
{
	if (queue->rover == NULL)
		return NULL;

	return (queue->rover = queue->rover->next);
}






void M_QueueResetIterator(mqueue_t *queue)
{
	queue->rover = &(queue->head);
}






void M_QueueFree(mqueue_t *queue)
{
	mqueueitem_t *rover = queue->head.next;

	while (rover)
	{
		mqueueitem_t *next = rover->next;
		free(rover);

		rover = next;
	}

	M_QueueInit(queue);
}


