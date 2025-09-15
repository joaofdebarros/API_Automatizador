/*
 * Connect.c
 *
 *  Created on: 11 de set. de 2025
 *      Author: joao.victor
 */

#include "Connect.h"
/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/
ConnectLR_t ConnectLR;
extern UARTDRV_Handle_t sl_uartdrv_usart_central_handle;

#ifndef LED_INSTANCE_0
#define LED_INSTANCE_0      sl_led_led0
#endif

#ifndef LED_INSTANCE_1
#define LED_INSTANCE_1      sl_led_led1
#endif

/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/
uint8_t data_received[5];

void rx_done_callback(UARTDRV_Handle_t handle,
                       Ecode_t          status,
                       uint8_t         *data,
                       UARTDRV_Count_t  len);
void packet_receive_PGM(uint8_t *byte_receive);
/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/



/***************************************************************************//**
 * Initialize example.
 ******************************************************************************/
static volatile uint32_t irq_flag = 0;

void connect_init(void)
{
  // Enable the clock for the timer.
  sl_clock_manager_enable_bus_clock(SL_BUS_CLOCK_TIMER0);

  // Initialize a timer and enable it.
  sl_hal_timer_init_t init = SL_HAL_TIMER_INIT_DEFAULT;
  sl_hal_timer_init(TIMER0, &init);
  sl_hal_timer_enable(TIMER0);

//  sl_hal_timer_set_top(TIMER0, 3*30000);

  sl_hal_timer_clear_interrupts(TIMER0, _TIMER_IF_MASK);
  sl_hal_timer_enable_interrupts(TIMER0, _TIMER_IEN_MASK);
  sl_interrupt_manager_enable_irq(TIMER0_IRQn);

  sl_hal_timer_start(TIMER0);

  UARTDRV_Receive(sl_uartdrv_usart_central_handle,
                    &ConnectLR.byte_receive, 1, rx_done_callback);
  USTIMER_Init();
  return;
}

void TIMER0_IRQHandler(void)
{
  // Get all pending and enabled interrupts.
  uint32_t pending = sl_hal_timer_get_enabled_pending_interrupts(TIMER0);
  static uint32_t count = 0;
  // Handle different IRQ flags here, execute callbacks, etc.
  if (pending) {
      count++;
    sl_hal_timer_clear_interrupts(TIMER0, pending);
    irq_flag = pending;
  }

  if(count == 600){
      gate_get_status();
      count = 0;
  }
}

void gate_cmd(uint8_t cmd){
  uint8_t buffer_size = 10;
  uint8_t Buffer_TX[40];
  uint8_t data[4] = {cmd,0,0,0};
  montar_pacote(Buffer_TX, buffer_size, 0x02, 0x00,'C', data, 4, true);

  gate_packet_transmit(Buffer_TX, buffer_size);
  USTIMER_Delay(10000);
}

void gate_get_status(){
  uint8_t buffer_size = 10;
  uint8_t Buffer_TX[40];
  buffer_size = 6;
  montar_pacote(Buffer_TX, buffer_size, 0x02, 0x00,'H', 0X00, 0, true);

  gate_packet_transmit(Buffer_TX, buffer_size);
}

void gate_status(uint8_t *status){
  memcpy(status, &ConnectLR.gate_info.state, sizeof(ConnectLR.gate_info.state));
}

uint8_t calculate_checksum(uint8_t *buffer, uint8_t payload_size,
                           bool automatizador) {
  uint8_t sum = 0;
  uint8_t header_size = 0;

  if (automatizador) {
    header_size = 4;
  } else {
    header_size = 5;
  }

  for (uint8_t i = 0; i < (header_size + payload_size); i++) {
    sum ^= buffer[i];
  }

  return ~sum;
}

uint8_t montar_pacote(uint8_t *tx, uint8_t size, uint8_t id, uint8_t adrs,
                      uint8_t fnct, uint8_t *data, uint8_t payload_size,
                      bool automatizador) {
  uint8_t total_size = 0;

  if (automatizador) {
    total_size = payload_size + 6;

    tx[0] = start_byte;
    tx[1] = size;
    tx[2] = id;
    tx[3] = fnct;

    for (int i = 0; i < payload_size; i++) {
      tx[4 + i] = data[i];
    }

    tx[4 + payload_size] = calculate_checksum(tx, payload_size, automatizador);
    tx[5 + payload_size] = stop_byte;
  } else {
    total_size = payload_size + 7;

    tx[0] = start_byte;
    tx[1] = size;
    tx[2] = id;
    tx[3] = adrs;
    tx[4] = fnct;

    for (int i = 0; i < payload_size; i++) {
      tx[5 + i] = data[i];
    }

    tx[5 + payload_size] = calculate_checksum(tx, payload_size, automatizador);
    tx[6 + payload_size] = stop_byte;
  }
}

