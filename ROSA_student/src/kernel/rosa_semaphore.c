/*
 * rosa_semaphore.c
 *
 * Created: 12/2/2018 2:46:35 PM
 *  Author: haris
 */ 

#include <stdlib.h>
#include <stdint.h>

//Kernel includes
#include "kernel/rosa_def.h"
#include "kernel/rosa_ext.h"
#include "kernel/rosa_ker.h"
#include "kernel/rosa_tim.h"
#include "kernel/rosa_scheduler.h"
#include "kernel/rosa_semaphore.h"
#include "kernel/rosa_ext.h"

//Driver includes
#include "drivers/button.h"
#include "drivers/led.h"
#include "drivers/pot.h"
#include "drivers/usart.h"

/***********************************************************
 * Global variable that contain the list of Semaphores that
 * 	that are currently locked
 ***********************************************************/
ROSA_semaphoreHandle * LOCKEDSEMAPHORELIST; //list of pointers to the locked semaphores

/***********************************************************
 * MaxLockedCeiling
 *
 * Comment:
 * 	Returns a maximum ceiling of all currently locked semaphores
 *
 **********************************************************/
static uint8_t MaxLockedCeiling(void)
{
	if (LOCKEDSEMAPHORELIST==NULL)
	{
		return 0;
	}
	else
	{
	uint8_t maxCeil=LOCKEDSEMAPHORELIST->ceiling;
	ROSA_semaphoreHandle * it = LOCKEDSEMAPHORELIST;
	while (it->nextLockedSemaphore!=NULL)
	{
		if (it->ceiling>maxCeil)
		{
			maxCeil=it->ceiling;
		}
		it=it->nextLockedSemaphore;
	}
	
	return maxCeil;
	}
}

/***********************************************************
 * ROSA_semaphoreCreate
 *
 * Comment:
 * 	Create a semaphore
 *
 **********************************************************/
int16_t ROSA_semaphoreCreate(ROSA_semaphoreHandle_t * mutex, uint8_t ceiling) {
	**mutex = (ROSA_semaphoreHandle *) calloc(1, sizeof(***mutex));
	(**mutex)->holder = NULL;
	(**mutex)->ceiling = ceiling;
	(**mutex)->nextLockedSemaphore=NULL;	
	
	return 0;
}
/***********************************************************
 * ROSA_semaphoreDelete
 *
 * Comment:
 * 	Delete a semaphore, if deletion is successful return 0, otherwise -1
 *
 **********************************************************/
int16_t ROSA_semaphoreDelete(ROSA_semaphoreHandle_t mutex) {

	if ((*mutex)->holder == NULL) {					
		free(*mutex);
		free(mutex);
		mutex=NULL;
		return 0;
	}
	else {
		return - 1;
	}
}
/***********************************************************
 * ROSA_semaphorePeek
 *
 * Comment:
 * 	Check if semaphore is locked, return 1 if its unlocked, 0 otherwise
 *
 **********************************************************/
int16_t ROSA_semaphorePeek(ROSA_semaphoreHandle_t  mutex) {
	if (mutex == NULL) return -1;
	return ((*mutex)->holder == NULL) ? 1 : 0;
}
/***********************************************************
 * ROSA_semaphoreLock
 *
 * Comment:
 * 	Lock the semaphore, return nonnegative value if successful, otherwise return negative value
 *
 **********************************************************/
int16_t ROSA_semaphoreLock(ROSA_semaphoreHandle_t  mutex) {
	
	if (*mutex==NULL)
	{
		return -1; // if the passed pointer to the semaphore handle is non-existent
	}
	
	while ((*mutex)->holder != NULL || EXECTASK->priority<=MaxLockedCeiling()) //if the semaphore is already locked or IPCP condition P(task)>maxLockedCeil
		ROSA_yield();		
	
	(*mutex)->holder = EXECTASK;
	if (EXECTASK->priority < (*mutex)->ceiling)
	{
		readyQueueExtract(EXECTASK);
		EXECTASK->priority=(*mutex)->ceiling; //IPCP priority inheritance		
		readyQueueInsert(EXECTASK);
	}
		
	if (LOCKEDSEMAPHORELIST==NULL)
	{
		LOCKEDSEMAPHORELIST=mutex;
	}
	else
	{
		ROSA_semaphoreHandle * it; //finding the last semaphore in the list and changing its nextLockedSemaphore field to point to the just locked semaphore
		
		it = LOCKEDSEMAPHORELIST;
		
		while(it->nextLockedSemaphore!=NULL)
		{
			it=it->nextLockedSemaphore;
		}
		
		it->nextLockedSemaphore = *mutex;
	}
	
	return 0;
}
/***********************************************************
 * ROSA_semaphoreUnlock
 *
 * Comment:
 * 	Unlock the semaphore, return nonnegative value if successful, otherwise return negative value
 *
 **********************************************************/
int16_t ROSA_semaphoreUnlock(ROSA_semaphoreHandle_t  mutex) {
	(*mutex)->holder = NULL;
	if (*mutex == LOCKEDSEMAPHORELIST)
	{
		LOCKEDSEMAPHORELIST = (*mutex)->nextLockedSemaphore; //if first locked semaphore needs to be unlocked
	}
	else
	{
		ROSA_semaphoreHandle * it;
		it=LOCKEDSEMAPHORELIST;
		while (it->nextLockedSemaphore != *mutex) //find the locked semaphore before the one that needs to be unlocked
		{
			it=it->nextLockedSemaphore;
		}
		it->nextLockedSemaphore=(*mutex)->nextLockedSemaphore;
	}
	readyQueueExtract(EXECTASK);
	EXECTASK->priority=EXECTASK->originalPriority; //IPCP priority inheritance
	readyQueueInsert(EXECTASK);
	
	return 0;
}
