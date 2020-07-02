#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stdarg.h"

#include "qapi_diag.h"
#include "qapi_socket.h"
#include "qapi_dnsc.h"
#include "qapi_dss.h"
#include "qapi_timer.h"
#include "qapi_uart.h"
#include "qapi_fs.h"
#include "qapi_status.h"
#include "qapi_netservices.h"
#include "qapi_httpc.h"
#include "qapi_device_info.h"


#include "quectel_utils.h"
#include "quectel_uart_apis.h"
#include "quectel_http.h"

/*===========================================================================
						   Header file
===========================================================================*/
/*===========================================================================
                             DEFINITION
===========================================================================*/
#define QL_DEF_APN	        "CMNET"
#define DSS_ADDR_INFO_SIZE	5
#define DSS_ADDR_SIZE       16

#define GET_ADDR_INFO_MIN(a, b) ((a) > (b) ? (b) : (a))

/*===========================================================================
                           Global variable
===========================================================================*/
#define TRUE 1
#define FALSE 0
typedef  unsigned char      boolean; 

TX_EVENT_FLAGS_GROUP *http_signal_handle;


void* http_nw_signal_handle = NULL;	/* Related to nework indication */

qapi_DSS_Hndl_t http_dss_handle = NULL;	            /* Related to DSS netctrl */

static char apn[QUEC_APN_LEN];					/* APN */
static char apn_username[QUEC_APN_USER_LEN];	/* APN username */
static char apn_passwd[QUEC_APN_PASS_LEN];		/* APN password */

/* @Note: If netctrl library fail to initialize, set this value will be 1,
 * We should not release library when it is 1. 
 */
DSS_Lib_Status_e http_netctl_lib_status = DSS_LIB_STAT_INVALID_E;
uint8 http_datacall_status = DSS_EVT_INVALID_E;
static http_session_policy_t http_session_policy;          /* http session policy */
void azure_data_call_start(void);

extern QT_UART_CONF_PARA uart1_conf; 

#define HTTP_UART_DBG(...)	\
{\
	http_uart_debug_print(__VA_ARGS__);	\
}


void http_uart_debug_print(const char* fmt, ...) 
{
	va_list arg_list;
	char dbg_buffer[512] = {0};

	va_start(arg_list, fmt);
	vsnprintf((char *)(dbg_buffer), sizeof(dbg_buffer), (char *)fmt, arg_list);
	va_end(arg_list);
		
	qapi_UART_Transmit(uart1_conf.hdlr, dbg_buffer, strlen(dbg_buffer), NULL);
	qapi_UART_Transmit(uart1_conf.hdlr, "\r\n\n", strlen("\r\n\n"), NULL);
    //qapi_Timer_Sleep(10, QAPI_TIMER_UNIT_MSEC, true);   //Do not add delay unless necessary
}


/*
@func
	dss_net_event_cb
@brief
	Initializes the DSS netctrl library for the specified operating mode.
*/
static void http_net_event_cb
( 
	qapi_DSS_Hndl_t 		hndl,
	void 				   *user_data,
	qapi_DSS_Net_Evt_t 		evt,
	qapi_DSS_Evt_Payload_t *payload_ptr 
)
{
	
	HTTP_UART_DBG("Data test event callback, event: %d\n", evt);

	switch (evt)
	{
		case QAPI_DSS_EVT_NET_NEWADDR_E:
		case QAPI_DSS_EVT_NET_IS_CONN_E:
		{
			HTTP_UART_DBG("Data Call Connected.\n");
			http_show_sysinfo();
			/* Signal main task */
  			tx_event_flags_set(http_signal_handle, DSS_SIG_EVT_CONN_E, TX_OR);
			http_datacall_status = DSS_EVT_NET_IS_CONN_E;
			break;
		}
		case QAPI_DSS_EVT_NET_DELADDR_E:
		case QAPI_DSS_EVT_NET_NO_NET_E:
		{
			HTTP_UART_DBG("Data Call Disconnected.\n");
			
			if (DSS_EVT_NET_IS_CONN_E == http_datacall_status)
			{
				http_datacall_status = DSS_EVT_NET_NO_NET_E;
                tx_event_flags_set(http_signal_handle, DSS_SIG_EVT_EXIT_E, TX_OR);
			}
            else
            {
                tx_event_flags_set(http_signal_handle, DSS_SIG_EVT_NO_CONN_E, TX_OR);
            }
			break;
		}
        
		default:
		{
			HTTP_UART_DBG("Data Call status is invalid.\n");
			tx_event_flags_set(http_signal_handle, DSS_SIG_EVT_INV_E, TX_OR);
			http_datacall_status = DSS_EVT_INVALID_E;
			break;
		}
	}
}

