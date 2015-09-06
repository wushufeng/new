/*
 * myMB.c
 *
 *  Created on: Sep 6, 2015
 *      Author: wsf
 */


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
//#include <termios.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>
#include <fcntl.h>

#include "myMB.h"

#include "../../database/database.h"
#include "../../port/portserial.h"

int mbReply(comm_t *ctx, const  unsigned char *req,
        int req_length, mb_mapping_t *mb_mapping)
{



	return 0;
}
