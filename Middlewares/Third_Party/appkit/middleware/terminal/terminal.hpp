#pragma once

#include <common_rw.hpp>
#include <osal_thread.hpp>
#include <ramfs.hpp>

namespace appkit
{
    class Terminal
    {
    private:
        // RamFs::Dir &bin_;       ///< /bin目录引用
        osal::Thread input_thread_; ///< 输入线程

    public:
    };
}