/*
 * myMB.h
 *
 *  Created on: Sep 6, 2015
 *      Author: wsf
 */

#ifndef SRC_SEVER_SERIALGPRS_MYMB_H_
#define SRC_SEVER_SERIALGPRS_MYMB_H_

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

#endif /* SRC_SEVER_SERIALGPRS_MYMB_H_ */
