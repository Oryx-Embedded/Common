/**
 * @file os_port_px5.c
 * @brief RTOS abstraction layer (PX5)
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
#include "os_port_px5.h"
#include "debug.h"

//Pthread start routine
typedef void *(*PthreadTaskCode) (void *param);

//Default task parameters
const OsTaskParameters OS_TASK_DEFAULT_PARAMS =
{
   NULL, //Stack
   0,    //Size of the stack
   0     //Task priority
};

//Forward declaration of functions
void *osAllocMemCallback(u_int type, u_long size);
void osFreeMemCallback(u_int type, void *p);


/**
 * @brief Kernel initialization
 **/

void osInitKernel(void)
{
   //Start RTOS
   px5_pthread_start(1, NULL, 0);

   //Setup the memory manager allocate and release memory routines
   px5_pthread_memory_manager_set(osAllocMemCallback, osFreeMemCallback);
}


/**
 * @brief Start kernel
 **/

void osStartKernel(void)
{
   //Not implemented
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
   int_t ret;
   px5_pthread_t thread;
   px5_pthread_attr_t threadAttr;

   //Set thread attributes
   ret = pthread_attr_init(&threadAttr);

   //Check status code
   if(ret == PX5_SUCCESS)
   {
      //Static allocation?
      if(params->stack != NULL)
      {
         //Specify the stack address
         ret = px5_pthread_attr_setstackaddr(&threadAttr, params->stack);
      }
   }

   //Check status code
   if(ret == PX5_SUCCESS)
   {
      //Specify the size of the stack
      ret = px5_pthread_attr_setstacksize(&threadAttr,
         params->stackSize * sizeof(uint32_t));
   }

   //Check status code
   if(ret == PX5_SUCCESS)
   {
      //Specify the priority of the task
      ret = px5_pthread_attr_setpriority(&threadAttr, params->priority);
   }

   //Create a new thread
   ret = px5_pthread_create(&thread, &threadAttr, (PthreadTaskCode) taskCode,
      arg);

   //Return a pointer to the newly created thread
   if(ret == PX5_SUCCESS)
   {
      return (OsTaskId) thread;
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
   //Delete the calling thread?
   if(taskId == OS_SELF_TASK_ID)
   {
      //Kill ourselves
      px5_pthread_exit(NULL);
   }
   else
   {
      //Delete the specified thread
      pthread_cancel(taskId);
   }
}


/**
 * @brief Delay routine
 * @param[in] delay Amount of time for which the calling task should block
 **/

void osDelayTask(systime_t delay)
{
   //Delay the task for the specified duration
   px5_pthread_tick_sleep(OS_MS_TO_SYSTICKS(delay));
}


/**
 * @brief Yield control to the next task
 **/

void osSwitchTask(void)
{
   //Force a context switch
   px5_sched_yield();
}


/**
 * @brief Suspend scheduler activity
 **/

void osSuspendAllTasks(void)
{
   //Not implemented
}


/**
 * @brief Resume scheduler activity
 **/

void osResumeAllTasks(void)
{
   //Not implemented
}


/**
 * @brief Create an event object
 * @param[in] event Pointer to the event object
 * @return The function returns TRUE if the event object was successfully
 *   created. Otherwise, FALSE is returned
 **/

bool_t osCreateEvent(OsEvent *event)
{
   int_t ret;

   //Create a semaphore object
   ret = px5_sem_init(event, 0, 0);

   //Check whether the semaphore was successfully created
   if(ret == PX5_SUCCESS)
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
   //Properly dispose the event object
   px5_sem_destroy(event);
}


/**
 * @brief Set the specified event object to the signaled state
 * @param[in] event Pointer to the event object
 **/

void osSetEvent(OsEvent *event)
{
   //Set the specified event to the signaled state
   px5_sem_post(event);
}


/**
 * @brief Set the specified event object to the nonsignaled state
 * @param[in] event Pointer to the event object
 **/

void osResetEvent(OsEvent *event)
{
   int_t ret;

   //Force the specified event to the nonsignaled state
   do
   {
      //Decrement the semaphore's count by one
      ret = px5_sem_trywait(event);

      //Check status
   } while(ret == PX5_SUCCESS);
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
   int_t ret;

   //Wait until the specified event is in the signaled state or the timeout
   //interval elapses
   if(timeout == 0)
   {
      //Non-blocking call
      ret = px5_sem_trywait(event);
   }
   else if(timeout == INFINITE_DELAY)
   {
      //Infinite timeout period
      ret = px5_sem_wait(event);
   }
   else
   {
      //Wait until the specified event becomes set
      ret = px5_sem_timedwait(event, OS_MS_TO_SYSTICKS(timeout));
   }

   //Check whether the specified event is set
   if(ret == PX5_SUCCESS)
   {
      //Force the event back to the nonsignaled state
      do
      {
         //Decrement the semaphore's count by one
         ret = px5_sem_trywait(event);

         //Check status
      } while(ret == PX5_SUCCESS);

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
   px5_sem_post(event);

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
   int_t ret;

   //Create a semaphore object
   ret = px5_sem_init(semaphore, 0, count);

   //Check whether the semaphore was successfully created
   if(ret == PX5_SUCCESS)
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
   //Properly dispose the semaphore object
   px5_sem_destroy(semaphore);
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
   int_t ret;

   //Wait until the semaphore is available or the timeout interval elapses
   if(timeout == 0)
   {
      //Non-blocking call
      ret = px5_sem_trywait(semaphore);
   }
   else if(timeout == INFINITE_DELAY)
   {
      //Infinite timeout period
      ret = px5_sem_wait(semaphore);
   }
   else
   {
      //Wait until the specified semaphore becomes available
      ret = px5_sem_timedwait(semaphore, OS_MS_TO_SYSTICKS(timeout));
   }

   //Check whether the specified semaphore is available
   if(ret == PX5_SUCCESS)
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
   px5_sem_post(semaphore);
}


/**
 * @brief Create a mutex object
 * @param[in] mutex Pointer to the mutex object
 * @return The function returns TRUE if the mutex was successfully
 *   created. Otherwise, FALSE is returned
 **/

bool_t osCreateMutex(OsMutex *mutex)
{
   int_t ret;

   //Create a mutex object
   ret = px5_pthread_mutex_init(mutex, NULL);

   //Check whether the mutex was successfully created
   if(ret == PX5_SUCCESS)
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
   //Properly dispose the mutex object
   px5_pthread_mutex_destroy(mutex);
}


/**
 * @brief Acquire ownership of the specified mutex object
 * @param[in] mutex Pointer to the mutex object
 **/

void osAcquireMutex(OsMutex *mutex)
{
   //Obtain ownership of the mutex object
   px5_pthread_mutex_lock(mutex);
}


/**
 * @brief Release ownership of the specified mutex object
 * @param[in] mutex Pointer to the mutex object
 **/

void osReleaseMutex(OsMutex *mutex)
{
   //Release ownership of the mutex object
   px5_pthread_mutex_unlock(mutex);
}


/**
 * @brief Retrieve system time
 * @return Number of milliseconds elapsed since the system was last started
 **/

systime_t osGetSystemTime(void)
{
   systime_t time;

   //Get current tick count
   time = px5_pthread_ticks_get();

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


/**
 * @brief Memory manager allocate function
 * @param[in] type Unused parameter
 * @param[in] size Bytes to allocate
 * @return A pointer to the allocated memory block or NULL if
 *   there is insufficient memory available
 **/

void *osAllocMemCallback(u_int type, u_long size)
{
   //Allocate a memory block
   return osAllocMem(size);
}


/**
 * @brief Memory manager release function
 * @param[in] type Unused parameter
 * @param[in] p Previously allocated memory block to be freed
 **/

void osFreeMemCallback(u_int type, void *p)
{
   //Free memory block
   osFreeMem(p);
}

