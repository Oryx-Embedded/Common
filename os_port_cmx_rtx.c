/**
 * @file os_port_cmx_rtx.c
 * @brief RTOS abstraction layer (CMX-RTX)
 *
 * @section License
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2010-2025 Oryx Embedded SARL. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.5.2
 **/

//Switch to the appropriate trace level
#define TRACE_LEVEL TRACE_LEVEL_OFF

//Dependencies
#include <stdio.h>
#include <stdlib.h>
#include "os_port.h"
#include "os_port_cmx_rtx.h"
#include "debug.h"

//Global variable
static bool_t semaphoreTable[OS_MAX_SEMAPHORES];

//Default task parameters
const OsTaskParameters OS_TASK_DEFAULT_PARAMS =
{
   NULL, //Function pointer
   NULL, //Stack
   256,  //Size of the stack
   1     //Task priority
};


/**
 * @brief Allocate a new semaphore identifier
 * @return Semaphore identifier
 **/

uint8_t osAllocateSemaphoreId(void)
{
   uint_t i;
   uint8_t id;

   //Initialize identifier
   id = OS_INVALID_SEMAPHORE_ID;

   //Enter critical section
   __disable_irq();

   //Loop through the allocation table
   for(i = 0; i < OS_MAX_SEMAPHORES && id == OS_INVALID_SEMAPHORE_ID; i++)
   {
      //Check whether the current ID is available for use
      if(semaphoreTable[i] == FALSE)
      {
         //Mark the entry as currently used
         semaphoreTable[i] = TRUE;
         //Return the current ID
         id = (uint8_t) i;
      }
   }

   //Exit critical section
   __enable_irq();

   //Return semaphore identifier
   return id;
}


/**
 * @brief Release semaphore identifier
 * @param[in] id Semaphore identifier to be released
 **/

void osFreeSemaphoreId(uint8_t id)
{
   //Check whether the semaphore identifier is valid
   if(id < OS_MAX_SEMAPHORES)
   {
      //Enter critical section
      __disable_irq();

      //Mark the entry as free
      semaphoreTable[id] = FALSE;

      //Exit critical section
      __enable_irq();
   }
}


/**
 * @brief Kernel initialization
 **/

void osInitKernel(void)
{
   //Clear semaphore ID allocation table
   osMemset(semaphoreTable, 0, sizeof(semaphoreTable));
   //Initialize the kernel
   K_OS_Init();
}


/**
 * @brief Start kernel
 **/

void osStartKernel(void)
{
   //Start the scheduler
   K_OS_Start();
}


/**
 * @brief Create a task
 * @param[in] name NULL-terminated string identifying the task
 * @param[in] taskCode Pointer to the task entry function
 * @param[in] arg Argument passed to the task function
 * @param[in] params Task parameters
 * @return Task identifier referencing the newly created task
 **/

OsTaskId osCreateTask(const char_t *name, OsTaskCode taskCode, void *arg,
   const OsTaskParameters *params)
{
   uint8_t status;
   uint8_t slot;

   //Valid function pointer?
   if(params->fp != NULL)
   {
      taskCode = (OsTaskCode) params->fp;
   }

   //Static allocation?
   if(params->stack != NULL)
   {
      //Create a new task
      status = K_Task_Create_Stack(params->priority, &slot, (CMX_FP) taskCode,
         params->stack + params->stackSize - 1);
   }
   else
   {
      //Create a new task
      status = K_Task_Create(params->priority, &slot, (CMX_FP) taskCode,
         params->stackSize * sizeof(uint32_t));
   }

#if 0
   //Check status code
   if(status == K_OK)
   {
      //Assign a name to the task
      status = K_Task_Name(slot, (char_t *) name);
   }
#endif

   //Check status code
   if(status == K_OK)
   {
      //Start the task
      status = K_Task_Start(slot);
   }

   //Check status code
   if(status == K_OK)
   {
      return (OsTaskId) slot;
   }
   else
   {
      return OS_INVALID_TASK_ID;
   }
}


/**
 * @brief Delete a task
 * @param[in] taskId Task identifier referencing the task to be deleted
 **/

void osDeleteTask(OsTaskId taskId)
{
   //Delete the specified task
   if(taskId == OS_SELF_TASK_ID)
   {
      K_Task_End();
   }
   else
   {
      K_Task_Delete(taskId);
   }
}


/**
 * @brief Delay routine
 * @param[in] delay Amount of time for which the calling task should block
 **/

void osDelayTask(systime_t delay)
{
   //Delay the task for the specified duration
   K_Task_Wait(OS_MS_TO_SYSTICKS(delay));
}


/**
 * @brief Yield control to the next task
 **/

