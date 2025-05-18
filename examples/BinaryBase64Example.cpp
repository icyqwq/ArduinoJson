#include <ArduinoJson.h>
#include <iostream>
#include <string>
#include <stdint.h>
#include <stdio.h>

using namespace ArduinoJson;

int main() {
  // 创建一些二进制数据
  uint8_t binaryData[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0xFF, 0xAA, 0xBB, 0xCC, 0xDD};
  
  // 创建一个文档
  JsonDocument doc;
  
  // 模拟MsgPack二进制数据
  uint8_t msgpack[] = {
    0x82,              // map with 2 entries
    0xA3, 'k', 'e', 'y', // key = "key" (string)
    0xA5, 'v', 'a', 'l', 'u', 'e', // value = "value" (string)
    0xA4, 'd', 'a', 't', 'a',   // key = "data" (string)
    0xC4, 0x0A, 0x01, 0x02, 0x03, 0x04, 0x05, 0xFF, 0xAA, 0xBB, 0xCC, 0xDD  // binary data (10 bytes)
  };
  
  // 从MsgPack数据中解析，这会使用我们的优化，对二进制数据直接存储引用
  DeserializationError error = deserializeMsgPack(doc, msgpack);
  
  // 检查解析是否成功
  if (error) {
    printf("解析失败: %s\n", error.c_str());
    return 1;
  }
  
  // 访问文档中的内容
  const char* key = doc["key"];
  printf("key = %s\n", key);
  
  // 获取二进制数据并使用MsgPackBinary类型访问
  MsgPackBinary binary = doc["data"].as<MsgPackBinary>();
  
  // 确认我们获取到了正确的二进制数据
  const uint8_t* data = static_cast<const uint8_t*>(binary.data());
  size_t size = binary.size();
  
  printf("二进制数据大小: %zu 字节\n", size);
  printf("二进制数据内容（十六进制）: ");
  for (size_t i = 0; i < size; i++) {
    printf("%02X ", data[i]);
  }
  printf("\n");
  
  // 将数据序列化为JSON，现在应该以Base64格式输出
  char buffer[256];
  size_t length = serializeJson(doc, buffer, sizeof(buffer));
  
  printf("JSON (minified, Base64): %s\n", buffer);
  
  // 同样也测试美化版本
  length = serializeJsonPretty(doc, buffer, sizeof(buffer));
  printf("JSON (prettified, Base64):\n%s\n", buffer);
  
  return 0;
} 