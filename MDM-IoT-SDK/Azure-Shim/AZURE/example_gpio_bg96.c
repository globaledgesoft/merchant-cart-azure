/******************************************************************************
*@file    example_gpio.c
*@brief   example for gpio operation
*
*  ---------------------------------------------------------------------------
*
*  Copyright (c) 2018 Quectel Technologies, Inc.
*  All Rights Reserved.
*  Confidential and Proprietary - Quectel Technologies, Inc.
*  ---------------------------------------------------------------------------
*******************************************************************************/
#if defined(__EXAMPLE_GPIO__)
#include "qapi_tlmm.h"
#include "qapi_timer.h"
#include "qapi_diag.h"
#include "quectel_utils.h"
#include "example_gpio.h"
#include "quectel_gpio.h"
#include "qapi_uart.h"
#include "qapi_diag.h"
#include "quectel_uart_apis.h"

/**************************************************************************
*                                 GLOBAL
***************************************************************************/
/*  !!! This Pin Enumeration Only Applicable BG96-OPEN Project !!!
 */
GPIO_MAP_TBL gpio_map_tbl[PIN_E_GPIO_MAX] = {
/* PIN NUM,     PIN NAME,    GPIO ID  GPIO FUNC */
	{  4, 		"GPIO01",  		23, 	 0},
	{  5, 		"GPIO02",  		20, 	 0},
	{  6, 		"GPIO03",  		21, 	 0},
	{  7, 		"GPIO04",  		22, 	 0},
	{ 18, 		"GPIO05",  		11, 	 0},
	{ 19, 		"GPIO06",  		10, 	 0},
	{ 22, 		"GPIO07",  		 9, 	 0},
	{ 23, 		"GPIO08",  	 	 8, 	 0},
	{ 26, 		"GPIO09",  		15, 	 0},
	{ 27, 		"GPIO10",  		12, 	 0},
	{ 28, 		"GPIO11",  		13, 	 0},
	{ 40, 		"GPIO19",  		19, 	 0},
	{ 41, 		"GPIO20",  		18, 	 0},
	{ 64, 		"GPIO21",  		07, 	 0},
};

/* gpio id table */
qapi_GPIO_ID_t gpio_id_tbl[PIN_E_GPIO_MAX];

/* gpio tlmm config table */
qapi_TLMM_Config_t tlmm_config[PIN_E_GPIO_MAX];
	
/* Modify this pin num which you want to test */
MODULE_PIN_ENUM  g_test_pin_num = PIN_E_GPIO_01;



/* uart1 rx tx buffer */
static char rx1_buff[1024];
static char tx1_buff[1024];

extern QT_UART_CONF_PARA uart1_conf; 

#if 0
/* uart config para*/
static QT_UART_CONF_PARA uart1_conf =
{
	NULL,
	QT_UART_PORT_02,
	tx1_buff,
	sizeof(tx1_buff),
	rx1_buff,
	sizeof(rx1_buff),
	115200
};
#endif

void read_gpio();
int inventory_status();
int i_tray_gpio_ids[5] = {0, 1, 2, 3, 5};
unsigned char i_tray_status[5] = {1};
unsigned char i_tray_prev_status[5] = {1};


/**************************************************************************
*                           FUNCTION DECLARATION
***************************************************************************/


/**************************************************************************
*                                 FUNCTION
***************************************************************************/
/*
@func
  gpio_config
@brief
  [in]  m_pin
  		MODULE_PIN_ENUM type; the GPIO pin which customer want used for operation;
  [in]  gpio_dir
  		qapi_GPIO_Direction_t type; GPIO pin direction.
  [in]  gpio_pull
  		qapi_GPIO_Pull_t type; GPIO pin pull type.
  [in]  gpio_drive
  		qapi_GPIO_Drive_t type; GPIO pin drive strength. 
*/
void gpio_config(MODULE_PIN_ENUM m_pin,
				 qapi_GPIO_Direction_t gpio_dir,
				 qapi_GPIO_Pull_t gpio_pull,
				 qapi_GPIO_Drive_t gpio_drive
				 )
{
	qapi_Status_t status = QAPI_OK;

	tlmm_config[m_pin].pin   = gpio_map_tbl[m_pin].gpio_id;
	tlmm_config[m_pin].func  = gpio_map_tbl[m_pin].gpio_func;
	tlmm_config[m_pin].dir   = gpio_dir;
	tlmm_config[m_pin].pull  = gpio_pull;
	tlmm_config[m_pin].drive = gpio_drive;

	// the default here
	status = qapi_TLMM_Get_Gpio_ID(&tlmm_config[m_pin], &gpio_id_tbl[m_pin]);
	qt_uart_dbg(uart1_conf.hdlr,"QT# gpio_id[%d] status = %d", gpio_map_tbl[m_pin].gpio_id, status);
	if (status == QAPI_OK)
	{
		status = qapi_TLMM_Config_Gpio(gpio_id_tbl[m_pin], &tlmm_config[m_pin]);
		qt_uart_dbg(uart1_conf.hdlr,"QT# gpio_id[%d] status = %d", gpio_map_tbl[m_pin].gpio_id, status);
		if (status != QAPI_OK)
		{
			qt_uart_dbg(uart1_conf.hdlr,"QT# gpio_config failed");
		}
	}
}



