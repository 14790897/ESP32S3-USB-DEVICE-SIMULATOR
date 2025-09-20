#include "Adafruit_TinyUSB.h"

// MSC 配置
#define DISK_BLOCK_NUM  64   // 32KB RAM磁盘
#define DISK_BLOCK_SIZE 512

// RAM磁盘存储
uint8_t msc_disk[DISK_BLOCK_NUM][DISK_BLOCK_SIZE];

Adafruit_USBD_MSC usb_msc;

// 前置声明
int32_t msc_read_callback(uint32_t lba, void* buffer, uint32_t bufsize);
int32_t msc_write_callback(uint32_t lba, uint8_t* buffer, uint32_t bufsize);
void msc_flush_callback(void);
bool msc_start_stop_callback(uint8_t power_condition, bool start, bool load_eject);
void create_fat12_filesystem(void);

void setup() {
  // 初始化 TinyUSB
  if (!TinyUSBDevice.isInitialized()) {
    TinyUSBDevice.begin(0);
  }
  
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=== TinyUSB MSC 高级版本 ===");
  Serial.println("带FAT12文件系统的虚拟U盘");
  
  // 创建FAT12文件系统
  create_fat12_filesystem();
  
  // 配置MSC设备
  usb_msc.setID("ESP32-S3", "TinyUSB Drive", "2.0");
  usb_msc.setCapacity(DISK_BLOCK_NUM, DISK_BLOCK_SIZE);
  usb_msc.setReadWriteCallback(msc_read_callback, msc_write_callback, msc_flush_callback);
  usb_msc.setStartStopCallback(msc_start_stop_callback);
  usb_msc.setUnitReady(true);
  usb_msc.begin();
  
  // 重新枚举USB设备
  if (TinyUSBDevice.mounted()) {
    TinyUSBDevice.detach();
    delay(10);
    TinyUSBDevice.attach();
  }
  
  Serial.println("✅ TinyUSB MSC 初始化完成");
  Serial.printf("📀 磁盘容量: %d KB (%d blocks × %d bytes)\n", 
                (DISK_BLOCK_NUM * DISK_BLOCK_SIZE) / 1024,
                DISK_BLOCK_NUM, DISK_BLOCK_SIZE);
}

void loop() {
  #ifdef TINYUSB_NEED_POLLING_TASK
  TinyUSBDevice.task();
  #endif
  
  static uint32_t last_status = 0;
  if (millis() - last_status > 3000) {
    last_status = millis();
    
    if (TinyUSBDevice.mounted()) {
      Serial.println("🔗 USB已连接 - 设备可作为U盘访问");
    } else {
      Serial.println("❌ USB未连接 - 等待主机连接...");
    }
  }
  
  delay(100);
}

