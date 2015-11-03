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

#include "../log/rtulog.h"
#include "../database/database.h"
#include "../port/portserial.h"
#include "../A11_sysAttr/a11sysattr.h"
#include "../inifile/inifile.h"
#include "../sever/sysdatetime/getsysdatetime.h"

extern E1_sys_attribute *psysattr;

typedef struct _sft {
    int slave;
    int function;
    int t_id;
} sft_t;

/* Table of CRC values for high-order byte */
static const unsigned char table_crc_hi[] = {
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40
};

/* Table of CRC values for low-order byte */
static const unsigned char table_crc_lo[] = {
    0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06,
    0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD,
    0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
    0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A,
    0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4,
    0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
    0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3,
    0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,
    0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
    0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29,
    0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED,
    0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
    0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60,
    0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67,
    0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
    0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,
    0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E,
    0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
    0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71,
    0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92,
    0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
    0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B,
    0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B,
    0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
    0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42,
    0x43, 0x83, 0x41, 0x81, 0x80, 0x40
};
int mbWriteMultiReg(const uint8_t *req, int offset);
int mbWriteSigleRegister(uint16_t address, int data);
static unsigned short int crc16(unsigned char *buffer, unsigned short int buffer_length);
static int response_exception(comm_t *ctx, sft_t *sft, int exception_code, uint8_t *rsp);
static int _modbus_rtu_build_response_basis(sft_t *sft, uint8_t *rsp);


static unsigned short int crc16(unsigned char *buffer, unsigned short int buffer_length)
{
    uint8_t crc_hi = 0xFF; /* high CRC byte initialized */
    uint8_t crc_lo = 0xFF; /* low CRC byte initialized */
    unsigned int i; /* will index into CRC lookup */

    /* pass through message buffer */
    while (buffer_length--) {
        i = crc_hi ^ *buffer++; /* calculate the CRC  */
        crc_hi = crc_lo ^ table_crc_hi[i];
        crc_lo = table_crc_lo[i];
    }

    return (crc_hi << 8 | crc_lo);
}


