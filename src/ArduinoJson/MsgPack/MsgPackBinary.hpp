#pragma once

#include <ArduinoJson/Variant/Converter.hpp>
#include <ArduinoJson/Variant/VariantContent.hpp>

ARDUINOJSON_BEGIN_PUBLIC_NAMESPACE

class MsgPackBinary {
 public:
  MsgPackBinary() : data_(nullptr), size_(0) {}
  explicit MsgPackBinary(const void* c, size_t size) : data_(c), size_(size) {}

  const void* data() const {
    return data_;
  }

  size_t size() const {
    return size_;
  }

 private:
  const void* data_;
  size_t size_;
};

template <>
struct Converter<MsgPackBinary> : private detail::VariantAttorney {
  static void toJson(MsgPackBinary src, JsonVariant dst) {
    auto data = VariantAttorney::getData(dst);
    if (!data)
      return;
    auto resources = getResourceManager(dst);
    data->clear(resources);
    if (src.data()) {
      // 首先尝试直接存储为二进制引用
      data->setLinkedBinary(src.data(), src.size());
      return;
    }
  }

  static MsgPackBinary fromJson(JsonVariantConst src) {
    auto data = getData(src);
    if (!data)
      return {};

    // 先检查是否是LinkedBinary类型
    auto binaryItem = data->asBinary();
    if (binaryItem.data != nullptr) {
      return MsgPackBinary(binaryItem.data, binaryItem.size);
    }

    // 再检查是否是RawString类型（为了兼容性）
    auto rawstr = data->asRawString();
    auto p = reinterpret_cast<const uint8_t*>(rawstr.c_str());
    auto n = rawstr.size();
    if (n >= 2 && p[0] == 0xc4) {  // bin 8
      size_t size = p[1];
      if (size + 2 == n)
        return MsgPackBinary(p + 2, size);
    } else if (n >= 3 && p[0] == 0xc5) {  // bin 16
      size_t size = size_t(p[1] << 8) | p[2];
      if (size + 3 == n)
        return MsgPackBinary(p + 3, size);
    } else if (n >= 5 && p[0] == 0xc6) {  // bin 32
      size_t size =
          size_t(p[1] << 24) | size_t(p[2] << 16) | size_t(p[3] << 8) | p[4];
      if (size + 5 == n)
        return MsgPackBinary(p + 5, size);
    }
    return {};
  }

  static bool checkJson(JsonVariantConst src) {
    auto data = getData(src);
    if (!data)
      return false;
    
    // 检查是否是二进制类型
    if (data->type() == detail::VariantType::LinkedBinary)
      return true;
    
    // 兼容性检查：是否是以二进制头开始的RawString
    return fromJson(src).data() != nullptr;
  }
};

ARDUINOJSON_END_PUBLIC_NAMESPACE
