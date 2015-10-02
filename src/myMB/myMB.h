/*
 * myMB.h
 *
 *  Created on: Sep 6, 2015
 *      Author: wsf
 */

#ifndef SRC_SEVER_SERIALGPRS_MYMB_H_
#define SRC_SEVER_SERIALGPRS_MYMB_H_

#include "../port/portserial.h"
#include <stdint.h>

/* Function codes */
#define _FC_READ_COILS                0x01
#define _FC_READ_DISCRETE_INPUTS      0x02
#define _FC_READ_HOLDING_REGISTERS    0x03
#define _FC_READ_INPUT_REGISTERS      0x04
#define _FC_WRITE_SINGLE_COIL         0x05
#define _FC_WRITE_SINGLE_REGISTER     0x06
#define _FC_READ_EXCEPTION_STATUS     0x07
#define _FC_DIAGNOSTICS				  0x08
#define _FC_WRITE_MULTIPLE_COILS      0x0F
#define _FC_WRITE_MULTIPLE_REGISTERS  0x10
#define _FC_REPORT_SLAVE_ID           0x11
#define _FC_WRITE_AND_READ_REGISTERS  0x17


typedef struct {
    int nb_bits;
    int nb_input_bits;
    int nb_input_registers;
    int nb_registers;
    uint8_t *tab_bits;
    uint8_t *tab_input_bits;
    uint16_t *tab_input_registers;
    uint16_t *tab_registers;
} mb_mapping_t;

typedef enum
{
    MB_ERROR_RECOVERY_NONE          = 0,
    MB_ERROR_RECOVERY_LINK          = (1<<1),
    MB_ERROR_RECOVERY_PROTOCOL      = (1<<2),
} mb_error_recovery_mode;

void mb_set_debug(comm_t *ctx, int boolean);
int mb_set_slave(comm_t *ctx, int slave);
int mbReply(comm_t *ctx, const  unsigned char *rev, unsigned char *rsp, int req_length, mb_mapping_t *mb_mapping);
#endif /* SRC_SEVER_SERIALGPRS_MYMB_H_ */