// 创建简单的FAT12文件系统
void create_fat12_filesystem(void) {
  Serial.println("📝 创建FAT12文件系统...");
  
  // 清零整个磁盘
  memset(msc_disk, 0, sizeof(msc_disk));
  
  // === 引导扇区 (扇区 0) ===
  uint8_t* boot_sector = msc_disk[0];
  
  // 跳转指令
  boot_sector[0] = 0xEB;
  boot_sector[1] = 0x3C;
  boot_sector[2] = 0x90;
  
  // OEM标识
  memcpy(boot_sector + 3, "ESP32S3 ", 8);
  
  // BIOS参数块 (BPB)
  boot_sector[11] = DISK_BLOCK_SIZE & 0xFF;         // 每扇区字节数 (低字节)
  boot_sector[12] = (DISK_BLOCK_SIZE >> 8) & 0xFF;  // 每扇区字节数 (高字节)
  boot_sector[13] = 1;                              // 每簇扇区数
  boot_sector[14] = 1;                              // 保留扇区数 (低字节)
  boot_sector[15] = 0;                              // 保留扇区数 (高字节)
  boot_sector[16] = 2;                              // FAT表数量
  boot_sector[17] = 16;                             // 根目录条目数 (低字节)
  boot_sector[18] = 0;                              // 根目录条目数 (高字节)
  boot_sector[19] = DISK_BLOCK_NUM & 0xFF;          // 总扇区数 (低字节)
  boot_sector[20] = (DISK_BLOCK_NUM >> 8) & 0xFF;   // 总扇区数 (高字节)
  boot_sector[21] = 0xF8;                           // 媒体描述符
  boot_sector[22] = 1;                              // 每FAT扇区数 (低字节)
  boot_sector[23] = 0;                              // 每FAT扇区数 (高字节)
  boot_sector[24] = 1;                              // 每磁道扇区数
  boot_sector[25] = 0;
  boot_sector[26] = 1;                              // 磁头数
  boot_sector[27] = 0;
  boot_sector[28] = 0;                              // 隐藏扇区数
  boot_sector[29] = 0;
  boot_sector[30] = 0;
  boot_sector[31] = 0;
  
  // 引导签名
  boot_sector[510] = 0x55;
  boot_sector[511] = 0xAA;
  
  // === FAT表 (扇区 1-2) ===
  uint8_t* fat1 = msc_disk[1];
  uint8_t* fat2 = msc_disk[2];
  
  // FAT12格式的前几个条目
  fat1[0] = 0xF8;  fat1[1] = 0xFF;  fat1[2] = 0xFF;  // 媒体描述符 + EOF
  fat2[0] = 0xF8;  fat2[1] = 0xFF;  fat2[2] = 0xFF;  // 备份FAT
  
  // === 根目录 (扇区 3) ===
  uint8_t* root_dir = msc_disk[3];
  
  // 卷标条目 (11字节文件名，全大写，属性0x08)
  memcpy(root_dir, "ESP32-S3   ", 11);  // 注意：总共11个字符，用空格填充
  root_dir[11] = 0x08;  // 卷标属性
  
  // README.TXT 文件条目 (从偏移32开始，每个条目32字节)
  uint8_t* readme_entry = root_dir + 32;
  memcpy(readme_entry, "README  TXT", 11);  // 8.3格式文件名
  readme_entry[11] = 0x20;  // 普通文件属性
  readme_entry[26] = 4;     // 起始簇号 (低字节)
  readme_entry[27] = 0;     // 起始簇号 (高字节)
  
  // 文件大小 (假设100字节)
  const uint32_t file_size = 100;
  readme_entry[28] = file_size & 0xFF;
  readme_entry[29] = (file_size >> 8) & 0xFF;
  readme_entry[30] = (file_size >> 16) & 0xFF;
  readme_entry[31] = (file_size >> 24) & 0xFF;
  
  // === 数据区 - README.TXT 内容 (扇区 4) ===
  uint8_t* data_sector = msc_disk[4];
  const char* readme_content = 
    "ESP32-S3 TinyUSB Virtual Drive\r\n"
    "===============================\r\n"
    "\r\n"
    "This is a virtual USB drive implemented using TinyUSB library.\r\n"
    "\r\n"
    "Features:\r\n"
    "- 32KB RAM disk\r\n"
    "- FAT12 file system\r\n"
    "- Cross-platform compatibility\r\n"
    "\r\n"
    "Powered by ESP32-S3 & TinyUSB\r\n";
  
  strcpy((char*)data_sector, readme_content);
  
  Serial.println("✅ FAT12文件系统创建完成");
  Serial.println("📁 包含文件:");
  Serial.println("   - README.TXT (欢迎文件)");
}

// MSC读取回调
int32_t msc_read_callback(uint32_t lba, void* buffer, uint32_t bufsize) {
  if (lba >= DISK_BLOCK_NUM) {
    Serial.printf("❌ 读取错误: LBA %d 超出范围 (最大: %d)\n", lba, DISK_BLOCK_NUM-1);
    return 0;
  }
  
  memcpy(buffer, msc_disk[lba], bufsize);
  Serial.printf("📖 读取 LBA:%d 大小:%d字节\n", lba, bufsize);
  return bufsize;
}

// MSC写入回调
int32_t msc_write_callback(uint32_t lba, uint8_t* buffer, uint32_t bufsize) {
  if (lba >= DISK_BLOCK_NUM) {
    Serial.printf("❌ 写入错误: LBA %d 超出范围 (最大: %d)\n", lba, DISK_BLOCK_NUM-1);
    return 0;
  }
  
  memcpy(msc_disk[lba], buffer, bufsize);
  Serial.printf("💾 写入 LBA:%d 大小:%d字节\n", lba, bufsize);
  return bufsize;
}

// MSC刷新回调
void msc_flush_callback(void) {
  Serial.println("🔄 刷新缓存");
}

// MSC开始/停止回调
bool msc_start_stop_callback(uint8_t power_condition, bool start, bool load_eject) {
  Serial.printf("⚡ 电源管理: 条件:%u 开始:%u 弹出:%u\n", 
                power_condition, start, load_eject);
  return true;
}