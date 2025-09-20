#include "Adafruit_TinyUSB.h"

// 8KB是Windows允许挂载的最小大小
#define DISK_BLOCK_NUM  16
#define DISK_BLOCK_SIZE 512

// RAM Disk 存储数组
uint8_t msc_disk[DISK_BLOCK_NUM][DISK_BLOCK_SIZE];

Adafruit_USBD_MSC usb_msc;

// MSC 回调函数
int32_t msc_read_callback(uint32_t lba, void* buffer, uint32_t bufsize);
int32_t msc_write_callback(uint32_t lba, uint8_t* buffer, uint32_t bufsize);
void msc_flush_callback(void);
bool msc_start_stop_callback(uint8_t power_condition, bool start, bool load_eject);

void setup() {
  // 手动初始化 TinyUSB（某些核心需要）
  if (!TinyUSBDevice.isInitialized()) {
    TinyUSBDevice.begin(0);
  }
  
  Serial.begin(115200);
  
  // 等待串口连接
  delay(1000);
  Serial.println("=== Adafruit TinyUSB MSC RAM Disk Example ===");
  
  // 设置设备信息字符串（最多8、16、4个字符）
  usb_msc.setID("Adafruit", "TinyUSB Disk", "1.0");
  
  // 设置磁盘大小
  usb_msc.setCapacity(DISK_BLOCK_NUM, DISK_BLOCK_SIZE);
  
  // 设置回调函数
  usb_msc.setReadWriteCallback(msc_read_callback, msc_write_callback, msc_flush_callback);
  usb_msc.setStartStopCallback(msc_start_stop_callback);
  
  // 设置设备就绪（RAM磁盘总是就绪的）
  usb_msc.setUnitReady(true);
  
  // 开始MSC服务
  usb_msc.begin();
  
  // 如果设备已经枚举，需要重新连接以应用更改
  if (TinyUSBDevice.mounted()) {
    TinyUSBDevice.detach();
    delay(10);
    TinyUSBDevice.attach();
  }
  
  Serial.println("TinyUSB MSC 初始化完成");
  Serial.printf("磁盘大小: %d blocks x %d bytes = %d bytes\n", 
                DISK_BLOCK_NUM, DISK_BLOCK_SIZE, 
                DISK_BLOCK_NUM * DISK_BLOCK_SIZE);
}

void loop() {
  #ifdef TINYUSB_NEED_POLLING_TASK
  // 手动调用 tud_task，某些核心需要
  TinyUSBDevice.task();
  #endif
  
  // 每5秒打印状态
  static uint32_t last_print = 0;
  if (millis() - last_print > 5000) {
    last_print = millis();
    Serial.printf("[TinyUSB] 挂载状态: %s\n", 
                  TinyUSBDevice.mounted() ? "已挂载" : "未挂载");
  }
  
  delay(100);
}

// 读取回调：当接收到READ10命令时调用
// 将磁盘数据复制到缓冲区（最多bufsize字节）
// 返回复制的字节数（必须是块大小的倍数）
int32_t msc_read_callback(uint32_t lba, void* buffer, uint32_t bufsize) {
  if (lba >= DISK_BLOCK_NUM) {
    Serial.printf("[MSC] 读取错误: LBA %d 超出范围\n", lba);
    return 0;
  }
  
  uint8_t const* addr = msc_disk[lba];
  memcpy(buffer, addr, bufsize);
  
  Serial.printf("[MSC] 读取 LBA %d, 大小 %d 字节\n", lba, bufsize);
  return bufsize;
}

// 写入回调：当接收到WRITE10命令时调用
// 将缓冲区数据写入磁盘
// 返回写入的字节数（必须是块大小的倍数）
int32_t msc_write_callback(uint32_t lba, uint8_t* buffer, uint32_t bufsize) {
  if (lba >= DISK_BLOCK_NUM) {
    Serial.printf("[MSC] 写入错误: LBA %d 超出范围\n", lba);
    return 0;
  }
  
  uint8_t* addr = msc_disk[lba];
  memcpy(addr, buffer, bufsize);
  
  Serial.printf("[MSC] 写入 LBA %d, 大小 %d 字节\n", lba, bufsize);
  return bufsize;
}

// 刷新回调：当接收到FLUSH命令时调用
// 用于刷新任何待处理的缓存
void msc_flush_callback(void) {
  Serial.println("[MSC] 刷新缓存");
  // RAM磁盘不需要特殊的刷新操作
}

// 开始/停止回调：处理电源管理命令
bool msc_start_stop_callback(uint8_t power_condition, bool start, bool load_eject) {
  Serial.printf("[MSC] 开始/停止回调: 电源条件 %u, 开始 %u, 弹出 %u\n", 
                power_condition, start, load_eject);
  return true;
}