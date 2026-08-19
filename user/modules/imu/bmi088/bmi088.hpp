#pragma once

#ifdef ENABLED_MODULES_BMI088

#include <imu.hpp>
#include <bmi088_reg.hpp>

#include <spi.hpp>
#include <gpio.hpp>

#include <osal_thread.hpp>
#include <osal_semaphore.hpp>
#include <osal_queue.hpp>

namespace sensor::imu
{
    /**
     * @brief BMI088惯性测量单元类
     */
    class BMI088 final
    {
    public:
        /**
         * @brief 枚举传感器状态
         */
        typedef enum
        {
            NO_ERROR = 0x00,                     ///< 无错误
            ACC_PWR_CTRL_ERROR = 0x01,           ///< 加速度计电源控制错误
            ACC_PWR_CONF_ERROR = 0x02,           ///< 加速度计电源配置错误
            ACC_CONF_ERROR = 0x03,               ///< 加速度计配置错误
            ACC_SELF_TEST_ERROR = 0x04,          ///< 加速度计自检错误
            ACC_RANGE_ERROR = 0x05,              ///< 加速度计量程错误
            INT1_IO_CTRL_ERROR = 0x06,           ///< 加速度计INT1中断引脚控制错误
            INT_MAP_DATA_ERROR = 0x07,           ///< 加速度计数据中断映射错误
            GYRO_RANGE_ERROR = 0x08,             ///< 陀螺仪量程错误
            GYRO_BANDWIDTH_ERROR = 0x09,         ///< 陀螺仪带宽错误
            GYRO_LPM1_ERROR = 0x0A,              ///< 陀螺仪低功耗模式错误
            GYRO_CTRL_ERROR = 0x0B,              ///< 陀螺仪控制错误
            GYRO_INT3_INT4_IO_CONF_ERROR = 0x0C, ///< 陀螺仪INT3/INT4中断引脚配置错误
            GYRO_INT3_INT4_IO_MAP_ERROR = 0x0D,  ///< 陀螺仪INT3/INT4中断引脚映射错误

            SELF_TEST_ACCEL_ERROR = 0x80, ///< 加速度计自检错误
            SELF_TEST_GYRO_ERROR = 0x40,  ///< 陀螺仪自检错误
            NO_SENSOR = 0xFF              ///< 传感器不存在
        } Status;

    private:
#pragma pack(push, 1)
        /**
         * @brief 陀螺仪原始数据联合体定义
         */
        using GyroRawData = union
        {
            struct
            {
                uint8_t : 8; ///< 保留位
                union
                {
                    uint8_t chip_id; ///< 芯片ID
                    int16_t axis[3]; ///< 轴数据
                };
            } data;
            uint8_t raw[sizeof(data)]; ///< 原始数据字节数组
        };

        /**
         * @brief 加速度计原始数据联合体定义
         */
        using AccelRawData = union
        {
            struct
            {
                uint16_t : 16; ///< 保留位
                union
                {
                    uint8_t chip_id; ///< 芯片ID
                    int16_t axis[3]; ///< 轴数据

                    struct
                    {
                        uint8_t temp_h; ///< 温度高字节
                        uint8_t temp_l; ///< 温度低字节
                    } temp;             ///< 温度数据
                };
            } data;
            uint8_t raw[sizeof(data)]; ///< 原始数据字节数组
        };
#pragma pack(pop)

        static const appkit::Duration BMI088_SHORT_DELAY_TIME;     ///< 短延时
        static const appkit::Duration BMI088_LONG_DELAY_TIME;      ///< 长延时
        static const appkit::Duration BMI088_COM_WAIT_SENSOR_TIME; ///< 等待传感器时间

        // 硬件接口
        appkit::SPI accSpi_{}, gyroSpi_{}; ///< SPI接口对象
        appkit::GPIO gyroInt_{};           ///< GPIO中断接口对象

        // 传感器状态
        Status status_{NO_SENSOR};  ///< 传感器状态
        bool inCalibration_{false}; ///< 是否在校准中
        float temperature_{0.0f};   ///< 温度

        // 读取
        appkit::osal::Semaphore opSem_{};                  ///< 操作信号量
        appkit::ReadOperation readOp_{};                   ///< 读取操作对象
        appkit::osal::Thread readThread_{};                ///< 读取线程对象
        appkit::osal::Queue<appkit::TimePoint> readQueue_; ///< 读取时间点队列

        // 话题
        appkit::Topic topic_{};   ///< IMU数据话题
        Data imuData_{};          ///< IMU数据
        Convars imuDataConvar_{}; ///< IMU数据条件变量

        // 校准变量
        uint32_t calibCount_{0};              ///< 校准计数
        appkit::math::Vectorf<3> gyroBias_{}; ///< 陀螺仪偏置

        // 中间变量
        GyroRawData gyroRawData_{};               ///< 陀螺仪原始数据
        AccelRawData accelRawData_{};             ///< 加速度计原始数据
        appkit::math::Vectorf<3> gyroCaliData_{}; ///< 陀螺仪校准数据

        /**
         * @brief 延时函数
         * @param duration 延时时间
         */
        static void Delay(const appkit::Duration &duration);

        /**
         * @brief 写入寄存器
         * @param spi SPI设备引用
         * @param reg 寄存器地址
         * @param data 数据位置
         * @param length 写入数据长度
         * @return 错误码
         */
        appkit::ErrorCode WriteReg(appkit::SPI &spi, uint8_t reg, const uint8_t data);

        /**
         * @brief 读取加速度计数据
         * @param reg 寄存器地址
         * @param length 读取数据长度
         * @return 错误码
         * @note 读取的数据会存放在accel_raw_data_中
         */
        appkit::ErrorCode AccelRead(uint8_t reg, uint32_t length);

        /**
         * @brief 读取陀螺仪数据
         * @param reg 寄存器地址
         * @param length 读取数据长度
         * @return 错误码
         * @note 读取的数据会存放在gyro_raw_data_中
         */
        appkit::ErrorCode GyroRead(uint8_t reg, uint32_t length);

        /**
         * @brief 初始化函数
         * @return 是否初始化成功
         */
        bool Init();

        /**
         * @brief 读取线程函数
         */
        void ReadThread();

        /**
         * @brief 校准函数
         */
        void Calibration();

    public:
        /**
         * @brief 构造函数
         * @param acc_spi_name 加速度计SPI设备名称
         * @param gyro_spi_name 陀螺仪SPI设备名称
         * @param gyro_int_name 陀螺仪中断引脚名称
         */
        BMI088(const char *acc_spi_name,
               const char *gyro_spi_name,
               const char *gyro_int_name);

        /**
         * @brief 获取传感器温度
         * @return 温度，单位：摄氏度
         */
        FORCE_INLINE float GetTemperature() const
        {
            return temperature_;
        }

        /**
         * @brief 获取IMU协方差矩阵
         * @return IMU协方差矩阵
         */
        FORCE_INLINE const Convars &GetImuDataConvar() const
        {
            return imuDataConvar_;
        }
    };
} // namespace sensor::imu

#endif // ENABLED_MODULES_BMI088