// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include <stdint.h>
#include <stdlib.h>
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/singlylinkedlist.h"
#include "txm_module.h"
#include "qapi_timer.h"


MU_DEFINE_ENUM_STRINGS(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);

#define AZ_THREAD_PRIORITY 200
#define AZ_THREAD_STACK_SIZE 8192

#define DEL_THRD_EVT 0x01

#ifdef AZURE_9206
#define TRUE 1
#define FALSE 0
#endif


typedef struct THREAD_INSTANCE_TAG
{
  unsigned int DirtyInstance; 
  THREAD_START_FUNC ThreadStartFunc;
  void *Arg;
  void *ThreadStackAddr; 
  TX_THREAD *ThreadHandle;
} THREAD_INSTANCE;


static TX_THREAD *thrd_mgmt_hndl = NULL;
static SINGLYLINKEDLIST_HANDLE thrd_mgmt_lst = NULL;
static TX_MUTEX *thrd_mgmt_sync = NULL; 
static TX_EVENT_FLAGS_GROUP *thrd_mgmt_evt = NULL;



static bool Thread_Find(LIST_ITEM_HANDLE list_item, const void* match_context)
{
  bool result = FALSE;
  THREAD_INSTANCE *thrd_inst = NULL;

  if ((NULL == list_item) ||
      (NULL == match_context))
    return result;

  thrd_inst = (THREAD_INSTANCE *)singlylinkedlist_item_get_value(list_item);
  if ((thrd_inst) && 
#ifdef AZURE_9206
      (thrd_inst->ThreadHandle == (TX_THREAD *)match_context))
#else
      (thrd_inst->ThreadHandle = (TX_THREAD *)match_context))
#endif
    result = TRUE; 

  return result;
}


static void Thread_Wrapper(void* threadInstanceArg)
{
  unsigned int status = TX_THREAD_ERROR;
  THREAD_INSTANCE *thrd_inst = (THREAD_INSTANCE *)threadInstanceArg;
  
  /* Entry function */ 
  thrd_inst->ThreadStartFunc(thrd_inst->Arg);
    
  /* Completion case - Mark the thread block as dirty and clean-up the resources */
  thrd_inst->DirtyInstance = TRUE; 

  /* Signal thread manager to clean-up the resources associated with the thread */
  status = tx_event_flags_set(thrd_mgmt_evt, DEL_THRD_EVT, TX_OR);
  if (status != TX_SUCCESS)
    LogError("Setting the event flags failed for handle[0x%x] \n", thrd_mgmt_evt);
   
  return;
}


static void Thread_Manager(void *args)
{
  unsigned int status = TX_THREAD_ERROR; 
  unsigned long actual_flags = 0x00;

  for ( ;  ; )
  { 
    LIST_ITEM_HANDLE list_item;
    
    /* Wait for signal to clean-up the resources associated with the terminated or completed thread */
    status = tx_event_flags_get(thrd_mgmt_evt, DEL_THRD_EVT, TX_OR_CLEAR, &actual_flags, TX_WAIT_FOREVER);
    if ((status != TX_SUCCESS) || !(actual_flags & DEL_THRD_EVT))
    {
      LogError("Getting the event flags failed with status[0x%x]/actual_flags[0x%x]\n", status, actual_flags);
      continue;
    }

    tx_mutex_get(thrd_mgmt_sync, TX_WAIT_FOREVER);

    /* Allow the thread to terminate before performing the clean-up */
    ThreadAPI_Sleep(1000);
    
    /* Traverse through the list and remove thread control blocks that have dirty bit set */
    list_item = singlylinkedlist_get_head_item(thrd_mgmt_lst);        
    while (list_item != NULL)
    {
      THREAD_INSTANCE *thrd_inst = (THREAD_INSTANCE *)singlylinkedlist_item_get_value(list_item);

      if ((thrd_inst) &&
          (thrd_inst->DirtyInstance))
      {
        /* Release the resources associated with the thread */
        LOG(AZ_LOG_INFO, LOG_LINE, "Thread  deleted successfully with handle[0x%x]\n", thrd_inst->ThreadHandle);
        tx_thread_delete(thrd_inst->ThreadHandle);
       
        /* Release the stack specific resources */
        if (thrd_inst->ThreadStackAddr)
          free((void *)thrd_inst->ThreadStackAddr);
  
        /* Remove the thread control block from the list */
        singlylinkedlist_remove(thrd_mgmt_lst, list_item); 

        /* Release the thread instance inforamtion */ 
        free(thrd_inst);

        break;        
      }  
      
      list_item = singlylinkedlist_get_next_item(list_item);
    }

    tx_mutex_put(thrd_mgmt_sync);
  }
}



