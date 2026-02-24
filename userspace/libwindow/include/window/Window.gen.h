
#ifndef BEKOS_IPC_GEN_WINDOW
#define BEKOS_IPC_GEN_WINDOW
#include <window/core.h>
#include <window/ipc_core.h>
#include <ipc/connection.h>
namespace window {

class WindowServerRaw: public ipc::Connection {
public:
    using Connection::Connection;
    enum class Messages: u32 {
    WINDOW_STATE_CHANGE,
    MOUSE_MOVE,
    MOUSE_CLICK,
    KEYDOWN,
    KEYUP,
    PING,
    ERROR,
    END_OF_MESSAGES
    };
    void window_state_change(u32 id, Rect size);
    void mouse_move(u32 window_id, Vec position, u32 buttons);
    void mouse_click(u32 window_id, Vec position, u32 buttons);
    void keydown(u32 window_id, u32 codepoint);
    void keyup(u32 window_id, u32 codepoint);
    void ping();
    void error(ErrorCode code);
    virtual void on_create_surface(u32 id, OwningBitmap region) = 0;
    virtual void on_reconfigure_surface(u32 id, Vec size, u32 stride) = 0;
    virtual void on_destroy_surface(u32 id) = 0;
    virtual void on_create_window(u32 id, Vec requested_size) = 0;
    virtual void on_destroy_window(u32 id) = 0;
    virtual void on_flip_window(u32 window_id, u32 surface_id) = 0;
    virtual void on_begin_window_operation(u32 window_id, u32 operation) = 0;
    virtual void on_ping_response() = 0;
protected:
    ErrorCode dispatch_message(u32 id, ipc::Message& buffer) override;
};

class WindowClientRaw: public ipc::Connection {
public:
    using Connection::Connection;
    enum class Messages: u32 {
    CREATE_SURFACE,
    RECONFIGURE_SURFACE,
    DESTROY_SURFACE,
    CREATE_WINDOW,
    DESTROY_WINDOW,
    FLIP_WINDOW,
    BEGIN_WINDOW_OPERATION,
    PING_RESPONSE,
    END_OF_MESSAGES
    };
    void create_surface(u32 id, const OwningBitmap& region);
    void reconfigure_surface(u32 id, Vec size, u32 stride);
    void destroy_surface(u32 id);
    void create_window(u32 id, Vec requested_size);
    void destroy_window(u32 id);
    void flip_window(u32 window_id, u32 surface_id);
    void begin_window_operation(u32 window_id, u32 operation);
    void ping_response();
    virtual void on_window_state_change(u32 id, Rect size) = 0;
    virtual void on_mouse_move(u32 window_id, Vec position, u32 buttons) = 0;
    virtual void on_mouse_click(u32 window_id, Vec position, u32 buttons) = 0;
    virtual void on_keydown(u32 window_id, u32 codepoint) = 0;
    virtual void on_keyup(u32 window_id, u32 codepoint) = 0;
    virtual void on_ping() = 0;
    virtual void on_error(ErrorCode code) = 0;
protected:
    ErrorCode dispatch_message(u32 id, ipc::Message& buffer) override;
};

}
#endif