int mbReply(comm_t *ctx, const  unsigned char *rev, unsigned char *snd, int req_length, mb_mapping_t *mb_mapping)
{
	int offset = 1;
    uint16_t crc_calculated;
    uint16_t crc_received;
    int slave = rev[0];
    int function = rev[1];
    uint16_t address = (rev[2] << 8) + rev[3];	// 计算出两字节起始地址
//    uint8_t rsp[256];				// 申请长度为260的用于返回数据的数组rsp
    int rsp_length = 0;													// 设定数组rsp游标rsp_length为0
    sft_t sft;

    sft.slave = slave;
    sft.function = function;
    sft.t_id = 0;

    crc_calculated = crc16((unsigned char *)rev, req_length - 2);
    crc_received = (rev[req_length - 2] << 8) | rev[req_length - 1];

    /* Check CRC of msg */
    if (crc_calculated == crc_received)
    {
    	switch (function)
    	{
    	case 0x03:
    	{
            int nb = (rev[offset + 3] << 8) + rev[offset + 4];

            if (nb < 1 || 125 < nb) {
                if (ctx->debug) {
                	zlog_warn(c,"非法的读保持寄存器数量(最大125),当前数量为:%d", nb);
                }
                rsp_length = response_exception(ctx, &sft, (int)0x03, snd);
            } else if ((address + nb) > mb_mapping->nb_registers) {
                if (ctx->debug) {
                	zlog_warn(c,"非法的读保持寄存器地址:%0X",  address + nb);
                }
                rsp_length = response_exception(ctx, &sft, (int)0x02, snd);
            } else {
                int i;

                rsp_length = _modbus_rtu_build_response_basis(&sft, snd);
                snd[rsp_length++] = nb << 1;			// 数量*2
                for (i = address; i < address + nb; i++) {
                    snd[rsp_length++] = mb_mapping->tab_registers[i] >> 8;
                    snd[rsp_length++] = mb_mapping->tab_registers[i] & 0xFF;
                }
            }
        }
            break;
		case 0x06:
	    {
	    	int data = (rev[offset + 3] << 8) + rev[offset + 4];
	    	switch(mbWriteSigleRegister(address, data))
	    	{
	    		case 0x02:	// 地址错误
	    			rsp_length = response_exception(ctx, &sft,0x02, snd);
	    			if (ctx->debug) {
	    				zlog_debug(c, "0x06功能码不支持该地址%0X的写入", address);
	    			}
	    			break;
	    		case 0x03:	//寄存器值错误
	    			if (ctx->debug) {
	    				zlog_debug(c, "0x06功能码写入地址%0X的值非法", address);
	    			}
	    			rsp_length = response_exception(ctx, &sft,0x03, snd);
	    			break;
	    		case 4:
	    			zlog_warn(c, "0x06功能码写入地址%0X的值%d存入配置文件失败",address, data);
	    			break;
	    		default:
	                mb_mapping->tab_registers[address] = data;
	    			break;
	      	}
			memcpy(snd, rev, req_length);
			rsp_length = req_length;
	        break;
	    }
		case 0x08:
		{
	    	if(address != 0x0000)
	    	{
	    		if(ctx->debug){
	    			zlog_debug(c, "非法的子功能码%0X", address);
	    		}
	    		 rsp_length = response_exception(ctx, &sft, 0x02, snd);
	        } else
	        {
	        	memcpy(snd, rev, req_length);
	            rsp_length = req_length;
	        }
			break;
		}
		case 0x10:
		{
			int nb = (rev[offset + 3] << 8) + rev[offset + 4];
	    	switch(mbWriteMultiReg(rev, offset))
	    	{
	    		case 0x02:	// 地址错误
	    			rsp_length = response_exception(ctx, &sft,(int)0x02, snd);
	    			if (ctx->debug) {
	    				zlog_debug(c, "0x10功能码不支持该地址%0X的写入", address);
	    			}
	    			break;
	    		case 0x03:	//寄存器值错误
	    			if (ctx->debug) {
	    				zlog_debug(c, "0x10功能码写入地址%0X的数量非法", address);
	    			}
	    			rsp_length = response_exception(ctx, &sft,0x03, snd);
	    			break;
	    		case 0x04:
	    			if (ctx->debug) {
	    				zlog_debug(c, "0x10功能码写入地址%0X失败", address);
	    			}
	    			rsp_length = response_exception(ctx, &sft,0x04, snd);
	    			break;
	    		default:
	    		{
	                int i, j;
	                for (i = address, j = 6; i < address + nb; i++, j += 2) {
	                    /* 6 and 7 = first value */
	                    mb_mapping->tab_registers[i] =
	                        (rev[offset + j] << 8) + rev[offset + j + 1];
	                }
	    			break;
	    		}
	      	}
	    	rsp_length = _modbus_rtu_build_response_basis(&sft, snd);
            /* 4 to copy the address (2) and the no. of registers */
            memcpy(snd + rsp_length, rev + rsp_length, 4);
            rsp_length += 4;
			break;
		}
        default:
            rsp_length = response_exception(ctx, &sft, 0x01, snd);
            break;
    	}
        uint16_t crc = crc16(snd, rsp_length);
        snd[rsp_length++] = crc >> 8;
        snd[rsp_length++] = crc & 0x00FF;
    }
    else
    {
        if (ctx->debug) {
        	zlog_warn(c,"接收到得CRC%0X不等于计算出得CRC%0X", crc_received, crc_calculated);

        }
//        if (ctx->error_recovery & MODBUS_ERROR_RECOVERY_PROTOCOL) {
//            _modbus_rtu_flush(ctx);
//        }
//        errno = EMBBADCRC;
        return -1;
    }
	if(slave == 0)	// 广播地址
		return 0;
	return rsp_length;
}

