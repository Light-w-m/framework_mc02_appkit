#pragma once

#include <proxy.hpp>
#include <can.hpp>

class FDCANProxy final
{
public:
    /**
     * @brief 构造函数
     * @param fdcan_name FDCAN设备名称
     * @param msr_id 主机ID
     * @param rx_ids 接收ID列表
     */
    FDCANProxy(const char *fdcan_name, uint32_t msr_id, std::initializer_list<uint32_t> rx_ids);

    /**
     * @brief 获取代理对象引用
     * @return 代理对象引用
     */
    FORCE_INLINE appkit::Proxy &GetProxy() noexcept
    {
        return proxy_;
    }

    /**
     * @brief 获取FDCAN对象引用
     * @return FDCAN对象引用
     */
    FORCE_INLINE appkit::FDCAN &GetFDCAN() noexcept
    {
        return fdcan_;
    }

private:
    appkit::Proxy proxy_; ///< 代理对象
    appkit::FDCAN fdcan_; ///< FDCAN硬件接口

    uint32_t msrId_; ///< 主机ID

    /**
     * @brief 发送回调函数
     * @param in_isr 是否在中断中调用
     * @param self FDCANProxy对象指针
     * @param data 发送数据
     */
    static void SendCallback(bool in_isr, FDCANProxy *self, appkit::ConstRawData data);
};