# Logger

`logger`是基于`appkit::STDIO`实现的日志模块

## 参数

```yaml
appkit:
    logger:
        buffer_size: 64         # 日志缓冲区大小
        level: DEBUG            # 日志级别
        output_level: INFO      # 日志输出级别
        publish_msg: true       # 是否通过消息发布日志
```

## 使用说明

对于不同等级的话题有以下宏定义可供使用

> 是否支持浮点数打印取决于使用的平台是否支持

```c++
APPKIT_LOG_DEBUG(fmt, ...)
APPKIT_LOG_INFO(fmt, ...)
APPKIT_LOG_WARNING(fmt, ...)
APPKIT_LOG_ERROR(fmt, ...)
```

在启用了日志话题发布的情况下可以通过`/sys/log`话题订阅，在满足打印等级要求时将通过标准输出端口输出