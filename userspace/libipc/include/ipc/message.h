// bekOS is a basic OS for the Raspberry Pi
// Copyright (C) 2025 Bekos Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef BEKOS_LIBIPC_MESSAGE_H
#define BEKOS_LIBIPC_MESSAGE_H
#include <bek/span.h>
#include <bek/vector.h>
#include <core/error.h>
#include <api/interlink.h>

namespace ipc {

class MessageBuffer {
public:
  explicit MessageBuffer(uSize size): m_buffer(size) {}
  explicit MessageBuffer(bek::vector<u8> buffer): m_buffer{bek::move(buffer)} {}
  MessageBuffer(u32 message_id, bek::span<u8> data, bek::span<long> fds, bek::span<bek::pair<void*, uSize>> memory_regions);

  const sc::interlink::MessageHeader& header() const { return *reinterpret_cast<const sc::interlink::MessageHeader*>(m_buffer.data()); }
  sc::interlink::MessageHeader& header() { return *reinterpret_cast<sc::interlink::MessageHeader*>(m_buffer.data()); }

  bek::span<u8> span() const { return bek::span{m_buffer}; }
  bek::span<u8> data_at(uSize offset, uSize size) const { return bek::span{m_buffer.data() + offset, m_buffer.data() + offset + size}; }

  bool verify() const;

  bek::span<const sc::interlink::MessageHeader::PayloadItem> payload_items() const;

  void ensure_capacity(uSize capacity);
private:
  bek::vector<u8> m_buffer;
};

class Message;

template <typename T>
struct Serializer;

template <typename T>
concept Serializable = requires (const T& obj, Message& msg)
{
  typename Serializer<T>;
  Serializer<T>::encode(obj, msg);
  { Serializer<T>::decode(msg) } -> bek::same_as<core::expected<T>>;
};

class Message {
public:
  static core::expected<Message> from_buffer(const MessageBuffer& buffer);
  MessageBuffer to_buffer() const;
  explicit Message(u32 message_id): m_message_id(message_id) {}

  template <Serializable T>
  void encode(const T& o) { Serializer<T>::encode(o, *this); }

  template <Serializable T>
  core::expected<T> decode() { return Serializer<T>::decode(*this); }

  void encode_bytes(bek::span<u8> bytes);
  void encode_fd(long fd);
  void encode_memory_region(void* ptr, uSize size);

  void start_decoding();
  core::expected<bek::span<u8>> decode_bytes(uSize length);
  core::expected<long> decode_fd();
  core::expected<bek::pair<void*, uSize>> decode_memory_region();

private:
  Message(u32 message_id, bek::vector<u8> m_buffer, bek::vector<long> m_fds, bek::vector<bek::pair<void*, uSize>> m_memory_regions)
      : m_buffer(bek::move(m_buffer)),
        m_fds(bek::move(m_fds)),
        m_memory_regions(bek::move(m_memory_regions)),
        m_message_id(message_id) {}
  bek::vector<u8> m_buffer;
  bek::vector<long> m_fds;
  bek::vector<bek::pair<void*, uSize>> m_memory_regions;
  uSize m_cur_buffer_position{};
  u32 m_cur_fd_i{};
  u32 m_cur_region_i{};
  u32 m_message_id;
};

template <typename T, T MAX_VALUE>
struct enum_traits {
  static u32 from_enum(T v) {
    return static_cast<__underlying_type(T)>(v);
  }

  static core::expected<T> to_enum(u32 v) {
    if (v < static_cast<__underlying_type(T)>(MAX_VALUE)) {
      return static_cast<T>(v);
    } else {
      return EINVAL;
    }
  }
};


template <typename T>
struct ByteWiseSerializer {
  static void encode(const T& o, Message& msg) {
    auto x = o;
    auto* x_bytes = reinterpret_cast<u8*>(&x);
    msg.encode_bytes({x_bytes, x_bytes + sizeof(T)});
  }

  static core::expected<T> decode(Message& msg) {
    return msg.decode_bytes(sizeof(T)).map_value([](bek::span<u8> m) {
      T o;
      bek::memcopy(&o, m.data(), sizeof(T));
      return o;
    });
  }
};

template <bek::integral T>
struct Serializer<T>: public ByteWiseSerializer<T> {};

}

#endif //BEKOS_LIBIPC_MESSAGE_H
