/*
 *
 * FocalTech TouchScreen driver.
 *
 * Copyright (c) 2012-2018, FocalTech Systems, Ltd., all rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/************************************************************************
*
* File Name: focaltech_spi.c
*
*    Author: FocalTech Driver Team
*
*   Created: 2018-11-17
*
*  Abstract: spi communication with TP
*
*   Version: v1.1
*
* Revision History:
*
************************************************************************/

/*****************************************************************************
* Included header files
*****************************************************************************/
#include "focaltech_core.h"

/*****************************************************************************
* Private constant and macro definitions using #define
*****************************************************************************/
#define STATUS_PACKAGE              0x05
#define COMMAND_PACKAGE             0xC0
#define DATA_PACKAGE                0x3F
#define BUSY_QUERY_TIMEOUT          100
#define BUSY_QUERY_DELAY            150 /* unit: us */
#define CS_HIGH_DELAY               150 /* unit: us */
#define DELAY_AFTER_FIRST_BYTE      30
#define SPI_HEADER_LENGTH           4

#define DATA_CRC_EN                 0x20
#define WRITE_CMD                   0x00
#define READ_CMD                    (0x80 | DATA_CRC_EN)
#define CD_PACKAGE_BUFLEN           4

#define MTK_SPI_PACKET_SIZE         1024

/*****************************************************************************
* Private enumerations, structures and unions using typedef
*****************************************************************************/

/*****************************************************************************
* Static variables
*****************************************************************************/

/*****************************************************************************
* Global variable or extern global variabls/functions
*****************************************************************************/

/*****************************************************************************
* Static function prototypes
*****************************************************************************/

/*****************************************************************************
* functions body
*****************************************************************************/
/* spi interface */
static int fts_spi_transfer(u8 *tx_buf, u8 *rx_buf, u32 len)
{
    int ret = 0;
    struct spi_device *spi = fts_data_V2->spi;
    struct spi_message msg;
    struct spi_transfer xfer[3];
    int i = 0;
    int packet_num = 0;
    int packet_length = 0;
    int remainder = 0;
    int data_len = 0;
    int data_pos = 0;

    memset(&xfer[0], 0, sizeof(struct spi_transfer));
    memset(&xfer[1], 0, sizeof(struct spi_transfer));
    memset(&xfer[2], 0, sizeof(struct spi_transfer));

    spi_message_init(&msg);
    xfer[0].tx_buf = &tx_buf[0];
    xfer[0].len = 1;
    xfer[0].delay_usecs = DELAY_AFTER_FIRST_BYTE;
    spi_message_add_tail(&xfer[0], &msg);

    if (len > CD_PACKAGE_BUFLEN) {
        data_len = len - CD_PACKAGE_BUFLEN;
        data_pos = CD_PACKAGE_BUFLEN;
        packet_num = 2; /* must 2 packets at least */
#if 0
        remainder = data_len % MTK_SPI_PACKET_SIZE;
        if ((data_len > MTK_SPI_PACKET_SIZE) && remainder) {
            packet_num++;
        }
        packet_length = data_len - remainder;
#else
        packet_length = data_len;
#endif

        for (i = 1; i < packet_num; i++) {
            if ((i == (packet_num - 1)) && remainder)
                packet_length = remainder;
            xfer[i].tx_buf = &tx_buf[data_pos];
            if (rx_buf)
                xfer[i].rx_buf = &rx_buf[data_pos];
            xfer[i].len = packet_length;
            spi_message_add_tail(&xfer[i], &msg);
            data_pos += packet_length;
        }
    }

    ret = spi_sync(spi, &msg);
    if (ret) {
        FTS_ERROR("spi_sync fail,ret:%d", ret);
        return ret;
    }

    udelay(CS_HIGH_DELAY);
    return ret;
}

static void crckermit(u8 *data, u16 len, u16 *crc_out)
{
    u16 i = 0;
    u16 j = 0;
    u16 crc = 0xFFFF;

    for ( i = 0; i < len; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            if (crc & 0x01)
                crc = (crc >> 1) ^ 0x8408;
            else
                crc = (crc >> 1);
        }
    }

    *crc_out = crc;
}

static int rdata_check(u8 *rdata, u32 rlen)
{
    u16 crc_calc = 0;
    u16 crc_read = 0;

    crckermit(rdata, rlen - 2, &crc_calc);
    crc_read = (u16)(rdata[rlen - 1] << 8) + rdata[rlen - 2];
    if (crc_calc != crc_read) {
        return -EIO;
    }

    return 0;
}

