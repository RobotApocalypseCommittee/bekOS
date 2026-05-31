
#include "Window.gen.h"


ErrorCode window::WindowServerRaw::dispatch_message(u32 id, ipc::Message& buffer) {
    auto message_id = EXPECTED_TRY((ipc::enum_traits<window::WindowClientRaw::Messages, window::WindowClientRaw::Messages::END_OF_MESSAGES>::to_enum(id)));
    switch (message_id) {
        case window::WindowClientRaw::Messages::CREATE_SURFACE: {
        auto arg_id = EXPECTED_TRY(buffer.decode<u32>());
auto arg_region = EXPECTED_TRY(buffer.decode<OwningBitmap>());
        on_create_surface(arg_id, bek::move(arg_region));
        return ESUCCESS;
        }
case window::WindowClientRaw::Messages::RECONFIGURE_SURFACE: {
        auto arg_id = EXPECTED_TRY(buffer.decode<u32>());
auto arg_size = EXPECTED_TRY(buffer.decode<Vec>());
auto arg_stride = EXPECTED_TRY(buffer.decode<u32>());
        on_reconfigure_surface(arg_id, arg_size, arg_stride);
        return ESUCCESS;
        }
case window::WindowClientRaw::Messages::DESTROY_SURFACE: {
        auto arg_id = EXPECTED_TRY(buffer.decode<u32>());
        on_destroy_surface(arg_id);
        return ESUCCESS;
        }
case window::WindowClientRaw::Messages::CREATE_WINDOW: {
        auto arg_id = EXPECTED_TRY(buffer.decode<u32>());
auto arg_requested_size = EXPECTED_TRY(buffer.decode<Vec>());
        on_create_window(arg_id, arg_requested_size);
        return ESUCCESS;
        }
case window::WindowClientRaw::Messages::DESTROY_WINDOW: {
        auto arg_id = EXPECTED_TRY(buffer.decode<u32>());
        on_destroy_window(arg_id);
        return ESUCCESS;
        }
case window::WindowClientRaw::Messages::FLIP_WINDOW: {
        auto arg_window_id = EXPECTED_TRY(buffer.decode<u32>());
auto arg_surface_id = EXPECTED_TRY(buffer.decode<u32>());
        on_flip_window(arg_window_id, arg_surface_id);
        return ESUCCESS;
        }
case window::WindowClientRaw::Messages::BEGIN_WINDOW_OPERATION: {
        auto arg_window_id = EXPECTED_TRY(buffer.decode<u32>());
auto arg_operation = EXPECTED_TRY(buffer.decode<u32>());
        on_begin_window_operation(arg_window_id, arg_operation);
        return ESUCCESS;
        }
case window::WindowClientRaw::Messages::PING_RESPONSE: {
        
        on_ping_response();
        return ESUCCESS;
        }
        default: return EINVAL;
    }
}

void window::WindowServerRaw::window_state_change(u32 id, Rect size) {
    ipc::Message message{ipc::enum_traits<window::WindowServerRaw::Messages, window::WindowServerRaw::Messages::END_OF_MESSAGES>::from_enum(window::WindowServerRaw::Messages::WINDOW_STATE_CHANGE)};
    message.encode(id);
    message.encode(size);
    send_message(message);
}

void window::WindowServerRaw::focus_change(u32 window_id, u32 focused) {
    ipc::Message message{ipc::enum_traits<window::WindowServerRaw::Messages, window::WindowServerRaw::Messages::END_OF_MESSAGES>::from_enum(window::WindowServerRaw::Messages::FOCUS_CHANGE)};
    message.encode(window_id);
    message.encode(focused);
    send_message(message);
}

void window::WindowServerRaw::mouse_move(u32 window_id, Vec position, u32 buttons) {
    ipc::Message message{ipc::enum_traits<window::WindowServerRaw::Messages, window::WindowServerRaw::Messages::END_OF_MESSAGES>::from_enum(window::WindowServerRaw::Messages::MOUSE_MOVE)};
    message.encode(window_id);
    message.encode(position);
    message.encode(buttons);
    send_message(message);
}

