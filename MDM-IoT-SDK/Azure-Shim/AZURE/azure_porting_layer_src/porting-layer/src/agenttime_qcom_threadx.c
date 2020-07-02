// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

/* This is a template file used for porting */

/* TODO: If all the below adapters call the standard C functions, please simply use the agentime.c that already exists. */

#include <time.h>
#include "azure_c_shared_utility/agenttime.h"
#include "qapi_status.h"
#include "qapi_timer.h"



time_t get_time(time_t* p)
{
  qapi_time_get_t current_time;

  /* Retrieve the current time (SECS) */ 
  qapi_time_get(QAPI_TIME_SECS, &current_time);
  
  if (p)
    *p = (time_t)current_time.time_secs + 315964700;

  return (time_t)current_time.time_secs + 315964700;
} 


struct tm* get_gmtime(time_t* currentTime)
{
  /* TODO: replace the gmtime call below with your own. Note that converting to struct tm needs to be done ... 
   * return gmtime(currentTime);
   */

  /* Not supported for now */
  return NULL;
}


time_t get_mktime(struct tm* cal_time)
{
  /* TODO: replace the mktime call below with your own. Note that converting to time_t needs to be done ... 
   * return mktime(cal_time);
   */

  /* Not supported for now */
  return (time_t)NULL;
}


char* get_ctime(time_t* timeToGet)
{
  /* TODO: replace the ctime call below with calls available on your platform.
   * return ctime(timeToGet);
   */

  /* Not supported for now */
  return NULL;
}


double get_difftime(time_t stopTime, time_t startTime)
{
  
  return ((double)(stopTime - startTime));
}