void osSwitchTask(void)
{
   //Force a context switch
   K_Task_Coop_Sched();
}


/**
 * @brief Suspend scheduler activity
 **/

void osSuspendAllTasks(void)
{
   //Suspend all tasks
   K_Task_Lock();
}


/**
 * @brief Resume scheduler activity
 **/

void osResumeAllTasks(void)
{
   //Resume all tasks
   K_Task_Unlock();
}


/**
 * @brief Create an event object
 * @param[in] event Pointer to the event object
 * @return The function returns TRUE if the event object was successfully
 *   created. Otherwise, FALSE is returned
 **/

bool_t osCreateEvent(OsEvent *event)
{
   uint8_t status;

   //Allocate a new semaphore identifier
   event->id = osAllocateSemaphoreId();

   //Valid semaphore identifier?
   if(event->id != OS_INVALID_SEMAPHORE_ID)
   {
      //Create a semaphore object
      status = K_Semaphore_Create(event->id, 0);
   }
   else
   {
      //Failed to allocate a new identifier
      status = K_ERROR;
   }

   //Check whether the semaphore was successfully created
   if(status == K_OK)
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}


/**
 * @brief Delete an event object
 * @param[in] event Pointer to the event object
 **/

void osDeleteEvent(OsEvent *event)
{
   //Release semaphore identifier
   osFreeSemaphoreId(event->id);
}


/**
 * @brief Set the specified event object to the signaled state
 * @param[in] event Pointer to the event object
 **/

void osSetEvent(OsEvent *event)
{
   //Set the specified event to the signaled state
   K_Semaphore_Post(event->id);
}


/**
 * @brief Set the specified event object to the nonsignaled state
 * @param[in] event Pointer to the event object
 **/

void osResetEvent(OsEvent *event)
{
   uint8_t status;

   //Force the specified event to the nonsignaled state
   do
   {
      //Decrement the semaphore's count by one
      status = K_Semaphore_Get(event->id);

      //Check status
   } while(status == K_OK);
}


/**
 * @brief Wait until the specified event is in the signaled state
 * @param[in] event Pointer to the event object
 * @param[in] timeout Timeout interval
 * @return The function returns TRUE if the state of the specified object is
 *   signaled. FALSE is returned if the timeout interval elapsed
 **/

bool_t osWaitForEvent(OsEvent *event, systime_t timeout)
{
   uint8_t status;

   //Wait until the specified event is in the signaled state or the timeout
   //interval elapses
   if(timeout == 0)
   {
      //Non-blocking call
      status = K_Semaphore_Get(event->id);
   }
   else if(timeout == INFINITE_DELAY)
   {
      //Infinite timeout period
      status = K_Semaphore_Wait(event->id, 0);
   }
   else
   {
      //Wait for the specified time interval
      status = K_Semaphore_Wait(event->id, OS_MS_TO_SYSTICKS(timeout));
   }

   //Check whether the specified event is set
   if(status == K_OK)
   {
      //Force the event back to the nonsignaled state
      do
      {
         //Decrement the semaphore's count by one
         status = K_Semaphore_Get(event->id);

         //Check status
      } while(status == K_OK);

      //The specified event is in the signaled state
      return TRUE;
   }
   else
   {
      //The timeout interval elapsed
      return FALSE;
   }
}


/**
 * @brief Set an event object to the signaled state from an interrupt service routine
 * @param[in] event Pointer to the event object
 * @return TRUE if setting the event to signaled state caused a task to unblock
 *   and the unblocked task has a priority higher than the currently running task
 **/

bool_t osSetEventFromIsr(OsEvent *event)
{
   //Set the specified event to the signaled state
   K_Intrp_Semaphore_Post(event->id);

   //The return value is not relevant
   return FALSE;
}


/**
 * @brief Create a semaphore object
 * @param[in] semaphore Pointer to the semaphore object
 * @param[in] count The maximum count for the semaphore object. This value
 *   must be greater than zero
 * @return The function returns TRUE if the semaphore was successfully
 *   created. Otherwise, FALSE is returned
 **/

bool_t osCreateSemaphore(OsSemaphore *semaphore, uint_t count)
{
   uint8_t status;

   //Allocate a new semaphore identifier
   semaphore->id = osAllocateSemaphoreId();

   //Valid semaphore identifier?
   if(semaphore->id != OS_INVALID_SEMAPHORE_ID)
   {
      //Create a semaphore object
      status = K_Semaphore_Create(semaphore->id, count);
   }
   else
   {
      //Failed to allocate a new identifier
      status = K_ERROR;
   }

   //Check whether the semaphore was successfully created
   if(status == K_OK)
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}


/**
 * @brief Delete a semaphore object
 * @param[in] semaphore Pointer to the semaphore object
 **/