void window::WindowServerRaw::mouse_click(u32 window_id, Vec position, u32 buttons) {
    ipc::Message message{ipc::enum_traits<window::WindowServerRaw::Messages, window::WindowServerRaw::Messages::END_OF_MESSAGES>::from_enum(window::WindowServerRaw::Messages::MOUSE_CLICK)};
    message.encode(window_id);
    message.encode(position);
    message.encode(buttons);
    send_message(message);
}

void window::WindowServerRaw::keydown(u32 window_id, u32 codepoint) {
    ipc::Message message{ipc::enum_traits<window::WindowServerRaw::Messages, window::WindowServerRaw::Messages::END_OF_MESSAGES>::from_enum(window::WindowServerRaw::Messages::KEYDOWN)};
    message.encode(window_id);
    message.encode(codepoint);
    send_message(message);
}

void window::WindowServerRaw::keyup(u32 window_id, u32 codepoint) {
    ipc::Message message{ipc::enum_traits<window::WindowServerRaw::Messages, window::WindowServerRaw::Messages::END_OF_MESSAGES>::from_enum(window::WindowServerRaw::Messages::KEYUP)};
    message.encode(window_id);
    message.encode(codepoint);
    send_message(message);
}

void window::WindowServerRaw::ping() {
    ipc::Message message{ipc::enum_traits<window::WindowServerRaw::Messages, window::WindowServerRaw::Messages::END_OF_MESSAGES>::from_enum(window::WindowServerRaw::Messages::PING)};
    
    send_message(message);
}

void window::WindowServerRaw::error(ErrorCode code) {
    ipc::Message message{ipc::enum_traits<window::WindowServerRaw::Messages, window::WindowServerRaw::Messages::END_OF_MESSAGES>::from_enum(window::WindowServerRaw::Messages::ERROR)};
    message.encode(static_cast<bek::underlying_type<ErrorCode>>(code));
    send_message(message);
}


ErrorCode window::WindowClientRaw::dispatch_message(u32 id, ipc::Message& buffer) {
    auto message_id = EXPECTED_TRY((ipc::enum_traits<window::WindowServerRaw::Messages, window::WindowServerRaw::Messages::END_OF_MESSAGES>::to_enum(id)));
    switch (message_id) {
        case window::WindowServerRaw::Messages::WINDOW_STATE_CHANGE: {
        auto arg_id = EXPECTED_TRY(buffer.decode<u32>());
auto arg_size = EXPECTED_TRY(buffer.decode<Rect>());
        on_window_state_change(arg_id, arg_size);
        return ESUCCESS;
        }
case window::WindowServerRaw::Messages::FOCUS_CHANGE: {
        auto arg_window_id = EXPECTED_TRY(buffer.decode<u32>());
auto arg_focused = EXPECTED_TRY(buffer.decode<u32>());
        on_focus_change(arg_window_id, arg_focused);
        return ESUCCESS;
        }
case window::WindowServerRaw::Messages::MOUSE_MOVE: {
        auto arg_window_id = EXPECTED_TRY(buffer.decode<u32>());
auto arg_position = EXPECTED_TRY(buffer.decode<Vec>());
auto arg_buttons = EXPECTED_TRY(buffer.decode<u32>());
        on_mouse_move(arg_window_id, arg_position, arg_buttons);
        return ESUCCESS;
        }
case window::WindowServerRaw::Messages::MOUSE_CLICK: {
        auto arg_window_id = EXPECTED_TRY(buffer.decode<u32>());
auto arg_position = EXPECTED_TRY(buffer.decode<Vec>());
auto arg_buttons = EXPECTED_TRY(buffer.decode<u32>());
        on_mouse_click(arg_window_id, arg_position, arg_buttons);
        return ESUCCESS;
        }
case window::WindowServerRaw::Messages::KEYDOWN: {
        auto arg_window_id = EXPECTED_TRY(buffer.decode<u32>());
auto arg_codepoint = EXPECTED_TRY(buffer.decode<u32>());
        on_keydown(arg_window_id, arg_codepoint);
        return ESUCCESS;
        }
case window::WindowServerRaw::Messages::KEYUP: {
        auto arg_window_id = EXPECTED_TRY(buffer.decode<u32>());
auto arg_codepoint = EXPECTED_TRY(buffer.decode<u32>());
        on_keyup(arg_window_id, arg_codepoint);
        return ESUCCESS;
        }
case window::WindowServerRaw::Messages::PING: {
        
        on_ping();
        return ESUCCESS;
        }
case window::WindowServerRaw::Messages::ERROR: {
        auto arg_code = static_cast<ErrorCode>(EXPECTED_TRY(buffer.decode<bek::underlying_type<ErrorCode>>()));
        on_error(arg_code);
        return ESUCCESS;
        }
        default: return EINVAL;
    }
}

