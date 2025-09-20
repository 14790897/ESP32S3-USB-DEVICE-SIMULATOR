
#include <Arduino.h>
#include "USB.h"
#include "USBMSC.h"

// 使用 ESP32-S3 原生 USB MSC
USBMSC MSC;

// RAM Disk 参数
#define DISK_SECTOR_COUNT 32 
#define DISK_SECTOR_SIZE  512
uint8_t msc_disk[DISK_SECTOR_COUNT][DISK_SECTOR_SIZE];

// MSC 回调函数
static int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {
  if (lba >= DISK_SECTOR_COUNT) {
    return 0;
  }
  memcpy(msc_disk[lba] + offset, buffer, bufsize);
  return bufsize;
}

static int32_t onRead(uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
  if (lba >= DISK_SECTOR_COUNT) {
    return 0;
  }
  memcpy(buffer, msc_disk[lba] + offset, bufsize);
  return bufsize;
}

static bool onStartStop(uint8_t power_condition, bool start, bool load_eject) {
  Serial0.printf("[MSC] Start=%d, Eject=%d\n", start, load_eject);
  return true;
}

// 初始化存储内容
void init_msc_storage() {
  memset(msc_disk, 0, sizeof(msc_disk));
  
  // 创建简单的 FAT12 文件系统头
  // Boot sector (sector 0)
  uint8_t* boot_sector = msc_disk[0];
  
  // FAT12 Boot Sector
  boot_sector[0] = 0xEB; boot_sector[1] = 0x3C; boot_sector[2] = 0x90; // Jump instruction
  memcpy(boot_sector + 3, "MSWIN4.1", 8); // OEM identifier
  
  // BIOS Parameter Block
  boot_sector[11] = DISK_SECTOR_SIZE & 0xFF;        // Bytes per sector (low)
  boot_sector[12] = (DISK_SECTOR_SIZE >> 8) & 0xFF; // Bytes per sector (high)
  boot_sector[13] = 1;                              // Sectors per cluster
  boot_sector[14] = 1;                              // Reserved sectors
  boot_sector[15] = 0;                              // Reserved sectors (high)
  boot_sector[16] = 2;                              // Number of FATs
  boot_sector[17] = 16;                             // Root dir entries (low)
  boot_sector[18] = 0;                              // Root dir entries (high)
  boot_sector[19] = DISK_SECTOR_COUNT & 0xFF;       // Total sectors (low)
  boot_sector[20] = (DISK_SECTOR_COUNT >> 8) & 0xFF; // Total sectors (high)
  boot_sector[21] = 0xF8;                           // Media descriptor
  boot_sector[22] = 1;                              // Sectors per FAT
  boot_sector[23] = 0;                              // Sectors per FAT (high)
  
  // Boot signature
  boot_sector[510] = 0x55;
  boot_sector[511] = 0xAA;
  
  Serial0.println("[MSC] Storage initialized with FAT12");
}

// put function declarations here:
int myFunction(int, int);

void setup() {
  // 初始化串口
  Serial0.begin(115200);
  delay(100);
  Serial0.println("[BOOT] ESP32-S3 USB MSC Demo");

  // 初始化存储
  init_msc_storage();
  
  // 初始化 USB
  USB.begin();
  
  // 初始化 MSC
  MSC.vendorID("ESP32");         // 最多8字符
  MSC.productID("USB_DISK");     // 最多16字符  
  MSC.productRevision("1.0");    // 最多4字符
  MSC.onRead(onRead);
  MSC.onWrite(onWrite);
  MSC.onStartStop(onStartStop);
  
  MSC.mediaPresent(true);
  MSC.begin(DISK_SECTOR_COUNT, DISK_SECTOR_SIZE);
  
  Serial0.println("[MSC] USB Mass Storage initialized");
  Serial0.printf("[MSC] Disk: %d sectors x %d bytes = %d bytes\n", 
                  DISK_SECTOR_COUNT, DISK_SECTOR_SIZE, 
                  DISK_SECTOR_COUNT * DISK_SECTOR_SIZE);
}

void loop() {
  static uint32_t last_print = 0;
  
  if (millis() - last_print > 5000) {
    last_print = millis();
    Serial0.println("[MSC] Running - Check host for USB drive");
  }
  
  delay(100);
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}
