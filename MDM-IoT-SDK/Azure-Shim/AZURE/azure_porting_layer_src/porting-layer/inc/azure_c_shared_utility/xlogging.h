// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef XLOGGING_H
#define XLOGGING_H

#include "qapi_diag.h"
#ifndef AZURE_9206
#include "qflog_utils.h"
#else
#include "qapi_uart.h"
#include "quectel_utils.h"
#include "quectel_uart_apis.h"
extern QT_UART_CONF_PARA uart1_conf; 
#endif

#define AZURE_LOG_BUF_SIZE 256

#define LOG_NONE 0x00
#define LOG_LINE 0x01

typedef enum LOG_CATEGORY_TAG
{
  AZ_LOG_ERROR,
  AZ_LOG_INFO,
  AZ_LOG_TRACE
} LOG_CATEGORY;

//extern void azure_sdk_format_log(char *buffer, size_t buf_size, const char *fmt, ...);

#ifdef AZURE_9206

#define LOG(log_category, log_options, ...)  //do {\
//  char *azure_log_buf = malloc(AZURE_LOG_BUF_SIZE);\
//  if (azure_log_buf) {\
//    azure_sdk_format_log(azure_log_buf, AZURE_LOG_BUF_SIZE, __VA_ARGS__);\
//    azure_9206_log(azure_log_buf);\
//    free(azure_log_buf); }\
//} while(0)

#define LogInfo(...) do {\
    qt_uart_dbg(uart1_conf.hdlr, __VA_ARGS__);\
}while(0)\

//do {\
//  char *azure_log_buf = malloc(AZURE_LOG_BUF_SIZE);\
//  if (azure_log_buf) {\
//    azure_sdk_format_log(azure_log_buf, AZURE_LOG_BUF_SIZE, __VA_ARGS__);\
//    azure_9206_log(azure_log_buf);\
//    free(azure_log_buf); }\
//} while(0)

#define LogError(...) do {\
    qt_uart_dbg(uart1_conf.hdlr, __VA_ARGS__);\
}while(0)\

#else
#define LOG(log_category, log_options, ...)  do {\
  char *azure_log_buf = malloc(AZURE_LOG_BUF_SIZE);\
  if (azure_log_buf) {\
    azure_sdk_format_log(azure_log_buf, AZURE_LOG_BUF_SIZE, __VA_ARGS__);\
    if (LOG_NONE == log_options) {\
      /* Do nothing */ }\
    else if (AZ_LOG_ERROR == log_category) {\
      QFLOG_MSG(MSG_SSID_LINUX_DATA, MSG_LEGACY_ERROR, "%s", azure_log_buf);\
      QAPI_MSG_SPRINTF(MSG_SSID_LINUX_DATA, MSG_LEGACY_ERROR, "%s", azure_log_buf);}\
    else if (AZ_LOG_INFO == log_category) {\
      QFLOG_MSG(MSG_SSID_LINUX_DATA, MSG_LEGACY_HIGH, "%s", azure_log_buf);\
      QAPI_MSG_SPRINTF(MSG_SSID_LINUX_DATA, MSG_LEGACY_HIGH, "%s", azure_log_buf);}\
    else if (AZ_LOG_TRACE == log_category) {\
      QFLOG_MSG(MSG_SSID_LINUX_DATA, MSG_LEGACY_MED, "%s", azure_log_buf);\
      QAPI_MSG_SPRINTF(MSG_SSID_LINUX_DATA, MSG_LEGACY_MED, "%s", azure_log_buf);}\
    free(azure_log_buf); }\
} while(0)

#define LogInfo(...) do {\
  char *azure_log_buf = malloc(AZURE_LOG_BUF_SIZE);\
  if (azure_log_buf) {\
    azure_sdk_format_log(azure_log_buf, AZURE_LOG_BUF_SIZE, __VA_ARGS__);\
    QAPI_MSG_SPRINTF(MSG_SSID_LINUX_DATA, MSG_LEGACY_HIGH, "%s", azure_log_buf);\
    QFLOG_MSG(MSG_SSID_LINUX_DATA, MSG_LEGACY_HIGH, "%s", azure_log_buf);\
    free(azure_log_buf); }\
} while(0)

#define LogError(...) do {\
  char *azure_log_buf = malloc(AZURE_LOG_BUF_SIZE);\
  if (azure_log_buf) {\
  azure_sdk_format_log(azure_log_buf, AZURE_LOG_BUF_SIZE, __VA_ARGS__);\
  QAPI_MSG_SPRINTF(MSG_SSID_LINUX_DATA, MSG_LEGACY_ERROR, "%s", azure_log_buf);\
  QFLOG_MSG(MSG_SSID_LINUX_DATA, MSG_LEGACY_ERROR, "%s", azure_log_buf);\
  free(azure_log_buf); }\
} while (0)

#endif

#define LogBinary(...)
#define xlogging_get_log_function() NULL
#define xlogging_set_log_function(...)
#define LogErrorWinHTTPWithGetLastErrorAsString(...)
#define UNUSED(x) (void)(x)


#endif /* XLOGGING_H */