packet_error_e gate_packet_demount(uint8_t *datain, uint16_t len,
                                           gate_packet_t *packet) {
  uint8_t checksum_validate;
  uint16_t i, size, lHold;

  if (datain == NULL || packet == NULL) {
    return PACKET_FAIL_UNKNOWN;
  }
  lHold = (len - 6);
  checksum_validate = 0x7E;
  // Transport all bytes to the struct
  size = 0;
  memset(packet, 0, sizeof(gate_packet_t));

  for (i = 0; i < PACKET_LENGTH_LEN; i++) {
    packet->len = (datain[size++]);
  }

  checksum_validate ^= packet->len;

  for (i = 0; i < PACKET_FUNCTION_LEN; i++) {
    packet->id = datain[size++];
  }
  checksum_validate ^= packet->id;

  for (i = 0; i < PACKET_FUNCTION_LEN; i++) {
    packet->function = datain[size++];
  }
  checksum_validate ^= packet->function;

  if (lHold > sizeof(packet->data)) {
    return PACKET_FAIL_UNKNOWN;  // pacote inválido
  }

  memcpy(packet->data, &datain[size], lHold);
  size += lHold;

  for (i = 0; i < lHold; i++) {
    checksum_validate ^= packet->data[i];
  }

  for (i = 0; i < PACKET_CHECKSUM_LEN; i++) {
    packet->checksum = (datain[size++]);
  }
  checksum_validate = ~checksum_validate;

  for (i = 0; i < PACKET_TAIL_LEN; i++) {
    packet->tail = (datain[size++]);
  }

  if (checksum_validate != packet->checksum) {
    return PACKET_FAIL_CHECKSUM;
  }

  return PACKET_OK;
}

void packet_receive(uint8_t *byte_receive){
  uint8_t lenght;
  uint8_t id;
  uint8_t function;

  switch (ConnectLR.state) {
  case START_ST:
    if (*byte_receive == 0x7E) {

        ConnectLR.state = SIZE_ST;
    }
    UARTDRV_Receive(sl_uartdrv_usart_central_handle,
                        &ConnectLR.byte_receive, 1, rx_done_callback);

    break;
  case SIZE_ST:
    lenght = *byte_receive;
    ConnectLR.data[0] = lenght;
    ConnectLR.state = IDT_ST;
    UARTDRV_Receive(sl_uartdrv_usart_central_handle,
                        &ConnectLR.byte_receive, 1, rx_done_callback);

    break;

  case IDT_ST:
    id = *byte_receive;

    ConnectLR.data[1] = id;
    ConnectLR.state = FUNCTION_ST;

    UARTDRV_Receive(sl_uartdrv_usart_central_handle,
                          &ConnectLR.byte_receive, 1 , rx_done_callback);

    break;

  case FUNCTION_ST:
      function = *byte_receive;
      ConnectLR.data[2] = function;
      ConnectLR.state = DATA_ST;
      UARTDRV_Receive(sl_uartdrv_usart_central_handle,
                          &ConnectLR.data[3], (ConnectLR.data[0] - 4), rx_done_callback);

      break;

  case DATA_ST:
    ConnectLR.state = START_ST;
    ConnectLR.packetError =
        gate_packet_demount(ConnectLR.data, ConnectLR.data[0], &ConnectLR.gate_packet);

    if (ConnectLR.packetError == PACKET_OK) {
        if(ConnectLR.gate_packet.function == 'H'){
            ConnectLR.gate_info.state = ConnectLR.gate_packet.data[5];
            switch (ConnectLR.gate_info.state) {
              case ABERTO:
                sl_led_turn_on(&LED_INSTANCE_1);
                break;
              case ABRINDO:
                sl_led_toggle(&LED_INSTANCE_0);
                break;
              case FECHADO:
                sl_led_turn_off(&LED_INSTANCE_1);
                break;
              case FECHANDO:
                sl_led_toggle(&LED_INSTANCE_0);
                break;
              case SEMIABERTO:
                sl_led_turn_on(&LED_INSTANCE_1);
                break;

              default:
                break;
            }

        }else if(ConnectLR.gate_packet.function == 'C'){
            ConnectLR.gate_info.action = ConnectLR.gate_packet.data[0];
        }
    }
    UARTDRV_Receive(sl_uartdrv_usart_central_handle,
                        &ConnectLR.byte_receive, 1, rx_done_callback);
    break;

  default:
  }
}

void gate_packet_transmit(uint8_t *byte_transmit,uint8_t len){
  UARTDRV_Transmit(sl_uartdrv_usart_central_handle, byte_transmit, len, NULL);
}

void rx_done_callback(UARTDRV_Handle_t handle,
                             Ecode_t   status,
                             uint8_t *data,
                             UARTDRV_Count_t  len)
{
  packet_receive(&ConnectLR.byte_receive);
}







