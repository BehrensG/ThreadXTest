/*
 * SCPI_Task.c
 *
 *  Created on: Jun 2, 2024
 *      Author: grzegorz
 */

#include "scpi/scpi.h"
#include "SCPI_Def.h"




static size_t hislip_frame_len = 0;

size_t SCPI_Write(scpi_t * context, const char * data, size_t len) {

	hislip_frame_len += len;

    return 0;
}

scpi_result_t SCPI_Flush(scpi_t * context) {

    return SCPI_RES_OK;
}

int SCPI_Error(scpi_t * context, int_fast16_t err) {

    return 0;
}

scpi_result_t SCPI_Control(scpi_t * context, scpi_ctrl_name_t ctrl, scpi_reg_val_t val) {

    return SCPI_RES_OK;
}

scpi_result_t SCPI_Reset(scpi_t * context) {

    (void) context;

    return SCPI_RES_OK;
}


void SCPI_RequestControl(void) {

}

void SCPI_AddError(int16_t err) {
}



