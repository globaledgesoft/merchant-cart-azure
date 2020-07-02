// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
 
#include <stdlib.h>
#include "azure_c_shared_utility/lock.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_macro_utils/macro_utils.h"
#include "txm_module.h"

MU_DEFINE_ENUM_STRINGS(LOCK_RESULT, LOCK_RESULT_VALUES);

LOCK_HANDLE Lock_Init(void)
{
  TX_MUTEX *mtx_hndl = NULL;
  unsigned int status = TX_THREAD_ERROR;

  /* Allocate resources for the mutex */ 
  txm_module_object_allocate(&mtx_hndl, sizeof(TX_MUTEX));
  if (NULL == mtx_hndl)
  {
    LogError("Allocating resources for the mutex failed.");
    goto mtx_create_error; 
  }

  /* Create the mutex */
  status = tx_mutex_create(mtx_hndl, "azure_sdk_sync_obj", TX_NO_INHERIT);
  if (status != TX_SUCCESS)
  {
    /* Release the resources associated with the mutex */
    txm_module_object_deallocate(mtx_hndl);

    LogError("Creating mutex failed with status[%d]", status);
    goto mtx_create_error;
  } 

mtx_create_error:
  return (LOCK_HANDLE)mtx_hndl;
}


LOCK_RESULT Lock_Deinit(LOCK_HANDLE handle)
{
  LOCK_RESULT result = LOCK_OK;

  if (handle == NULL)
  {
    /*SRS_LOCK_99_007:[ This API on NULL handle passed returns LOCK_ERROR]*/
    result = LOCK_ERROR;
    LogError("(result = %s)", MU_ENUM_TO_STRING(LOCK_RESULT, result));
  }
  else
  {
    /* Delete the mutex */ 
    tx_mutex_delete(handle);
    /* Release the resources associated with the mutex */
    txm_module_object_deallocate(handle); 
  }

  return result;
}


LOCK_RESULT Lock(LOCK_HANDLE handle)
{
  LOCK_RESULT result = LOCK_OK;

  if (handle == NULL)
  {
    result = LOCK_ERROR;
    LogError("(result = %s)", MU_ENUM_TO_STRING(LOCK_RESULT, result));
  }
  else 
  {
    /* Acquire the mutex */ 
    tx_mutex_get(handle, TX_WAIT_FOREVER);
  }  
  
  return result;
}


LOCK_RESULT Unlock(LOCK_HANDLE handle)
{
  LOCK_RESULT result = LOCK_OK;

  if (handle == NULL)
  {
    result = LOCK_ERROR;
    LogError("(result = %s)", MU_ENUM_TO_STRING(LOCK_RESULT, result));
  }
  else
  {
    /* Release the acquired mutex */ 
    tx_mutex_put(handle);
  }
    
  return result;
}