void window::WindowClientRaw::create_surface(u32 id, const OwningBitmap& region) {
    ipc::Message message{ipc::enum_traits<window::WindowClientRaw::Messages, window::WindowClientRaw::Messages::END_OF_MESSAGES>::from_enum(window::WindowClientRaw::Messages::CREATE_SURFACE)};
    message.encode(id);
    message.encode(region);
    send_message(message);
}

void window::WindowClientRaw::reconfigure_surface(u32 id, Vec size, u32 stride) {
    ipc::Message message{ipc::enum_traits<window::WindowClientRaw::Messages, window::WindowClientRaw::Messages::END_OF_MESSAGES>::from_enum(window::WindowClientRaw::Messages::RECONFIGURE_SURFACE)};
    message.encode(id);
    message.encode(size);
    message.encode(stride);
    send_message(message);
}

void window::WindowClientRaw::destroy_surface(u32 id) {
    ipc::Message message{ipc::enum_traits<window::WindowClientRaw::Messages, window::WindowClientRaw::Messages::END_OF_MESSAGES>::from_enum(window::WindowClientRaw::Messages::DESTROY_SURFACE)};
    message.encode(id);
    send_message(message);
}

void window::WindowClientRaw::create_window(u32 id, Vec requested_size) {
    ipc::Message message{ipc::enum_traits<window::WindowClientRaw::Messages, window::WindowClientRaw::Messages::END_OF_MESSAGES>::from_enum(window::WindowClientRaw::Messages::CREATE_WINDOW)};
    message.encode(id);
    message.encode(requested_size);
    send_message(message);
}

void window::WindowClientRaw::destroy_window(u32 id) {
    ipc::Message message{ipc::enum_traits<window::WindowClientRaw::Messages, window::WindowClientRaw::Messages::END_OF_MESSAGES>::from_enum(window::WindowClientRaw::Messages::DESTROY_WINDOW)};
    message.encode(id);
    send_message(message);
}

void window::WindowClientRaw::flip_window(u32 window_id, u32 surface_id) {
    ipc::Message message{ipc::enum_traits<window::WindowClientRaw::Messages, window::WindowClientRaw::Messages::END_OF_MESSAGES>::from_enum(window::WindowClientRaw::Messages::FLIP_WINDOW)};
    message.encode(window_id);
    message.encode(surface_id);
    send_message(message);
}

void window::WindowClientRaw::begin_window_operation(u32 window_id, u32 operation) {
    ipc::Message message{ipc::enum_traits<window::WindowClientRaw::Messages, window::WindowClientRaw::Messages::END_OF_MESSAGES>::from_enum(window::WindowClientRaw::Messages::BEGIN_WINDOW_OPERATION)};
    message.encode(window_id);
    message.encode(operation);
    send_message(message);
}

void window::WindowClientRaw::ping_response() {
    ipc::Message message{ipc::enum_traits<window::WindowClientRaw::Messages, window::WindowClientRaw::Messages::END_OF_MESSAGES>::from_enum(window::WindowClientRaw::Messages::PING_RESPONSE)};
    
    send_message(message);
}