/* Build the exception response */
static int response_exception(comm_t *ctx, sft_t *sft,
                              int exception_code, uint8_t *rsp)
{
    int rsp_length;

    sft->function = sft->function + 0x80;
    rsp_length = _modbus_rtu_build_response_basis(sft, rsp);

    /* Positive exception code */
    rsp[rsp_length++] = exception_code;

    return rsp_length;
}
/* Builds a RTU response header */
static int _modbus_rtu_build_response_basis(sft_t *sft, uint8_t *rsp)
{
    /* In this case, the slave is certainly valid because a check is already
     * done in _modbus_rtu_listen */
    rsp[0] = sft->slave;
    rsp[1] = sft->function;

    return 2;
}
int mbWriteSigleRegister(uint16_t address, int data)
{
	char str[32];
	int res = 0;
//	static char data_flag_ip = 0;
	switch(address)
	{
//		case 0:		// 应用的井站类型
//			sprintf(str,"%03d",psysattr->baseinfo.well_station_type);
//			if (!write_profile_string("rtuBaseInfo","well_station_type",str,A11filename))
//				return -1;
//			break;
//		case 1:		// 设备厂家
//			sprintf(str,"%d",psysattr->baseinfo.dev_company);
//			if (!write_profile_string("rtuBaseInfo","dev_company",str,A11filename))
//				return -1;
//			break;
//		case 2:		// 型号版本
//			sprintf(str,"%d",psysattr->baseinfo.type_version);
//			if (!write_profile_string("rtuBaseInfo","type_version",str,A11filename))
//				return -1;
//			break;
		case 3:		// 口令
			sprintf(str,"%d",psysattr->baseinfo.password);
			if (!write_profile_string("rtuBaseInfo","password",str,A11filename))
				return 4;
			break;
		case 6:		// 系统时间
		case 7:
		case 8:

			break;
		case 9:		// 系统日期
		case 10:
		case 11:

			break;
		case 12:	// 日时间起点
		case 13:
		case 14:

			break;
		case 30:		// 通信方式
		{
			if((data >= 0)&&(data <= 5))
			{
				psysattr->commparam.communication_mode = data;
				sprintf(str,"%d",psysattr->commparam.communication_mode);
				if (!write_profile_string("commParam","communication_mode",str,A11filename))
					return 4;
			}
			else
				res = 3;
			break;
		}
		case 31:		// 通讯协议
		case 9019:
			if((data >= 0)&&(data <= 3))
			{
				psysattr->commparam.communication_protocols = data;
				sprintf(str,"%d",psysattr->commparam.communication_protocols);
				if (!write_profile_string("commParam","communication_protocols",str,A11filename))
					return 4;
				if (!write_profile_string("manufacturers_of_custom","communication_protocols",str,A11filename))
					return 4;
			}
			else
				res = 3;
			break;
		case 32:		// 终端通信地址
			psysattr->commparam.terminal_comm_address = data;
			sprintf(str,"%d",psysattr->commparam.terminal_comm_address);
			if (!write_profile_string("commParam","terminal_comm_address",str,A11filename))
				return -1;
			break;
		case 33:		// 波特率
			if((data >= 0)&&(data <= 7))
			{
				psysattr->commparam.comm_baudrate = data;
				sprintf(str,"%d",psysattr->commparam.comm_baudrate);
				if (!write_profile_string("commParam","comm_baudrate",str,A11filename))
					return 4;
			}
			else
				res = 3;
			break;
		case 34:		// 数据位
			if((data >= 0)&&(data <= 1))
			{
				psysattr->commparam.comm_databit = data;
				sprintf(str,"%d",psysattr->commparam.comm_databit);
				if (!write_profile_string("commParam","comm_databit",str,A11filename))
					return 4;
			}
			else
				res = 3;
			break;
		case 35:		// 停止位
			if((data >= 0)&&(data <= 1))
			{
				psysattr->commparam.comm_stopbit = data;
				sprintf(str,"%d",psysattr->commparam.comm_stopbit);
				if (!write_profile_string("commParam","comm_stopbit",str,A11filename))
					return 4;
			}
			else
				res = 3;
			break;
		case 36:		//奇偶校验
			if((data >= 0)&&(data <= 2))
			{
				psysattr->commparam.comm_paritybit = data;
				sprintf(str,"%d",psysattr->commparam.comm_paritybit);
				if (!write_profile_string("commParam","comm_paritybit",str,A11filename))
					return 4;
			}
			else
				res = 3;
			break;
		case 37:		// 半/全双工
		case 9664:
			if((data >= 0)&&(data <= 1))
			{
				psysattr->commparam.comm_duplex = data;
				sprintf(str,"%d",psysattr->commparam.comm_duplex);
				if (!write_profile_string("commParam","comm_duplex",str,A11filename))
					return 4;
				if (!write_profile_string("manufacturers_of_custom","comm_duplex",str,A11filename))
					return 4;
			}
			else
				res = 3;
			break;
		case 38:		// 本地IP地址
		case 39:
		case 40:
		case 41:
//			psysattr->commparam.ip_address[0] = data;
//			data_flag_ip |= 0x01;
//			if(data_flag_ip == 0x0F)
//			{
//				data_flag_ip = 0x00;
			sprintf(str,"%d.%d.%d.%d",psysattr->commparam.ip_address[0],
					psysattr->commparam.ip_address[1],
					psysattr->commparam.ip_address[2],
					psysattr->commparam.ip_address[3]);
			if (!write_profile_string("commParam","ip_address",str,A11filename))
				return 4;
//			}
			break;
		case 42:		// 子网掩码
		case 43:
		case 44:
		case 45:
//			psysattr->commparam.subnet_mask[0] = data;
			sprintf(str,"%d.%d.%d.%d",psysattr->commparam.subnet_mask[0],
						psysattr->commparam.subnet_mask[1],
						psysattr->commparam.subnet_mask[2],
						psysattr->commparam.subnet_mask[3]);
				if (!write_profile_string("commParam","subnet_mask",str,A11filename))
					return 4;
			break;
		case 46:		// 网关
		case 47:
		case 48:
		case 49:
			sprintf(str,"%d.%d.%d.%d",psysattr->commparam.gateway[0],
						psysattr->commparam.gateway[1],
						psysattr->commparam.gateway[2],
						psysattr->commparam.gateway[3]);
				if (!write_profile_string("commParam","gateway",str,A11filename))
					return 4;
			break;
		case 50:		// MAC地址
		case 51:
		case 52:
		case 53:
		case 54:
		case 55:
			sprintf(str,"%02X:%02X:%02X:%02X:%02X:%02X",psysattr->commparam.mac_address[0],
					psysattr->commparam.mac_address[1],
					psysattr->commparam.mac_address[2],
					psysattr->commparam.mac_address[3],
					psysattr->commparam.mac_address[4],
					psysattr->commparam.mac_address[5]);
			if (!write_profile_string("commParam","mac_address",str,A11filename))
				return -1;
			break;
		case 56:		// TCP/UDP标识
		case 9677:
			if((data >= 0)&&(data <= 1))
			{
				sprintf(str,"%d",psysattr->commparam.tcp_udp_identity);
				if (!write_profile_string("commParam","tcp_udp_identity",str,A11filename))
					return 4;
				if (!write_profile_string("manufacturers_of_custom","IPv4_tcp_udp_identity",str,A11filename))
					return 4;
			}
			else
				res = 3;
			break;
		case 57:		// 本地UDP端口号
		case 9679:
			sprintf(str,"%d",psysattr->commparam.upd_port);
			if (!write_profile_string("commParam","upd_port",str,A11filename))
				return 4;
			if (!write_profile_string("manufacturers_of_custom","IPv4_upd_port",str,A11filename))
				return 4;
			break;
		case 58:		// 本地TCP端口号
		case 9680:
			sprintf(str,"%d",psysattr->commparam.tcp_port);
			if (!write_profile_string("commParam","tcp_port",str,A11filename))
				return 4;
			if (!write_profile_string("manufacturers_of_custom","IPv4_tcp_port",str,A11filename))
				return 4;
			break;
		case 59:		// 主站IP地址
		case 60:
		case 61:
		case 62:
			sprintf(str,"%d.%d.%d.%d",psysattr->commparam.master_ip_address[0],
						psysattr->commparam.master_ip_address[1],
						psysattr->commparam.master_ip_address[2],
						psysattr->commparam.master_ip_address[3]);
			if (!write_profile_string("commParam","master_ip_address",str,A11filename))
				return 4;
			break;
		case 63:		// 主站端口号
		case 9685:
			psysattr->commparam.master_port = data;
			sprintf(str,"%d",psysattr->commparam.master_port);
			if (!write_profile_string("commParam","master_port",str,A11filename))
				return 4;
			if (!write_profile_string("manufacturers_of_custom","IPv4_master_port",str,A11filename))
				return 4;
			break;
		case 64:		// 主站通讯方式
			if(((data >= 0) && (data <= 5)) || (data == 0xFF))	// 0xFF无效
			{
				psysattr->commparam.master_comm_mode = data;
				sprintf(str,"%d",psysattr->commparam.master_comm_mode);
				if (!write_profile_string("commParam","master_comm_mode",str,A11filename))
					return 4;
			}
			else
				res = 3;
			break;
		case 65:		// 下行通信接口
			if(((data >= 0) && (data <= 7)) || (data == 0xFF))	// 0xFF无效
			{
				psysattr->commparam.downlink_comm_interface = data;
				sprintf(str,"%d",psysattr->commparam.downlink_comm_interface);
				if (!write_profile_string("commParam","downlink_comm_interface",str,A11filename))
					return 4;
			}
			else
				res = 3;
			break;
		case 66:		// 下行通信接收超时时间
			psysattr->commparam.downlink_recv_timeout = data;
			sprintf(str,"%d",psysattr->commparam.downlink_recv_timeout);
			if (!write_profile_string("commParam","downlink_recv_timeout",str,A11filename))
				return 4;
			break;
		case 67:		// 下行通讯失败重发次数
			psysattr->commparam.downlink_resend_num = data;
			sprintf(str,"%d",psysattr->commparam.downlink_resend_num);
			if (!write_profile_string("commParam","downlink_resend_num",str,A11filename))
				return 4;
			break;
		case 9025:		// 功图测试类型
			sprintf(str,"%d",data);
			if (!write_profile_string("manufacturers_of_custom","dynagraph_mode",str,A11filename))
				return 4;
			break;
		case 9026:		// 功图测试点数
			sprintf(str,"%d",data);
			if (!write_profile_string("manufacturers_of_custom","dynagraph_dot",str,A11filename))
				return 4;
			break;
		case 9027:		// 电量图测试状态
			sprintf(str,"%d",data);
			if (!write_profile_string("manufacturers_of_custom","elec_sta",str,A11filename))
				return 4;
			break;
		case 9028:		// 断网存储状态
			sprintf(str,"%d",data);
			if (!write_profile_string("manufacturers_of_custom","offline_savedata_switch",str,A11filename))
				return 4;
			break;
		case 9029:		// A1 报警
			sprintf(str,"%d",data);
			if (!write_profile_string("manufacturers_of_custom","A1alram_switch",str,A11filename))
				return 4;
			break;
		case 9030:		// A1 报警上传周期
			sprintf(str,"%d",data);
			if (!write_profile_string("manufacturers_of_custom","A1alram_upload_cycle",str,A11filename))
				return 4;
			break;
		case 9031:		// 更新单元单包发送字节数
			if(data % 32)
			{
				sprintf(str,"%d",data);
				if (!write_profile_string("manufacturers_of_custom","update_packet_size",str,A11filename))
					return 4;
			}
			else
				res = 3;
			break;
		case 9034:		// 功图巡检时间
			sprintf(str,"%d",data);
			if (!write_profile_string("manufacturers_of_custom","dynagraph_patroltime",str,A11filename))
				return 4;
			break;
		case 9035:		// 压力巡检时间
			sprintf(str,"%d",data);
			if (!write_profile_string("manufacturers_of_custom","press_patroltime",str,A11filename))
				return 4;
			break;
		case 9036:		// 温度巡检时间
			sprintf(str,"%d",data);
			if (!write_profile_string("manufacturers_of_custom","tempreture_patroltime",str,A11filename))
				return 4;
			break;
		case 9037:		// 电参巡检时间
			sprintf(str,"%d",data);
			if (!write_profile_string("manufacturers_of_custom","elec_patroltime",str,A11filename))
				return 4;
			break;
		case 9038:		// 角位移巡检时间
			sprintf(str,"%d",data);
			if (!write_profile_string("manufacturers_of_custom","angledisplacement_patroltime",str,A11filename))
				return 4;
			break;
		case 9039:		// 载荷巡检时间
			sprintf(str,"%d",data);
			if (!write_profile_string("manufacturers_of_custom","load_patroltime",str,A11filename))
				return 4;
			break;
		case 9040:		// 扭矩巡检时间
			sprintf(str,"%d",data);
			if (!write_profile_string("manufacturers_of_custom","touque_patroltime",str,A11filename))
				return 4;
			break;
		case 9041:		// 液面巡检时间
			sprintf(str,"%d",data);
			if (!write_profile_string("manufacturers_of_custom","liquid_patroltime",str,A11filename))
				return 4;
			break;
		case 9042:		// 扭矩转速巡检时间
			sprintf(str,"%d",data);
			if (!write_profile_string("manufacturers_of_custom","rpm_patroltime",str,A11filename))
				return 4;
			break;
		case 9043:		// 模拟量巡检时间
			sprintf(str,"%d",data);
			if (!write_profile_string("manufacturers_of_custom","analog_patroltime",str,A11filename))
				return 4;
			break;
		case 9044:		// 开关量巡检时间
			sprintf(str,"%d",data);
			if (!write_profile_string("manufacturers_of_custom","switch_patroltime",str,A11filename))
				return 4;
			break;
		default:
			if(address > 10000)
				res = 2;
			break;
	}

	return res;
}
/* @brief
 * modbus 0x10功能码
 * 写多个寄存器
 */
