// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
/*
 * Copyright (c) 2017 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "qapi_status.h"
#include "qapi_types.h"
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/tlsio.h"
#include "tlsio_qcom_threadx.h"
#include "azure_c_shared_utility/optimize_size.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "certs.h"

#undef ENOBUFS
#undef ETIMEDOUT
#undef EISCONN
#undef EOPNOTSUPP
#undef ECONNABORTED
#undef EWOULDBLOCK
#undef ECONNREFUSED
#undef ECONNRESET
#undef ENOTCONN
#undef EALREADY
#undef EMSGSIZE
#undef EPIPE
#undef EDESTADDRREQ
#undef ESHUTDOWN
#undef ENOPROTOOPT
#undef EADDRNOTAVAIL
#undef EADDRINUSE
#undef EAFNOSUPPORT
#undef EINPROGRESS
#undef ENOTSOCK
#undef ETOOMANYREFS
#undef EFAULT
#undef ENETUNREACH

#include "qapi_socket.h"
#include "qapi_ns_utils.h"
#include "qapi_ssl.h"
#include "qapi_netservices.h"


#define MAX_IFACE_NAME 15
#define htons(s)    ((((s) >> 8) & 0xff) | (((s) << 8) & 0xff00))
#define __FAILURE__ -1

/* SSL/TLS instance information */
typedef struct ssl_instance
{
  qapi_Net_SSL_Obj_Hdl_t ssl_ctx;
  qapi_Net_SSL_Con_Hdl_t ssl_conn;
  qapi_Net_SSL_Config_t config;
  uint8_t config_set;
  qapi_Net_SSL_Role_t role;
} SSL_INSTANCE;


/* SSL/TLS state information */
typedef enum TLSIO_STATE_TAG
{
  TLSIO_STATE_NOT_OPEN,
  TLSIO_STATE_OPEN,
  TLSIO_STATE_ERROR
} TLSIO_STATE;


typedef void* TlsContext;

/* SSL/TLS IO instance information */
typedef struct TLS_IO_INSTANCE_TAG
{
  ON_IO_OPEN_COMPLETE on_io_open_complete;
  void* on_io_open_complete_context;
  ON_BYTES_RECEIVED on_bytes_received;
  void* on_bytes_received_context;
  ON_IO_ERROR on_io_error;
  void* on_io_error_context;
  TLSIO_STATE tlsio_state;
  char* hostname;
  int port;
  char* certificate;
  int socket;
  TlsContext tls_context;
  char* x509_cert;  
  struct ip46addr server_ip;
  struct ip46addr client_ip;
  char iface_name[MAX_IFACE_NAME];
} TLS_IO_INSTANCE;

#define  QCOM_LOG_DATA_PACKET  1
/* Dump the Tx and Rx packets in the log in a formatted manner */
static void dump_packet (
  uint8_t *buffer,
  size_t ulength)
{
#define MAX_PACKET_SIZE 63
#define MAX_PACKET_BYTES_DUMP_PER_LINE 16
#define MAX_PACKET_HEX_DUMP_PER_LINE 57

  uint32_t i = 0, j = 0, k = 0, offset=0;
  static char logBuffer[MAX_PACKET_HEX_DUMP_PER_LINE];
  static char tempBuffer[MAX_PACKET_BYTES_DUMP_PER_LINE];
  uint8_t copyLen = 0;

  if (buffer == NULL)
    return;

  while (k < ulength)
  {
    uint32_t  copy_size = 0;

    if ((k + MAX_PACKET_BYTES_DUMP_PER_LINE) < ulength)
      copyLen =  MAX_PACKET_BYTES_DUMP_PER_LINE;
    else
      copyLen = ulength - k;

    memset(logBuffer, 0, MAX_PACKET_HEX_DUMP_PER_LINE);
    
    copy_size = (MAX_PACKET_BYTES_DUMP_PER_LINE <= copyLen)? MAX_PACKET_BYTES_DUMP_PER_LINE : copyLen;
    memcpy(tempBuffer, buffer + k, copy_size);
     
    i = 0;

    snprintf( (char *)logBuffer + i, MAX_PACKET_HEX_DUMP_PER_LINE - i, "%07X ", (unsigned int)(offset*MAX_PACKET_BYTES_DUMP_PER_LINE));

    i += 8; j = 0;

    while (i < MAX_PACKET_HEX_DUMP_PER_LINE && (j < copyLen) && (j < MAX_PACKET_BYTES_DUMP_PER_LINE))
    {
      snprintf((char *)logBuffer + i, MAX_PACKET_HEX_DUMP_PER_LINE - i, "%02X ", (unsigned int)tempBuffer[j]);
      i += 3;
      j++;
    }

    LogInfo(logBuffer);

    k += (MAX_PACKET_BYTES_DUMP_PER_LINE);
    offset++;
  }
}


