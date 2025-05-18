#include <ArduinoJson.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <cstdint>
#include <cstdio>

using namespace ArduinoJson;

int main() {
  // 生成大量测试数据（约10KB）
  const size_t dataSize = 10 * 1024;
  std::vector<uint8_t> binaryData(dataSize);
  
  // 填充测试数据
  for (size_t i = 0; i < dataSize; i++) {
    binaryData[i] = static_cast<uint8_t>(i % 256);
  }
  
  // 创建一个文档
  JsonDocument doc;
  
  // 设置一个普通键值对
  doc["name"] = "Large Binary Test";
  
  // 使用二进制引用来设置大型二进制数据
  MsgPackBinary binValue(binaryData.data(), binaryData.size());
  doc["binary"] = binValue;
  
  // 序列化为JSON，二进制数据会被编码为base64
  // 由于数据很大，我们不能用一个简单的buffer，需要流式处理
  size_t jsonSize = measureJson(doc);
  printf("估计JSON大小：%zu 字节\n", jsonSize);
  
  // 创建足够大的缓冲区来存储JSON
  std::vector<char> jsonBuffer(jsonSize + 1); // +1 为了null终止符
  
  // 序列化到缓冲区
  size_t actualSize = serializeJson(doc, jsonBuffer.data(), jsonBuffer.size());
  
  printf("实际JSON大小：%zu 字节\n", actualSize);
  
  // 输出JSON的前100个字符和最后100个字符
  printf("JSON前100字符：\n");
  if (actualSize > 100) {
    jsonBuffer[100] = '\0';
    printf("%s...\n", jsonBuffer.data());
  } else {
    printf("%s\n", jsonBuffer.data());
  }
  
  if (actualSize > 200) {
    printf("JSON最后100字符：\n");
    printf("...%s\n", jsonBuffer.data() + actualSize - 100);
  }
  
  // 解析回来测试
  JsonDocument docParsed;
  DeserializationError error = deserializeJson(docParsed, jsonBuffer.data());
  
  // 检查解析是否成功
  if (error) {
    printf("解析回JSON失败: %s\n", error.c_str());
    return 1;
  }
  
  // 验证name字段
  const char* name = docParsed["name"];
  printf("name = %s\n", name);
  
  // 验证JSON数据中是否包含binary字段（作为base64编码）
  if (docParsed.containsKey("binary")) {
    printf("包含binary字段，验证成功！\n");
  } else {
    printf("错误：找不到binary字段\n");
  }
  
  return 0;
} 