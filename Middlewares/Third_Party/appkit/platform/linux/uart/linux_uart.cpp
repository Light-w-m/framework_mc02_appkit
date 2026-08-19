#include <linux_uart.hpp>

#include <fcntl.h>
#include <libudev.h>
#include <linux/serial.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include <filesystem>
#include <logger.hpp>

using namespace appkit::time_literals;

namespace appkit
{
    LinuxUart::LinuxUart(const char *name, const std::string &device_path, size_t tx_queue_size, size_t buffer_size, size_t thread_stack_size)
        : devicePath_(device_path), read_(buffer_size), write_(tx_queue_size, tx_queue_size * buffer_size), buffer_size_(buffer_size),
          tx_buffer_(new uint8_t[buffer_size]), rx_buffer_(new uint8_t[buffer_size])
    {
        APPKIT_RAISE_IF_NOT(!device_path.empty(), "Device path must not be empty");
        APPKIT_RAISE_IF_NOT(tx_queue_size > 0, "TX queue size must be greater than 0");
        APPKIT_RAISE_IF_NOT(buffer_size > 0, "RX buffer size must be greater than 0");

        // 等待设备节点出现
        while (!std::filesystem::exists(device_path))
        {
            APPKIT_LOG_WARNING("Waiting for device %s to be available...", device_path.c_str());
            osal::this_thread::SleepFor(100_ms);
        }

        // 打开设备文件
        fd_ = open(device_path.c_str(), O_RDWR | O_NOCTTY);
        if (fd_ < 0)
        {
            APPKIT_LOG_ERROR("Failed to open device %s", device_path.c_str());
            APPKIT_RAISE("Failed to open UART device");
        }

        // 设置默认配置
        APPKIT_RAISE_IF_NOT(Check(SetConfig(config_)), "Failed to set default UART configuration");

        connected_ = true;
        APPKIT_LOG_DEBUG("Device %s opened with fd %d", device_path.c_str(), fd_);

        // 注册读写函数
        read_ = ReadFun;
        write_ = WriteFun;

        // 创建读取线程
        readThread_.Create(
            [this]()
            { ReadThreadFunc(); },
            ("Read_" + device_path).c_str(), thread_stack_size, osal::Thread::Priority::REALTIME);

        // 创建写入线程
        writeThread_.Create(
            [this]()
            { WriteThreadFunc(); },
            ("Write_" + device_path).c_str(), thread_stack_size, osal::Thread::Priority::REALTIME);

        // 注册设备
        APPKIT_RAISE_IF_NOT(Check(RegisterDevice(name)), "Failed to register UART device");
    }

    LinuxUart::~LinuxUart()
    {
        if (fd_ >= 0)
        {
            close(fd_);
            fd_ = -1;
        }
    }