/* Add the CA list to the SSL store */ 
static qapi_Status_t add_ca_list (
  TLS_IO_INSTANCE *tls_io_instance
)
{
  qapi_Status_t result = QAPI_ERROR;
  qapi_NET_SSL_CA_Info_t ca_info;
  qapi_Net_SSL_Cert_Info_t cert_info;

  /* Validate the input parameters */
  if (NULL == tls_io_instance)
  {
    LogError("Invalid input parameters - tls_io_instance[0x%x]\n", tls_io_instance);
    return QAPI_ERR_INVALID_PARAM;
  }

  LogInfo("Adding CA list to the internal SSL/TSL store\n");

  memset(&ca_info, 0x00, sizeof( qapi_NET_SSL_CA_Info_t));
  memset(&cert_info, 0x00, sizeof(qapi_Net_SSL_Cert_Info_t));

  /* SSL certificate authority list information */
  ca_info.ca_Buf = (uint8_t *)tls_io_instance->certificate;
  ca_info.ca_Size = strlen(tls_io_instance->certificate);
  
  /* SSL certificate authority list information */
  cert_info.cert_Type = QAPI_NET_SSL_CA_LIST_E;
  cert_info.info.ca_List.ca_Cnt = 0x01;  
  cert_info.info.ca_List.ca_Info[0] = &ca_info;

  /* Covert and store the certificate authority list information */ 
  if ((result = qapi_Net_SSL_Cert_Convert_And_Store(&cert_info, (const uint8_t *)"ca_list.bin")) != QAPI_OK)
  {
    LogError("Converting and storing CA list internally failed with status[%d] for handle[0x%x]\n", result, tls_io_instance);
  }
  else
  {
    LogInfo("Successfully converted and stored the CA list internally for handle[0x%x]\n", tls_io_instance); 
  }
  
  return result;
}


/* Resolve the hostname using DNS client */ 
static qapi_Status_t resolve_host (
  TLS_IO_INSTANCE *tls_io_instance
)
{
  struct ip46addr ip_addr;
  char *host_name = NULL;
  qapi_Status_t result = QAPI_ERROR;

  /* Validate the input parameters */
  if (NULL == tls_io_instance)
  {
    LogError("Invalid input parameters - tls_io_instance[0x%x]\n", tls_io_instance);
    return QAPI_ERR_INVALID_PARAM;
  }

  /* Ensure DNS client is started */
  if (!qapi_Net_DNSc_Is_Started())
  {
    LogError("DNS client not started and primary/secondary DNS servers not configured!\n");
    return result;    
  }

  /* Hostname */
  host_name = tls_io_instance->hostname;
        
  /* IPv4 address */
  if (!(inet_pton(AF_INET, (char *)host_name, (void *)&(tls_io_instance->server_ip.a.addr4))))
  {
    tls_io_instance->server_ip.type = AF_INET;
    LogInfo("Successfully resolved hostname[%s]->address[%d] for handle[0x%x]\n", host_name, tls_io_instance->server_ip.a.addr4, tls_io_instance);

    return QAPI_OK;
  }

  /* IPv6 address */
  if (!(inet_pton(AF_INET6, (char *)host_name, (void *)&(tls_io_instance->server_ip.a.addr6))))
  {
    char ntop_buf[40];

    tls_io_instance->server_ip.type = AF_INET6;
    LogInfo("Successfully resolved hostname[%s]->address[%s] for handle[0x%x]\n", host_name, 
                                                                                  inet_ntop(AF_INET6, (void *)&(tls_io_instance->server_ip.a.addr6), ntop_buf, sizeof(ntop_buf)), 
                                                                                  tls_io_instance);
    return QAPI_OK;
  }

  memset (&ip_addr, 0x00, sizeof(struct ip46addr));  
  LogInfo("Resolving hostname [%s] on interface[%s]\n", tls_io_instance->hostname, tls_io_instance->iface_name);

  /* Resolving using IPv4 DNS server */
  ip_addr.type = AF_INET;
  if (QAPI_OK == (result = qapi_Net_DNSc_Reshost_on_iface(host_name, &ip_addr, tls_io_instance->iface_name)))
  {
    tls_io_instance->server_ip.type = AF_INET;
    tls_io_instance->server_ip.a.addr4 = ip_addr.a.addr4; /* Network order */

    LogInfo("Successfully resolved hostname[%s]->address[%d] for handle[0x%x]\n", host_name, tls_io_instance->server_ip.a.addr4, tls_io_instance);
  
    return QAPI_OK;
  }
  
  /* Resolve using IPv6 DNS server */
  ip_addr.type = AF_INET6;
  if (QAPI_OK == (result = qapi_Net_DNSc_Reshost_on_iface(host_name, &ip_addr, tls_io_instance->iface_name)))
  {
    char ntop_buf[40];

    tls_io_instance->server_ip.type = AF_INET6;
    memcpy(&tls_io_instance->server_ip.a.addr6, &ip_addr.a.addr6, sizeof(ip6_addr));

    LogInfo("Successfully resolved hostname[%s]->address[%s] for handle[0x%x]\n", host_name, 
                                                                                  inet_ntop(AF_INET6, (void *)&(tls_io_instance->server_ip.a.addr6), ntop_buf, sizeof(ntop_buf)), 
                                                                                  tls_io_instance);
    return QAPI_OK;
  }
  
  LogError("Resolving hostname[%s] failed with status[%d] for handle[0x%x]\n", host_name, result, tls_io_instance);
  return result;
}


