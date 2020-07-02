// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/agenttime.h"
#include "tlsio_qcom_threadx.h"
#include "txm_module.h"
#include "qapi_diag.h"


/* Azure platform library version */
#define AZURE_SDK_PLATFORM_LIB_VER "qcom_threadx_v_1.0"

/* Byte pool size */
#define AZURE_BYTE_POOL_SIZE    102400

/* Define the pool space in the bss section of the module. ULONG is used to get the word alignment.  */
unsigned long azure_module_pool_space[AZURE_BYTE_POOL_SIZE/4];
/* Application byte pool handle */
TX_BYTE_POOL *byte_pool_hndl = NULL;

extern int Thread_Init(void);


int platform_init(void)
{
  unsigned int status = TX_NOT_AVAILABLE;

  if (byte_pool_hndl)
    return TX_SUCCESS;

  /* Allocate the resources for the byte pool object */ 
  status = txm_module_object_allocate(&byte_pool_hndl, sizeof(TX_BYTE_POOL));
  if (status != TX_SUCCESS)
  {
    LogError("Allocating an object for memory pool failed with status[0x%x]\n", status);
    goto init_error;
  }
    
  /* Create a byte memory pool from which we allocate memory for the Azure SDK + Application */
  status = tx_byte_pool_create(byte_pool_hndl, "azure_mem_pool", azure_module_pool_space, AZURE_BYTE_POOL_SIZE);
  if (status != TX_SUCCESS)
  {
    LogError("Creating the memory pool failed with status[0x%x]\n", status);
    goto init_error;
  }  
      
  /* Initialize the thread management framework */
  status = Thread_Init();
  if (status)
    goto init_error; 
  
  status = TX_SUCCESS; 
 
init_error:
  
  /* Clean-up the resources */
  if (status != TX_SUCCESS)
  {
    if (byte_pool_hndl)
    {
      tx_byte_pool_delete(byte_pool_hndl);
      txm_module_object_deallocate(byte_pool_hndl);

      byte_pool_hndl = NULL;
    }
  }

  return status;
}


const IO_INTERFACE_DESCRIPTION* platform_get_default_tlsio(void)
{

  return (tlsio_qcom_threadx_get_interface_description());
}


void platform_deinit(void)
{

  return;
}


STRING_HANDLE platform_get_platform_info(void)
{

  return STRING_construct(AZURE_SDK_PLATFORM_LIB_VER);
}


void *malloc(size_t size)
{
  size_t *new_ptr = NULL; 
  UINT status = TX_SUCCESS;

  if (!size)
    return (void *)new_ptr; 

  /* Allocate resources from the byte pool */  
  status = tx_byte_allocate(byte_pool_hndl, (VOID **)&new_ptr, (sizeof(size_t) + size), TX_NO_WAIT);
  if ((TX_SUCCESS == status) &&
      (new_ptr))
  { 
    *new_ptr = size; 
    new_ptr += 1;   
  }

  return (void *)new_ptr;
}


void free(void *ptr)
{
  size_t *rel_ptr = NULL; 

  if (!ptr)
    return;
 
  rel_ptr = ((size_t *)ptr - 1);

  /* Release resources allocated from the byte pool */
  tx_byte_release((void *)rel_ptr);
}


void *calloc(size_t n_items, size_t size)
{
  size_t *new_ptr = NULL; 
  size_t total = n_items * size;
  UINT status = TX_SUCCESS;

  if (!total)
    return (void *)new_ptr; 

  /* Allocate resources from the byte pool */
  status = tx_byte_allocate(byte_pool_hndl, (VOID **)&new_ptr, (sizeof(size_t) + total), TX_NO_WAIT);
  if ((TX_SUCCESS == status) &&
      (new_ptr))
  { 
    *new_ptr = size; 
    new_ptr += 1;
    memset(new_ptr, 0x00, total);   
  }  

  return (void *)new_ptr;
}


void *realloc(void *ptr, size_t size)
{
  size_t *new_ptr = NULL; 
  UINT status = TX_SUCCESS;
  
  if (!size)
    return (void *)new_ptr; 
  
  if (!ptr)
  {
    /* Allocate resources from the byte pool */
    status = tx_byte_allocate(byte_pool_hndl, (VOID **)&new_ptr, (size + sizeof(size_t)), TX_NO_WAIT);
    if ((TX_SUCCESS == status) &&
        (new_ptr))
    {
     *new_ptr = size; 
      new_ptr += 1;
      
      return new_ptr; 
    }
  }
  else 
  {
    size_t ptr_size = *((size_t *)ptr - 1);

    if (ptr_size > size)
    {  
      return ptr;  
    }
    else
    {
      /* Allocate resources from the byte pool */
      status = tx_byte_allocate(byte_pool_hndl, (VOID **)&new_ptr, (size + sizeof(size_t)), TX_NO_WAIT);
      if ((TX_SUCCESS == status) &&
          (new_ptr))
      {        
        *new_ptr = size; 
        new_ptr += 1;
   
        /* Copy the resources to the new location */
        memcpy(new_ptr, ptr, ptr_size);  
        /* Release the old location */
        free(ptr);

        return new_ptr;
      }     
    }
  }

  return NULL; 
}


