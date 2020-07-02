// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// CAVEAT: This sample is to demonstrate azure IoT client concepts only and is not a guide design principles or style
// Checking of return codes and error values shall be omitted for brevity.  Please practice sound engineering practices
// when writing production code.

#ifdef AZURE_9206
#include <stdbool.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "iothub.h"
#include "iothub_device_client_ll.h"
#include "iothub_client_options.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_macro_utils/macro_utils.h"

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

#ifdef AZURE_9206

#include "qapi_uart.h"
#include "quectel_utils.h"
#include "quectel_uart_apis.h"
#include "qapi_diag.h"
#include "qapi_timer.h"
#include "txm_module.h"                                                          
#include "qapi_status.h"                                                         
#include "qapi_gpioint.h"
#include "quectel_gpio.h"

extern void read_gpio(void);
extern int inventory_status(void);

#define QT_SUB1_THREAD_PRIORITY   	180
#define QT_SUB1_THREAD_STACK_SIZE 	(1024 * 32)
static TX_THREAD* qt_sub1_thread_handle; 
static unsigned char qt_sub1_thread_stack[QT_SUB1_THREAD_STACK_SIZE];

/**************************************************************************
*                                 DEFINE
***************************************************************************/


/**************************************************************************
*                                 GLOBAL
***************************************************************************/
/* uart rx tx buffer */
static char rx_buff[1024];
static char tx_buff[1024];

#ifdef AZURE_BG95
extern void azure_data_call_start(void);
#else
extern int quec_data_call_init(void);
#endif

void app_get_time(qapi_time_unit_type type, qapi_time_get_t *time_info)
{
	memset(time_info, 0, sizeof(qapi_time_get_t));
    qapi_time_get(type, time_info); 
    /*if(type == QAPI_TIME_MSECS)
        app_print(time_info);*/
}

/* uart config para*/
QT_UART_CONF_PARA uart1_conf =
{
	NULL,
	QT_QAPI_UART_PORT_01,
	tx_buff,
	sizeof(tx_buff),
	rx_buff,
	sizeof(rx_buff),
	115200
};

#endif

/* This sample uses the _LL APIs of iothub_client for example purposes.
Simply changing the using the convenience layer (functions not having _LL)
and removing calls to _DoWork will yield the same results. */

// The protocol you wish to use should be uncommented
//
#define SAMPLE_MQTT
//#define SAMPLE_MQTT_OVER_WEBSOCKETS
//#define SAMPLE_AMQP
//#define SAMPLE_AMQP_OVER_WEBSOCKETS
//#define SAMPLE_HTTP

#ifdef SAMPLE_MQTT
    #include "iothubtransportmqtt.h"
#endif // SAMPLE_MQTT
#ifdef SAMPLE_MQTT_OVER_WEBSOCKETS
    #include "iothubtransportmqtt_websockets.h"
#endif // SAMPLE_MQTT_OVER_WEBSOCKETS
#ifdef SAMPLE_AMQP
    #include "iothubtransportamqp.h"
#endif // SAMPLE_AMQP
#ifdef SAMPLE_AMQP_OVER_WEBSOCKETS
    #include "iothubtransportamqp_websockets.h"
#endif // SAMPLE_AMQP_OVER_WEBSOCKETS
#ifdef SAMPLE_HTTP
    #include "iothubtransporthttp.h"
#endif // SAMPLE_HTTP
#if 0
static GPIO_MAP_TBL gpio_map_tbl[PIN_E_GPIO_MAX] = {
/* PIN NUM,     PIN NAME,    GPIO ID  GPIO FUNC */
	{  4, 		"GPIO01",  		23, 	 0},
	{  5, 		"GPIO02",  		20, 	 0},
	{  6, 		"GPIO03",  		21, 	 0},
	{  7, 		"GPIO04",  		22, 	 0},
	{ 19, 		"GPIO06",  		10, 	 0},
	{ 22, 		"GPIO07",  		 9, 	 0},
	{ 23, 		"GPIO08",  	 	 8, 	 0},
	{ 26, 		"GPIO09",  		15, 	 0},
    { 18, 		"GPIO05",  		11, 	 0},
	{ 27, 		"GPIO10",  		12, 	 0},
	{ 28, 		"GPIO11",  		13, 	 0},
	{ 40, 		"GPIO19",  		19, 	 0},
	{ 41, 		"GPIO20",  		18, 	 0},
	{ 64, 		"GPIO21",  		07, 	 0},
};
static qapi_Instance_Handle_t InventoryGPIOUpdateIntHdlr;
static int inventory_update = 0;
static unsigned int num_sensors = 5;

