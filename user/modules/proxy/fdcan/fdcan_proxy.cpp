#include <fdcan_proxy.hpp>

FDCANProxy::FDCANProxy(const char *fdcan_name, uint32_t msr_id, std::initializer_list<uint32_t> rx_ids)
    : proxy_(64, appkit::Proxy::SendCallback::Create(SendCallback, this)),
      msrId_(msr_id)
{
    // 打开FDCAN设备
    APPKIT_RAISE_IF_NOT(appkit::Check(fdcan_.Open(fdcan_name)), "Failed to open FDCAN device");

    // 注册接收ID过滤器
    for (const auto rx_id : rx_ids)
    {
        APPKIT_RAISE_IF_NOT(
            appkit::Check(fdcan_.Register(
                appkit::FDCAN::Callback::Create(
                    [](bool in_isr, FDCANProxy *self, const appkit::FDCAN::FDPack &pack)
                    {
                        // 推送数据到代理
                        self->proxy_.PushData(in_isr, appkit::ConstRawData(pack.data, pack.data_len));
                    },
                    this),
                appkit::FDCAN::PackType::STANDARD, appkit::FDCAN::FilterMode::EQUAL, rx_id)),
            "Failed to register FDCAN filter");
    }
}

void FDCANProxy::SendCallback(bool in_isr, FDCANProxy *self, appkit::ConstRawData data)
{
    // 准备发送数据包
    appkit::FDCAN::FDPack tx_pack;
    tx_pack.id = self->msrId_;
    tx_pack.type = appkit::FDCAN::PackType::STANDARD;
    tx_pack.data_len = data.GetSize();
    data.CopyTo(appkit::RawData{tx_pack.data});

    // 发送数据包
    const auto ret = self->fdcan_.AddMessage(tx_pack);
    if (!appkit::Check(ret) && !in_isr)
    {
        APPKIT_LOG_ERROR("Failed to send FDCAN message");
    }
}