int Thread_Init(void)
{
  unsigned int status = TX_THREAD_ERROR;
  void *thrd_stck_addr = NULL; 

  /* Create a list for thread management */
  thrd_mgmt_lst = singlylinkedlist_create( );
  if (NULL == thrd_mgmt_lst)
    goto init_error; 

  /* Allocate resources for synchronization object */ 
  status = txm_module_object_allocate(&thrd_mgmt_sync, sizeof(TX_MUTEX));
  if (status != TX_SUCCESS)
  {
    LogError("Allocating synchronization object failed with status[0x%x]\n", status);
    goto init_error;
  }

  /* Create synchronization object for thread management */
  status = tx_mutex_create(thrd_mgmt_sync, "azure_thrd_mgmt_sync_obj", TX_NO_INHERIT);
  if (status != TX_SUCCESS)
  {  
    LogError("Creating synchronization object failed with status[0x%x]\n", status);
    goto init_error;
  }

  /* Allocate resources for event object */ 
  status = txm_module_object_allocate(&thrd_mgmt_evt, sizeof(TX_EVENT_FLAGS_GROUP));
  if (status != TX_SUCCESS)
  {
    LogError("Allocating synchronization object failed with status[0x%x]\n", status);
    goto init_error;
  }

  /* Create event object for thread management */
  status = tx_event_flags_create(thrd_mgmt_evt, "azure_thrd_mgmt_evt_obj");
  if (status != TX_SUCCESS)
  {  
    LogError("Creating event object failed with status[0x%x]\n", status);
    goto init_error;
  }

  /* Allocate resources for thread manager */ 
  status = txm_module_object_allocate(&thrd_mgmt_hndl, sizeof(TX_THREAD));
  if (status != TX_SUCCESS)
  {
    LogError("Allocating thread manager resources failed with status[0x%x]\n", status);
    goto init_error;
  }

  /* Allocate resources for the thread management stack */ 
  thrd_stck_addr = malloc(AZ_THREAD_STACK_SIZE);
  if (NULL == thrd_stck_addr)
  {  
    LogError("Memory allocation failed for thread management stack with status[0x%x]\n", thrd_stck_addr);
    goto init_error;
  }

  /* Create a thread for internal thread management */
  status = tx_thread_create(thrd_mgmt_hndl, "azure_thrd_manager", Thread_Manager,
                            NULL, thrd_stck_addr, AZ_THREAD_STACK_SIZE,
                            AZ_THREAD_PRIORITY, AZ_THREAD_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);
  if (status != TX_SUCCESS)
  {
    LogError("Memory allocation failed for thread management stack with status[0x%x]\n", thrd_stck_addr);
    goto init_error; 
  }

  status = TX_SUCCESS; 
      
init_error:

  /* Clean-up the resources */
  if (status)
  {
    if (thrd_stck_addr)
      free(thrd_stck_addr);

    if (thrd_mgmt_lst)
      singlylinkedlist_destroy(thrd_mgmt_lst);

    if (thrd_mgmt_hndl)
      free(thrd_mgmt_hndl);

    if (thrd_mgmt_evt)
      free(thrd_mgmt_evt);

    if (thrd_mgmt_sync)
      free(thrd_mgmt_sync);
  }

  return status; 
}


