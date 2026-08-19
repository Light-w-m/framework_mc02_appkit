#include <linux_random.hpp>

#include <fcntl.h>
#include <unistd.h>

#include <logger.hpp>

namespace appkit
{
    LinuxRandom::LinuxRandom(const char *name)
    {
        APPKIT_RAISE_IF_NOT(name != nullptr, "Device name must not be null");

        // 打开随机数设备文件
        fd_ = open("/dev/urandom", O_RDONLY);
        APPKIT_RAISE_IF(fd_ < 0, "Failed to open random device");

        APPKIT_LOG_DEBUG("Random device %s opened with fd %d", name, fd_);

        // 注册设备
        APPKIT_RAISE_IF_NOT(Check(RegisterDevice(name)), "Failed to register random device");
    }

    LinuxRandom::~LinuxRandom()
    {
        if (fd_ >= 0)
        {
            close(fd_);
            fd_ = -1;
            APPKIT_LOG_DEBUG("Random device closed");
        }
    }

    size_t LinuxRandom::GetRawNum()
    {
        osal::LockGuard lock(mutex_);

        size_t random_value = 0;
        ssize_t result = read(fd_, &random_value, sizeof(random_value));
        if (result != sizeof(random_value))
        {
            APPKIT_LOG_ERROR("Failed to read random number from device");
            return 0;
        }

        return random_value;
    }
} // namespace appkit