void http_show_sysinfo(void)
{
	int i   = 0;
	int j 	= 0;
	unsigned int len = 0;
	uint8 buff[DSS_ADDR_SIZE] = {0}; 
	qapi_Status_t status;
	qapi_DSS_Addr_Info_t info_ptr[DSS_ADDR_INFO_SIZE];

	status = qapi_DSS_Get_IP_Addr_Count(http_dss_handle, &len);
	if (QAPI_ERROR == status)
	{
		HTTP_UART_DBG("Get IP address count error\n");
		return;
	}
		
	status = qapi_DSS_Get_IP_Addr(http_dss_handle, info_ptr, len);
	if (QAPI_ERROR == status)
	{
		HTTP_UART_DBG("Get IP address error\n");
		return;
	}
	
	j = GET_ADDR_INFO_MIN(len, DSS_ADDR_INFO_SIZE);
	
	for (i = 0; i < j; i++)
	{
		HTTP_UART_DBG("<--- static IP address information --->\n");
		http_inet_ntoa(info_ptr[i].iface_addr_s, buff, DSS_ADDR_SIZE);
		HTTP_UART_DBG("static IP: %s\n", buff);

		memset(buff, 0, sizeof(buff));
		http_inet_ntoa(info_ptr[i].gtwy_addr_s, buff, DSS_ADDR_SIZE);
		HTTP_UART_DBG("Gateway IP: %s\n", buff);

		memset(buff, 0, sizeof(buff));
		http_inet_ntoa(info_ptr[i].dnsp_addr_s, buff, DSS_ADDR_SIZE);
		HTTP_UART_DBG("Primary DNS IP: %s\n", buff);

		memset(buff, 0, sizeof(buff));
		http_inet_ntoa(info_ptr[i].dnss_addr_s, buff, DSS_ADDR_SIZE);
		HTTP_UART_DBG("Second DNS IP: %s\n", buff);
	}

	HTTP_UART_DBG("<--- End of system info --->\n");
}