/**************************************************************************
*                                 FUNCTION
***************************************************************************/
void InventoryUpdateCallback(qapi_GPIOINT_Callback_Data_t data)
{	
	inventory_update++;
}
#endif

void azure_9206_log(char *buf);

/* Paste in the your iothub connection string  */
static const char* connectionString = "your iothub connection string";

//"HostName=Azure-QC-PORT-HUB.azure-devices.net;DeviceId=MyNodeDevice;SharedAccessKey=D3vD0m9XXVe2Vhh+MDQeAlU70vDivhC5geTOcfT98Hc=";

#define MESSAGE_COUNT        5
static bool g_continueRunning = true;
static size_t g_message_count_send_confirmations = 0;

static void send_confirm_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    (void)userContextCallback;
    // When a message is sent this callback will get envoked
    g_message_count_send_confirmations++;
    LogInfo("Confirmation callback received for message %lu with result %s\r\n", (unsigned long)g_message_count_send_confirmations, MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
}

static void connection_status_callback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* user_context)
{
    (void)reason;
    (void)user_context;
    // This sample DOES NOT take into consideration network outages.
    if (result == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED)
    {
        LogInfo("The device client is connected to iothub\r\n");
    }
    else
    {
        LogInfo("The device client has been disconnected\r\n");
    }
}

#ifdef AZURE_9206

void azure_9206_log(char *buf)
{
    qt_uart_dbg(uart1_conf.hdlr, buf);
    qt_uart_dbg(uart1_conf.hdlr, "\n");
}

#endif

#ifdef AZURE_9206
int quectel_azure_task_entry(void);

int quectel_task_entry(void)
{
	int ret = -1;
	int gpio = 0;

    /* uart 1 init */
    uart_init(&uart1_conf);

    /* start uart 1 receive */
    uart_recv(&uart1_conf);

    /* prompt task running */
    LogInfo("[azure task_create] start task ~");

    while(1)
    {
	qapi_time_get_t time_info, ms_time_info;
	qapi_Timer_Sleep(1, QAPI_TIMER_UNIT_SEC, true);
	app_get_time(QAPI_TIME_JULIAN, &time_info);
	app_get_time(QAPI_TIME_MSECS, &ms_time_info);
	qt_uart_dbg(uart1_conf.hdlr, "current time %lld ms", ms_time_info.time_msecs);
	qt_uart_dbg(uart1_conf.hdlr, "current year %04d", time_info.time_julian.year);
	if(time_info.time_julian.year != 1980)
		break;
    }


    //LogInfo("[azure task_create] start task ~");
    //qt_uart_dbg(uart1_conf.hdlr,"[azure task_create] start task ~");

#ifdef AZURE_BG95
    azure_data_call_start();
#else
    quec_data_call_init();
#endif
    /* create a new task */
    if(TX_SUCCESS != txm_module_object_allocate((VOID *)&qt_sub1_thread_handle, sizeof(TX_THREAD))) 
    {
        LogInfo("[task_create] txm_module_object_allocate failed ~");
        return - 1;
    }

#if 1
    /* create a new task : sub1 */
    ret = tx_thread_create(qt_sub1_thread_handle,
        "Azure Main Thread",
        quectel_azure_task_entry,
	NULL,
	qt_sub1_thread_stack,
	QT_SUB1_THREAD_STACK_SIZE,
	QT_SUB1_THREAD_PRIORITY,
	QT_SUB1_THREAD_PRIORITY,
	TX_NO_TIME_SLICE,
	TX_AUTO_START
	);
	      
    if(ret != TX_SUCCESS)
    {
        //LogError("[task_create] : Thread creation failed");
        LogError("[task_create] : Thread creation failed");

    }
    else
    {
        LogInfo("[task_create] : Azure thread creation success");
    }
#endif

#if 0
    while(gpio < num_sensors) {
        if (qapi_GPIOINT_Register_Interrupt(&InventoryGPIOUpdateIntHdlr,
	    gpio_map_tbl[gpio].gpio_id,
	    InventoryUpdateCallback,
	    NULL,
	    QAPI_GPIOINT_TRIGGER_EDGE_FALLING_E,
	    QAPI_GPIOINT_PRIO_MEDIUM_E,
	    false)!= QAPI_OK)
        {
            qt_uart_dbg(uart1_conf.hdlr,"GPIO Interrupt Registeration failed\n");
            return -1;
        }

        gpio++;
    }
#endif

    while (1)
    {
        //handle any events in main task
        /* sleep 2 seconds */
        qapi_Timer_Sleep(30, QAPI_TIMER_UNIT_SEC, true);
        LogInfo("Inside the while loop");

    }

    return 0;
}
#endif

#if 1

