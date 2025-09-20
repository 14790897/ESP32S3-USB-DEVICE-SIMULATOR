# ESP32-S3 自定义分区表说明

## 分区表文件: partitions_custom.csv

### 总容量: 4MB (0x400000)

| 分区名    | 类型     | 子类型    | 起始地址  | 大小     | 说明                    |
|-----------|----------|-----------|-----------|----------|-------------------------|
| nvs       | data     | nvs       | 0x9000    | 24KB     | 非易失性存储 (WiFi配置等) |
| otadata   | data     | ota       | 0xF000    | 8KB      | OTA数据分区              |
| app0      | app      | ota_0     | 0x20000   | 1.5MB    | 主应用程序分区           |
| app1      | app      | ota_1     | 0x1A0000  | 1.5MB    | OTA备用应用程序分区      |
| spiffs    | data     | spiffs    | 0x320000  | 768KB    | SPIFFS文件系统          |
| coredump  | data     | coredump  | 0x3E0000  | 64KB     | 崩溃转储存储            |
| storage   | data     | fat       | 0x3F0000  | 64KB     | 用户数据存储            |

### 特点优势:

1. **更大的应用空间**: 1.5MB vs 默认1.3MB，适合复杂应用
2. **OTA支持**: 双分区设计，支持无线更新
3. **文件系统**: 768KB SPIFFS空间存储用户文件
4. **调试支持**: 专用coredump分区用于故障分析
5. **用户存储**: 额外的FAT分区用于用户数据

### 使用方法:

在代码中可以这样使用分区：

```cpp
#include "SPIFFS.h"
#include "esp_partition.h"

void setup() {
    // 初始化SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
        return;
    }
    
    // 获取分区信息
    const esp_partition_t* storage_partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, "storage");
    
    if (storage_partition != NULL) {
        Serial.printf("Storage partition size: %d bytes\n", storage_partition->size);
    }
}
```

### 分区表修改指南:

如需修改分区大小，请注意：
1. 所有偏移量必须4KB对齐 (0x1000的倍数)
2. 总大小不能超过4MB (0x400000)
3. app分区必须大于应用程序大小
4. nvs和otadata分区位置固定，不建议修改