/**
 * @file os_port_safertos.c
 * @brief RTOS abstraction layer (SafeRTOS)
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "os_port.h"
#include "os_port_safertos.h"
#include "debug.h"

//Default task parameters
const OsTaskParameters OS_TASK_DEFAULT_PARAMS =
{
   NULL,                  //Task control block
   NULL,                  //Stack
   0,                     //Size of the stack
   taskIDLE_PRIORITY + 1, //Task priority
};

//Declaration of functions
extern portBaseType xInitializeScheduler(void);


/**
 * @brief Kernel initialization
 **/

void osInitKernel(void)
{
   //Initialize kernel
   xInitializeScheduler();
}


/**
 * @brief Start kernel
 **/

void osStartKernel(void)
{
   //Start the scheduler
   xTaskStartScheduler(pdTRUE);
}


/**
 * @brief Create a task
 * @param[in] name NULL-terminated string identifying the task
 * @param[in] taskCode Pointer to the task entry function
 * @param[in] arg Argument passed to the task function
 * @param[in] params Task parameters
 * @return Task identifier referencing the newly created task
 **/

__weak_func OsTaskId osCreateTask(const char_t *name, OsTaskCode taskCode,
   void *arg, const OsTaskParameters *params)
{
   portBaseType status;
   portTaskHandleType handle;
   xTaskParameters taskParams;

   //Check parameters
   if(params->tcb != NULL && params->stack != NULL)
   {
      //Initialize task parameters
      memset(&taskParams, 0, sizeof(taskParams));

      //Set task parameters
      taskParams.pvTaskCode = (pdTASK_CODE) taskCode;
      taskParams.pcTaskName = name;
      taskParams.pxTCB = params->tcb;
      taskParams.pcStackBuffer = params->stack;
      taskParams.ulStackDepthBytes = params->stackSize;
      taskParams.pvParameters = arg;
      taskParams.uxPriority = params->priority;
      taskParams.xUsingFPU =  pdTRUE;
      taskParams.xMPUParameters.uxPrivilegeLevel = mpuPRIVILEGED_TASK;

      //Create a new task
      status = xTaskCreate(&taskParams, &handle);

      //Failed to create task?
      if(status != pdPASS)
      {
         handle = OS_INVALID_TASK_ID;
      }
   }
   else
   {
      //Invalid parameters
      handle = OS_INVALID_TASK_ID;
   }

   //Return the handle referencing the newly created task
   return (OsTaskId) handle;
}


/**
 * @brief Delete a task
 * @param[in] taskId Task identifier referencing the task to be deleted
 **/

void osDeleteTask(OsTaskId taskId)
{
   //Delete the specified task
   xTaskDelete((portTaskHandleType) taskId);
}


/**
 * @brief Delay routine
 * @param[in] delay Amount of time for which the calling task should block
 **/

void osDelayTask(systime_t delay)
{
   //Delay the task for the specified duration
   xTaskDelay(OS_MS_TO_SYSTICKS(delay));
}


/**
 * @brief Yield control to the next task
 **/

void osSwitchTask(void)
{
   //Force a context switch
   taskYIELD();
}


/**
 * @brief Suspend scheduler activity
 **/

void osSuspendAllTasks(void)
{
   //Make sure the operating system is running
   if(xTaskIsSchedulerStarted() == pdTRUE)
   {
      //Suspend all tasks
      vTaskSuspendScheduler();
   }
}


/**
 * @brief Resume scheduler activity
 **/

void osResumeAllTasks(void)
{
   //Make sure the operating system is running
   if(xTaskIsSchedulerStarted() == pdTRUE)
   {
      //Resume all tasks
      xTaskResumeScheduler();
   }
}


/**
 * @brief Create an event object
 * @param[in] event Pointer to the event object
 * @return The function returns TRUE if the event object was successfully
 *   created. Otherwise, FALSE is returned
 **/

bool_t osCreateEvent(OsEvent *event)
{
   portBaseType status;

   uint32_t n;
   uint32_t p = (uint32_t) event->buffer;
   n = p % portQUEUE_OVERHEAD_BYTES;
   p += portQUEUE_OVERHEAD_BYTES - n;

   //Create a binary semaphore
   status = xSemaphoreCreateBinary((portInt8Type *) p, &event->handle);

   //Check whether the semaphore was successfully created
   if(status == pdPASS)
   {
      //Force the event to the nonsignaled state
      xSemaphoreTake(event->handle, 0);
      //Event successfully created
      return TRUE;
   }
   else
   {
      //Failed to create event object
      return FALSE;
   }
}