int quectel_azure_task_entry(void)
{
    IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol;
    IOTHUB_DEVICE_CLIENT_LL_HANDLE device_ll_handle;
    IOTHUB_MESSAGE_HANDLE message_handle;
    size_t messages_sent = 0;
    char telemetry_msg[70] = {0};

    // Select the Protocol to use with the connection
#ifdef SAMPLE_MQTT
    protocol = MQTT_Protocol;
#endif // SAMPLE_MQTT
#ifdef SAMPLE_MQTT_OVER_WEBSOCKETS
    protocol = MQTT_WebSocket_Protocol;
#endif // SAMPLE_MQTT_OVER_WEBSOCKETS
#ifdef SAMPLE_AMQP
    protocol = AMQP_Protocol;
#endif // SAMPLE_AMQP
#ifdef SAMPLE_AMQP_OVER_WEBSOCKETS
    protocol = AMQP_Protocol_over_WebSocketsTls;
#endif // SAMPLE_AMQP_OVER_WEBSOCKETS
#ifdef SAMPLE_HTTP
    protocol = HTTP_Protocol;
#endif // SAMPLE_HTTP

    // Used to initialize IoTHub SDK subsystem
    (void)IoTHub_Init();


    LogInfo("Creating IoTHub Device handle\r\n");
    // Create the iothub handle here
    device_ll_handle = IoTHubDeviceClient_LL_CreateFromConnectionString(connectionString, protocol);
    if (device_ll_handle == NULL)
    {
        LogInfo("Failure createing Iothub device.  Hint: Check you connection string.\r\n");
    }
    else
    {
        int sales = 0;
        // Set any option that are neccessary.
        // For available options please see the iothub_sdk_options.md documentation

#ifndef SAMPLE_HTTP
        // Can not set this options in HTTP
        bool traceOn = true;
        IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_LOG_TRACE, &traceOn);
#endif

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
        // Setting the Trusted Certificate.  This is only necessary on system with without
        // built in certificate stores.

            IoTHubDeviceClient_LL_SetOption(device_ll_handle, OPTION_TRUSTED_CERT, certificates);
#endif // SET_TRUSTED_CERT_IN_SAMPLES

#if defined SAMPLE_MQTT || defined SAMPLE_MQTT_WS
        //Setting the auto URL Encoder (recommended for MQTT). Please use this option unless
        //you are URL Encoding inputs yourself.
        //ONLY valid for use with MQTT
        //bool urlEncodeOn = true;
        //IoTHubDeviceClient_LL_SetOption(iothub_ll_handle, OPTION_AUTO_URL_ENCODE_DECODE, &urlEncodeOn);
#endif

        // Setting connection status callback to get indication of connection to iothub
        (void)IoTHubDeviceClient_LL_SetConnectionStatusCallback(device_ll_handle, connection_status_callback, NULL);
        LogInfo("Ready to send messages to the hub\r\n");
        do
        {
            read_gpio();
            sales = inventory_status();
            if(sales) {
                //inventory_update = 0;
                telemetry_msg[0] = 'A' + sales;
                telemetry_msg[1] = '\0';
                // Construct the iothub message from a string or a byte array
                message_handle = IoTHubMessage_CreateFromString(telemetry_msg);
                //message_handle = IoTHubMessage_CreateFromByteArray((const unsigned char*)msgText, strlen(msgText)));

                // Set Message property
                /*(void)IoTHubMessage_SetMessageId(message_handle, "MSG_ID");
                (void)IoTHubMessage_SetCorrelationId(message_handle, "CORE_ID");
                (void)IoTHubMessage_SetContentTypeSystemProperty(message_handle, "application%2fjson");
                (void)IoTHubMessage_SetContentEncodingSystemProperty(message_handle, "utf-8");*/

                // Add custom properties to message
                (void)IoTHubMessage_SetProperty(message_handle, "sales", telemetry_msg);

                LogInfo("Sending message %d to IoTHub\r\n", (int)(messages_sent + 1));
                IoTHubDeviceClient_LL_SendEventAsync(device_ll_handle, message_handle, send_confirm_callback, NULL);

                // The message is copied to the sdk so the we can destroy it
                IoTHubMessage_Destroy(message_handle);

                messages_sent++;
            }

            IoTHubDeviceClient_LL_DoWork(device_ll_handle);
            ThreadAPI_Sleep(3);

        } while (g_continueRunning);
        // Clean up the iothub sdk handle
        IoTHubDeviceClient_LL_Destroy(device_ll_handle);
    }
    // Free all the sdk subsystem
    IoTHub_Deinit();

    LogInfo("Press any key to continue");
    //(void)getchar();

    return 0;
}

#endif
