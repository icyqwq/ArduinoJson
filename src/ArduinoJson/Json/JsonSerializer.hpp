// ArduinoJson - https://arduinojson.org
// Copyright © 2014-2025, Benoit BLANCHON
// MIT License

#pragma once

#include <ArduinoJson/Json/TextFormatter.hpp>
#include <ArduinoJson/Serialization/measure.hpp>
#include <ArduinoJson/Serialization/serialize.hpp>
#include <ArduinoJson/Variant/VariantDataVisitor.hpp>

ARDUINOJSON_BEGIN_PRIVATE_NAMESPACE

// 在Arduino平台以外使用自定义的base64实现
inline size_t base64_encode_expected_len(size_t length) {
  return ((length + 2) / 3) * 4;
}

inline char base64_encode_byte(uint8_t n) {
  static const char* alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  return alphabet[n & 0x3F];
}

inline void base64_encode_block(const uint8_t* in, size_t len, char* out) {
  size_t i;
  char* p = out;

  // 每次处理3个字节
  for (i = 0; i < len - 2; i += 3) {
    *p++ = base64_encode_byte(in[i] >> 2);
    *p++ = base64_encode_byte(((in[i] & 0x03) << 4) | ((in[i+1] & 0xf0) >> 4));
    *p++ = base64_encode_byte(((in[i+1] & 0x0f) << 2) | ((in[i+2] & 0xc0) >> 6));
    *p++ = base64_encode_byte(in[i+2] & 0x3f);
  }

  // 处理剩余字节
  if (i < len) {
    *p++ = base64_encode_byte(in[i] >> 2);
    if (i + 1 < len) {
      *p++ = base64_encode_byte(((in[i] & 0x03) << 4) | ((in[i+1] & 0xf0) >> 4));
      *p++ = base64_encode_byte((in[i+1] & 0x0f) << 2);
      *p++ = '=';
    } else {
      *p++ = base64_encode_byte(((in[i] & 0x03) << 4));
      *p++ = '=';
      *p++ = '=';
    }
  }
}

template <typename TWriter>
class JsonSerializer : public VariantDataVisitor<size_t> {
 public:
  static const bool producesText = true;

  JsonSerializer(TWriter writer, const ResourceManager* resources)
      : formatter_(writer), resources_(resources) {}

  size_t visit(const ArrayData& array) {
    write('[');

    auto slotId = array.head();

    while (slotId != NULL_SLOT) {
      auto slot = resources_->getVariant(slotId);

      slot->accept(*this, resources_);

      slotId = slot->next();

      if (slotId != NULL_SLOT)
        write(',');
    }

    write(']');
    return bytesWritten();
  }

  size_t visit(const ObjectData& object) {
    write('{');

    auto slotId = object.head();

    bool isKey = true;

    while (slotId != NULL_SLOT) {
      auto slot = resources_->getVariant(slotId);
      slot->accept(*this, resources_);

      slotId = slot->next();

      if (slotId != NULL_SLOT)
        write(isKey ? ':' : ',');

      isKey = !isKey;
    }

    write('}');
    return bytesWritten();
  }

  template <typename T>
  enable_if_t<is_floating_point<T>::value, size_t> visit(T value) {
    formatter_.writeFloat(value);
    return bytesWritten();
  }

  size_t visit(const char* value) {
    formatter_.writeString(value);
    return bytesWritten();
  }

  size_t visit(JsonString value) {
    formatter_.writeString(value.c_str(), value.size());
    return bytesWritten();
  }

  size_t visit(RawString value) {
    formatter_.writeRaw(value.data(), value.size());
    return bytesWritten();
  }

  size_t visit(const BinaryItem& binary) {
    write('"');
    
    const uint8_t* bytes = static_cast<const uint8_t*>(binary.data);
    size_t size = binary.size;
    
    // 使用固定大小的缓冲区分块处理，避免大型二进制数据内存溢出
    // 注意：由于base64编码每3个字节输入生成4个字节输出，我们需要确保每个块是3的倍数
    const size_t CHUNK_SIZE = 1020; // 等于 340 * 3，确保是3的倍数
    // 缓冲区大小：编码后的数据大小加上额外的填充空间
    const size_t BUFFER_SIZE = (CHUNK_SIZE * 4 / 3) + 4; // 约1360字节
    
    char buffer[BUFFER_SIZE];
    
    // 分块处理数据
    size_t bytesLeft = size;
    const uint8_t* currentPos = bytes;
    
    while (bytesLeft > 0) {
      // 确定当前块的大小，确保是3的倍数（base64的输入块大小）
      // 除非这是最后一块
      size_t currentChunkSize;
      if (bytesLeft <= CHUNK_SIZE) {
        // 最后一块，直接处理所有剩余数据
        currentChunkSize = bytesLeft;
      } else {
        // 不是最后一块，处理3的倍数大小的数据
        currentChunkSize = CHUNK_SIZE;
      }
      
      // 计算当前块编码后的大小
      size_t encodedSize = base64_encode_expected_len(currentChunkSize);
      
      // 对当前块进行base64编码
      base64_encode_block(currentPos, currentChunkSize, buffer);
      
      // 写入编码后的数据
      formatter_.writeRaw(buffer, encodedSize);
      
      // 更新指针和剩余字节数
      currentPos += currentChunkSize;
      bytesLeft -= currentChunkSize;
    }
    
    write('"');
    return bytesWritten();
  }

  size_t visit(JsonInteger value) {
    formatter_.writeInteger(value);
    return bytesWritten();
  }

  size_t visit(JsonUInt value) {
    formatter_.writeInteger(value);
    return bytesWritten();
  }

  size_t visit(bool value) {
    formatter_.writeBoolean(value);
    return bytesWritten();
  }

  size_t visit(nullptr_t) {
    formatter_.writeRaw("null");
    return bytesWritten();
  }

 protected:
  size_t bytesWritten() const {
    return formatter_.bytesWritten();
  }

  void write(char c) {
    formatter_.writeRaw(c);
  }

  void write(const char* s) {
    formatter_.writeRaw(s);
  }

 private:
  TextFormatter<TWriter> formatter_;

 protected:
  const ResourceManager* resources_;
};

ARDUINOJSON_END_PRIVATE_NAMESPACE

ARDUINOJSON_BEGIN_PUBLIC_NAMESPACE

// Produces a minified JSON document.
// https://arduinojson.org/v7/api/json/serializejson/
template <
    typename TDestination,
    detail::enable_if_t<!detail::is_pointer<TDestination>::value, int> = 0>
size_t serializeJson(JsonVariantConst source, TDestination& destination) {
  using namespace detail;
  return serialize<JsonSerializer>(source, destination);
}

// Produces a minified JSON document.
// https://arduinojson.org/v7/api/json/serializejson/
inline size_t serializeJson(JsonVariantConst source, void* buffer,
                            size_t bufferSize) {
  using namespace detail;
  return serialize<JsonSerializer>(source, buffer, bufferSize);
}

// Computes the length of the document that serializeJson() produces.
// https://arduinojson.org/v7/api/json/measurejson/
inline size_t measureJson(JsonVariantConst source) {
  using namespace detail;
  return measure<JsonSerializer>(source);
}

#if ARDUINOJSON_ENABLE_STD_STREAM
template <typename T,
          detail::enable_if_t<
              detail::is_convertible<T, JsonVariantConst>::value, int> = 0>
inline std::ostream& operator<<(std::ostream& os, const T& source) {
  serializeJson(source, os);
  return os;
}
#endif

ARDUINOJSON_END_PUBLIC_NAMESPACE
