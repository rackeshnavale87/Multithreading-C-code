#include <stdio.h>
#include <stdlib.h>
#include"my402list.h" //external private header file

int  My402ListLength(My402List* list) {
	return list->num_members;
}
int My402ListInit(My402List* list) {
	(list->anchor).next = &(list->anchor);
	(list->anchor).prev = &(list->anchor);
	(list->anchor).obj=NULL;
	list->num_members = 0;
return 1;
}
int  My402ListEmpty(My402List* list) {
	if(list->num_members==0)
		return TRUE;
	else 
		return FALSE;
}
//-----------------------------------------------------------------------------------------
int  My402ListAppend(My402List* list, void* vptr) {
	My402ListElem *temp=NULL;
	temp = (My402ListElem *)malloc(sizeof(My402ListElem));
	temp->obj=vptr;
	if(My402ListEmpty(list)) {
		(list->anchor).next = temp;
		(list->anchor).prev = temp;
		temp->next = &(list->anchor);
		temp->prev = &(list->anchor);
		list->num_members++;
		return TRUE;
	}
	else {
		temp->next = &list->anchor;
		temp->prev = list->anchor.prev;
		((list->anchor).prev)->next = temp;
		(list->anchor).prev = temp;
		list->num_members++;
		return TRUE;
	}
return FALSE;
}
int  My402ListPrepend(My402List* list, void* vptr) {
	My402ListElem *temp=NULL;
	temp = (My402ListElem *)malloc(sizeof(My402ListElem));
	temp->obj=vptr;
	if(list->num_members==0) {
		(list->anchor).next = temp;
		(list->anchor).prev = temp;
		temp->next = &list->anchor;
		temp->prev = &list->anchor;
		list->num_members++;
		return TRUE;
	}
	else {
		temp->next = (list->anchor).next;
		((list->anchor).next)->prev = temp;
		(list->anchor).next = temp;
		temp->prev = &list->anchor;
		list->num_members++;
		return TRUE;
	}
return FALSE;
}
//-----------------------------------------------------------------------------------------
void My402ListUnlink(My402List* list, My402ListElem* listelem) {
if (list->num_members > 1) {
	(listelem->next)->prev = listelem->prev;
	(listelem->prev)->next = listelem->next;
	listelem->next=NULL;	listelem->prev=NULL;	listelem->obj=NULL;
	list->num_members--;
	free(listelem);//free memory
}
else
	(void)My402ListInit(list);
}
void My402ListUnlinkAll(My402List* list) {
	int totalelem = list->num_members,n;
	My402ListElem *elem = My402ListLast(list),*temp;
	for (n=1 ; n<=totalelem ; n++) {
		temp = My402ListPrev(list,elem);
		My402ListUnlink(list,elem);
		elem = temp;
	}
	My402ListUnlink(list,My402ListFirst(list));
}
//-----------------------------------------------------------------------------------------
int  My402ListInsertAfter(My402List* list, void* vptr, My402ListElem* listelem) {
My402ListElem *elem=NULL; My402ListElem *temp=NULL;
elem = (My402ListElem *)malloc(sizeof(My402ListElem)); temp = (My402ListElem *)malloc(sizeof(My402ListElem));
	for (temp=My402ListFirst(list); temp != NULL; temp=My402ListNext(list, temp)) {
		if(listelem == temp) {
			elem->obj=vptr;
			elem->next = listelem->next;
			(listelem->next)->prev = elem;
			listelem->next = elem;
			elem->prev = listelem;
			return list->num_members++;
		}
	}
free(temp); return 0;
}
int  My402ListInsertBefore(My402List* list, void* vptr, My402ListElem* listelem) {
My402ListElem *elem=NULL; My402ListElem *temp=NULL;
elem = (My402ListElem *)malloc(sizeof(My402ListElem)); temp = (My402ListElem *)malloc(sizeof(My402ListElem));
	for (temp=My402ListFirst(list); temp != NULL; temp=My402ListNext(list, temp)) {
		if(listelem == temp) {
			elem->obj=vptr;
			elem->next = listelem;
			elem->prev = listelem->prev;
			(listelem->prev)->next = elem;
			listelem->prev = elem;
			return list->num_members++;
		}
	} free(temp);
return 0;
}
//-----------------------------------------------------------------------------------------
My402ListElem *My402ListFirst(My402List* list) {
if (My402ListLength(list) >0)
	return list->anchor.next;
else 
	return NULL;
}
My402ListElem *My402ListLast(My402List* list) {
if (My402ListLength(list) >0)
	return list->anchor.prev;
else 
	return NULL;
}
My402ListElem *My402ListNext(My402List* list, My402ListElem* listelem) {
My402ListElem *elem=NULL; int totalelem = list->num_members,n=0; elem=My402ListFirst(list);
	for (n=0;n<totalelem;n++) {
		if(elem->next==&list->anchor || list->num_members ==0 || list->num_members ==0)
			return NULL;
		if(elem==listelem)	
			return elem->next;
		elem=elem->next;
	}
return NULL;
}
My402ListElem *My402ListPrev(My402List* list, My402ListElem* listelem) {
My402ListElem *elem=NULL; int totalelem = list->num_members,n=0; elem=My402ListLast(list);
	for (n=0;n<totalelem;n++) {
		if(elem->prev==&list->anchor || list->num_members ==0 || list->num_members ==0)
			return NULL;
		if(elem==listelem)
			return elem->prev;
		elem=elem->prev;
	}
return NULL;
}
//-----------------------------------------------------------------------------------------
My402ListElem *My402ListFind(My402List* list, void* vptr) {
My402ListElem *elem=NULL;
	for (elem=My402ListFirst(list); elem != NULL; elem=My402ListNext(list, elem)) {
		if(elem->obj == vptr) {	return elem; }
	} 
return NULL;
}