static int fts_wait_idle(void)
{
    int ret = 0;
    int i = 0;
    int status = 0xFF;
    u8 cmd[1 + CD_PACKAGE_BUFLEN] = { STATUS_PACKAGE, 0xFF, 0xFF, 0xFF, 0xFF };
    u8 value[1 + CD_PACKAGE_BUFLEN] = { 0 };

    for (i = 0; i < BUSY_QUERY_TIMEOUT; i++) {
        udelay(BUSY_QUERY_DELAY);
        ret = fts_spi_transfer(cmd, value, 1 + CD_PACKAGE_BUFLEN);
        if (ret >= 0) {
            status = (int)value[CD_PACKAGE_BUFLEN];
            if ((fts_data_V2->fw_is_running && (0x01 == (status & 0x81)))
				|| (!fts_data_V2->fw_is_running && (0x00 == (status & 0x81)))) {
                break;
            }
        }
    }

    if (i >= BUSY_QUERY_TIMEOUT) {
        FTS_ERROR("spi is busy, fts_data_V2->fw_is_running:%d,status:0x%x", fts_data_V2->fw_is_running, status);
        return -EIO;
    }

    return (int)status;
}

static int fts_cmdpkg_wirte(u8 ctrl, u8 *cmd, u32 cmdlen)
{
    int i = 0;
    int pos = 0;
	u8 *buf = fts_data_V2->spi_cmd;

	memset(buf, 0, SPI_MAX_COMMAND_LENGTH);

    if (!cmd || (cmdlen >= SPI_MAX_COMMAND_LENGTH - SPI_HEADER_LENGTH)) {
        FTS_ERROR("cmd/cmdlen fail");
        return -EINVAL;
    }

    buf[0] = COMMAND_PACKAGE;
    pos = pos + CD_PACKAGE_BUFLEN;
    buf[pos++] = ctrl | (cmdlen & 0x0F);
    for (i = 0; i < cmdlen; i++) {
        buf[pos++] = cmd[i];
    }

    return fts_spi_transfer(buf, NULL, pos);
}

static int fts_boot_write(u8 *cmd, u32 cmdlen, u8 *data, u32 datalen)
{
    int ret = 0;
    u8 *txbuf = NULL;
    u32 txlen = 0;
    u8 tmpcmd[SPI_MAX_COMMAND_LENGTH] = { 0 };

    if ((!cmd) || (!cmdlen)
        || (cmdlen > SPI_MAX_COMMAND_LENGTH - SPI_HEADER_LENGTH)) {
        FTS_ERROR("cmd/cmdlen is invalid");
        return -EINVAL;
    }

    mutex_lock(&fts_data_V2->spilock);
    /* wait spi idle */
    ret = fts_wait_idle();
    if (ret < 0) {
        FTS_ERROR("wait spi idle fail");
        goto err_boot_write;
    }

    /* write cmd */
    memcpy(tmpcmd, cmd, cmdlen);
    if (fts_data_V2->fw_is_running && data && datalen) {
        tmpcmd[cmdlen++] = (datalen >> 8) & 0xFF;
        tmpcmd[cmdlen++] = datalen & 0xFF;
    }
    ret = fts_cmdpkg_wirte(WRITE_CMD, tmpcmd, cmdlen);
    if (ret < 0) {
        FTS_ERROR("command package wirte fail");
        goto err_boot_write;
    }

    /* have data, transfer data */
    if (data && datalen) {
        /* wait spi idle */
        ret = fts_wait_idle();
        if (ret < 0) {
            FTS_ERROR("wait spi idle from cmd fail");
            goto err_boot_write;
        }

        /* write data */
        if (datalen > SPI_BUF_LENGTH - SPI_HEADER_LENGTH) {
			if (!fts_data_V2->fw_spi) {
				fts_data_V2->fw_spi = kzalloc(datalen + SPI_HEADER_LENGTH, GFP_KERNEL | GFP_DMA);
				if (NULL == fts_data_V2->fw_spi) {
	                FTS_ERROR("txbuf malloc fail");
	                ret = -ENOMEM;
	                goto err_boot_write;
            	}
			}
			txbuf = fts_data_V2->fw_spi;
        } else {
            txbuf = fts_data_V2->spi_buf;
        }
        memset(txbuf, 0xFF, datalen + SPI_HEADER_LENGTH);
        txbuf[0] = DATA_PACKAGE;
        txlen = datalen + CD_PACKAGE_BUFLEN;
        memcpy(txbuf + CD_PACKAGE_BUFLEN, data, datalen);

        ret = fts_spi_transfer(txbuf, NULL, txlen);
        if (ret < 0) {
            FTS_ERROR("data wirte fail");
        }
    }

err_boot_write:
    mutex_unlock(&fts_data_V2->spilock);
    return ret;
}