void azure_sdk_format_log(char *buffer, size_t buf_size, const char *fmt, ...)
{
  va_list arg_list;
     
  va_start(arg_list, fmt);  
  (void)vsnprintf(buffer, buf_size, fmt, arg_list);
  va_end(arg_list);
}


#ifdef QCOM_LLVM_STANDARD_LIB_OVERRIDE

time_t time(time_t* timer)
{
  return get_time(timer);
}

#endif


#ifdef QCOM_ARM_RVCT_STANDARD_LIB_OVERRIDE

/* 
 * strtoul.c --
 *
 *          Source code for the "strtoul" library procedure.
 *
 * Copyright (c) 1988 The Regents of the University of California.
 * Copyright (c) 1994 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id: //components/rel/data-iot-sdk/1.0/AZURE_SDK/porting-layer/src/platform_qcom_threadx.c#1 $
 */

#include <ctype.h>

/*
 * The table below is used to convert from ASCII digits to a
 * numerical equivalent.  It maps from '0' through 'z' to integers
 * (100 for non-digit characters).
 */

static char cvtIn[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,                    /* '0' - '9' */
    100, 100, 100, 100, 100, 100, 100,               /* punctuation */
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19,          /* 'A' - 'Z' */
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
    30, 31, 32, 33, 34, 35,
    100, 100, 100, 100, 100, 100,                    /* punctuation */
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19,          /* 'a' - 'z' */
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
    30, 31, 32, 33, 34, 35};

/*
 *----------------------------------------------------------------------
 *
 * strtoul --
 *
 *          Convert an ASCII string into an integer.
 *
 * Results:
 *          The return value is the integer equivalent of string.  If endPtr
 *          is non-NULL, then *endPtr is filled in with the character
 *          after the last one that was part of the integer.  If string
 *          doesn't contain a valid integer value, then zero is returned
 *          and *endPtr is set to string.
 *
 * Side effects:
 *          None.
 *
 *----------------------------------------------------------------------
 */ 
unsigned long int strtoul(const char *string, char **endPtr, int base)
{
    char *p;
    unsigned long int result = 0;
    unsigned digit;
    int anyDigits = 0;

    /*
     * Skip any leading blanks.
     */

    p = (char *)string;
    while (*p == ' ') {
          p += 1;
    }

    /*
     * If no base was provided, pick one from the leading characters
     * of the string.
     */
    
    if (base == 0)
    {
          if (*p == '0') {
              p += 1;
              if (*p == 'x') {
                    p += 1;
                    base = 16;
              } else {

                    /*
                     * Must set anyDigits here, otherwise "0" produces a
                     * "no digits" error.
                     */

                    anyDigits = 1;
                    base = 8;
              }
          }
          else base = 10;
    } else if (base == 16) {

          /*
           * Skip a leading "0x" from hex numbers.
           */

          if ((p[0] == '0') && (p[1] == 'x')) {
              p += 2;
          }
    }

    /*
     * Sorry this code is so messy, but speed seems important.  Do
     * different things for base 8, 10, 16, and other.
     */

    if (base == 8) {
          for ( ; ; p += 1) {
              digit = *p - '0';
              if (digit > 7) {
                    break;
              }
              result = (result << 3) + digit;
              anyDigits = 1;
          }
    } else if (base == 10) {
          for ( ; ; p += 1) {
              digit = *p - '0';
              if (digit > 9) {
                    break;
              }
              result = (10*result) + digit;
              anyDigits = 1;
          }
    } else if (base == 16) {
          for ( ; ; p += 1) {
              digit = *p - '0';
              if (digit > ('z' - '0')) {
                    break;
              }
              digit = cvtIn[digit];
              if (digit > 15) {
                    break;
              }
              result = (result << 4) + digit;
              anyDigits = 1;
          }
    } else {
          for ( ; ; p += 1) {
              digit = *p - '0';
              if (digit > ('z' - '0')) {
                    break;
              }
              digit = cvtIn[digit];
              if (digit >= base) {
                    break;
              }
              result = result*base + digit;
              anyDigits = 1;
          }
    }

    /*
     * See if there were any digits at all.
     */

    if (!anyDigits) {
          p = (char *)string;
    }

    if (endPtr != 0) {
          *endPtr = p;
    }

    return result;
}

#endif