    ErrorCode LinuxUart::SetConfig(const Uart::Config &config)
    {
        if (fd_ < 0)
        {
            return ErrorCode::FAILED;
        }

        struct termios tty{};

        // 获取当前配置
        if (tcgetattr(fd_, &tty) != 0)
        {
            APPKIT_LOG_ERROR("Failed to get attributes for device %s", devicePath_.c_str());
            return ErrorCode::FAILED;
        }

        // 设置波特率
        speed_t baud_rate;
        switch (config.baud_rate)
        {
        case Uart::BaudRate::BR_9600:
            baud_rate = B9600;
            break;
        case Uart::BaudRate::BR_19200:
            baud_rate = B19200;
            break;
        case Uart::BaudRate::BR_38400:
            baud_rate = B38400;
            break;
        case Uart::BaudRate::BR_57600:
            baud_rate = B57600;
            break;
        case Uart::BaudRate::BR_115200:
            baud_rate = B115200;
            break;
        case Uart::BaudRate::BR_230400:
            baud_rate = B230400;
            break;
        case Uart::BaudRate::BR_460800:
            baud_rate = B460800;
            break;
        case Uart::BaudRate::BR_921600:
            baud_rate = B921600;
            break;
        case Uart::BaudRate::BR_1000000:
            baud_rate = B1000000;
            break;
        case Uart::BaudRate::BR_2000000:
            baud_rate = B2000000;
            break;
        case Uart::BaudRate::BR_3000000:
            baud_rate = B3000000;
            break;
        case Uart::BaudRate::BR_4000000:
            baud_rate = B4000000;
            break;
        default:
            APPKIT_LOG_ERROR("Unsupported baud rate: %u", config.baud_rate);
            return ErrorCode::INVALID_ARG;
        }
        cfsetispeed(&tty, baud_rate);
        cfsetospeed(&tty, baud_rate);

        // 设置数据位
        tty.c_cflag &= ~CSIZE;
        switch (config.data_bits)
        {
        case Uart::DataBits::FIVE:
            tty.c_cflag |= CS5;
            break;
        case Uart::DataBits::SIX:
            tty.c_cflag |= CS6;
            break;
        case Uart::DataBits::SEVEN:
            tty.c_cflag |= CS7;
            break;
        case Uart::DataBits::EIGHT:
            tty.c_cflag |= CS8;
            break;
        default:
            APPKIT_LOG_ERROR("Unsupported data bits: %u", static_cast<uint8_t>(config.data_bits));
            return ErrorCode::INVALID_ARG;
        }

        // 设置停止位
        switch (config.stop_bits)
        {
        case Uart::StopBits::ONE:
            tty.c_cflag &= ~CSTOPB;
            break;
        case Uart::StopBits::TWO:
            tty.c_cflag |= CSTOPB;
            break;
        default:
            APPKIT_LOG_ERROR("Unsupported stop bits: %u", static_cast<uint8_t>(config.stop_bits));
            return ErrorCode::INVALID_ARG;
        }

        // 设置奇偶校验
        switch (config.parity)
        {
        case Uart::Parity::NONE:
            tty.c_cflag &= ~PARENB;
            break;
        case Uart::Parity::EVEN:
            tty.c_cflag |= PARENB;
            tty.c_cflag &= ~PARODD;
            break;
        case Uart::Parity::ODD:
            tty.c_cflag |= PARENB;
            tty.c_cflag |= PARODD;
            break;
        default:
            APPKIT_LOG_ERROR("Unsupported parity: %u", static_cast<uint8_t>(config.parity));
            return ErrorCode::INVALID_ARG;
        }

        // 设置流控
        switch (config.flow_control)
        {
        case Uart::FlowControl::NONE:
            tty.c_cflag &= ~CRTSCTS;
            tty.c_iflag &= ~(IXON | IXOFF | IXANY);
            break;
        case Uart::FlowControl::RTS_CTS:
            tty.c_cflag |= CRTSCTS;
            break;
        case Uart::FlowControl::XON_XOFF:
            tty.c_iflag |= (IXON | IXOFF | IXANY);
            break;
        default:
            APPKIT_LOG_ERROR("Unsupported flow control: %u", static_cast<uint8_t>(config.flow_control));
            return ErrorCode::INVALID_ARG;
        }

        // 启用本地模式、读功能
        tty.c_cflag |= (CLOCAL | CREAD);

        // 输入模式：关闭软件流控、特殊字符处理
        tty.c_iflag &= ~(IXON | IXOFF | IXANY | ISTRIP | IGNCR | INLCR | ICRNL
#ifdef IUCLC
                         | IUCLC
#endif
        );

        // 输出模式：关闭所有加工
        tty.c_oflag &= ~(OPOST
#ifdef ONLCR
                         | ONLCR
#endif
#ifdef OCRNL
                         | OCRNL
#endif
#ifdef ONOCR
                         | ONOCR
#endif
#ifdef ONLRET
                         | ONLRET
#endif
        );

        // 本地模式：禁用行缓冲、回显、信号中断
        tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

        tty.c_cc[VMIN] = 0;   // 最小读取字符数
        tty.c_cc[VTIME] = 10; // 读取超时,1秒

        // 应用配置
        if (tcsetattr(fd_, TCSANOW, &tty) != 0)
        {
            APPKIT_LOG_ERROR("Failed to set attributes for device %s", devicePath_.c_str());
            return ErrorCode::FAILED;
        }

        // 设置低延迟模式
        if (struct serial_struct serial;
            ioctl(fd_, TIOCGSERIAL, &serial) == 0)
        {
            serial.flags |= ASYNC_LOW_LATENCY;
            if (ioctl(fd_, TIOCSSERIAL, &serial) != 0)
            {
                APPKIT_LOG_WARNING("Failed to set low latency mode for device %s", devicePath_.c_str());
            }
        }
        else
        {
            APPKIT_LOG_WARNING("Failed to get serial struct for device %s", devicePath_.c_str());
        }

        // 刷新输入输出缓冲区
        tcflush(fd_, TCIOFLUSH);

        config_ = config;
        return ErrorCode::OK;
    }

