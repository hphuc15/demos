/**
 * openssl req -x509 -newkey rsa:2048 -nodes -keyout server.key -out server.crt -days 365 -subj "/CN=localhost"
 * Build: gcc server.c -o server -lmsquic
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <msquic.h>

#define ALPN_NAME "echo-demo"

static const QUIC_API_TABLE *MsQuic;    /* Just store the address pointer to the APIs addresses table. */
static HQUIC Registration;              /* Quic Handle. */
static HQUIC Configuration;

static const QUIC_REGISTRATION_CONFIG RegConfig = {
    "quic-echo-server", QUIC_EXECUTION_PROFILE_LOW_LATENCY
};

static const QUIC_BUFFER Alpn = {
    .Length = sizeof(ALPN_NAME) - 1,
    .Buffer = (uint8_t *)(ALPN_NAME)
};

static void send_reply(HQUIC stream) {
    static const char resp[] = "Done.\n";
    size_t len = sizeof(resp) - 1;

    QUIC_BUFFER *qb = malloc(sizeof(QUIC_BUFFER) + len);
    if(qb == NULL) {
        return;
    }
    qb->Buffer = (uint8_t *)(qb + 1);
    qb->Length = (uint32_t)len;
    memcpy(qb->Buffer, resp, len);

    if(QUIC_FAILED(MsQuic->StreamSend(stream, qb, 1, QUIC_SEND_FLAG_NONE, qb))) {
        printf("StreamSend failed\n");
        free(qb);
    }
}

/** @brief Handle the new stream opened by the remote peer. */
static QUIC_STATUS QUIC_API stream_callback(HQUIC stream, void *ctx, QUIC_STREAM_EVENT *event) {
    (void)ctx;
    switch (event->Type) {
        
        case QUIC_STREAM_EVENT_RECEIVE:
            for(uint32_t i = 0; i < event->RECEIVE.BufferCount; i++) {
                const QUIC_BUFFER *b = &event->RECEIVE.Buffers[i];
                fprintf(stdout, "Received: %.*s", (int)b->Length, (const char*)b->Buffer);
            }
            fflush(stdout);
            send_reply(stream);
            break;

        case QUIC_STREAM_EVENT_SEND_COMPLETE:
            free(event->SEND_COMPLETE.ClientContext);
            break;

        case QUIC_STREAM_EVENT_PEER_SEND_SHUTDOWN:
            fprintf(stdout, "Client closed its side of the stream.\n");
            MsQuic->StreamShutdown(stream, QUIC_STREAM_SHUTDOWN_FLAG_GRACEFUL, 0);
            break;

        case QUIC_STREAM_EVENT_PEER_SEND_ABORTED:

        case QUIC_STREAM_EVENT_PEER_RECEIVE_ABORTED:
            MsQuic->StreamShutdown(stream, QUIC_STREAM_SHUTDOWN_FLAG_ABORT, 0);
            break;

        case QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE:
            MsQuic->StreamClose(stream);
            break;

        default:
            break;
    }
    
    return QUIC_STATUS_SUCCESS;
}


/** @brief Trigger when any event caused on new connection. */
static QUIC_STATUS QUIC_API connection_callback(HQUIC conn, void *ctx, QUIC_CONNECTION_EVENT *event){
    (void)ctx;
    switch (event->Type) {
        /* Handshake done. */
        case QUIC_CONNECTION_EVENT_CONNECTED:
            fprintf(stdout, "Client connected (Handshake done).\n");
            break;

        /* The peer opens a new streams. */
        case QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED:
            MsQuic->SetCallbackHandler(event->PEER_STREAM_STARTED.Stream, (void*)stream_callback, NULL);
            break;

        /* The connection drops by transport or protocol errors (timeout, loss, ...). */
        case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_TRANSPORT:

        /* The peer gracefully initiates connection close. */
        case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_PEER:
            fprintf(stdout, "Client disconnected.\n");
            break;

        /* Shutdown finished, ConnectionClose to free resource. */
        case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE:
            MsQuic->ConnectionClose(conn);
            break;
        
        default:
            break;
    }

    return QUIC_STATUS_SUCCESS;
}

