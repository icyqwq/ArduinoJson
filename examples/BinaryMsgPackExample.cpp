#include <ArduinoJson.h>
#include <vector>
#include <iostream>
#include <string>

using namespace ArduinoJson;

// 为了解决#include错误，我们直接包含一些基本头文件
#include <stdint.h>
#include <stdio.h>

int main() {
  // 创建一些二进制数据
  uint8_t binaryData[] = {0x01, 0x02, 0x03, 0x04, 0x05};
  
  // 创建一个文档
  JsonDocument doc;
  
  // 模拟MsgPack二进制数据
  uint8_t msgpack[] = {
    0x82,              // map with 2 entries
    0xA3, 'k', 'e', 'y', // key = "key" (string)
    0xA5, 'v', 'a', 'l', 'u', 'e', // value = "value" (string)
    0xA4, 'd', 'a', 't', 'a',   // key = "data" (string)
    0xC4, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05  // value = binary data (5 bytes)
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
  printf("二进制数据内容: ");
  for (size_t i = 0; i < size; i++) {
    printf("%02X ", data[i]);
  }
  printf("\n");
  
  // 创建一个新文档，并设置一些值，包括二进制数据
  JsonDocument newDoc;
  newDoc["text"] = "Hello";
  newDoc["number"] = 42;
  
  // 使用二进制引用来设置值
  MsgPackBinary binValue(binaryData, sizeof(binaryData));
  newDoc["binary"] = binValue;
  
  // 序列化为JSON格式（二进制数据会转为数组）
  char jsonOutput[256];
  size_t jsonLen = serializeJson(newDoc, jsonOutput, sizeof(jsonOutput));
  printf("JSON输出 (%zu 字节): %s\n", jsonLen, jsonOutput);
  
  // 序列化为MsgPack格式
  uint8_t msgpackOutput[100];
  size_t msgpackLen = serializeMsgPack(newDoc, msgpackOutput, sizeof(msgpackOutput));
  
  printf("MsgPack输出 (%zu 字节):", msgpackLen);
  for (size_t i = 0; i < msgpackLen; i++) {
    if (i % 16 == 0) printf("\n");
    printf("%02X ", msgpackOutput[i]);
  }
  printf("\n");
  
  return 0;
} 