/**
 * A global, single-client TCP server implementation
 *
 * BOTWTOOLKIT_TCP_SEND must be defined to enable this feature
 * (otherwise the functions are just stubs)
 */
#pragma once

#include <cstdint>

namespace botw::tcp {

enum class ServerStatus {
    Ready = 0,
    NotInit = 1,
    InitFail_CreateThread,
    ReadyToInit,
    NifmInitFail,
    SocketInitFail,
    WaitingForNetworkRequest,
    NetworkNotAvailable,
    SocketCreateFail,
    BindFail,
    ListenFail,
    WaitingForClient,
    AcceptFail,
};

/**
 * Get the current server status
 */
ServerStatus get_status();

/**
 * Initialize the components
 *
 * This patches code, so need to be run in module's main call stack
 */
void init();

/**
 * Start the server on the specified port.
 *
 * Call when you know socket can be safely created
 */
void start_server(uint16_t port);

/**
 * Send a message to the client (not thread safe)
 */
#ifdef BOTWTOOLKIT_TCP_SEND
void sendf(const char* format, ...);
#else
inline void sendf(const char*, ...) {}
#endif

} // namespace botw::tcp
