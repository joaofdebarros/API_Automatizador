/*
 * Connect.h
 *
 *  Created on: 11 de set. de 2025
 *      Author: joao.victor
 */

#ifndef API_CONNECT_CONNECT_H_
#define API_CONNECT_CONNECT_H_

#include "API/Button/simple_button_baremetal.h"
#include "uartdrv.h"
#include "sl_uartdrv_instances.h"
#include <string.h>
#include "sl_hal_timer.h"
#include "ustimer.h"

#define PACKET_HEADER_LEN 1
#define PACKET_LENGTH_LEN 1
#define PACKET_FUNCTION_LEN 1
#define PACKET_CHECKSUM_LEN 1
#define PACKET_TAIL_LEN 1

#define start_byte 0x7E
#define stop_byte 0x81


typedef enum{
  ABERTO = 1,
  ABRINDO,
  FECHADO,
  FECHANDO,
  SEMIABERTO,
  TRAVADO,
  LENDO_ABRE,
  LENDO_FECHA,
  INICIAL,
} gate_status_t;

typedef enum{
  ABRIR = 1,
  FECHAR,
  TRAVAR,
  PARAR,
  LG = 6,
  ACIONARMOTOR = 12,
} gate_cmd_t;

typedef struct{
  uint8_t state;
  uint8_t action;
} gate_info_t;

typedef struct {
  uint8_t len;
  uint8_t id;
  uint8_t function;
  uint8_t data[40];
  uint8_t checksum;
  uint8_t tail;
} gate_packet_t;

typedef enum {
  START_ST = 0,
  SIZE_ST,
  IDT_ST,
  FUNCTION_ST,
  DATA_ST,
} state_receive_t;

typedef enum {
  PACKET_OK,
  PACKET_FAIL_HEADER,
  PACKET_FAIL_TAIL,
  PACKET_FAIL_LENGHT,
  PACKET_FAIL_CHECKSUM,
  PACKET_FAIL_UNKNOWN = 0xFF
} packet_error_e;

typedef struct {
  packet_error_e packetError;
  gate_packet_t gate_packet;
  gate_info_t gate_info;
  uint8_t byte_receive;
  uint8_t data[40];
  state_receive_t state;
} ConnectLR_t;

void connect_init(void);

packet_error_e gate_packet_demount(uint8_t *datain, uint16_t len,
                                           gate_packet_t *packet);
uint8_t montar_pacote(uint8_t *tx, uint8_t size, uint8_t id, uint8_t adrs,
                      uint8_t fnct, uint8_t *data, uint8_t payload_size,
                      bool automatizador);
uint8_t calculate_checksum(uint8_t *buffer, uint8_t payload_size,
                           bool automatizador);
void gate_status(uint8_t *status);
void gate_packet_transmit(uint8_t *byte_transmit,uint8_t len);
void gate_cmd(uint8_t cmd);
void gate_get_status();
#endif /* API_CONNECT_CONNECT_H_ */