/* Perform the TLS handshake and connect to the server (internal) */
static qapi_Status_t tls_connect (
  TLS_IO_INSTANCE *tls_io_instance
)
{
  int family = 0x00;
  struct sockaddr_in s_addr;
  struct sockaddr_in6 s_addr6;
  struct sockaddr *to = NULL;
  uint32_t tolen = 0x00;
  SSL_INSTANCE *ssl = NULL;
  qapi_Status_t result = QAPI_ERROR;  

  /* Validate the input parameters */
  if (NULL == tls_io_instance)
  {
    LogError("Invalid input parameters - tls_io_instance[0x%x]\n", tls_io_instance);
    return QAPI_ERR_INVALID_PARAM;
  }

  /* Resolve the hostname */
  LogInfo("Resolving Hostname [%s]\n", tls_io_instance->hostname);  
  if (resolve_host(tls_io_instance) != QAPI_OK)
    goto TLS_CONNECT_ERROR;

  /* Retrieve the IP family information */
  family = tls_io_instance->server_ip.type;

  /* Create socket */
  if (-1 == (tls_io_instance->socket = qapi_socket(family, SOCK_STREAM, 0x00)))
  {
    LogError("Unable to create socket\n");
    goto TLS_CONNECT_ERROR;
  }

  /* Bind the socket to a specific interface (if configured for non-default APN) */
  if (tls_io_instance->client_ip.type)
  {
    if (AF_INET == tls_io_instance->client_ip.type)
    {
      memset(&s_addr, 0, sizeof(struct sockaddr_in));
      s_addr.sin_addr.s_addr = tls_io_instance->client_ip.a.addr4;
      s_addr.sin_port = 0x00;
      s_addr.sin_family = AF_INET;
      to = (struct sockaddr *)&s_addr;
      tolen = sizeof(s_addr);
    }
    else
    {
      memset(&s_addr6, 0, sizeof(struct sockaddr_in6));
      s_addr6.sin_family = family;
      memcpy(&s_addr6.sin_addr, &tls_io_instance->client_ip.a.addr6, sizeof(ip6_addr));
      s_addr6.sin_port = 0x00;
      to = (struct sockaddr *)&s_addr6;
      tolen = sizeof(s_addr6);
    }
 
    if ((result = qapi_bind(tls_io_instance->socket, to, tolen)) != QAPI_OK)
    {
      LogError("Binding the client IP to the socket failed with status[0x%x]\n", result);
      goto TLS_CONNECT_ERROR;
    }
  }
    
  /* IPv4 */
  if (AF_INET == family)
  {
    memset(&s_addr, 0, sizeof(struct sockaddr_in));
    s_addr.sin_addr.s_addr = tls_io_instance->server_ip.a.addr4;
    s_addr.sin_port = htons(tls_io_instance->port);
    s_addr.sin_family = family;
    to = (struct sockaddr *)&s_addr;
    tolen = sizeof(s_addr);
  }
  else /* IPv6 */
  {
    memset(&s_addr6, 0, sizeof(struct sockaddr_in6));
    s_addr6.sin_family = family;
    memcpy(&s_addr6.sin_addr, &tls_io_instance->server_ip.a.addr6, sizeof(ip6_addr));
    s_addr6.sin_port = htons(tls_io_instance->port);
    to = (struct sockaddr *)&s_addr6;
    tolen = sizeof(s_addr6);
  }
    
  /* Connect to the server */
  if ((result = qapi_connect(tls_io_instance->socket, to, tolen)) != QAPI_OK)
  {
    LogError("Connecting to server failed with status[%d] for handle[0x%x]\n", result, tls_io_instance);
    goto TLS_CONNECT_ERROR;
  } 
    
  ssl = tls_io_instance->tls_context;

  if (ssl->ssl_conn == QAPI_NET_SSL_INVALID_HANDLE)
  {
    /* Load the X.509 client certificate */      
    if (tls_io_instance->x509_cert)
    {
      if ((result = qapi_Net_SSL_Cert_Load(ssl->ssl_ctx, QAPI_NET_SSL_CERTIFICATE_E, tls_io_instance->x509_cert)) != QAPI_OK)
      {
        LogError("Loading X.509 certificate failed with status[%d] for handle[0x%x]\n", result, tls_io_instance);
        goto TLS_CONNECT_ERROR;
      }
    }
              
    /* Load the server root certificate authority list */
    if ((result = qapi_Net_SSL_Cert_Load(ssl->ssl_ctx, QAPI_NET_SSL_CA_LIST_E, (const char*)"ca_list.bin")) != QAPI_OK )
    {
      LogError("Loading \"ca_list.bin\" failed with status[%d] for handle[0x%x]\n", result, tls_io_instance);
      goto TLS_CONNECT_ERROR;
    }
  
    /* Create SSL connection object */
    if (QAPI_NET_SSL_INVALID_HANDLE == (ssl->ssl_conn = qapi_Net_SSL_Con_New(ssl->ssl_ctx, QAPI_NET_SSL_TLS_E)))
    {
      LogError("Creating SSL connection object failed with status[0x%x] for handle[0x%x]\n", QAPI_NET_SSL_INVALID_HANDLE, tls_io_instance);
      goto TLS_CONNECT_ERROR;
    }
            
    /* Configure the SSL connection */
    if (ssl->config_set)
    {
      if ((result = qapi_Net_SSL_Configure(ssl->ssl_conn, &ssl->config)) != QAPI_OK)
      {
        LogError("SSL configuration failed with status[0x%x] for handle[0x%x]\n", result, tls_io_instance);
        goto TLS_CONNECT_ERROR;
      }
    }
  }

  /* Add socket handle to SSL connection */
  if ((result = qapi_Net_SSL_Fd_Set(ssl->ssl_conn, tls_io_instance->socket)) != QAPI_OK)
  {
    LogError("Add socket handle to SSL connection failed with status[%d]\n", result);
    goto TLS_CONNECT_ERROR;
  }

  /* Perform SSL/TLS handshake with server */
  result = qapi_Net_SSL_Connect(ssl->ssl_conn);
  if (QAPI_SSL_OK_HS == result)
  {
    /* Peer's SSL certificate is trusted, CN matches the host name, time is valid */
    LogInfo("Certificate is trusted and the connection is successful to host[%s] for handle[0x%x]\n", tls_io_instance->hostname, tls_io_instance);
    result = QAPI_OK;
  }
  else
  {
    LogError("SSL connect failed with status[%d] for handle[0x%x]\n", result, tls_io_instance);
    goto TLS_CONNECT_ERROR;
  }
      
TLS_CONNECT_ERROR:

  /* Clean-up the resources */
  if (result != QAPI_OK)
  {
    if ((ssl) &&
        (ssl->ssl_conn != QAPI_NET_SSL_INVALID_HANDLE))
    {
      /* Close SSL connection */
      qapi_Net_SSL_Shutdown(ssl->ssl_conn);
      ssl->ssl_conn = QAPI_NET_SSL_INVALID_HANDLE;
    }

    /* Close the opened socket handle */
    qapi_socketclose(tls_io_instance->socket);
  }

  return result;
}