/**
 * @brief Delete an event object
 * @param[in] event Pointer to the event object
 **/

void osDeleteEvent(OsEvent *event)
{
   //Not implemented
}


/**
 * @brief Set the specified event object to the signaled state
 * @param[in] event Pointer to the event object
 **/

void osSetEvent(OsEvent *event)
{
   //Set the specified event to the signaled state
   xSemaphoreGive(event->handle);
}


/**
 * @brief Set the specified event object to the nonsignaled state
 * @param[in] event Pointer to the event object
 **/

void osResetEvent(OsEvent *event)
{
   //Force the specified event to the nonsignaled state
   xSemaphoreTake(event->handle, 0);
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
   portBaseType status;

   //Wait until the specified event is in the signaled state or the timeout
   //interval elapses
   if(timeout == INFINITE_DELAY)
   {
      //Infinite timeout period
      status = xSemaphoreTake(event->handle, portMAX_DELAY);
   }
   else
   {
      //Wait for the specified time interval
      status = xSemaphoreTake(event->handle, OS_MS_TO_SYSTICKS(timeout));
   }

   //The return value tells whether the event is set
   if(status == pdPASS)
   {
      return TRUE;
   }
   else
   {
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
   portBaseType flag = FALSE;

   //Set the specified event to the signaled state
   xSemaphoreGiveFromISR(event->handle, &flag);

   //A higher priority task has been woken?
   return flag;
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
   portBaseType status;

   uint32_t n;
   uint32_t p = (uint32_t) semaphore->buffer;
   n = p % portQUEUE_OVERHEAD_BYTES;
   p += portQUEUE_OVERHEAD_BYTES - n;

   //Create a semaphore
   status = xSemaphoreCreateCounting(count, count, (portInt8Type *) p,
      &semaphore->handle);

   //Check whether the semaphore was successfully created
   if(status == pdPASS)
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
   //Not implemented
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
   portBaseType status;

   //Wait until the specified semaphore becomes available
   if(timeout == INFINITE_DELAY)
   {
      //Infinite timeout period
      status = xSemaphoreTake(semaphore->handle, portMAX_DELAY);
   }
   else
   {
      //Wait for the specified time interval
      status = xSemaphoreTake(semaphore->handle, OS_MS_TO_SYSTICKS(timeout));
   }

   //The return value tells whether the semaphore is available
   if(status == pdPASS)
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
   xSemaphoreGive(semaphore->handle);
}


/**
 * @brief Create a mutex object
 * @param[in] mutex Pointer to the mutex object
 * @return The function returns TRUE if the mutex was successfully
 *   created. Otherwise, FALSE is returned
 **/

bool_t osCreateMutex(OsMutex *mutex)
{
   portBaseType status;

   uint32_t n;
   uint32_t p = (uint32_t) mutex->buffer;
   n = p % portQUEUE_OVERHEAD_BYTES;
   p += portQUEUE_OVERHEAD_BYTES - n;

   //Create a binary semaphore
   status = xSemaphoreCreateBinary((portInt8Type *) p, &mutex->handle);

   //Check whether the semaphore was successfully created
   if(status == pdPASS)
   {
      //Force the binary semaphore to the signaled state
      xSemaphoreGive(mutex->handle);

      //Semaphore successfully created
      return TRUE;
   }
   else
   {
      //Failed to create semaphore
      return FALSE;
   }
}


/**
 * @brief Delete a mutex object
 * @param[in] mutex Pointer to the mutex object
 **/

void osDeleteMutex(OsMutex *mutex)
{
   //Not implemented
}


/**
 * @brief Acquire ownership of the specified mutex object
 * @param[in] mutex Pointer to the mutex object
 **/

void osAcquireMutex(OsMutex *mutex)
{
   //Obtain ownership of the mutex object
   xSemaphoreTake(mutex->handle, portMAX_DELAY);
}


/**
 * @brief Release ownership of the specified mutex object
 * @param[in] mutex Pointer to the mutex object
 **/

void osReleaseMutex(OsMutex *mutex)
{
   //Release ownership of the mutex object
   xSemaphoreGive(mutex->handle);
}


/**
 * @brief Retrieve system time
 * @return Number of milliseconds elapsed since the system was last started
 **/

systime_t osGetSystemTime(void)
{
   systime_t time;

   //Get current tick count
   time = xTaskGetTickCount();

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
   //Not implemented
   return NULL;
}


/**
 * @brief Release a previously allocated memory block
 * @param[in] p Previously allocated memory block to be freed
 **/

__weak_func void osFreeMem(void *p)
{
   //Not implemented
}
