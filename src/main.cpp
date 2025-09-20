/*
   ESP32-S3 USB MSC (Mass Storage Class) Example

   This example creates a USB Mass Storage device that appears as a
   32KB drive to the host computer. It uses ESP32-S3's native USB OTG
   functionality without external libraries.

   Hardware: ESP32-S3 DevKit with USB OTG support

   The drive appears as "ESP32S3_USB" and provides 32KB of storage
   implemented as a RAM disk.
*/

#include "Arduino.h"
#include "USB.h"
#include "USBMSC.h"

USBMSC MSC;

#define DISK_SECTOR_COUNT 64 // 32KB = 64 * 512 bytes
#define DISK_SECTOR_SIZE 512

// 32KB RAM disk
static uint8_t msc_disk[DISK_SECTOR_COUNT][DISK_SECTOR_SIZE];
static bool ejected = false;

// Callback functions for MSC
static int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {
  Serial.printf("MSC WRITE: lba=%u, offset=%u, bufsize=%u\n", lba, offset, bufsize);

  // Bounds checking
  if (lba >= DISK_SECTOR_COUNT) {
    Serial.println("WRITE: LBA out of range");
    return 0;
  }

  // Copy data to RAM disk
  memcpy(msc_disk[lba] + offset, buffer, bufsize);

  return bufsize;
}

static int32_t onRead(uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
  Serial.printf("MSC READ: lba=%u, offset=%u, bufsize=%u\n", lba, offset, bufsize);

  // Bounds checking
  if (lba >= DISK_SECTOR_COUNT) {
    Serial.println("READ: LBA out of range");
    return 0;
  }

  // Copy data from RAM disk
  memcpy(buffer, msc_disk[lba] + offset, bufsize);

  return bufsize;
}

static bool onStartStop(uint8_t power_condition, bool start, bool load_eject) {
  Serial.printf("MSC START/STOP: power=%u, start=%s, eject=%s\n",
                power_condition, start ? "true" : "false", load_eject ? "true" : "false");

  if (load_eject)
  {
    if (start)
    {
      // Loading disk
      ejected = false;
    }
    else
    {
      // Ejecting disk
      ejected = true;
    }
  }

  return true;
}

void setup()
{
  Serial.begin(115200);
  Serial.println("ESP32-S3 USB MSC Example Starting...");

  // Initialize RAM disk to zeros
  memset(msc_disk, 0, sizeof(msc_disk));

  // Create a simple boot sector for FAT12 filesystem recognition
  // This makes the drive more likely to be recognized by operating systems
  uint8_t* boot_sector = msc_disk[0];
  
  // FAT12 Boot Sector
  boot_sector[0] = 0xEB; // Jump instruction
  boot_sector[1] = 0x3C;
  boot_sector[2] = 0x90;

  // OEM Name
  memcpy(&boot_sector[3], "ESP32USB", 8);

  // Bytes per sector (512)
  boot_sector[11] = 0x00;
  boot_sector[12] = 0x02;

  // Sectors per cluster (1)
  boot_sector[13] = 0x01;

  // Reserved sectors (1)
  boot_sector[14] = 0x01;
  boot_sector[15] = 0x00;

  // Number of FATs (2)
  boot_sector[16] = 0x02;

  // Root entries (16)
  boot_sector[17] = 0x10;
  boot_sector[18] = 0x00;

  // Total sectors (64)
  boot_sector[19] = 0x40;
  boot_sector[20] = 0x00;

  // Media descriptor (0xF8 = hard disk)
  boot_sector[21] = 0xF8;

  // Sectors per FAT (1)
  boot_sector[22] = 0x01;
  boot_sector[23] = 0x00;

  // Boot signature
  boot_sector[510] = 0x55;
  boot_sector[511] = 0xAA;

  // Initialize USB and MSC
  MSC.vendorID("ESP32");
  MSC.productID("S3_USB_MSC");
  MSC.productRevision("1.0");
  MSC.onRead(onRead);
  MSC.onWrite(onWrite);
  MSC.onStartStop(onStartStop);
  MSC.mediaPresent(true);
  MSC.begin(DISK_SECTOR_COUNT, DISK_SECTOR_SIZE);

  USB.begin();

  Serial.println("USB MSC initialized successfully!");
  Serial.printf("Drive size: %d KB (%d sectors x %d bytes)\n",
                (DISK_SECTOR_COUNT * DISK_SECTOR_SIZE) / 1024,
                DISK_SECTOR_COUNT, DISK_SECTOR_SIZE);
  Serial.println("Connect USB cable to see drive on your computer");
}

void loop() {
  static unsigned long lastPrint = 0;
  static bool lastEjectState = false;

  // Print status every 5 seconds
  if (millis() - lastPrint > 5000)
  {
    lastPrint = millis();
    Serial.printf("USB MSC Status: Drive: %s\n",
                  ejected ? "Ejected" : "Available");
  }

  // Detect eject/insert state changes
  if (ejected != lastEjectState)
  {
    lastEjectState = ejected;
    if (ejected)
    {
      Serial.println("💾 Drive ejected by host");
    }
    else
    {
      Serial.println("💾 Drive inserted/mounted");
    }
  }

  delay(100);
}