/* Read application data received securely from the server (internal) */
static int tls_read(TLS_IO_INSTANCE* tls_io_instance,unsigned char* buffer, int size, int* received)
{
  SSL_INSTANCE* ssl = tls_io_instance->tls_context;
  fd_set rset;
  int conn_sock;

  qapi_fd_zero(&rset);
  qapi_fd_set(tls_io_instance->socket, &rset);
    
  conn_sock = qapi_select(&rset, NULL, NULL, 500);
  /* No activity */
  if (0x00 == conn_sock) 
    *received = 0;
  else 
    *received = qapi_Net_SSL_Read(ssl->ssl_conn, buffer, size);

  return QAPI_OK;
}


/* Clone an option give a name and value */
static void* tlsio_qcom_threadx_clone_option (
  const char* name, 
  const void* value
)
{
  void* result = NULL;

  /* Validate input parameters */
  if ((name == NULL) || (value == NULL))
  {
    LogError("Invalid params - name[0x%x], value[0x%x]\n", name, value);
    goto TLS_CLONE_ERROR;
  }

  LogInfo("Cloning tlsio option - [%s]\n", name);  
  
  if (strncmp(name, "TrustedCerts", strlen(name)) == 0)
  {
    if (mallocAndStrcpy_s((char**)&result, value) != 0)
    {
      LogError("Unable to clone option[%s]\n", name);
      goto TLS_CLONE_ERROR;
    }
  }
  else if (strncmp(name, "x509", strlen(name)) == 0)
  {
    if (mallocAndStrcpy_s((char**)&result, value) != 0)
    {
      LogError("Unable to clone option[%s]\n", name);
      goto TLS_CLONE_ERROR;
    }
  }
  else if (strncmp(name, "InterfaceName", strlen(name)) == 0)
  {
    if (mallocAndStrcpy_s((char**)&result, value) != 0)
    {
      LogError("Unable to clone option[%s]\n", name);
    }
  }
  else if (strncmp(name, "InterfaceAddress", strlen(name)) == 0)
  {
    result = malloc(sizeof(struct ip46addr));
    if (NULL == result)
    {
      LogError("Unable to clone option[%s]\n", name);
    }
    else
    {    
      memcpy(result, value, sizeof(struct ip46addr));            
    }
  }
  else
  {
    LogError("Invalid option[%s] - not handled\n" , name);
    goto TLS_CLONE_ERROR;
  }
  
  LogInfo("Successfully cloned tlsio option - [%s]\n", name);

TLS_CLONE_ERROR:
  return result;
}


/* Destroys an option */
static void tlsio_qcom_threadx_destroy_option (
  const char* name, 
  const void* value
)
{
  /* All options for this layer are actually string copies., disposing of one is just calling free */
  if ((name == NULL) || (value == NULL))
  {
    LogError("Invalid params - name[0x%x], value[0x%x]\n", name, value);
    goto TLS_DESTROY_OPTION;
  }

  LogInfo("Destroying tlsio option - [%s]\n", name);  
  
  if ((strncmp(name, "TrustedCerts", strlen(name)) == 0)||
      (strncmp(name, "x509", strlen(name)) == 0))
  {
    free((void*)value);
  }
  else if ((strncmp(name, "InterfaceAddress", strlen(name)) == 0) ||
           (strncmp(name, "InterfaceName", strlen(name)) == 0))
  {
     /* Ignore the case */
      
  }
  else
  {
    LogError("Invalid option[%s] - not handled\n" , name);
    goto TLS_DESTROY_OPTION;
  }

  LogInfo("Successfully destroying tlsio option - [%s]\n", name);  
    
TLS_DESTROY_OPTION:
  return;
}