    ErrorCode LinuxUart::WriteFun(WritePort &port)
    {
        LinuxUart *self = CONTAINER_OF(&port, LinuxUart, write_);
        self->writeSemaphore_.Post();
        return ErrorCode::OK;
    }

    ErrorCode LinuxUart::ReadFun(ReadPort &port)
    {
        return ErrorCode::EMPTY;
    }

    void LinuxUart::ReadThreadFunc()
    {
        for (;;)
        {
            // 判断连接状态
            if (!connected_)
            {
                // 尝试重新连接
                if (fd_ >= 0)
                {
                    close(fd_);
                    fd_ = -1;
                }

                // 等待设备节点出现
                if (!std::filesystem::exists(devicePath_))
                {
                    APPKIT_LOG_WARNING("Waiting for device %s to be available...", devicePath_.c_str());
                    osal::this_thread::SleepFor(100_ms);
                    continue;
                }

                fd_ = open(devicePath_.c_str(), O_RDWR | O_NOCTTY);
                if (fd_ >= 0)
                {
                    SetConfig(config_);
                    connected_ = true;
                    APPKIT_LOG_INFO("Reconnected to device %s", devicePath_.c_str());
                }
                else
                {
                    APPKIT_LOG_WARNING("Failed to reconnect to device %s", devicePath_.c_str());
                    osal::this_thread::SleepFor(1_s);
                    continue;
                }
            }

            // 读取数据
            auto n = ::read(fd_, rx_buffer_, buffer_size_);
            if (n > 0)
            {
                // 将数据放入读取端口缓冲区
                read_.buffer_->PushBatch(rx_buffer_, static_cast<size_t>(n));
                read_.ProcessPendingReads(false);
            }
            else if (n < 0)
            {
                // 读取错误，标记为未连接
                APPKIT_LOG_ERROR("Read error on device %s(%s)", devicePath_.c_str(), strerror(errno));
                connected_ = false;
            }
        }
    }

    void LinuxUart::WriteThreadFunc()
    {
        thread_local uint8_t tx_buffer[128];

        for (;;)
        {
            // 判断连接状态
            if (!connected_)
            {
                osal::this_thread::SleepFor(1_ms);
                continue;
            }

            // 等待写入信号
            if (!Check(writeSemaphore_.Wait(Duration::Max())))
            {
                continue;
            }

            // 获取写入信息
            WriteInfo info;
            if (!Check(write_.info_buffer_->Pop(info)))
            {
                continue;
            }

            // 写入数据
            size_t total_written = 0;
            for (auto leave_size = info.data.GetSize(); leave_size > 0;)
            {
                size_t to_write = std::min(leave_size, buffer_size_);
                if (to_write == 0)
                {
                    break;
                }

                // 复制数据到本地缓冲区
                write_.data_buffer_->PopBatch(tx_buffer, to_write);

                // 写入数据到设备
                ssize_t n = ::write(fd_, tx_buffer, to_write);
                if (n > 0)
                {
                    total_written += static_cast<size_t>(n);
                    leave_size -= static_cast<size_t>(n);
                }
                else if (n < 0)
                {
                    // 写入错误，标记为未连接
                    APPKIT_LOG_ERROR("Write error on device %s", devicePath_.c_str());
                    connected_ = false;
                    break;
                }
            }

            // 完成写入操作
            write_.Finish(false, total_written == info.data.GetSize() ? ErrorCode::OK : ErrorCode::FAILED, info, total_written);
        }
    }
}