/**
 * @file os_port_zephyr.c
 * @brief RTOS abstraction layer (Zephyr)
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
#include "os_port_zephyr.h"
#include "debug.h"

//Default task parameters
const OsTaskParameters OS_TASK_DEFAULT_PARAMS =
{
   NULL,                             //Task control block
   NULL,                             //Stack
   0,                                //Size of the stack
   CONFIG_NUM_PREEMPT_PRIORITIES - 1 //Task priority
};


/**
 * @brief Kernel initialization
 **/

void osInitKernel(void)
{
   //Not implemented
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
   k_tid_t tid;

   //Check parameters
   if(params->tcb != NULL && params->stack != NULL)
   {
      //Create a new thread
      tid = k_thread_create(params->tcb, params->stack, params->stackSize,
         (k_thread_entry_t) taskCode, arg, NULL, NULL, params->priority, 0,
         K_NO_WAIT);

      //Check whether the thread was successfully created
      if(tid != OS_INVALID_TASK_ID)
      {
         //Set current thread name
         k_thread_name_set(tid, name);
      }
   }
   else
   {
      //Invalid parameters
      tid = OS_INVALID_TASK_ID;
   }

   //Return the handle referencing the newly created thread
   return (OsTaskId) tid;
}


/**
 * @brief Delete a task
 * @param[in] taskId Task identifier referencing the task to be deleted
 **/

void osDeleteTask(OsTaskId taskId)
{
   k_tid_t tid;

   //Delete the currently running thread?
   if(taskId == OS_SELF_TASK_ID)
   {
      tid = k_current_get();
   }
   else
   {
      tid = (k_tid_t) taskId;
   }

   //Abort the thread
   k_thread_abort(tid);
}


/**
 * @brief Delay routine
 * @param[in] delay Amount of time for which the calling task should block
 **/

void osDelayTask(systime_t delay)
{
   //Put the current thread to sleep
   k_sleep(K_MSEC(delay));
}


/**
 * @brief Yield control to the next task
 **/

void osSwitchTask(void)
{
   //Wake up a sleeping thread
   k_yield();
}


/**
 * @brief Suspend scheduler activity
 **/

void osSuspendAllTasks(void)
{
   //Lock the scheduler
   k_sched_lock();
}


/**
 * @brief Resume scheduler activity
 **/

void osResumeAllTasks(void)
{
   //Unlock the scheduler
   k_sched_unlock();
}


/**
 * @brief Create an event object
 * @param[in] event Pointer to the event object
 * @return The function returns TRUE if the event object was successfully
 *   created. Otherwise, FALSE is returned
 **/

bool_t osCreateEvent(OsEvent *event)
{
   int err;

   //Create a binary semaphore
   err = k_sem_init(event, 0, 1);

   //Check whether the semaphore was successfully created
   if(err)
   {
      return FALSE;
   }
   else
   {
      return TRUE;
   }
}


/**
 * @brief Delete an event object
 * @param[in] event Pointer to the event object
 **/

void osDeleteEvent(OsEvent *event)
{
   //No resource to release
}


/**
 * @brief Set the specified event object to the signaled state
 * @param[in] event Pointer to the event object
 **/

void osSetEvent(OsEvent *event)
{
   //Set the specified event to the signaled state
   k_sem_give(event);
}


/**
 * @brief Set the specified event object to the nonsignaled state
 * @param[in] event Pointer to the event object
 **/

void osResetEvent(OsEvent *event)
{
   //Force the specified event to the nonsignaled state
   k_sem_reset(event);
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
   int err;

   //Wait until the specified event is in the signaled state or the timeout
   //interval elapses
   if(timeout == 0)
   {
      //Non-blocking call
      err = k_sem_take(event, K_NO_WAIT);
   }
   else if(timeout == INFINITE_DELAY)
   {
      //Infinite timeout period
      err = k_sem_take(event, K_FOREVER);
   }
   else
   {
      //Wait until the specified event becomes set
      err = k_sem_take(event, K_MSEC(timeout));
   }

   //Check whether the specified event is set
   if(err)
   {
      return FALSE;
   }
   else
   {
      return TRUE;
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
   k_sem_give(event);

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
   int err;

   //Initialize the semaphore object
   err = k_sem_init(semaphore, count, count);

   //Check whether the semaphore was successfully created
   if(err)
   {
      return FALSE;
   }
   else
   {
      return TRUE;
   }
}


/**
 * @brief Delete a semaphore object
 * @param[in] semaphore Pointer to the semaphore object
 **/

void osDeleteSemaphore(OsSemaphore *semaphore)
{
   //No resource to release
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
   int err;

   //Wait until the semaphore is available or the timeout interval elapses
   if(timeout == 0)
   {
      //Non-blocking call
      err = k_sem_take(semaphore, K_NO_WAIT);
   }
   else if(timeout == INFINITE_DELAY)
   {
      //Infinite timeout period
      err = k_sem_take(semaphore, K_FOREVER);
   }
   else
   {
      //Wait until the specified semaphore becomes available
      err = k_sem_take(semaphore, K_MSEC(timeout));
   }

   //Check whether the specified semaphore is available
   if(err)
   {
      return FALSE;
   }
   else
   {
      return TRUE;
   }
}


/**
 * @brief Release the specified semaphore object
 * @param[in] semaphore Pointer to the semaphore object
 **/

void osReleaseSemaphore(OsSemaphore *semaphore)
{
   //Give the semaphore
   k_sem_give(semaphore);
}


/**
 * @brief Create a mutex object
 * @param[in] mutex Pointer to the mutex object
 * @return The function returns TRUE if the mutex was successfully
 *   created. Otherwise, FALSE is returned
 **/

bool_t osCreateMutex(OsMutex *mutex)
{
   int err;

   //Initialize the mutex object
   err = k_mutex_init(mutex);

   //Check whether the mutex was successfully created
   if(err)
   {
      return FALSE;
   }
   else
   {
      return TRUE;
   }
}


/**
 * @brief Delete a mutex object
 * @param[in] mutex Pointer to the mutex object
 **/

void osDeleteMutex(OsMutex *mutex)
{
   //No resource to release
}


/**
 * @brief Acquire ownership of the specified mutex object
 * @param[in] mutex Pointer to the mutex object
 **/

void osAcquireMutex(OsMutex *mutex)
{
   //Lock the mutex
   k_mutex_lock(mutex, K_FOREVER);
}


/**
 * @brief Release ownership of the specified mutex object
 * @param[in] mutex Pointer to the mutex object
 **/

void osReleaseMutex(OsMutex *mutex)
{
   //Unlock the mutex
   k_mutex_unlock(mutex);
}


/**
 * @brief Retrieve system time
 * @return Number of milliseconds elapsed since the system was last started
 **/

systime_t osGetSystemTime(void)
{
   //Get the elapsed time since the system booted, in milliseconds
   return (systime_t) k_uptime_get();
}


/**
 * @brief Retrieve 64-bit system time
 * @return Number of milliseconds elapsed since the system was last started
 **/

uint64_t osGetSystemTime64(void)
{
   //Get the elapsed time since the system booted, in milliseconds
   return (uint64_t) k_uptime_get();
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
   p = k_malloc(size);
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
      k_free(p);
      //Leave critical section
      osResumeAllTasks();
   }
}