/** @brief Trigger when any event is catched by listener.
 * Then check if it is `QUIC_LISTENER_EVENT_NEW_CONNECTION`,
 * register connection_callback for any event cause in the
 * connection which on behalf of event->NEW_CONNECTION.Connection.
 */
static QUIC_STATUS QUIC_API listener_callback(HQUIC listener, void *ctx, QUIC_LISTENER_EVENT *event) {
    (void)(listener);
    (void)ctx;
    if(event->Type == QUIC_LISTENER_EVENT_NEW_CONNECTION) {
        MsQuic->SetCallbackHandler(event->NEW_CONNECTION.Connection, (void*)connection_callback, NULL);
        return MsQuic->ConnectionSetConfiguration(event->NEW_CONNECTION.Connection, Configuration);
    }
    return QUIC_STATUS_SUCCESS;
}

/**
 * @brief 
 * @param argc arguments passing to the terminal command.
 * @param argv arguments vectors table.
 * argv[1] - port of the socket.
 */
int main(int argc, char *argv[]) {
    if(argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }
    uint16_t port = (uint16_t)atoi(argv[1]);
    QUIC_STATUS s = QUIC_STATUS_SUCCESS;



    /** PHASE 1. OPEN API TABLE. */
    /* Opens a new handle to the MsQuic library with version 2 API table. */
    s = MsQuicOpen2(&MsQuic);
    if(QUIC_FAILED(s)) {
        fprintf(stderr, "MsQuicOpen2 failed: %d\n", (int)(s));
        return 1;
    }


    /** PHASE 2. CREATE A NEW CONFIGURATION. */
    /**
     * @brief Creates a new registration.
     * @param Config An optional QUIC_REGISTRATION_CONFIG to specify how to configure the execution context of the registration.
     * @param Registration On success, returns a handle to the newly created registration.
     * @retval The function returns a QUIC_STATUS. The app may use QUIC_FAILED or QUIC_SUCCEEDED to determine if the function failed or succeeded.
     * @remarks
     * A registration represents an execution context for the application. This consists of one or more system threads that are used to process the protocol logic for the application's connections. Each execution context is completely independent from another. This allows for different applications in the same process (or kernel space) to execute generally independent.
     * A caveat to this independence is that until a packet or connection can be determined to belong to a particular registration there is shared processing.
     */
    s = MsQuic->RegistrationOpen(&RegConfig, &Registration);
    if(QUIC_FAILED(s)) {
        fprintf(stderr, "RegistrationOpen failed: %d\n", (int)(s));
        return 1;
    }

    QUIC_SETTINGS settings;
    memset(&settings, 0, sizeof(QUIC_SETTINGS));
    settings.IdleTimeoutMs              = 60000;
    settings.IsSet.IdleTimeoutMs        = TRUE;
    settings.PeerBidiStreamCount        = 1;
    settings.IsSet.PeerBidiStreamCount  = TRUE;

    /**
     * @brief Creates a new configuration.
     * @param Registration The valid handle to an open registration object.
     * @param AlpnBuffers An array of QUIC_BUFFER structs that each contain a pointer and length to a different Application Layer Protocol Negotiation (ALPN) buffer.
     * @param AlpnBufferCount The number of QUIC_BUFFER structs in the AlpnBuffers array.
     * @param Settings An optional pointer to a QUIC_SETTINGS struct that defines the initial parameters for this configuration.
     * @param SettingSize The size (in bytes) of the Settings parameter.
     * @param Context The application context pointer (possibly null) to be associated with the configuration object.
     * @param Configuration On success, returns a handle to the newly opened configuration object.
     * @retval The function returns a QUIC_STATUS. The app may use QUIC_FAILED or QUIC_SUCCEEDED to determine if the function failed or succeeded.
     * @remarks
     * On success, ConfigurationOpen creates a new configuration object. A configuration object abstracts all connection settings and security configuration.
     * Once the configuration is loaded (via ConfigurationLoadCredential) it can be used for a connection; ConnectionStart on client; ConnectionSetConfiguration on server.
     * The configuration must be cleaned up via ConfigurationClose when the application is done with it.
     */
    s = MsQuic->ConfigurationOpen(Registration, &Alpn, 1, &settings, sizeof(settings), NULL, &Configuration);
    if(QUIC_FAILED(s)) {
        fprintf(stderr, "ConfigurationOpen failed: %d\n", (int)(s));
        return 1;
    }

    /* PHASE 3. TLS. */
    /* --- TLS is mandatory in QUIC --- */
    // Generate bt OpenSSL
    QUIC_CERTIFICATE_FILE cert_file = {
        .CertificateFile = "server.crt",
        .PrivateKeyFile  = "server.key"
    };
    QUIC_CREDENTIAL_CONFIG cred;
    memset(&cred, 0, sizeof(QUIC_CREDENTIAL_CONFIG));
    cred.Type  = QUIC_CREDENTIAL_TYPE_CERTIFICATE_FILE;
    cred.Flags = QUIC_CREDENTIAL_FLAG_NONE;
    cred.CertificateFile = &cert_file;

    /** @brief Loads the specified credential configuration for the configuration object. */
    s = MsQuic->ConfigurationLoadCredential(Configuration, &cred);
    if(QUIC_FAILED(s)) {
        fprintf(stderr, "Failed to load certificate/key: %d\n", (int)(s));
        return 1;
    }


    /* PHASE 4. LISTENER. */
    HQUIC listener = NULL;
    /**
     * @brief Creates a new listener.
     * @param Registration The valid handle to an open registration object.
     * @param Handler A pointer to the app's callback handler to be invoked for all listener events.
     * @param Context The app context pointer (possibly null) to be associated with the listener object.
     * @param Listener On success, returns a handle to the newly opened listener object.
     * @retval The function returns a QUIC_STATUS. The app may use QUIC_FAILED or QUIC_SUCCEEDED to determine if the function failed or succeeded.
     * @remarks
     * ListenerOpen is used to allocate resources for a server application to listen for QUIC connections. The server doesn't start listening for connection attempts until ListenerStart is successfully called. For a client application, ConnectionOpen is called to create a new connection, and ConnectionStart to start that new connection.
     * The application may call ListenerStart and ListenerStop multiple times over the lifetime of a listener object, if it needs to start and stop listening for connections. Most server applications will call ListenerStart once at start up, and then ListenerStop at shutdown.
     * Every listener created with a call to ListenerOpen MUST be cleaned up with a call to ListenerClose, otherwise a memory leak will occur.
     */
    s = MsQuic->ListenerOpen(Registration, listener_callback, NULL, &listener);
    if(QUIC_FAILED(s)) {
        fprintf(stderr, "ListenOpen failed: %d\n", (int)(s));
        return 1;
    }
    
    QUIC_ADDR addr;
    memset(&addr, 0, sizeof(addr));
    QuicAddrSetFamily(&addr, QUIC_ADDRESS_FAMILY_UNSPEC);
    QuicAddrSetPort(&addr, port);

    s = MsQuic->ListenerStart(listener, &Alpn, 1, &addr);
    if(QUIC_FAILED(s)) {
        fprintf(stderr, "ListenerStart failed: %d\n", (int)(s));
        return 1;
    }

    fprintf(stdout, "Listening on UDP port %d... (press Enter to stop)\n", port);
    getchar();

    MsQuic->ListenerClose(listener);
    MsQuic->ConfigurationClose(Configuration);
    MsQuic->RegistrationClose(Registration);
    MsQuicClose(MsQuic);
    return 0;
}


/**
 * Note:
 * 1. Status code của QUIC hơi ảo, nó định nghĩa nguyên dương là mã lỗi.
 * Ref: `msquic_posix.h`
 * 
 * 2. 
 */
