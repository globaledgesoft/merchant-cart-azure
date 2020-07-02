// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include <stdlib.h>
#include <stdint.h>
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/xlogging.h"
#ifdef AZURE_9206
#define __FAILURE__ -1
#include "qapi_timer.h"
#else
#include "qurt_timetick.h"
#endif

typedef struct TICK_COUNTER_INSTANCE_TAG
{
  int dummy : 1;

} TICK_COUNTER_INSTANCE;


TICK_COUNTER_HANDLE tickcounter_create(void)
{
  TICK_COUNTER_INSTANCE* handle = NULL;

  /* Allocate the resources for the tick counter */ 
  handle = (TICK_COUNTER_INSTANCE *)malloc(sizeof(TICK_COUNTER_INSTANCE));    
  if (NULL == handle)
    /* Add any per instance initialization (starting a timer for example) here if needed (most platforms will not need this) */
    LogError("Cannot create tick counter!");
  
  return handle;
}


void tickcounter_destroy(TICK_COUNTER_HANDLE tick_counter)
{
  /* Release the tick counter associated resources */
  if (tick_counter != NULL)
    free(tick_counter);
}


int tickcounter_get_current_ms(TICK_COUNTER_HANDLE tick_counter, tickcounter_ms_t *current_ms)
{
  int result = 0;
  qapi_time_get_t time_info;
  qapi_Status_t status = QAPI_OK;

  if (tick_counter == NULL)
  {
    LogError("tickcounter failed: Invalid Arguments.");
    result = __FAILURE__;
  }
  else
  {
#ifdef AZURE_9206
    /* Convert ticks to milliseconds */
    status =  qapi_time_get(QAPI_TIME_MSECS, &time_info);
    if(QAPI_OK != status)
    {
       LogError("[time] qapi_time_get failed [%d]", status);
       result = __FAILURE__;
    }
    
    *current_ms = time_info.time_msecs;
#else
    /* Convert ticks to milliseconds */
    *current_ms = qurt_timer_convert_ticks_to_time(qurt_timer_get_ticks(), QURT_TIME_MSEC);
#endif
  }

  return result;
}