THREADAPI_RESULT ThreadAPI_Create(THREAD_HANDLE* threadHandle, THREAD_START_FUNC func, void* arg)
{
  THREAD_INSTANCE *thrd_inst = NULL;
  unsigned int status = TX_THREAD_ERROR;
  THREADAPI_RESULT result = THREADAPI_ERROR;
  void *thrd_stck_addr = NULL; 
  TX_THREAD *thrd_hndl = NULL; 
 
  if ((threadHandle == NULL) ||
      (func == NULL))
  {
    result = THREADAPI_INVALID_ARG;
    LogError("(result = %s)", MU_ENUM_TO_STRING(THREADAPI_RESULT, result));

    goto thrdx_create_error;
  }
  
  /* Allocate resources for thread control information block */
  thrd_inst = malloc(sizeof(THREAD_INSTANCE));
  if (thrd_inst == NULL)
  {
    result = THREADAPI_NO_MEMORY;
    LogError("(result = %s)", MU_ENUM_TO_STRING(THREADAPI_RESULT, result));

    goto thrdx_create_error; 
  }

  /* Allocate the resources from thread */ 
  txm_module_object_allocate(&thrd_hndl, sizeof(TX_THREAD));
  if (NULL == thrd_hndl)
  {
    result = THREADAPI_ERROR;
    LogError("(result = %s)", MU_ENUM_TO_STRING(THREADAPI_RESULT, result));

    goto thrdx_create_error; 
  }
       
  /* Allocate resources for the thread stack */ 
  thrd_stck_addr = malloc(AZ_THREAD_STACK_SIZE);
  if (thrd_stck_addr == NULL)
  {
    result = THREADAPI_NO_MEMORY;
    LogError("(result = %s)", MU_ENUM_TO_STRING(THREADAPI_RESULT, result));

    goto thrdx_create_error;
  }
  
  /* Assign the application thread specific control information */ 
  memset(thrd_inst, 0x00, sizeof(THREAD_INSTANCE));
  thrd_inst->ThreadStartFunc = func;
  thrd_inst->Arg = arg;
  thrd_inst->ThreadStackAddr = thrd_stck_addr;

  tx_mutex_get(thrd_mgmt_sync, TX_WAIT_FOREVER);
  
  /* Add the instance entry into the thread control list */  
  if (NULL == singlylinkedlist_add(thrd_mgmt_lst, (const void *) thrd_inst))
  {
    result = THREADAPI_NO_MEMORY;
    LogError("(result = %s)", MU_ENUM_TO_STRING(THREADAPI_RESULT, result));

    goto thrdx_create_error;
  }  

  tx_mutex_put(thrd_mgmt_sync);
             
  /* Create the application thread.  */
  status = tx_thread_create(thrd_hndl, "azure_sdk_thrd", Thread_Wrapper,
                            thrd_inst, thrd_stck_addr, AZ_THREAD_STACK_SIZE,
                            AZ_THREAD_PRIORITY, AZ_THREAD_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);
  if (status != TX_SUCCESS)
  {
    result = THREADAPI_ERROR;
    LogError("(result = %s)", MU_ENUM_TO_STRING(THREADAPI_RESULT, result));

    goto thrdx_create_error;
  } 

  /* Set the thread handle to the created thread instance */
  thrd_inst->ThreadHandle = thrd_hndl;

 *threadHandle = thrd_inst;  
  result = THREADAPI_OK;

  LOG(AZ_LOG_INFO, LOG_LINE, "Thread  created successfully with handle[0x%x]\n", thrd_inst->ThreadHandle);

thrdx_create_error:

  /* Release allocated resources */
  if (result != THREADAPI_OK)
  {
    if (thrd_inst)
      free(thrd_inst);

    if (thrd_stck_addr)
      free(thrd_stck_addr);

    if (thrd_hndl)
      txm_module_object_deallocate(thrd_hndl);
  }

  return result;
}


THREADAPI_RESULT ThreadAPI_Join(THREAD_HANDLE threadHandle, int* res)
{
 
  /* Not supported for now */
  return THREADAPI_ERROR;    
}


void ThreadAPI_Exit(int res)
{
  TX_THREAD *thrd_hndl = NULL; 
  unsigned int status = TX_THREAD_ERROR;

  /* Identify the application thread */ 
  thrd_hndl = tx_thread_identify();
  if (thrd_hndl)
  {
    LIST_ITEM_HANDLE lst_item = NULL;
    THREAD_INSTANCE *thrd_inst = NULL;

    tx_mutex_get(thrd_mgmt_sync, TX_WAIT_FOREVER); 
  
    /* Termination case - Retrieve the handle information from the thread management list */
    lst_item = singlylinkedlist_find(thrd_mgmt_lst, Thread_Find, (const void *)thrd_hndl);
    if (NULL == lst_item)
    {
      tx_mutex_put(thrd_mgmt_sync);

      LogError("Information not found in the internal context for handle[0x%x]\n", thrd_hndl);
      goto exit_error; 
    }

    /* Mark the thread block as dirty and clean-up the resources */
    thrd_inst = (THREAD_INSTANCE *)singlylinkedlist_item_get_value(lst_item);
    thrd_inst->DirtyInstance = TRUE;

    /* Signal thread manager to clean-up the resources associated with the thread */
    status = tx_event_flags_set(thrd_mgmt_evt, DEL_THRD_EVT, TX_OR);
    if (status != TX_SUCCESS)
    {
      tx_mutex_put(thrd_mgmt_sync);

      LogError("Setting the event flags failed for handle[0x%x] with status[0x%x]\n", thrd_hndl, status);
      goto exit_error;
    }

    tx_mutex_put(thrd_mgmt_sync);

    /* Terminate the application thread */
    LOG(AZ_LOG_INFO, LOG_LINE, "Thread terminated successfully with handle[0x%x]", thrd_hndl);
    tx_thread_terminate(thrd_hndl);
  }

exit_error:
  
  return;
}


void ThreadAPI_Sleep(unsigned int milliseconds)
{
  /* Transition the thread to sleep state (deferrable) */ 
  // tx_thread_sleep(qurt_timer_convert_time_to_ticks(milliseconds, QURT_TIME_MSEC));

  /* Transition the thread to sleep state (non-deferrable(true)/deferrable(false)) */ 
  qapi_Timer_Sleep(milliseconds, QAPI_TIMER_UNIT_MSEC, TRUE); 
}


