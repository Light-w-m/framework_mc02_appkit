# Container

`container` 是 AppKit 中的一个轻量级组件容器，用于在不同函数间传递变量

## 使用说明

```c++
// 创建容器
int a;
float b;

appkit::Container container(
    appkit::Entry<int>(a, "variable_a"),
    appkit::Entry<float>(b, "variable_b"));

// 传递容器
void function(appkit::Container &container);
function(container);

// 查找变量
appkit::Result<int> result = container.Find<int>("variable_a");
```