/* Create TLS/SSL context IO handle */ 
static CONCRETE_IO_HANDLE tlsio_qcom_threadx_create (
  void *io_create_parameters
)
{
  TLS_IO_INSTANCE *tls_instance = NULL;
  SSL_INSTANCE *ssl = NULL;
  TLSIO_CONFIG *tls_io_config = io_create_parameters;  
  qapi_Status_t result = QAPI_ERROR;
  
  LogInfo("Creating TLS/SSL context IO handle[0x%x]\n", tls_instance);  

  /* Validate the input parameters */
  if (tls_io_config == NULL)
  {
    LogError("Invalid input parameters - io_create_parameters[0x%x]\n", tls_io_config);
    return NULL;
  }
    
  /* Validate the hostname */
  if (tls_io_config->hostname == NULL)
  {
    LogError("Invalid hostname [0x%x]\n", tls_io_config->hostname);
    return NULL;
  }

  /* Allocate resources for the TLS IO instance */
  tls_instance = malloc(sizeof(TLS_IO_INSTANCE));
  if (!tls_instance)
  {
    LogError("Allocating resources for TLS IO instance failed\n");
    return NULL;
  }

  memset(tls_instance, 0x00, sizeof(TLS_IO_INSTANCE));
  
  /* Store the default interface name */
  if (strcpy_s(tls_instance->iface_name, MAX_IFACE_NAME, "rmnet_data0") != 0)
  {
    LogError("Storing the interface name internally failed\n");
    goto TLS_CREATE_ERROR;
  }   

  /* Store the hostname for later use in open */
  if (mallocAndStrcpy_s(&tls_instance->hostname, tls_io_config->hostname) != 0)
  {
    LogError("Storing the hostname failed\n");
    goto TLS_CREATE_ERROR;
  }
     
  /* Store the TLS IO config information */
  tls_instance->port = tls_io_config->port;
  tls_instance->certificate = NULL;
  tls_instance->on_bytes_received = NULL;
  tls_instance->on_bytes_received_context = NULL;
  tls_instance->on_io_open_complete = NULL;
  tls_instance->on_io_open_complete_context = NULL;
  tls_instance->on_io_error = NULL;
  tls_instance->on_io_error_context = NULL;
  tls_instance->tlsio_state = TLSIO_STATE_NOT_OPEN;

  /* Allocate resource for the SSL instance */
  ssl = malloc(sizeof(SSL_INSTANCE)); 
  if (ssl == NULL) 
  {
    LogError("Unable to allocate resources for SSL instance\n");
    goto TLS_CREATE_ERROR;
  }

  memset(ssl, 0, sizeof(SSL_INSTANCE)); 

  /* Set SSL role to client */
  ssl->role = QAPI_NET_SSL_CLIENT_E;  

  /* Create a handle to the internal SSL/TLS context */  
  ssl->ssl_ctx = qapi_Net_SSL_Obj_New(ssl->role); 
  if (ssl->ssl_ctx == QAPI_NET_SSL_INVALID_HANDLE)
  {
    LogError("Unable to create SSL context\n");
    goto TLS_CREATE_ERROR;
  }

  memset(&ssl->config, 0, sizeof(qapi_Net_SSL_Config_t));
  
  /* TLS/SSL application specific configuration */
  ssl->config_set = 1;
  ssl->config.verify.time_Validity = 1;

  ssl->config.max_Frag_Len = 4096;

  /* Store the SSL instance information */
  tls_instance->tls_context = ssl; 

  result = QAPI_OK; 
  LogInfo("Successfully created TLS/SSL context IO handle[0x%x]\n", tls_instance);               
               
TLS_CREATE_ERROR:

  /* Clean-up the allocated resources */
  if (result != QAPI_OK)
  {
    if (ssl)
    {
      if (ssl->ssl_ctx)
        qapi_Net_SSL_Obj_Free(ssl->ssl_ctx);
      
      free(ssl);
    }
   
    if (tls_instance)
    {
      if (tls_instance->hostname)
        free(tls_instance->hostname);
    
      free(tls_instance);
      tls_instance = NULL; 
    }
  }

  return tls_instance;
}


/* Close the TLS/SSL connection */
static int tlsio_qcom_threadx_close (
  CONCRETE_IO_HANDLE tls_io, 
  ON_IO_CLOSE_COMPLETE on_io_close_complete, 
  void *on_io_close_complete_context
)
{
  qapi_Status_t result = QAPI_ERROR;
  TLS_IO_INSTANCE *tls_io_instance = (TLS_IO_INSTANCE *)tls_io;
  SSL_INSTANCE *ssl = NULL; 

  /* Validate the input parameters */
  if (NULL == tls_io_instance)
  {
    LogError("Invalid input parameters - tls_io[0x%x]\n", tls_io);
    goto TLS_CLOSE_ERROR;
  }

  ssl = tls_io_instance->tls_context;
    
  LogInfo("Closing TLS/SSL connection[0x%x] for handle[0x%x]\n", ssl->ssl_conn, tls_io_instance); 

  /* Validate the TLS state */
  if (TLSIO_STATE_NOT_OPEN == tls_io_instance->tlsio_state)
  {    
    LogError("TLS IO state invalid - current/expected state [%d/%d]\n", tls_io_instance->tlsio_state, TLSIO_STATE_NOT_OPEN);
    goto TLS_CLOSE_ERROR;
  }
        
  if (ssl->ssl_conn)
  {
    /* Close the SSL/TLS connection */
    if ((result = qapi_Net_SSL_Shutdown(ssl->ssl_conn)) != QAPI_OK)
    {
      LogError("Shutting down TLS connection[0x%x] failed with status[%d]\n", ssl->ssl_conn, result);
      goto TLS_CLOSE_ERROR;
    }
  
    /* Close the socket handle associated with the TLS/SSL connection */
    if ((result = qapi_socketclose(tls_io_instance->socket)) != QAPI_OK)
    {
      LogError("Closing the socket[0x%x] failed with status[%d]\n", tls_io_instance->socket, result);
      goto TLS_CLOSE_ERROR;
    }
    
    tls_io_instance->socket = (int)NULL;
    tls_io_instance->tlsio_state = TLSIO_STATE_NOT_OPEN;

    /* Trigger the TLS IO close completion callback */
    if (on_io_close_complete != NULL)
      on_io_close_complete(on_io_close_complete_context);
  }
       
  ssl->ssl_conn = QAPI_NET_SSL_INVALID_HANDLE;  

  result = QAPI_OK;  
  LogInfo("Successfully closed TLS/SSL connection[0x%x] for handle\n", ssl->ssl_conn, tls_io_instance);

TLS_CLOSE_ERROR:
  return (result) ? __FAILURE__ : 0x00; 
}


