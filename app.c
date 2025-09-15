/***************************************************************************//**
 * @file
 * @brief Top level application functions
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/


/***************************************************************************//**
 * Initialize application.
 ******************************************************************************/
#include "app.h"

extern bool bt0_press;
extern bool bt1_press;

void app_init(void)
{
  simple_button_init();
  connect_init();
}

/***************************************************************************//**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void)
{
  simple_button_process_action();

  if(bt0_press){
      bt0_press = false;
      gate_cmd(ACIONARMOTOR);

  }

  if(bt1_press){
      bt1_press = false;
      gate_cmd(FECHAR);
  }
}
