#include <cstddef>
#include <cstdio>
#include <cstring>
// not sure why but this is needed for some reason
#undef __BSD_VISIBLE
#define __BSD_VISIBLE 1
#include <netinet/in.h>
#include <nn/nifm.h>
#include <nn/os.h>
#include <nn/socket.h>
#include <sys/socket.h>

#include "toolkit/tcp.hpp"

#if BOTW_VERSION == 150
#include <megaton/patch.h>
#endif

extern "C" void* memalign(size_t alignment, size_t size);

namespace botw::tcp {

constexpr const size_t SOCKET_POOL_SIZE = 0x100000;
constexpr const size_t SOCKET_BUFFER_SIZE = 0x20000;

#ifdef BOTWTOOLKIT_TCP_SEND
static nn::os::ThreadType s_thread;
#endif
static ServerStatus s_status = ServerStatus::NotInit;
static uint16_t s_port = 5000;
static volatile bool s_ready = false;
static int s_socket = -1;
static volatile int s_client = -1;

ServerStatus get_status() { return s_status; }

static void send(const char* message, size_t length) {
    if (s_client < 0) {
        return;
    }
    if (nn::socket::Send(s_client, message, length, 0) < 0) {
        nn::socket::Close(s_client);
        s_client = -1;
        s_status = ServerStatus::WaitingForClient;
    }
}

void server_main(void*) {
    while (!s_ready) {
        nn::os::SleepThread(nn::TimeSpan::FromSeconds(1));
        nn::os::YieldThread();
    }
    nn::Result result;
    // initialize network interface module
    result = nn::nifm::Initialize();
    if (result.IsFailure()) {
        s_status = ServerStatus::NifmInitFail;
        return;
    }
    void* pool = memalign(0x1000, SOCKET_POOL_SIZE);
    result =
        nn::socket::Initialize(pool, SOCKET_POOL_SIZE, SOCKET_BUFFER_SIZE, 0x4);
    if (result.IsFailure()) {
        s_status = ServerStatus::SocketInitFail;
        return;
    }
    s_status = ServerStatus::WaitingForNetworkRequest;
    nn::nifm::SubmitNetworkRequest();
    while (nn::nifm::IsNetworkRequestOnHold()) {
    }
    if (!nn::nifm::IsNetworkAvailable()) {
        s_status = ServerStatus::NetworkNotAvailable;
        return;
    }

    // initialize server socket
    s_socket = nn::socket::Socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (s_socket < 0) {
        s_status = ServerStatus::SocketCreateFail;
        return;
    }

    // bind server socket
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = nn::socket::InetHtons(s_port);
    // using the C-compatible version of InetAton, since
    // there were breaking changes to the API across 1.5/1.6
    nnsocketInetAton("0.0.0.0", &server_addr.sin_addr);
    if (nn::socket::Bind(s_socket, reinterpret_cast<sockaddr*>(&server_addr),
                         sizeof(server_addr)) < 0) {
        s_status = ServerStatus::BindFail;
        return;
    }

    if (nn::socket::Listen(s_socket, 1) < 0) {
        s_status = ServerStatus::ListenFail;
        return;
    }

    s_status = ServerStatus::WaitingForClient;

    while (true) {
        if (s_client < 0) {
            s_client = nn::socket::Accept(s_socket, nullptr, nullptr);
            if (s_client >= 0) {
                const char* hello = "connected!\n";
                send(hello, strlen(hello));
            }
        }
        nn::os::SleepThread(nn::TimeSpan::FromSeconds(1));
        nn::os::YieldThread();
    }
}

void init() {
#ifdef BOTWTOOLKIT_TCP_SEND
#if BOTW_VERSION == 150
    // Don't initialize PosTrackerUploader
    megaton::patch::main_stream(0x00a8d070)
        << megaton::patch::repeat(exl::armv8::inst::Nop(), 23);
#endif
    const size_t s_stack_size = 0x80000;
    void* s_stack = memalign(0x1000, s_stack_size);

    nn::Result result = nn::os::CreateThread(&s_thread, server_main, nullptr,
                                             s_stack, s_stack_size, 16);
    if (result.IsFailure()) {
        s_status = ServerStatus::InitFail_CreateThread;
        return;
    }
    s_status = ServerStatus::ReadyToInit;
    nn::os::StartThread(&s_thread);
#endif
}

void start_server(uint16_t port) {
    s_port = port;
    s_ready = true;
}

#ifdef BOTWTOOLKIT_TCP_SEND
void sendf(const char* format, ...) {
    char buffer[0x1000];
    va_list args;
    va_start(args, format);
    int size = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    if (size <= 0) {
        return;
    }
    buffer[sizeof(buffer) - 1] = '\0';
    send(buffer, strlen(buffer));
}
#endif

} // namespace botw::tcp