/*
@func
  quectel_task_demo_entry
@brief
  Entry function for task. 
*/
/*=========================================================================*/
int quectel_task_entry(void)
{
	qapi_Status_t status;
#if !GPIO_TEST_MODE
	qapi_GPIO_Value_t level = QAPI_GPIO_HIGH_VALUE_E;
#endif

	/* UART 1 init */
	uart_init(&uart1_conf);

	/* prompt, task running */
	qt_uart_dbg(uart1_conf.hdlr,"[GPIO] start task ~");

	qapi_Timer_Sleep(5, QAPI_TIMER_UNIT_SEC, true);
	qt_uart_dbg(uart1_conf.hdlr,"QT# quectel_task_entry ~~~");
	qt_uart_dbg(uart1_conf.hdlr,"QT# ### Test pin is %d, gpio_id is %d ###", g_test_pin_num, gpio_map_tbl[g_test_pin_num].gpio_id);

#if GPIO_TEST_MODE /* Test output type */

	while(1)
	{
		gpio_config(g_test_pin_num, QAPI_GPIO_OUTPUT_E, QAPI_GPIO_NO_PULL_E, QAPI_GPIO_2MA_E);	
		status = qapi_TLMM_Drive_Gpio(gpio_id_tbl[g_test_pin_num], gpio_map_tbl[g_test_pin_num].gpio_id, QAPI_GPIO_LOW_VALUE_E);
		qt_uart_dbg(uart1_conf.hdlr,"QT# Set %d QAPI_GPIO_LOW_VALUE_E status = %d", g_test_pin_num, status);
		qapi_Timer_Sleep(2, QAPI_TIMER_UNIT_SEC, true);

		status = qapi_TLMM_Drive_Gpio(gpio_id_tbl[g_test_pin_num], gpio_map_tbl[g_test_pin_num].gpio_id, QAPI_GPIO_HIGH_VALUE_E);
		qt_uart_dbg(uart1_conf.hdlr,"QT# Set %d QAPI_GPIO_HIGH_VALUE_E status = %d", g_test_pin_num, status);
		qapi_Timer_Sleep(2, QAPI_TIMER_UNIT_SEC, true);

		status = qapi_TLMM_Release_Gpio_ID(&tlmm_config[g_test_pin_num], gpio_id_tbl[g_test_pin_num]);
		qt_uart_dbg(uart1_conf.hdlr,"QT# release %d status = %d", g_test_pin_num, status);

		g_test_pin_num ++;
		if(g_test_pin_num >= PIN_E_GPIO_MAX)
		{
			g_test_pin_num = PIN_E_GPIO_01;
			qapi_Timer_Sleep(5, QAPI_TIMER_UNIT_SEC, true);
		}
	}

#else /* Test iutput type */

#if 0
	while (1)
	{
		gpio_config(g_test_pin_num, QAPI_GPIO_INPUT_E, QAPI_GPIO_PULL_UP_E, QAPI_GPIO_2MA_E);	
		qapi_Timer_Sleep(3, QAPI_TIMER_UNIT_SEC, true);
		status = qapi_TLMM_Read_Gpio(gpio_id_tbl[g_test_pin_num], gpio_map_tbl[g_test_pin_num].gpio_id, &level);
		qt_uart_dbg("QT# read %d status = %d, level=%d", g_test_pin_num, status, level);
		status = qapi_TLMM_Release_Gpio_ID(&tlmm_config[g_test_pin_num], gpio_id_tbl[g_test_pin_num]);
		qt_uart_dbg("QT# release status = %d", status);

		g_test_pin_num ++;
		if(g_test_pin_num >= PIN_E_GPIO_MAX)
		{
			g_test_pin_num = PIN_E_GPIO_01;
			qapi_Timer_Sleep(5, QAPI_TIMER_UNIT_SEC, true);
		}
	}
#endif

	while (1)
	{
             int sales = 0;
	     qt_uart_dbg(uart1_conf.hdlr,"Reading GPIOs~~");
             read_gpio();
             sales = inventory_status();
	     qt_uart_dbg(uart1_conf.hdlr,"Total sales %d\n\n", sales);
	     qapi_Timer_Sleep(3, QAPI_TIMER_UNIT_SEC, true);

	}
#endif

	return 0;
}


void read_gpio()
{
        int i = 0, level;
	qapi_Status_t status;
	for (i = 0; i < 5; i++)
	{
                level = 1;

		gpio_config(i_tray_gpio_ids[i], QAPI_GPIO_INPUT_E, QAPI_GPIO_PULL_UP_E, QAPI_GPIO_2MA_E);	
		//qapi_Timer_Sleep(3, QAPI_TIMER_UNIT_SEC, true);
		status = qapi_TLMM_Read_Gpio(gpio_id_tbl[i_tray_gpio_ids[i]], gpio_map_tbl[i_tray_gpio_ids[i]].gpio_id, &level);
                i_tray_status[i] = (unsigned char) level;
		qt_uart_dbg(uart1_conf.hdlr,"QT# read %d status = %d, level=%d", i_tray_gpio_ids[i], status, level);
		status = qapi_TLMM_Release_Gpio_ID(&tlmm_config[i_tray_gpio_ids[i]], gpio_id_tbl[i_tray_gpio_ids[i]]);
		qt_uart_dbg(uart1_conf.hdlr,"QT# release status = %d", status);

		//g_test_pin_num ++;
		//if(g_test_pin_num >= PIN_E_GPIO_MAX)
		//{
		//	g_test_pin_num = PIN_E_GPIO_01;
		//	qapi_Timer_Sleep(5, QAPI_TIMER_UNIT_SEC, true);
		//}

	}
}


int inventory_status()
{

        int i = 0, sales = 0;
        
	for (i = 0; i < 5; i++)
	{
		if(i_tray_prev_status[i] == 1 && i_tray_status[i] == 0)
			sales++;

		i_tray_prev_status[i] = i_tray_status[i];
	}

        return sales;	
}

#endif /*__EXAMPLE_GPIO__*/