/* Destroy TLS/SSL context IO handle */ 
static void tlsio_qcom_threadx_destroy(CONCRETE_IO_HANDLE tls_io)
{
  TLS_IO_INSTANCE *tls_io_instance = (TLS_IO_INSTANCE *)tls_io;
  SSL_INSTANCE *ssl = NULL; 

  LogInfo("Destroying TLS/SSL context IO handle[0x%x]\n", tls_io_instance);

  /* Validate the input parameters */
  if (NULL == tls_io_instance)
  {
    LogError("Invalid input parameters - tls_io[0x%x]\n", tls_io);
    return;
  }
  
  /* Close any opened SSL/TLS connection */
  tlsio_qcom_threadx_close(tls_io, NULL, NULL);
  
  /* Release the SSL object information */
  ssl = tls_io_instance->tls_context;
  if (ssl->ssl_ctx)
  {
    qapi_Net_SSL_Obj_Free(ssl->ssl_ctx);
    ssl->ssl_ctx = QAPI_NET_SSL_INVALID_HANDLE;
  }

  /* Release the resources associated with the CA list */
  if (tls_io_instance->certificate != NULL)
    free(tls_io_instance->certificate);
  
  /* Release the resources associated with the client certificate */
  if (tls_io_instance->x509_cert != NULL)
    free(tls_io_instance->x509_cert);

  /* Release resources associated with the hostname */
  if (tls_io_instance->hostname)
    free(tls_io_instance->hostname);

  LogInfo("Successfully destroyed TLS/SSL context IO handle[0x%x]\n", tls_io_instance);
 
  /* Release the TLS IO instance */
  free(tls_io_instance);

  return;
}


/* Connect to the TLS/SSL server by performing the handshake */
static int tlsio_qcom_threadx_open (
  CONCRETE_IO_HANDLE tls_io, 
  ON_IO_OPEN_COMPLETE on_io_open_complete, 
  void *on_io_open_complete_context, 
  ON_BYTES_RECEIVED on_bytes_received, 
  void *on_bytes_received_context, 
  ON_IO_ERROR on_io_error, 
  void *on_io_error_context)
{
  qapi_Status_t result = QAPI_ERROR;  
  TLS_IO_INSTANCE *tls_io_instance = (TLS_IO_INSTANCE*)tls_io;

  LogInfo("Connecting to TLS/SSL server for handle[0x%x]\n", tls_io_instance);

  /* Validate the input parameters */
  if ((NULL == tls_io) ||
      (NULL == on_io_open_complete) ||
      (NULL == on_bytes_received) ||
      (NULL == on_io_error))
  {
    LogError("Invalid input parameters - tls_io[0x%x], on_io_open_complete[0x%x], on_bytes_received[0x%x], on_io_error[0x%x]\n", tls_io,
                                                                                                                                 on_io_open_complete,
                                                                                                                                 on_bytes_received,
                                                                                                                                 on_io_error);                                 
    goto TLS_CONNECT_ERROR;
  }
   
  /* Validate the TLS state */
  if (tls_io_instance->tlsio_state != TLSIO_STATE_NOT_OPEN)
  {    
    LogError("TLS IO state invalid - current/expected state [%d/%d]\n", tls_io_instance->tlsio_state, TLSIO_STATE_NOT_OPEN);
    goto TLS_CONNECT_ERROR;
  }

  /* Configure the TLS IO instance information */
  tls_io_instance->on_bytes_received = on_bytes_received;
  tls_io_instance->on_bytes_received_context = on_bytes_received_context;
  tls_io_instance->on_io_error = on_io_error;
  tls_io_instance->on_io_error_context = on_io_error_context;
  tls_io_instance->on_io_open_complete = on_io_open_complete;
       
  /* Perform the TLS connection */                   
  if ((result = tls_connect(tls_io_instance)) != QAPI_OK)
  {
    LogError("TLS IO connection failed with status[%d]\n", result);
    goto TLS_CONNECT_ERROR;
  }
                   
  /* Transition the TLS instance state to "open" */             
  tls_io_instance->tlsio_state = TLSIO_STATE_OPEN;
  /* Trigger the TLS IO open completion callback */
  on_io_open_complete(on_io_open_complete_context, IO_OPEN_OK);

  result = QAPI_OK;  
  LogInfo("Successfully connected to TLS/SSL server[%s] for handle[0x%x]\n", tls_io_instance->hostname, tls_io_instance);
                 
TLS_CONNECT_ERROR:
  return ((result) ? __FAILURE__ : 0x00); 
}


/* Send application data to the SSL/TLS server securely */
static int tlsio_qcom_threadx_send (
  CONCRETE_IO_HANDLE tls_io, 
  const void* buffer, 
  size_t size, 
  ON_SEND_COMPLETE on_send_complete, 
  void* on_send_complete_context)
{
  int data_tx = 0x00; 
  qapi_Status_t result = QAPI_ERROR;
  TLS_IO_INSTANCE *tls_io_instance = (TLS_IO_INSTANCE *)tls_io;
  SSL_INSTANCE *ssl = NULL; 

  LogInfo("Sending %d bytes of data to TLS/SSL server for handle[0x%x]\n", size, tls_io_instance);

  /* Validate the input parameters */
  if ((NULL == tls_io_instance) || 
      (NULL == buffer) ||
      (!size))
  {
    LogError("Invalid input parameters - tls_io[0x%x], buffer[0x%x], size[0x%x]\n", tls_io, buffer, size);
    goto TLS_SEND_ERROR;
  }

  ssl = tls_io_instance->tls_context;

  /* Validate the TLS state information */
  if (TLSIO_STATE_OPEN != tls_io_instance->tlsio_state)
  {    
    LogError("TLS IO state invalid - current/expected state [%d/%d]\n", tls_io_instance->tlsio_state, TLSIO_STATE_OPEN);
    goto TLS_SEND_ERROR;
  }
        
  /* Send the application data securely using SSL/TLS */
  if (0x00 == qapi_Net_SSL_Write(ssl->ssl_conn, (void*)buffer, size))
  {    
    LogError("TLS/SSL failed to encrypt the data with status[0x%x] for handle[0x%x]\n", data_tx, tls_io_instance);
    goto TLS_SEND_ERROR;
  }       
       
  /* Trigger the TLS IO send completion callback */   
  if (on_send_complete != NULL)
    on_send_complete(on_send_complete_context, IO_SEND_OK);

#ifdef QCOM_LOG_DATA_PACKET  
  {
    LogInfo("Tx packet start\n");                        
    dump_packet((uint8_t *)buffer, size);
    LogInfo("Tx packet end\n");  
  }
#endif 

  result = QAPI_OK;  
  LogInfo("Successfully sent %d bytes of data to TLS/SSL server[%s] for handle[0x%x]\n", size, tls_io_instance->hostname, tls_io_instance);
          
TLS_SEND_ERROR:
  return (result) ? __FAILURE__ : 0x00;
}


