/*
	From the 'Wizard2' engine by Spaddlewit Inc. ( http://www.spaddlewit.com )
	An experimental work-in-progress.

	Donated to Sonic Team Junior and adapted to work with
	Sonic Robo Blast 2. The license of this code matches whatever
	the licensing is for Sonic Robo Blast 2.
*/

#include "u_list.h"
#include "z_zone.h"














void ListAdd(void *pItem, listitem_t **itemHead)
{
	listitem_t *item = (listitem_t*)pItem;

	if (*itemHead == NULL)
	{
		*itemHead = item;
		(*itemHead)->prev = (*itemHead)->next = NULL;
	}
	else
	{
		listitem_t *tail;
		tail = *itemHead;

		while (tail->next != NULL)
			tail = tail->next;

		tail->next = item;

		tail->next->prev = tail;

		item->next = NULL;
	}
}







void ListAddFront(void *pItem, listitem_t **itemHead)
{
	listitem_t *item = (listitem_t*)pItem;

	if (*itemHead == NULL)
	{
		*itemHead = item;
		(*itemHead)->prev = (*itemHead)->next = NULL;
	}
	else
	{
		(*itemHead)->prev = item;
		item->next = (*itemHead);
		item->prev = NULL;
		*itemHead = item;
	}
}






void ListAddBefore(void *pItem, void *pSpot, listitem_t **itemHead)
{
	listitem_t *item = (listitem_t*)pItem;
	listitem_t *spot = (listitem_t*)pSpot;

	listitem_t *prev = spot->prev;

	if (!prev)
		ListAddFront(pItem, itemHead);
	else
	{
		item->next = spot;
		spot->prev = item;
		item->prev = prev;
		prev->next = item;
	}
}






void ListAddAfter(void *pItem, void *pSpot, listitem_t **itemHead)
{
	listitem_t *item = (listitem_t*)pItem;
	listitem_t *spot = (listitem_t*)pSpot;

	listitem_t *next = spot->next;

	if (!next)
		ListAdd(pItem, itemHead);
	else
	{
		item->prev = spot;
		spot->next = item;
		item->next = next;
		next->prev = item;
	}
}






void ListRemove(void *pItem, listitem_t **itemHead)
{
	listitem_t *item = (listitem_t*)pItem;

	if (item == *itemHead)  
	{
		*itemHead = item->next;

		if (*itemHead)
			(*itemHead)->prev = NULL;
	}
	else if (item->next == NULL)  
	{
		item->prev->next = NULL;
	}
	else  
	{
		item->prev->next = item->next;
		item->next->prev = item->prev;
	}

	Z_Free (item);
}






void ListRemoveAll(listitem_t **itemHead)
{
	listitem_t *item;
	listitem_t *next;
	for (item = *itemHead; item; item = next)
	{
		next = item->next;
		ListRemove(item, itemHead);
	}
}






void ListRemoveNoFree(void *pItem, listitem_t **itemHead)
{
	listitem_t *item = (listitem_t*)pItem;

	if (item == *itemHead)  
	{
		*itemHead = item->next;

		if (*itemHead)
			(*itemHead)->prev = NULL;
	}
	else if (item->next == NULL)  
	{
		item->prev->next = NULL;
	}
	else  
	{
		item->prev->next = item->next;
		item->next->prev = item->prev;
	}
}







unsigned int ListGetCount(void *itemHead)
{
	listitem_t *item = (listitem_t*)itemHead;

	unsigned int count = 0;
	for (; item; item = item->next)
		count++;

	return count;
}







listitem_t *ListGetByIndex(void *itemHead, unsigned int index)
{
	listitem_t *head = (listitem_t*)itemHead;
	unsigned int count = 0;
	listitem_t *node;
	for (node = head; node; node = node->next)
	{
		if (count == index)
			return node;

		count++;
	}

	return NULL;
}
