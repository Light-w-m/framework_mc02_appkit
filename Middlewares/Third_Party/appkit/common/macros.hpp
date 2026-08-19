/**
 * @file macros.h
 * @brief 通用宏定义
 * @author dusk
 * @date 2025-12-25
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <memory>

/**
 * @brief 强制内联宏定义
 */
#ifndef FORCE_INLINE
#if defined(__GNUC__)
#define FORCE_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#define FORCE_INLINE __forceinline
#elif defined(__ICCARM__)
#define FORCE_INLINE inline
#elif defined(__CC_ARM)
#define FORCE_INLINE __forceinline
#else
#define FORCE_INLINE inline
#endif
#endif

/**
 * @brief 系统缓存行大小
 */
#include <new>
#ifdef __cpp_lib_hardware_interference_size
constexpr inline size_t SYSTEM_CACHE_LINE_SIZE = std::hardware_constructive_interference_size;
#else
constexpr inline size_t SYSTEM_CACHE_LINE_SIZE = __SIZEOF_SIZE_T__ * 8;
#endif

/**
 * @brief 系统对齐大小
 */
#define SYSTEM_ALIGN_SIZE __SIZEOF_POINTER__

/**
 * @brief 属性声明宏
 * @param val_type 属性类型
 * @param val_name 属性名
 * @param defval 属性默认值
 */
#define DECL_PROPERTY(val_type, val_name, defval) \
private:                                          \
    val_type val_name##_ = defval;

/**
 * @brief 静态属性声明宏
 * @param permission 访问权限
 * @param val_type 属性类型
 * @param val_name 属性名
 * @param defval 属性默认值
 */
#define DECL_STATIC_PROPERTY(permission, val_type, val_name, defval) \
    permission:                                                      \
    static inline val_type val_name##_ = defval;

/**
 * @brief 属性访问宏
 * @param val_name 属性名
 */
#define PROPERTY(val_name) (val_name##_)

/**
 * @brief 类复制禁用宏
 * @param cls_name 要禁用复制的类名
 */
#define DECL_COPY_DISABLE(cls_name)      \
    cls_name(const cls_name &) = delete; \
    cls_name &operator=(const cls_name &) = delete;

/**
 * @brief 类移动禁用宏
 * @param cls_name 要禁用移动的类名
 */
#define DECL_MOVE_DISABLE(cls_name) \
    cls_name(cls_name &&) = delete; \
    cls_name &operator=(cls_name &&) = delete;

/**
 * @brief 未使用变量标记宏
 * @param x 未使用的变量
 */
#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

/**
 * @brief 计算数组大小宏
 * @param arr 数组名
 */
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

/**
 * @brief 计算结构体成员偏移量宏
 * @param type 结构体类型
 * @param member 成员名
 */
#ifndef OFFSET_OF
#define OFFSET_OF(type, member) ((size_t)&(((type *)0)->member))
#endif

/**
 * @brief 通过成员指针获取结构体指针宏
 * @param ptr 成员指针
 * @param type 结构体类型
 * @param member 成员名
 */
#ifndef CONTAINER_OF
#define CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - OFFSET_OF(type, member)))
#endif
