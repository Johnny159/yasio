//////////////////////////////////////////////////////////////////////////////////////////
// A cross platform socket APIs, support ios & android & wp8 & window store
// universal app version: 3.9.6
//////////////////////////////////////////////////////////////////////////////////////////
/*
The MIT License (MIT)

Copyright (c) 2012-2019 halx99

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include "ibinarystream.h"
#include "lyasio.h"
#include "yasio.h"
#include "obinarystream.h"
#include "sol.hpp"

#if _HAS_CXX17

namespace lyasio
{
class ibstream : public ibinarystream
{
public:
  ibstream(std::vector<char> blob) : ibinarystream(), blob_(std::move(blob))
  {
    this->assign(blob_.data(), blob_.size());
  }
  ibstream(const obinarystream *obs) : ibinarystream(), blob_(obs->buffer())
  {
    this->assign(blob_.data(), blob_.size());
  }

private:
  std::vector<char> blob_;
};
} // namespace lyasio

extern "C" {

YASIO_API int luaopen_yasio(lua_State *L)
{
  using namespace purelib::inet;
  sol::state_view sol2(L);

#  if 0
    auto t = sol2.create_named_table("simple_timer");
    // the simple timer implementation is here: https://github.com/halx99/x-studio365/blob/master/cocos2d-x-patch/cocos/editor-support/cocostudio/ext/SimpleTimer.h
    t.set_function("delay", simple_timer::delay);
    t.set_function("loop", simple_timer::loop);
    t.set_function("kill", simple_timer::kill);
#  endif
  auto yasio = sol2.create_named_table("yasio");

  yasio.new_usertype<io_hostent>(
      "io_hostent", sol::constructors<io_hostent(), io_hostent(const std::string &, u_short)>(),
      "host", &io_hostent::host_, "port", &io_hostent::port_);

  yasio.new_usertype<io_event>(
      "io_event", "channel_index", &io_event::channel_index, "kind", &io_event::type, "status",
      &io_event::status, "transport", &io_event::transport, "take_packet", [](io_event *event) {
        return std::unique_ptr<lyasio::ibstream>(new lyasio::ibstream(event->take_packet()));
      });

  yasio.new_usertype<io_service>(
      "io_service", "start_service",
      sol::overload(
          static_cast<void (io_service::*)(std::vector<io_hostent>, io_event_callback_t)>(
              &io_service::start_service),
          static_cast<void (io_service::*)(const io_hostent *channel_eps, io_event_callback_t cb)>(
              &io_service::start_service)),
      "stop_service", &io_service::stop_service, "set_option",
      [](io_service *service, int opt, sol::variadic_args va) {
        switch (opt)
        {
          case YASIO_OPT_TCP_KEEPALIVE:
            service->set_option(opt, static_cast<int>(va[0]), static_cast<int>(va[1]),
                                static_cast<int>(va[2]));
            break;
          case YASIO_OPT_LFIB_PARAMS:
            service->set_option(opt, static_cast<int>(va[0]), static_cast<int>(va[1]),
                                static_cast<int>(va[2]), static_cast<int>(va[3]));
            break;
          case YASIO_OPT_RESOLV_FUNCTION: // lua does not support set custom
                                          // resolv function
            break;
          case YASIO_OPT_IO_EVENT_CALLBACK:
            (void)0;
            {
              sol::function fn           = va[0];
              io_event_callback_t fnwrap = [=](event_ptr e) mutable -> void { fn(std::move(e)); };
              service->set_option(opt, std::addressof(fnwrap));
            }
            break;
          default:
            service->set_option(opt, static_cast<int>(va[0]));
        }
      },
      "dispatch_events", &io_service::dispatch_events, "open", &io_service::open, "write",
      sol::overload(
          [](io_service *service, transport_ptr transport, std::string_view s) {
            service->write(transport, std::vector<char>(s.data(), s.data() + s.length()));
          },
          [](io_service *service, transport_ptr transport, obinarystream *obs) {
            service->write(transport, obs->take_buffer());
          }));

  // ##-- obstream
  yasio.new_usertype<obinarystream>(
      "obstream", "push32", &obinarystream::push32, "pop32",
      sol::overload(static_cast<void (obinarystream ::*)()>(&obinarystream::pop32),
                    static_cast<void (obinarystream ::*)(uint32_t)>(&obinarystream::pop32)),
      "push24", &obinarystream::push24, "pop24",
      sol::overload(static_cast<void (obinarystream ::*)()>(&obinarystream::pop24),
                    static_cast<void (obinarystream ::*)(uint32_t)>(&obinarystream::pop24)),
      "push16", &obinarystream::push16, "pop16",
      sol::overload(static_cast<void (obinarystream ::*)()>(&obinarystream::pop16),
                    static_cast<void (obinarystream ::*)(uint16_t)>(&obinarystream::pop16)),
      "push8", &obinarystream::push8, "pop8",
      sol::overload(static_cast<void (obinarystream ::*)()>(&obinarystream::pop8),
                    static_cast<void (obinarystream ::*)(uint8_t)>(&obinarystream::pop8)),
      "write_bool", &obinarystream::write_i<bool>, "write_i8", &obinarystream::write_i<int8_t>,
      "write_i16", &obinarystream::write_i<int16_t>, "write_i24", &obinarystream::write_i24,
      "write_i32", &obinarystream::write_i<int32_t>, "write_i64", &obinarystream::write_i<int64_t>,
      "write_u8", &obinarystream::write_i<uint8_t>, "write_u16", &obinarystream::write_i<uint16_t>,
      "write_u32", &obinarystream::write_i<uint32_t>, "write_u64",
      &obinarystream::write_i<uint64_t>, "write_f", &obinarystream::write_i<float>, "write_lf",
      &obinarystream::write_i<double>,

      "write_string",
      static_cast<size_t (obinarystream::*)(std::string_view)>(&obinarystream::write_v), "length",
      &obinarystream::length, "to_string",
      [](obinarystream *obs) { return std::string_view(obs->data(), obs->length()); });

  // ##-- lyasio::ibstream
  yasio.new_usertype<lyasio::ibstream>(
      "lyasio::ibstream",
      sol::constructors<lyasio::ibstream(std::vector<char>),
                        lyasio::ibstream(const obinarystream *)>(),
      "read_bool", &lyasio::ibstream::read_ix<bool>, "read_i8", &lyasio::ibstream::read_ix<int8_t>,
      "read_i16", &lyasio::ibstream::read_ix<int16_t>, "read_i24", &lyasio::ibstream::read_i24,
      "read_i32", &lyasio::ibstream::read_ix<int32_t>, "read_i64",
      &lyasio::ibstream::read_ix<int64_t>, "read_u8", &lyasio::ibstream::read_ix<uint8_t>,
      "read_u16", &lyasio::ibstream::read_ix<uint16_t>, "read_u24", &lyasio::ibstream::read_u24,
      "read_u32", &lyasio::ibstream::read_ix<uint32_t>, "read_u64",
      &lyasio::ibstream::read_ix<uint64_t>, "read_f", &lyasio::ibstream::read_ix<float>, "read_lf",
      &lyasio::ibstream::read_ix<double>, "read_string",
      static_cast<std::string_view (lyasio::ibstream::*)()>(&lyasio::ibstream::read_v), "to_string",
      [](lyasio::ibstream *ibs) { return std::string_view(ibs->data(), ibs->size()); });

  // ##-- yasio enums
  yasio["CHANNEL_TCP_CLIENT"]           = channel_type::CHANNEL_TCP_CLIENT;
  yasio["CHANNEL_TCP_SERVER"]           = channel_type::CHANNEL_TCP_SERVER;
  yasio["YASIO_OPT_CONNECT_TIMEOUT"]    = YASIO_OPT_CONNECT_TIMEOUT;
  yasio["YASIO_OPT_SEND_TIMEOUT"]       = YASIO_OPT_CONNECT_TIMEOUT;
  yasio["YASIO_OPT_RECONNECT_TIMEOUT"]  = YASIO_OPT_RECONNECT_TIMEOUT;
  yasio["YASIO_OPT_DNS_CACHE_TIMEOUT"]  = YASIO_OPT_DNS_CACHE_TIMEOUT;
  yasio["YASIO_OPT_DEFER_EVENT"]        = YASIO_OPT_DEFER_EVENT;
  yasio["YASIO_OPT_TCP_KEEPALIVE"]      = YASIO_OPT_TCP_KEEPALIVE;
  yasio["YASIO_OPT_RESOLV_FUNCTION"]    = YASIO_OPT_RESOLV_FUNCTION;
  yasio["YASIO_OPT_LOG_FILE"]           = YASIO_OPT_LOG_FILE;
  yasio["YASIO_OPT_LFIB_PARAMS"]        = YASIO_OPT_LFIB_PARAMS;
  yasio["YASIO_OPT_IO_EVENT_CALLBACK"]  = YASIO_OPT_IO_EVENT_CALLBACK;
  yasio["YASIO_EVENT_CONNECT_RESPONSE"] = YASIO_EVENT_CONNECT_RESPONSE;
  yasio["YASIO_EVENT_CONNECTION_LOST"]  = YASIO_EVENT_CONNECTION_LOST;
  yasio["YASIO_EVENT_RECV_PACKET"]      = YASIO_EVENT_RECV_PACKET;

  return yasio.push(); /* return 'yasio' table */
}

} /* extern "C" */

#endif
