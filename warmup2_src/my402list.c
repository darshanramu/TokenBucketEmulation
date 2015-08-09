/******************************************************************************/
/* This file is used to create circular doubly linked list.                    */
/******************************************************************************/
/*
 * Author: Darshan V Ramu (darshanv@usc.edu)
 *:
 * @(#)$Id: my402list.c, v1.0 2015/01/27 $
 */

#include<stdio.h>
#include<stdlib.h>
#include "my402list.h"

int  My402ListLength(My402List *list)
{
if(list==NULL)
	return 0;
return list->num_members;
}

//Retuns True if list is empty
int  My402ListEmpty(My402List *list)
{
if(list==NULL)
	return TRUE;
if(list->num_members == 0 || (list->anchor.next==NULL && list->anchor.prev==NULL)){
	return TRUE;
}
else 
	return FALSE;
}

int  My402ListAppend(My402List *list, void *obj)
{
if(list==NULL)
	return FALSE;
//the new elemenet to be appended
My402ListElem *listelem = NULL;
listelem = (My402ListElem *) malloc( sizeof(My402ListElem) );
if(listelem == NULL)
	return FALSE;
//if list is empty just alter anchor ptrs
if(list->num_members == 0){
	list->anchor.next = listelem;
	list->anchor.prev = listelem;
	listelem->obj = (void*) obj;
	//store anchor address as it is not a ptr type
        listelem->next = &(list->anchor);
        listelem->prev = &(list->anchor);
        list->num_members++;
}
//Just append the new element to the last element
else{
	My402ListElem *lastelem = My402ListLast(list);
	if (lastelem != NULL){
		lastelem->next = listelem;
		list->anchor.prev = listelem;
		listelem->prev = lastelem;
		listelem->next = &(list->anchor);
		listelem->obj = (void*) obj;
		list->num_members++;
	}
}
return TRUE;
}

int  My402ListPrepend(My402List *list, void *obj)
{
if(list==NULL)
	return FALSE;
//the new elemenet to be prepended
My402ListElem *listelem = NULL;
listelem = (My402ListElem *) malloc( sizeof(My402ListElem) );
if(listelem == NULL)
	return FALSE;
//if list is empty just alter anchor ptrs
if(list->num_members == 0){
	list->anchor.next = listelem;
	list->anchor.prev = listelem;
	listelem->obj = (void*) obj;
	//store anchor address as it is not a ptr type
        listelem->next = &(list->anchor);
        listelem->prev = &(list->anchor);
        list->num_members++;
}
//Just prepend the new element to the first element
else{
	My402ListElem *firstelem = My402ListFirst(list);
	if (firstelem != NULL){
		firstelem->prev = listelem;
		list->anchor.next = listelem;
		listelem->prev = &(list->anchor);
		listelem->next = firstelem;
		listelem->obj = (void*) obj;
		list->num_members++;
	}
}
return TRUE;
}

//Insert after a given node
int  My402ListInsertAfter(My402List *list, void *obj, My402ListElem *givenelem)
{
if(list==NULL || givenelem==NULL)
	return FALSE;
My402ListElem *listelem = NULL;
listelem = (My402ListElem *) malloc( sizeof(My402ListElem) );
listelem->obj = obj;
listelem->next = givenelem->next;
listelem->prev = givenelem;
givenelem->next = listelem;
listelem->next->prev = listelem;
list->num_members++;
return TRUE;
}

//Insert before a given node
int  My402ListInsertBefore(My402List *list, void *obj, My402ListElem *givenelem)
{
if(list==NULL || givenelem==NULL)
	return FALSE;
My402ListElem *listelem = NULL;
listelem = (My402ListElem *) malloc( sizeof(My402ListElem) );
if(listelem==NULL)
	return FALSE;
listelem->obj = obj;
listelem->next = givenelem;
listelem->prev = givenelem->prev;
givenelem->prev = listelem;
listelem->prev->next = listelem;
list->num_members++;
return TRUE;
}

My402ListElem* My402ListFirst(My402List *list)
{
if(list==NULL)
	return NULL;
return list->anchor.next;
}

My402ListElem* My402ListLast(My402List *list)
{
if(list==NULL)
	return NULL;
return list->anchor.prev;
}

My402ListElem* My402ListFind(My402List *list, void *obj)
{
if(list==NULL)
	return NULL;
My402ListElem *listelem = &(list->anchor);
while (listelem->obj != obj){
	listelem = listelem->next;
}
if (listelem->obj == obj)
return listelem;
return NULL;
}

My402ListElem *My402ListNext(My402List *list, My402ListElem *listelem)
{
if(list==NULL || listelem==NULL)
	return NULL;
if(listelem == list->anchor.prev)
	return NULL;
return listelem->next;
}

My402ListElem *My402ListPrev(My402List *list, My402ListElem *listelem)
{
if(list==NULL || listelem==NULL)
	return NULL;
if(listelem == list->anchor.next)
	return NULL;
return listelem->prev;
}

//Only place i am decrementing num_members
void My402ListUnlink(My402List *list, My402ListElem *listelem)
{
if(&(list->anchor) == listelem)
	return;
else if(listelem!=NULL){
	listelem->next->prev=listelem->prev;
	listelem->prev->next=listelem->next;
	free(listelem);
	list->num_members--;
	if(list->num_members==0){
		list->anchor.next=NULL;
		list->anchor.prev=NULL;
	}
}
}

//Keep on deleting the node next to anchor
void My402ListUnlinkAll(My402List *list)
{
if(list==NULL)
	return;
My402ListElem *listelem;
while(list->anchor.next!=NULL && list->anchor.prev!=NULL)
{
listelem = list->anchor.next;
My402ListUnlink(list, listelem);
}
}

int My402ListInit(My402List *list)
{
if(list==NULL)
	return FALSE;
list->num_members = 0;
//setup anchor node
list->anchor.next = NULL;
list->anchor.prev = NULL;
list->anchor.obj = NULL;
return TRUE;
}
