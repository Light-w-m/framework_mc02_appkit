#include <ws2812.hpp>

using namespace appkit;

namespace led
{
    WS2812::WS2812(const char *spi_name)
    {
        APPKIT_RAISE_IF_NOT(appkit::Check(handle_.Open(spi_name)), "Failed to open SPI device");

        // 初始化发送缓冲区（只有前8位固定为0，后续位数根据颜色变化）
        for (auto i = 0; i < 8; i++)
        {
            txbuf[i] = 0;
        }
    }

    WS2812::~WS2812()
    {
    }

    ErrorCode WS2812::SetColor(uint8_t r, uint8_t g, uint8_t b)
    {
        if (!Check(op_.TestAndSetPolling()))
        {
            return ErrorCode::BUSY;
        }

        // WS2812B GRB
        for (int i = 0; i < 8; i++)
        {
            txbuf[15 - i] = ((g >> i) & 0x01) ? HIGH : LOW;
            txbuf[23 - i] = ((r >> i) & 0x01) ? HIGH : LOW;
            txbuf[31 - i] = ((b >> i) & 0x01) ? HIGH : LOW;
        }

        return handle_.Write(txdata_, op_);
    }
} // namespace led
