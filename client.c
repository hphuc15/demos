#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <msquic.h>

#define BUF_SIZE 1024
#define ALPN_NAME "echo-demo"

static const QUIC_API_TABLE *MsQuic;
static HQUIC Registration;
static HQUIC Configuration;

static sem_t connected_sem;   /* posted when handshake finishes (or fails) */
static sem_t reply_sem;       /* posted when a reply arrives or the stream closes */
static volatile int g_connected = 0;
static volatile int g_closed = 0;

static const QUIC_REGISTRATION_CONFIG RegConfig = {
    "quic-echo-client", QUIC_EXECUTION_PROFILE_LOW_LATENCY
};
static const QUIC_BUFFER Alpn = { sizeof(ALPN_NAME) - 1, (uint8_t *)ALPN_NAME };

static QUIC_STATUS QUIC_API stream_callback(HQUIC stream, void *ctx, QUIC_STREAM_EVENT *ev)
{
    (void)ctx;
    switch (ev->Type) {
    case QUIC_STREAM_EVENT_RECEIVE:
        for (uint32_t i = 0; i < ev->RECEIVE.BufferCount; i++) {
            const QUIC_BUFFER *b = &ev->RECEIVE.Buffers[i];
            printf("Response: %.*s", (int)b->Length, (const char *)b->Buffer);
        }
        fflush(stdout);
        sem_post(&reply_sem);
        break;

    case QUIC_STREAM_EVENT_SEND_COMPLETE:
        free(ev->SEND_COMPLETE.ClientContext);
        break;

    case QUIC_STREAM_EVENT_PEER_SEND_SHUTDOWN:
    case QUIC_STREAM_EVENT_PEER_SEND_ABORTED:
        printf("Server closed the connection.\n");
        g_closed = 1;
        sem_post(&reply_sem);
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
        g_connected = 1;
        sem_post(&connected_sem);
        break;

    case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_TRANSPORT:
        /* Handshake failure (bad ALPN, unreachable server, ...) ends up here. */
        fprintf(stderr, "Connection failed/closed by transport: 0x%x\n",
                (unsigned)ev->SHUTDOWN_INITIATED_BY_TRANSPORT.Status);
        g_closed = 1;
        sem_post(&connected_sem);
        sem_post(&reply_sem);
        break;

    case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_PEER:
        g_closed = 1;
        sem_post(&reply_sem);
        break;

    case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE:
        MsQuic->ConnectionClose(conn);
        break;

    default:
        break;
    }
    return QUIC_STATUS_SUCCESS;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>.\n", argv[0]);
        return 1;
    }
    uint16_t port = (uint16_t)atoi(argv[2]);

    sem_init(&connected_sem, 0, 0);
    sem_init(&reply_sem, 0, 0);

    if (QUIC_FAILED(MsQuicOpen2(&MsQuic))) return 1;
    if (QUIC_FAILED(MsQuic->RegistrationOpen(&RegConfig, &Registration))) return 1;

    QUIC_SETTINGS settings;
    memset(&settings, 0, sizeof(settings));
    settings.IdleTimeoutMs = 60000;
    settings.IsSet.IdleTimeoutMs = TRUE;

    if (QUIC_FAILED(MsQuic->ConfigurationOpen(Registration, &Alpn, 1,
                                              &settings, sizeof(settings), NULL, &Configuration))) {
        return 1;
    }

    /* DEMO ONLY: skip certificate validation because the server uses a self-signed cert.
       In production, remove NO_CERTIFICATE_VALIDATION and validate against a trusted CA. */
    QUIC_CREDENTIAL_CONFIG cred;
    memset(&cred, 0, sizeof(cred));
    cred.Type = QUIC_CREDENTIAL_TYPE_NONE;
    cred.Flags = QUIC_CREDENTIAL_FLAG_CLIENT | QUIC_CREDENTIAL_FLAG_NO_CERTIFICATE_VALIDATION;

    if (QUIC_FAILED(MsQuic->ConfigurationLoadCredential(Configuration, &cred))) return 1;

    /* Replaces socket() + connect(). */
    HQUIC conn = NULL;
    if (QUIC_FAILED(MsQuic->ConnectionOpen(Registration, connection_callback, NULL, &conn))) return 1;

    if (QUIC_FAILED(MsQuic->ConnectionStart(conn, Configuration,
                                            QUIC_ADDRESS_FAMILY_UNSPEC, argv[1], port))) {
        fprintf(stderr, "ConnectionStart failed\n");
        return 1;
    }

    sem_wait(&connected_sem);          /* block until handshake completes or fails */
    if (!g_connected) return 1;

    /* Open one bidirectional stream: this is the "TCP-like" byte stream. */
    HQUIC stream = NULL;
    if (QUIC_FAILED(MsQuic->StreamOpen(conn, QUIC_STREAM_OPEN_FLAG_NONE,
                                       stream_callback, NULL, &stream))) return 1;
    if (QUIC_FAILED(MsQuic->StreamStart(stream, QUIC_STREAM_START_FLAG_NONE))) return 1;

    printf("Connected to %s:%d. Type a message and press Enter (Ctrl+D to quit).\n", argv[1], port);

    char input[BUF_SIZE];
    while (!g_closed && fgets(input, sizeof(input), stdin) != NULL) {
        size_t len = strlen(input);

        /* Buffer must outlive StreamSend: it is freed in SEND_COMPLETE. */
        QUIC_BUFFER *qb = malloc(sizeof(QUIC_BUFFER) + len);
        if (qb == NULL) break;
        qb->Buffer = (uint8_t *)(qb + 1);
        qb->Length = (uint32_t)len;
        memcpy(qb->Buffer, input, len);

        if (QUIC_FAILED(MsQuic->StreamSend(stream, qb, 1, QUIC_SEND_FLAG_NONE, qb))) {
            free(qb);
            fprintf(stderr, "StreamSend failed\n");
            break;
        }
        sem_wait(&reply_sem);          /* wait for the server's reply */
    }

    /* Graceful close: send FIN on the stream, then close the connection. */
    MsQuic->StreamShutdown(stream, QUIC_STREAM_SHUTDOWN_FLAG_GRACEFUL, 0);
    MsQuic->ConnectionShutdown(conn, QUIC_CONNECTION_SHUTDOWN_FLAG_NONE, 0);
    usleep(200 * 1000);                /* demo only: give callbacks time to finish */

    MsQuic->ConfigurationClose(Configuration);
    MsQuic->RegistrationClose(Registration);
    MsQuicClose(MsQuic);
    return 0;
}