// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2005      by James Haley
// Copyright (C) 2005-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------















#ifndef M_DLLIST_H__
#define M_DLLIST_H__

#ifdef _MSC_VER
#pragma warning(disable : 4706)
#endif

typedef struct mdllistitem_s
{
	struct mdllistitem_s *next;
	struct mdllistitem_s **prev;
} mdllistitem_t;

FUNCINLINE static ATTRINLINE void M_DLListInsert(mdllistitem_t *item, mdllistitem_t **head)
{
	mdllistitem_t *next = *head;

	if ((item->next = next))
		next->prev = &item->next;
	item->prev = head;
	*head = item;
}

FUNCINLINE static ATTRINLINE void M_DLListRemove(mdllistitem_t *item)
{
	mdllistitem_t **prev = item->prev;
	mdllistitem_t *next  = item->next;

	if ((*prev = next))
		next->prev = prev;
}

#endif