/* Perform the requested work */ 
static void tlsio_qcom_threadx_dowork (

  CONCRETE_IO_HANDLE tls_io)
{
  unsigned char rx_buffer[64];
  int rx_bytes = 0x01;
  TLS_IO_INSTANCE *tls_io_instance = (TLS_IO_INSTANCE *)tls_io;

  LogInfo("Reading data from TLS/SSL server for handle[0x%x]\n", tls_io_instance);

  /* Validate the input parameters */
  if (NULL == tls_io_instance)
  {
    LogError("Invalid input parameters - tls_io[0x%x]\n", tls_io);
    goto TLS_DOWORK_ERROR;
  }
 
  /* Only perform work if we are not in error */
  if ((tls_io_instance->tlsio_state != TLSIO_STATE_NOT_OPEN) &&
      (tls_io_instance->tlsio_state != TLSIO_STATE_ERROR))
  {
    while(rx_bytes > 0)
    {
      if (tls_read(tls_io_instance, rx_buffer, sizeof(rx_buffer), &rx_bytes) != 0) 
      {
        LogError("Reading data failed from TLS/SSL server[%s] for handle[0x%x]\n", tls_io_instance->hostname, tls_io_instance);
  
        /* Mark state as error and indicate it to the upper layers */
        tls_io_instance->tlsio_state = TLSIO_STATE_ERROR;
        tls_io_instance->on_io_error(tls_io_instance->on_io_error_context);
      }
      else
      {
        if (rx_bytes > 0)
        {
          LogInfo("Successfully received [%d] bytes from TLS/SSL server[%s] for handle[0x%x]\n", rx_bytes, tls_io_instance->hostname, tls_io_instance);
  
#ifdef QCOM_LOG_DATA_PACKET
          {
            LogInfo("Rx packet start\n");                        
            dump_packet(rx_buffer, rx_bytes);
            LogInfo("Rx packet end\n"); 
          }
#endif 

          /* if bytes have been received indicate them */
          tls_io_instance->on_bytes_received(tls_io_instance->on_bytes_received_context, rx_buffer, rx_bytes);
        }
        else if (rx_bytes < 0)
        {
          LogError("Reading data failed with status[0x%x] from TLS/SSL server[%s] for handle[0x%x]\n", rx_bytes, tls_io_instance->hostname, tls_io_instance);
  
          /* Mark state as error and indicate it to the upper layers */
          tls_io_instance->tlsio_state = TLSIO_STATE_ERROR;
          tls_io_instance->on_io_error(tls_io_instance->on_io_error_context);
        }
      }
    }
  }

TLS_DOWORK_ERROR:
  return; 
}