/*
@func
	http_set_data_param
@brief
	Set the Parameter for Data Call, such as APN and network tech.
*/
static int http_set_data_param(void)
{
    qapi_DSS_Call_Param_Value_t param_info;
	
	/* Initial Data Call Parameter */
	memset(apn, 0, sizeof(apn));
	memset(apn_username, 0, sizeof(apn_username));
	memset(apn_passwd, 0, sizeof(apn_passwd));
	strlcpy(apn, QL_DEF_APN, QAPI_DSS_CALL_INFO_APN_MAX_LEN);

    if (NULL != http_dss_handle)
    {
        /* set data call param */
        param_info.buf_val = NULL;
        param_info.num_val = QAPI_DSS_RADIO_TECH_UNKNOWN;	//Automatic mode(or DSS_RADIO_TECH_LTE)
        HTTP_UART_DBG("Setting tech to Automatic\n");
        qapi_DSS_Set_Data_Call_Param(http_dss_handle, QAPI_DSS_CALL_INFO_TECH_PREF_E, &param_info);

		/* set apn */
        param_info.buf_val = apn;
        param_info.num_val = strlen(apn);
        HTTP_UART_DBG("Setting APN - %s\n", apn);
        qapi_DSS_Set_Data_Call_Param(http_dss_handle, QAPI_DSS_CALL_INFO_APN_NAME_E, &param_info);
#ifdef QUEC_CUSTOM_APN
		/* set apn username */
		param_info.buf_val = apn_username;
        param_info.num_val = strlen(apn_username);
        HTTP_UART_DBG("Setting APN USER - %s\n", apn_username);
        qapi_DSS_Set_Data_Call_Param(http_dss_handle, QAPI_DSS_CALL_INFO_USERNAME_E, &param_info);

		/* set apn password */
		param_info.buf_val = apn_passwd;
        param_info.num_val = strlen(apn_passwd);
        HTTP_UART_DBG("Setting APN PASSWORD - %s\n", apn_passwd);
        qapi_DSS_Set_Data_Call_Param(http_dss_handle, QAPI_DSS_CALL_INFO_PASSWORD_E, &param_info);

        /* set auth type */
		param_info.buf_val = NULL;
        param_info.num_val = QAPI_DSS_AUTH_PREF_PAP_CHAP_BOTH_ALLOWED_E; //PAP and CHAP auth
        //HTTP_UART_DBG("Setting AUTH - %d\n", param_info.num_val);
        qapi_DSS_Set_Data_Call_Param(handle, QAPI_DSS_CALL_INFO_AUTH_PREF_E, &param_info);
#endif
		/* set IP version(IPv4 or IPv6) */
        param_info.buf_val = NULL;
        param_info.num_val = QAPI_DSS_IP_VERSION_4;
        HTTP_UART_DBG("Setting family to IPv%d\n", param_info.num_val);
        qapi_DSS_Set_Data_Call_Param(http_dss_handle, QAPI_DSS_CALL_INFO_IP_VERSION_E, &param_info);
    }
    else
    {
        HTTP_UART_DBG("Dss handler is NULL!!!\n");
		return -1;
    }
	
    return 0;
}

/*
@func
	http_inet_ntoa
@brief
	utility interface to translate ip from "int" to x.x.x.x format.
*/
int32 http_inet_ntoa
(
	const qapi_DSS_Addr_t    inaddr, /* IPv4 address to be converted         */
	uint8                   *buf,    /* Buffer to hold the converted address */
	int32                    buflen  /* Length of buffer                     */
)
{
	uint8 *paddr  = (uint8 *)&inaddr.addr.v4;
	int32  rc = 0;

	if ((NULL == buf) || (0 >= buflen))
	{
		rc = -1;
	}
	else
	{
		if (-1 == snprintf((char*)buf, (unsigned int)buflen, "%d.%d.%d.%d",
							paddr[0],
							paddr[1],
							paddr[2],
							paddr[3]))
		{
			rc = -1;
		}
	}

	return rc;
} /* http_inet_ntoa() */

/*
@func
	http_netctrl_init
@brief
	Initializes the DSS netctrl library for the specified operating mode.
*/
static int http_netctrl_init(void)
{
	int ret_val = 0;
	qapi_Status_t status = QAPI_OK;

	HTTP_UART_DBG("Initializes the DSS netctrl library\n");

	/* Initializes the DSS netctrl library */
	if (QAPI_OK == qapi_DSS_Init(QAPI_DSS_MODE_GENERAL))
	{
		http_netctl_lib_status = DSS_LIB_STAT_SUCCESS_E;
		HTTP_UART_DBG("qapi_DSS_Init success\n");
	}
	else
	{
		/* @Note: netctrl library has been initialized */
		http_netctl_lib_status = DSS_LIB_STAT_FAIL_E;
		HTTP_UART_DBG("DSS netctrl library has been initialized.\n");
	}
	
	/* Registering callback http_dss_handleR */
	do
	{
		HTTP_UART_DBG("Registering Callback http_dss_handle\n");
		
		/* Obtain data service handle */
		status = qapi_DSS_Get_Data_Srvc_Hndl(http_net_event_cb, NULL, &http_dss_handle);
		HTTP_UART_DBG("http_dss_handle %d, status %d\n", http_dss_handle, status);
		
		if (NULL != http_dss_handle)
		{
			HTTP_UART_DBG("Registed http_dss_handler success\n");
			break;
		}

		/* Obtain data service handle failure, try again after 10ms */
		qapi_Timer_Sleep(10, QAPI_TIMER_UNIT_MSEC, true);
	} while(1);

	return ret_val;
}

