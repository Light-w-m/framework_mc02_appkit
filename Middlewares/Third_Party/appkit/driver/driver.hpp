#pragma once

#include <ramfs.hpp>
#include <logger.hpp>

namespace appkit
{
    /**
     * @brief 设备驱动基类
     * @tparam BlockType 设备驱动块类型
     */
    template <typename BlockType>
    class Driver
    {
    protected:
        RamFs::File file_; ///< 设备文件

        /**
         * @brief 注册设备文件
         * @param name 设备名称
         * @param file 设备文件引用
         * @return 错误码
         * @note 设备必须是RamFs中的文件，且文件类型必须是BlockType
         */
        ErrorCode RegisterDevice(this BlockType& self, const char *name)
        {
            static_assert(std::is_base_of_v<Driver<BlockType>, BlockType>, "BlockType must be derived from Driver<BlockType>");

            if (name == nullptr) [[unlikely]]
            {
                APPKIT_LOG_ERROR("Device name must not be null");
                return ErrorCode::INVALID_ARG;
            }

            self.file_ = RamFs::CreateFile<BlockType>(name, self);
            RamFs::Dev().Add(self.file_);

            APPKIT_LOG_INFO("Device /dev/%s registered", name);
            return ErrorCode::OK;
        }

    public:
        /**
         * @brief 获取设备文件和设备块
         * @param name 设备名称
         * @param file 设备文件指针引用
         * @param block 设备块指针引用
         * @return 错误码
         */
        FORCE_INLINE static ErrorCode GetDeviceBlock(const char *name, BlockType **block)
        {
            if (name == nullptr || block == nullptr) [[unlikely]]
            {
                return ErrorCode::INVALID_ARG;
            }

            const auto head = RamFs::Dev().Find(name);
            if (head == nullptr) [[unlikely]]
            {
                APPKIT_LOG_WARNING("Device %s not found", name);
                return ErrorCode::NO_FOUND;
            }

            RamFs::File *file;
            if (const auto ret = RamFs::AsFile(head, &file);
                !Check(ret)) [[unlikely]]
            {
                APPKIT_LOG_ERROR("Device %s is not a file", name);
                return ret;
            }

            if (!file->CheckSize<BlockType>()) [[unlikely]]
            {
                APPKIT_LOG_ERROR("Device %s type mismatch", name);
                return ErrorCode::NO_FOUND;
            }

            *block = std::addressof(file->Get<BlockType>());
            return ErrorCode::OK;
        }
    };
}