int mbWriteMultiReg(const uint8_t *req, int offset)
{
	char str[64];
	int res = 0;
	int nb = (req[offset + 3] << 8) + req[offset + 4];
	uint16_t address = (req[offset + 1] << 8) + req[offset + 2];	// 计算出两字节起始地址

//	if(nb < 1 || nb > 123)
//	{
//		return 3;
//	}
	switch(address)
	{
		case 4:				// 系统时间
			if(nb != 6)
			{
				return 3;
			}
//			if((address + nb) != (6 + 4))
//				return 2;
			else
			{
//				unsigned short int ttt[] = {0x0019, 0x0059, 0x0009, 0x2005, 0x0008, 0x0015};
//				if(setSystemTime(ttt) == 0)
//	            int i, j;
//	            unsigned short int tempdt[6];
//	            for (i = 0, j = 5; i < nb; i++, j += 2) {
//	                /* 6 and 7 = first value */
//	            	tempdt[i] =
//	                    (req[offset + j] << 8) + req[offset + j + 1];
//	            }
//				if(setSystemTime(tempdt) == 0)
				if(setSystemTime((void *)&req[offset + 6]) == 0)
				{
					zlog_info(c, "通过远端设置时间成功");
				}
				else
				{
					zlog_warn(c, "通过远端设置时间失败settimeofday-%d", errno);
					res = 4;
				}
			}
			break;
		case 38:			// 本地IP
			if(nb != 4)
			{
				return 3;
			}
			else
			{
				sprintf(str,"%d.%d.%d.%d",req[offset + 7],
						req[offset + 9],
						req[offset + 11],
						req[offset + 13]);
				if (!write_profile_string("commParam","ip_address",str,A11filename))
					res = 4;
				if (!write_profile_string("manufacturers_of_custom","IPv4_ip",str,A11filename))
					res = 4;
			}
			break;
		case 42:			// 子网掩码
			if(nb != 4)
				return 3;
			else
			{
				sprintf(str,"%d.%d.%d.%d",req[offset + 7],
						req[offset + 9],
						req[offset + 11],
						req[offset + 13]);
				if (!write_profile_string("commParam","subnet_mask",str,A11filename))
					res = 4;
				if (!write_profile_string("manufacturers_of_custom","IPv4_subnet_mask",str,A11filename))
					res = 4;
			}
			break;
		case 46:			// 网关
			if(nb != 4)
				return 3;
			else
			{
				sprintf(str,"%d.%d.%d.%d",req[offset + 7],
						req[offset + 9],
						req[offset + 11],
						req[offset + 13]);
				if (!write_profile_string("commParam","gateway",str,A11filename))
					res = 4;
				if (!write_profile_string("manufacturers_of_custom","IPv4_gateway", str,A11filename))
					res = 4;
			}
			break;
		case 50:			// MAC地址
			if(nb != 6)
				return 3;
			else
			{
				sprintf(str,"%02X:%02X:%02X:%02X:%02X:%02X",req[offset + 7],
						req[offset + 9],req[offset + 11],req[offset + 13],
						req[offset + 15],req[offset + 17]);
				if (!write_profile_string("commParam","mac_address", str,A11filename))
					res = 4;
				if (!write_profile_string("manufacturers_of_custom","mac_address", str,A11filename))
					res = 4;
			}
			break;
		case 59:			// 主站 IP 地址
			if(nb != 4)
			{
				return 3;
			}
			else
			{
				sprintf(str,"%d.%d.%d.%d",req[offset + 7],
						req[offset + 9],
						req[offset + 11],
						req[offset + 13]);
				if (!write_profile_string("commParam","master_ip_address",str,A11filename))
					res = 4;
				if (!write_profile_string("manufacturers_of_custom","IPv4_master_ip",str,A11filename))
					res = 4;
			}
			break;
		default:
			if(address > 10000)
				res = 2;
			break;
	}
	return res;
}
int mb_set_slave(comm_t *ctx, int slave)
{
    /* Broadcast address is 0 (MODBUS_BROADCAST_ADDRESS) */
    if (slave >= 0 && slave <= 247) {
        ctx->slave = slave;
    } else {
        errno = EINVAL;
        return -1;
    }

    return 0;
}
void mb_set_debug(comm_t *ctx, int boolean)
{
    ctx->debug = boolean;
}