/*
@func
	http_netctrl_start
@brief
	Start the DSS netctrl library, and startup data call.
*/
int http_netctrl_start(void)
{
	int rc = 0;
	qapi_Status_t status = QAPI_OK;
		
	rc = http_netctrl_init();
	if (0 == rc)
	{
		/* Get valid DSS handler and set the data call parameter */
		http_set_data_param();
	}
	else
	{
		HTTP_UART_DBG("quectel_dss_init fail.\n");
		return -1;
	}

	HTTP_UART_DBG("qapi_DSS_Start_Data_Call start!!!.\n");
	status = qapi_DSS_Start_Data_Call(http_dss_handle);
	if (QAPI_OK == status)
	{
		HTTP_UART_DBG("Start Data service success.\n");
		return 0;
	}
	else
	{
        HTTP_UART_DBG("Start Data service failed.\n");
		return -1;
	}
}

/*
@func
	http_netctrl_release
@brief
	Cleans up the DSS netctrl library and close data service.
*/
static void http_netctrl_stop(void)
{
	qapi_Status_t stat = QAPI_OK;
	
	if (http_dss_handle)
	{
		stat = qapi_DSS_Stop_Data_Call(http_dss_handle);
		if (QAPI_OK == stat)
		{
			HTTP_UART_DBG("Stop data call success\n");
		}
        stat = qapi_DSS_Rel_Data_Srvc_Hndl(http_dss_handle);
		if (QAPI_OK != stat)
		{
			HTTP_UART_DBG("Release data service handle failed:%d\n", stat);
		}
		http_dss_handle = NULL;
	}
	if (DSS_LIB_STAT_SUCCESS_E == http_netctl_lib_status)
	{
		qapi_DSS_Release(QAPI_DSS_MODE_GENERAL);
		http_netctl_lib_status = DSS_LIB_STAT_INVALID_E;
	}
	
}	

void azure_data_call_start(void)
{
	uint32 dss_event = 0;
	int ret = -1;

	/* Create event signal handle and clear signals */
	txm_module_object_allocate(&http_signal_handle, sizeof(TX_EVENT_FLAGS_GROUP));
	tx_event_flags_create(http_signal_handle, "dss_signal_event");
	tx_event_flags_set(http_signal_handle, 0x0, TX_AND);
	ret = http_netctrl_start();
	if (ret != 0)
	{
		return;
	}
    
	tx_event_flags_get(http_signal_handle, DSS_SIG_EVT_CONN_E|DSS_SIG_EVT_DIS_E|DSS_SIG_EVT_EXIT_E|DSS_SIG_EVT_NO_CONN_E, TX_OR_CLEAR, &dss_event, TX_WAIT_FOREVER);
	tx_event_flags_delete(http_signal_handle);
	if(dss_event&DSS_SIG_EVT_CONN_E)
	{
		HTTP_UART_DBG("dss_event:%x\n",dss_event);
		if(qapi_Net_DNSc_Is_Started() == 0)
		{
			qapi_Net_DNSc_Command(QAPI_NET_DNS_START_E);
			qapi_Net_DNSc_Add_Server("223.5.5.5", 0);
			qapi_Net_DNSc_Add_Server("114.114.114.114", 1);
		}
	}
	else if(dss_event&DSS_SIG_EVT_EXIT_E)
	{
		http_netctrl_stop();
		return;
	}
	else
	{
		return;
	}
}

