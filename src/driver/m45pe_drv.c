#include "m45pe_drv.h"
#include "app_error.h"
#include "app_util_platform.h"
#include "boards.h"
#include "nrf_delay.h"
#include "nrf_drv_spi.h"
#include "nrf_gpio.h"
#include "nrf_log.h"
#include <stdio.h>
#include <string.h>

#define WRITE_ENABLED 0x06
#define WRITE_DISABLED 0x04
#define READ_ID 0x9F
#define READ_STATUS 0x05
#define READ_BYTES 0x03
#define READ_BYTES_F 0x0B
#define WRITE_PAGE 0x0A
#define PROGRAM_PAGE 0x02
#define ERASE_PAGE 0xDB
#define ERASE_SECTOR 0xD8

#define SPI_INSTANCE 0
#define TX_LENGTH 12

static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(SPI_INSTANCE); /**< SPI instance. */
static volatile bool spi_xfer_done; /**< Flag used to indicate that SPI instance completed the transfer. */

static uint8_t m_rx_buf[TX_LENGTH + 1]; /**< RX buffer. */
static uint8_t m_tx_buf[TX_LENGTH]; /**< Address of RX buffer used as read operation result. */

/**
 * @brief SPI user event handler.
 * @param event
 */
void spi_event_handler(nrf_drv_spi_evt_t const* p_event)
{
    spi_xfer_done = true;
}

void m45pe_transfer_blocking(uint8_t tx_len, uint8_t rx_len)
{ /*
    NRF_LOG_PRINTF(" Writing: ");
    int i = 0;
    for (i = 0; i < tx_len; i++)
    {
        NRF_LOG_PRINTF("%x ", (unsigned char)m_tx_buf[i]);
    }
    NRF_LOG_PRINTF("\n\r");*/

    APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, m_tx_buf, tx_len, m_rx_buf, tx_len + rx_len));

    while (!spi_xfer_done) {
        __WFE();
    }

    nrf_delay_ms(10);
}

void m45pe_write(uint8_t key, uint8_t* val, uint8_t len)
{
    if (!spi_xfer_done)
        return;

    spi_xfer_done = false;
    memset(m_tx_buf, 0, TX_LENGTH);

    m_tx_buf[0] = WRITE_ENABLED;
    m45pe_transfer_blocking(1, 0);

    m_tx_buf[0] = WRITE_PAGE;
    m_tx_buf[3] = key;
    memcpy(m_tx_buf + 4, val, len);

    m45pe_transfer_blocking(len + 4, 0);
}

void m45pe_read(uint8_t key, uint8_t* val, uint8_t len)
{
    if (!spi_xfer_done)
        return;

    spi_xfer_done = false;
    memset(m_tx_buf, 0, TX_LENGTH);
    memset(m_rx_buf, 0, TX_LENGTH + 1);

    m_tx_buf[0] = READ_BYTES;
    m_tx_buf[3] = key;

    m45pe_transfer_blocking(4, len);

    memcpy(val, m_rx_buf + 4, len);
}

void log_rx_buf()
{
    NRF_LOG_PRINTF(" Received: ");
    int i = 0;
    for (i = 0; i < TX_LENGTH; i++) {
        NRF_LOG_PRINTF("%x ", (unsigned char)m_rx_buf[i]);
    }
    NRF_LOG_PRINTF("\n\r");
}

void m45_init()
{
    nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG(SPI_INSTANCE);
    spi_config.mosi_pin = 1;
    spi_config.miso_pin = 3;
    spi_config.ss_pin = 2;
    spi_config.sck_pin = 4;
    spi_xfer_done = true;

    APP_ERROR_CHECK(nrf_drv_spi_init(&spi, &spi_config, spi_event_handler));

    NRF_LOG_PRINTF("SPI Initialized\n");

    // uint8_t val[] = { 'a', 'v', 'd', 'a', '!' };
    uint8_t res[8];

    //m45pe_write(0x0A, val, 5);
    m45pe_read(0x0A, res, 5);

    NRF_LOG_PRINTF(" Received: ");
    int i = 0;
    for (i = 0; i < 5; i++) {
        printf("%c", res[i]);
    }
}