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


#ifndef BEKOS_LIBWINDOW_WINDOWSERVER_CLIENT_H
#define BEKOS_LIBWINDOW_WINDOWSERVER_CLIENT_H

#include <core/error.h>

#include "gfx.h"

#include "Window.gen.h"

namespace window {
class WSClient {
public:

  static core::expected<WSClient> connect();
  void poll();

  virtual void on_global_configure(u32 recommended_width, u32 recommended_height) = 0;


  void create_window(u32 window::Bitmap);
  void notify_window_drag(u32 window_id, Vec diff);
  void notify_window_action(u32 window_id, )




  WSClient(const WSClient&) = delete;
  WSClient& operator=(const WSClient&) = delete;
  WSClient(WSClient&&) = default;
  WSClient& operator=(WSClient&&) = default;
private:
  explicit WSClient(long connection_ed);
  long m_connection_ed;
};
}

#endif //BEKOS_LIBWINDOW_WINDOWSERVER_CLIENT_H