int fts_write_V2(u8 *writebuf, u32 writelen)
{
    u8 *cmd = NULL;
    u32 cmdlen = 0;
    u8 *data = NULL;
    u32 datalen = 0;

    if (!writebuf || !writelen) {
        FTS_ERROR("writebuf/len is invalid");
        return -EINVAL;
    }

    if (1 == writelen) {
        cmd = writebuf;
        cmdlen = 1;
        data = NULL;
        datalen = 0;
    } else {
        cmd = writebuf;
        cmdlen = 1;
        if (!fts_data_V2->fw_is_running) {
            if ((cmd[0] == 0xAE) || (cmd[0] == 0x85) || (cmd[0] == 0xF2)) {
                cmdlen = 6;
            } else if (cmd[0] == 0xCC) {
                cmdlen = 7;
            }
        }
        data = writebuf + cmdlen;
        datalen = writelen - cmdlen;
    }

    return fts_boot_write(&cmd[0], cmdlen, data, datalen);
}

int fts_write_then_read(u8 addr, u8 value)
{
	int ret = 0;
	u8 temp_data = 0;

	ret = fts_write_reg_byte_V2(addr, value);
	if (ret < 0)
		VTI("fail to wirte 0x%x register value: %d", addr, value);

	mdelay(5);
	ret = fts_read_reg_byte_V2(addr, &temp_data);
	if (ret < 0)
		VTI("fail to read 0x%x register value: %d", addr, temp_data);

	if (temp_data != value) {
		VTI("Fail to write 0x%x value: %d", addr, value);
		ret = -1;
	} else {
		VTI("Success to write 0x%x value: %d", addr, value);
		ret = 0;
	}
	return ret;
}

int fts_write_reg_byte_V2(u8 addr, u8 value)
{
    return fts_boot_write(&addr, 1, &value, 1);
}

int fts_read_V2(u8 *cmd, u32 cmdlen, u8 *data, u32 datalen)
{
    int ret = 0;
    u8 *txbuf = NULL;
    u32 txlen = 0;
    u8 ctrl = READ_CMD;
    u8 tmpcmd[SPI_MAX_COMMAND_LENGTH] = { 0 };

    mutex_lock(&fts_data_V2->spilock);
    if (cmd && cmdlen) {
        /* wait spi idle */
        ret = fts_wait_idle();
        if (ret < 0) {
            FTS_ERROR("wait spi idle fail");
            goto boot_read_err;
        }

        /* write cmd */
        memcpy(tmpcmd, cmd, cmdlen);
        if (fts_data_V2->fw_is_running) {
            tmpcmd[cmdlen++] = (datalen >> 8) & 0xFF;
            tmpcmd[cmdlen++] = datalen & 0xFF;
        }
        ret = fts_cmdpkg_wirte(ctrl, tmpcmd, cmdlen);
        if (ret < 0) {
            FTS_ERROR("command package wirte fail");
            goto boot_read_err;
        }

        /* wait spi idle */
        ret = fts_wait_idle();
        if (ret < 0) {
            FTS_ERROR("wait spi idle from cmd fail");
            goto boot_read_err;
        }
    }

    if (data && datalen) {
        /* write data */
        if (datalen > SPI_BUF_LENGTH - SPI_HEADER_LENGTH) {
            txbuf = kzalloc(datalen + SPI_HEADER_LENGTH, GFP_KERNEL | GFP_DMA);
            if (NULL == txbuf) {
                FTS_ERROR("txbuf kalloc fail");
                ret = -ENOMEM;
                goto boot_read_err;
            }
        } else {
            txbuf = fts_data_V2->spi_buf;
			if (NULL == txbuf){
				FTS_ERROR("spi_buf kalloc fail");
                ret = -ENOMEM;
                goto boot_read_err;
			}
        }
		
        memset(txbuf, 0xFF, datalen + SPI_HEADER_LENGTH);
        txbuf[0] = DATA_PACKAGE;
        txlen = datalen + CD_PACKAGE_BUFLEN;
        if (ctrl & DATA_CRC_EN) {
            txlen = txlen + 2;
        }
        ret = fts_spi_transfer(txbuf, txbuf, txlen);
        if (ret < 0) {
            FTS_ERROR("data read fail");
            goto boot_read_err;
        }
        memcpy(data, txbuf + CD_PACKAGE_BUFLEN, datalen);
        /* crc check */
        if (ctrl & DATA_CRC_EN) {
            ret = rdata_check(txbuf + CD_PACKAGE_BUFLEN,
                              txlen - CD_PACKAGE_BUFLEN);
            if (ret < 0) {
                FTS_INFO("read data crc check incorrect");
                goto boot_read_err;
            }
        }

        if (txbuf && (datalen > SPI_BUF_LENGTH - SPI_HEADER_LENGTH)) {
            kfree(txbuf);
            txbuf = NULL;
        }
    }
boot_read_err:
    mutex_unlock(&fts_data_V2->spilock);
    return ret;
}

int fts_read_reg_byte_V2(u8 addr, u8 *value)
{
    return fts_read_V2(&addr, 1, value, 1);
}