/* Set SSL/TLS options */
static int tlsio_qcom_threadx_setoption (
  CONCRETE_IO_HANDLE tls_io, 
  const char* optionName, 
  const void* value)
{
  qapi_Status_t result = QAPI_ERROR;
  TLS_IO_INSTANCE *tls_io_instance = (TLS_IO_INSTANCE *)tls_io;
  
  /* Validate the input parameters */
  if ((NULL == tls_io_instance) ||
      (NULL == optionName) ||
      (NULL == value))
  {
    LogError("Invalid input parameters - tls_io[0x%x], optionName[0x%x], value[0x%x]\n", tls_io, optionName, value);
    goto TLS_SET_OPT_ERROR;
  }

  LogInfo("Setting TLS/SSL option[%s] for handle[0x%x]\n", optionName, tls_io_instance);
          
  /* Server root certificate authority list */
  if (0x00 == strncmp(optionName, "TrustedCerts", strlen(optionName)))
  {
    LogInfo("Setting trusted certificate option\n"); 

    /* Release any allocated resources for any stored trusted certificates */
    if (tls_io_instance->certificate != NULL)
    {
      free(tls_io_instance->certificate);
      tls_io_instance->certificate = NULL;
    }

    /* Certificate authority information is not present */
    if (NULL == (const char *)value)
    {
      result = QAPI_OK;
      goto TLS_SET_OPT_ERROR;
    }
           
    /* Store the certificate internally in the context */
    if (mallocAndStrcpy_s(&tls_io_instance->certificate, (const char *)value) != 0)
    {
      LogError("Storing the trusted certificate internally failed\n");
      goto TLS_SET_OPT_ERROR;
    }

    /* Add the trusted certificate to the CA list in the TLS/SSL layer */
    if (tls_io_instance->certificate != NULL)
    {
      if (add_ca_list(tls_io_instance) != QAPI_OK)
        goto TLS_SET_OPT_ERROR;
    }               
  }
  /* Client certificate */
  else if (0x00 == strncmp(optionName, "x509", strlen(optionName)))
  {
    LogInfo("Setting X.509 certificate option\n"); 

    /* Release any allocated resources for any stored X.509 certificates */
    if (tls_io_instance->x509_cert != NULL)
    {
      free(tls_io_instance->x509_cert);
      tls_io_instance->x509_cert = NULL;
    }

    /* Certificate is not present */
    if (NULL == (const char *)value)
    {
      result = QAPI_OK;
      goto TLS_SET_OPT_ERROR;
    }

    /* Store the name of X.509 cert, it should have been stored before the client is initialized */
    if (mallocAndStrcpy_s(&tls_io_instance->x509_cert, (const char *)value) != 0)
    {
      LogError("Storing the X.509 certificate internally failed\n");
      goto TLS_SET_OPT_ERROR;
    }
  }
  /* Interface name */
  else if (0x00 == strncmp(optionName, "InterfaceName", strlen(optionName)))
  {
    LogInfo("Setting interface name option\n"); 

    if (strcpy_s(tls_io_instance->iface_name, MAX_IFACE_NAME, (const char *)value) != 0)
    {
      LogError("Storing the interface name internally failed\n");
      goto TLS_SET_OPT_ERROR;
    }          
  }
  /* Interface address */
  else if (0x00 == strncmp(optionName, "InterfaceAddress", strlen(optionName)))
  {
    LogInfo("Setting interface address option\n"); 
    memcpy(&tls_io_instance->client_ip, (struct ip46addr *)value, sizeof(struct ip46addr));
  }
  else
  {
    LogError("Unrecognized option[%s] not supported", optionName);
    goto TLS_SET_OPT_ERROR;
  }

  result = QAPI_OK;  
  LogInfo("Successfully set TLS/SSL option[%s] for handle[0x%x]\n", optionName, tls_io_instance);
  
TLS_SET_OPT_ERROR:
  return (result) ? __FAILURE__ : 0x00;
}


/* Retrieve SSL/TLS options */
static OPTIONHANDLER_HANDLE tlsio_qcom_threadx_retrieve_options(CONCRETE_IO_HANDLE tls_io)
{
  qapi_Status_t result = QAPI_ERROR;
  OPTIONHANDLER_HANDLE opt_hndl = NULL;
  TLS_IO_INSTANCE *tls_io_instance = (TLS_IO_INSTANCE *)tls_io;

  LogInfo("Retrieving TLS/SSL options for handle[0x%x]\n", tls_io_instance);
    
  /* Validate the input parameters */
  if (NULL == tls_io_instance) 
  {
    LogError("Invalid input parameters - tls_io[0x%x]\n", tls_io);
    return NULL;
  }

  opt_hndl = OptionHandler_Create(tlsio_qcom_threadx_clone_option, tlsio_qcom_threadx_destroy_option, tlsio_qcom_threadx_setoption);
  if (NULL == opt_hndl)
  {
    LogError("Unable to create option handler\n");
    goto TLS_RETRIEVE_OPT_ERROR;
  }
  
  /* Add the server root certificate authority option */
  if ((tls_io_instance->certificate) &&
      (OptionHandler_AddOption(opt_hndl, "TrustedCerts", tls_io_instance->certificate) != 0))
  {
    LogError("Unable to save 'TrustedCerts' option\n");
    goto TLS_RETRIEVE_OPT_ERROR;
  }

  /* Add the client certificates option */
  if ((tls_io_instance->x509_cert) &&
      (OptionHandler_AddOption(opt_hndl, "x509", tls_io_instance->x509_cert) != 0))
  {
    LogError("Unable to save 'TrustedCerts' option\n");
    goto TLS_RETRIEVE_OPT_ERROR;
  }

  /* Add the interface name */
  if (OptionHandler_AddOption(opt_hndl, "InterfaceName", tls_io_instance->iface_name) != 0)
  {
    LogError("Unable to retrieve 'InterfaceName' option\n");
    goto TLS_RETRIEVE_OPT_ERROR;
  }       

  /* Add the interface address */
  if (OptionHandler_AddOption(opt_hndl, "InterfaceAddress", &tls_io_instance->client_ip) != 0)
  {
    LogError("Unable to retrieve 'InterfaceName' option\n");
    goto TLS_RETRIEVE_OPT_ERROR;
  }     
       
  result = QAPI_OK;  
  LogInfo("Successfully retrieved TLS/SSL option['TrustedCerts'] for handle[0x%x]\n", tls_io_instance);

TLS_RETRIEVE_OPT_ERROR:
  
  /* Release any allocated resources */
  if (QAPI_OK != result)
  {
    OptionHandler_Destroy(opt_hndl);
    opt_hndl = NULL; 
  } 

  return opt_hndl;
}


static const IO_INTERFACE_DESCRIPTION tlsio_qcom_threadx_interface_description =
{
  tlsio_qcom_threadx_retrieve_options,
  tlsio_qcom_threadx_create,
  tlsio_qcom_threadx_destroy,
  tlsio_qcom_threadx_open,
  tlsio_qcom_threadx_close,
  tlsio_qcom_threadx_send,
  tlsio_qcom_threadx_dowork,
  tlsio_qcom_threadx_setoption
};


/* Simply returns the concrete implementations for the TLS adapter */
const IO_INTERFACE_DESCRIPTION* tlsio_qcom_threadx_get_interface_description(void)
{

  return &tlsio_qcom_threadx_interface_description;
}
