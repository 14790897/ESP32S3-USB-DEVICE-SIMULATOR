# ESP32-S3 USB RAM Disk

## 📖 项目简介

这是一个基于ESP32-S3的USB Mass Storage Class (MSC) RAM磁盘实现。该项目将ESP32-S3模拟为一个USB存储设备，使用内部RAM作为存储介质，为计算机提供一个高速的虚拟U盘。

## ✨ 特性

- **🚀 超高速读写** - 基于RAM的存储，读写速度极快
- **🔌 即插即用** - 使用USB OTG，无需额外驱动
- **💾 FAT12文件系统** - 兼容Windows/Linux/macOS
- **⚡ 零延迟访问** - 直接内存操作，无机械延迟
- **🛠️ 易于扩展** - 简洁的代码结构，便于二次开发

## 📋 技术规格

| 参数 | 值 |
|------|-----|
| **存储类型** | RAM Disk (易失性存储) |
| **当前容量** | 16KB (32个扇区 × 512字节) |
| **文件系统** | FAT12 |
| **接口** | USB 2.0 Mass Storage Class |
| **平台** | ESP32-S3 (需支持USB OTG) |
| **框架** | Arduino + ESP-IDF |

## ⚠️ 重要说明

### RAM Disk特性

- **易失性存储** - 设备重启或断电后，所有数据将**永久丢失**
- **高速访问** - 读写速度受限于USB接口，而非存储介质
- **容量限制** - 受ESP32-S3 RAM大小限制（约512KB SRAM可用）

### 适用场景

✅ **适合用于:**

- 临时文件传输
- 配置文件快速编辑
- 开发调试工具
- 数据采集缓存
- USB协议学习

❌ **不适合用于:**

- 长期数据存储
- 重要文件备份
- 大容量文件存储

## 🔧 硬件要求

- ESP32-S3开发板（支持USB OTG）
- USB-C数据线
- 计算机（Windows/Linux/macOS）

## 📦 软件依赖

- **PlatformIO** 或 **Arduino IDE**
- **ESP32 Arduino Core** >= 2.0.0
- **ESP32-S3 USB库**

## 🚀 快速开始

### 1. 克隆项目

```bash
git clone https://github.com/14790897/ESP32S3-USB-DEVICE-SIMULATOR.git
cd ESP32S3-USB-DEVICE-SIMULATOR
```

### 2. 编译上传

```bash
# 使用PlatformIO
pio run --target upload

# 或使用Arduino IDE直接编译上传
```

### 3. 连接使用

1. 将ESP32-S3通过USB-C连接到计算机
2. 等待设备识别（Windows可能需要几秒钟）
3. 打开"我的电脑"，会看到名为"ESP32USB"的U盘
4. 双击进入，即可像普通U盘一样使用

## 💻 代码结构

```text
src/
├── main_origin.cpp          # 主程序文件
├── (main.cpp)              # 扩展版本（如果存在）
└── ...

include/                    # 头文件目录
lib/                       # 库文件目录
platformio.ini             # PlatformIO配置
```

### 核心组件

```cpp
// RAM存储定义
#define DISK_SECTOR_COUNT 32    // 扇区数量
#define DISK_SECTOR_SIZE  512   // 每扇区字节数
uint8_t msc_disk[DISK_SECTOR_COUNT][DISK_SECTOR_SIZE];  // RAM存储数组

// MSC回调函数
static int32_t onWrite(...)  // 写入操作
static int32_t onRead(...)   // 读取操作
static bool onStartStop(...) // 启动/停止操作
```

## 🛠️ 自定义配置

### 调整存储大小

```cpp
// 修改扇区数量来调整容量
#define DISK_SECTOR_COUNT 64    // 32KB
#define DISK_SECTOR_COUNT 128   // 64KB
#define DISK_SECTOR_COUNT 256   // 128KB (接近RAM限制)
```

### 修改设备信息

```cpp
MSC.vendorID("ESP32");         // 厂商ID (最多8字符)
MSC.productID("USB_DISK");     // 产品ID (最多16字符)
MSC.productRevision("1.0");    // 版本号 (最多4字符)
```

## 🔍 调试信息

程序运行时会在串口输出详细的调试信息：

```text
[BOOT] ESP32-S3 USB MSC Demo
[MSC] Storage initialized with FAT12
[MSC] USB Mass Storage initialized
[MSC] Disk: 32 sectors x 512 bytes = 16384 bytes
[MSC] Running - Check host for USB drive
```

## 🚀 扩展想法

- **虚拟大容量** - 模拟16MB+存储，实际缓存热点数据
- **预置文件** - 启动时自动创建README等文件
- **传感器集成** - 实时数据记录到文件
- **配置管理** - 通过文件修改设备参数
- **外部存储** - 集成SD卡或SPI Flash

## 📝 许可证

MIT License - 详见 [LICENSE](LICENSE) 文件

## 🤝 贡献

欢迎提交Issue和Pull Request！

## 📧 联系方式

如有问题或建议，请创建Issue或联系维护者。

---

**⚠️ 提醒：这是一个RAM磁盘，数据在断电后会丢失！请勿用于重要数据存储。**