void osDeleteSemaphore(OsSemaphore *semaphore)
{
   //Release semaphore identifier
   osFreeSemaphoreId(semaphore->id);
}


/**
 * @brief Wait for the specified semaphore to be available
 * @param[in] semaphore Pointer to the semaphore object
 * @param[in] timeout Timeout interval
 * @return The function returns TRUE if the semaphore is available. FALSE is
 *   returned if the timeout interval elapsed
 **/

bool_t osWaitForSemaphore(OsSemaphore *semaphore, systime_t timeout)
{
   uint8_t status;

   //Wait until the semaphore is available or the timeout interval elapses
   if(timeout == 0)
   {
      //Non-blocking call
      status = K_Semaphore_Get(semaphore->id);
   }
   else if(timeout == INFINITE_DELAY)
   {
      //Infinite timeout period
      status = K_Semaphore_Wait(semaphore->id, 0);
   }
   else
   {
      //Wait for the specified time interval
      status = K_Semaphore_Wait(semaphore->id, OS_MS_TO_SYSTICKS(timeout));
   }

   //Check whether the specified semaphore is available
   if(status == K_OK)
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}


/**
 * @brief Release the specified semaphore object
 * @param[in] semaphore Pointer to the semaphore object
 **/

void osReleaseSemaphore(OsSemaphore *semaphore)
{
   //Release the semaphore
   K_Semaphore_Post(semaphore->id);
}


/**
 * @brief Create a mutex object
 * @param[in] mutex Pointer to the mutex object
 * @return The function returns TRUE if the mutex was successfully
 *   created. Otherwise, FALSE is returned
 **/

bool_t osCreateMutex(OsMutex *mutex)
{
   uint8_t status;

   //Allocate a new semaphore identifier
   mutex->id = osAllocateSemaphoreId();

   //Valid semaphore identifier?
   if(mutex->id != OS_INVALID_SEMAPHORE_ID)
   {
      //Create a semaphore object
      status = K_Semaphore_Create(mutex->id, 0);
   }
   else
   {
      //Failed to allocate a new identifier
      status = K_ERROR;
   }

   //Check status code
   if(status == K_OK)
   {
      //Release the semaphore
      status = K_Semaphore_Post(mutex->id);
   }

   //Check whether the semaphore was successfully created
   if(status == K_OK)
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}


/**
 * @brief Delete a mutex object
 * @param[in] mutex Pointer to the mutex object
 **/

void osDeleteMutex(OsMutex *mutex)
{
   //Release semaphore identifier
   osFreeSemaphoreId(mutex->id);
}


/**
 * @brief Acquire ownership of the specified mutex object
 * @param[in] mutex Pointer to the mutex object
 **/

void osAcquireMutex(OsMutex *mutex)
{
   //Obtain ownership of the mutex object
   K_Semaphore_Wait(mutex->id, 0);
}


/**
 * @brief Release ownership of the specified mutex object
 * @param[in] mutex Pointer to the mutex object
 **/

void osReleaseMutex(OsMutex *mutex)
{
   //Release ownership of the mutex object
   K_Semaphore_Post(mutex->id);
}


/**
 * @brief Retrieve system time
 * @return Number of milliseconds elapsed since the system was last started
 **/

systime_t osGetSystemTime(void)
{
   systime_t time;

   //Get current tick count
   time = K_OS_Tick_Get_Ctr();

   //Convert system ticks to milliseconds
   return OS_SYSTICKS_TO_MS(time);
}


/**
 * @brief Allocate a memory block
 * @param[in] size Bytes to allocate
 * @return A pointer to the allocated memory block or NULL if
 *   there is insufficient memory available
 **/

__weak_func void *osAllocMem(size_t size)
{
   void *p;

   //Enter critical section
   osSuspendAllTasks();
   //Allocate a memory block
   p = malloc(size);
   //Leave critical section
   osResumeAllTasks();

   //Debug message
   TRACE_DEBUG("Allocating %" PRIuSIZE " bytes at 0x%08" PRIXPTR "\r\n",
      size, (uintptr_t) p);

   //Return a pointer to the newly allocated memory block
   return p;
}


/**
 * @brief Release a previously allocated memory block
 * @param[in] p Previously allocated memory block to be freed
 **/

__weak_func void osFreeMem(void *p)
{
   //Make sure the pointer is valid
   if(p != NULL)
   {
      //Debug message
      TRACE_DEBUG("Freeing memory at 0x%08" PRIXPTR "\r\n", (uintptr_t) p);

      //Enter critical section
      osSuspendAllTasks();
      //Free memory block
      free(p);
      //Leave critical section
      osResumeAllTasks();
   }
}
