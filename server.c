#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <msquic.h>

#define ALPN_NAME "echo-demo"

static const QUIC_API_TABLE *MsQuic;
static HQUIC Registration;
static HQUIC Configuration;

static const QUIC_REGISTRATION_CONFIG RegConfig = {
    "quic-echo-server", QUIC_EXECUTION_PROFILE_LOW_LATENCY
};
static const QUIC_BUFFER Alpn = { sizeof(ALPN_NAME) - 1, (uint8_t *)ALPN_NAME };

/* Send "Done.\n" back on the stream. The buffer must stay valid until SEND_COMPLETE. */
static void send_reply(HQUIC stream)
{
    static const char resp[] = "Done.\n";
    size_t len = sizeof(resp) - 1;

    QUIC_BUFFER *qb = malloc(sizeof(QUIC_BUFFER) + len);
    if (qb == NULL) return;
    qb->Buffer = (uint8_t *)(qb + 1);
    qb->Length = (uint32_t)len;
    memcpy(qb->Buffer, resp, len);

    /* qb is passed as ClientContext so we can free it on SEND_COMPLETE. */
    if (QUIC_FAILED(MsQuic->StreamSend(stream, qb, 1, QUIC_SEND_FLAG_NONE, qb))) {
        free(qb);
    }
}

/* Replaces the inner "while(recv(...) > 0)" loop of the TCP version. */
static QUIC_STATUS QUIC_API stream_callback(HQUIC stream, void *ctx, QUIC_STREAM_EVENT *ev)
{
    (void)ctx;
    switch (ev->Type) {
    case QUIC_STREAM_EVENT_RECEIVE:
        /* Data may arrive split across several buffers (like recv() returning partial data). */
        for (uint32_t i = 0; i < ev->RECEIVE.BufferCount; i++) {
            const QUIC_BUFFER *b = &ev->RECEIVE.Buffers[i];
            printf("Received: %.*s", (int)b->Length, (const char *)b->Buffer);
        }
        fflush(stdout);
        send_reply(stream);
        break;

    case QUIC_STREAM_EVENT_SEND_COMPLETE:
        free(ev->SEND_COMPLETE.ClientContext);
        break;

    case QUIC_STREAM_EVENT_PEER_SEND_SHUTDOWN:
        /* Equivalent of recv() == 0: the client finished sending. */
        printf("Client closed its side of the stream.\n");
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

static QUIC_STATUS QUIC_API connection_callback(HQUIC conn, void *ctx, QUIC_CONNECTION_EVENT *ev)
{
    (void)ctx;
    switch (ev->Type) {
    case QUIC_CONNECTION_EVENT_CONNECTED:
        printf("Client connected (handshake done).\n");
        break;

    case QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED:
        /* Equivalent of "a new byte stream is available": attach our stream handler. */
        MsQuic->SetCallbackHandler(ev->PEER_STREAM_STARTED.Stream,
                                   (void *)stream_callback, NULL);
        break;

    case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_TRANSPORT:
    case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_PEER:
        printf("Client disconnected.\n");
        break;

    case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE:
        MsQuic->ConnectionClose(conn);
        break;

    default:
        break;
    }
    return QUIC_STATUS_SUCCESS;
}

/* Replaces accept(). */
static QUIC_STATUS QUIC_API listener_callback(HQUIC listener, void *ctx, QUIC_LISTENER_EVENT *ev)
{
    (void)listener; (void)ctx;
    if (ev->Type == QUIC_LISTENER_EVENT_NEW_CONNECTION) {
        MsQuic->SetCallbackHandler(ev->NEW_CONNECTION.Connection,
                                   (void *)connection_callback, NULL);
        return MsQuic->ConnectionSetConfiguration(ev->NEW_CONNECTION.Connection, Configuration);
    }
    return QUIC_STATUS_SUCCESS;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }
    uint16_t port = (uint16_t)atoi(argv[1]);

    if (QUIC_FAILED(MsQuicOpen2(&MsQuic))) {
        fprintf(stderr, "MsQuicOpen2 failed\n");
        return 1;
    }
    if (QUIC_FAILED(MsQuic->RegistrationOpen(&RegConfig, &Registration))) {
        fprintf(stderr, "RegistrationOpen failed\n");
        return 1;
    }

    /* Allow the client to open 1 bidirectional stream; idle timeout 60s. */
    QUIC_SETTINGS settings;
    memset(&settings, 0, sizeof(settings));
    settings.IdleTimeoutMs = 60000;
    settings.IsSet.IdleTimeoutMs = TRUE;
    settings.PeerBidiStreamCount = 1;
    settings.IsSet.PeerBidiStreamCount = TRUE;

    if (QUIC_FAILED(MsQuic->ConfigurationOpen(Registration, &Alpn, 1,
                                              &settings, sizeof(settings), NULL, &Configuration))) {
        fprintf(stderr, "ConfigurationOpen failed\n");
        return 1;
    }

    /* TLS is mandatory in QUIC: load certificate and private key. */
    QUIC_CERTIFICATE_FILE cert_file = { "server.key", "server.crt" };
    QUIC_CREDENTIAL_CONFIG cred;
    memset(&cred, 0, sizeof(cred));
    cred.Type = QUIC_CREDENTIAL_TYPE_CERTIFICATE_FILE;
    cred.Flags = QUIC_CREDENTIAL_FLAG_NONE;
    cred.CertificateFile = &cert_file;

    if (QUIC_FAILED(MsQuic->ConfigurationLoadCredential(Configuration, &cred))) {
        fprintf(stderr, "Failed to load certificate/key\n");
        return 1;
    }

    /* Replaces socket() + bind() + listen(). */
    HQUIC listener = NULL;
    if (QUIC_FAILED(MsQuic->ListenerOpen(Registration, listener_callback, NULL, &listener))) {
        fprintf(stderr, "ListenerOpen failed\n");
        return 1;
    }

    QUIC_ADDR addr;
    memset(&addr, 0, sizeof(addr));
    QuicAddrSetFamily(&addr, QUIC_ADDRESS_FAMILY_UNSPEC);  /* any local address */
    QuicAddrSetPort(&addr, port);

    if (QUIC_FAILED(MsQuic->ListenerStart(listener, &Alpn, 1, &addr))) {
        fprintf(stderr, "ListenerStart failed\n");
        return 1;
    }

    printf("Listening on UDP port %d... (press Enter to stop)\n", port);
    getchar();   /* Callbacks run on MsQuic worker threads; main thread just waits. */

    MsQuic->ListenerClose(listener);
    MsQuic->ConfigurationClose(Configuration);
    MsQuic->RegistrationClose(Registration);
    MsQuicClose(MsQuic);
    return 0;
}