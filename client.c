#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <msquic.h>
#include <semaphore.h>

#define ALPN_NAME "echo-demo"
#define BUF_SIZE  1024

static QUIC_API_TABLE *MsQuic;  /* QUIC APIs Table. */
static HQUIC Registration;      /* */
static HQUIC Configuration;

static sem_t connected_sem;
static sem_t reply_sem;
static volatile int g_connected = 0;
static volatile int g_closed = 0;

static const QUIC_REGISTRATION_CONFIG RegConfig = {
    .AppName = "quic-echo-client",
    .ExecutionProfile = QUIC_EXECUTION_PROFILE_LOW_LATENCY
};
static const QUIC_BUFFER Alpn = {
    .Length = sizeof(ALPN_NAME) - 1,
    .Buffer = (uint8_t *)ALPN_NAME
};


static QUIC_STATUS QUIC_API stream_callback(HQUIC stream, void *ctx, QUIC_STREAM_EVENT *event) {
    (void)ctx;
    switch (event->Type) {
        case QUIC_STREAM_EVENT_RECEIVE:
            for(uint32_t i = 0; i < event->RECEIVE.BufferCount; i++) {
                const QUIC_BUFFER *b = &event->RECEIVE.Buffers[i];
                fprintf(stdout, "ResponseL %.*s", (int)b->Length, (const char*)b->Buffer);
            }
            fflush(stdout);
            sem_post(&reply_sem);
            break;

        case QUIC_STREAM_EVENT_SEND_COMPLETE:
            free(event->SEND_COMPLETE.ClientContext);
            break;

        case QUIC_STREAM_EVENT_PEER_SEND_SHUTDOWN:

        case QUIC_STREAM_EVENT_PEER_SEND_ABORTED:
            fprintf(stdout, "Server closed the connection.\n");
            g_closed = 1;
            sem_post(&reply_sem);
            break;

        case QUIC_STREAM_EVENT_SEND_SHUTDOWN_COMPLETE:
            MsQuic->StreamClose(stream);
            break;

        default:
            break;
    }

    return QUIC_STATUS_SUCCESS;
}

static QUIC_STATUS QUIC_API connection_callback(HQUIC conn, void *ctx, QUIC_CONNECTION_EVENT *event) {
    (void)ctx;
    switch(event->Type) {
        case QUIC_CONNECTION_EVENT_CONNECTED:
            g_connected = 1;
            sem_post(&connected_sem);
            break;

        case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_TRANSPORT:
            fprintf(stderr, "Connnection failed/closed by transport: %d\n", (int)event->SHUTDOWN_INITIATED_BY_TRANSPORT.Status);
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


int main(int argc, char *argv[]) {
    if(argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>.\n", argv[0]);
        return 1;
    }
    uint16_t port = (uint16_t)atoi(argv[2]);
    QUIC_STATUS s = QUIC_STATUS_SUCCESS;

    sem_init(&connected_sem, 0, 0);
    sem_init(&reply_sem, 0, 0);

    s = MsQuicOpen2(&MsQuic);
    if(s > QUIC_STATUS_SUCCESS) {
        fprintf(stderr, "MsQuicOpen2 failed: %d\n", (int)s);
        return 1;
    }

    /* 
        Generate an internal object located on memory.
        The `Registration` variable store the address pointing to that object.
    */
    s = MsQuic->RegistrationOpen(&RegConfig, &Registration);
    if(s > QUIC_STATUS_SUCCESS) {
        fprintf(stderr, "RegistrationOpen failed: %d\n", (int)s);
        return 1;
    }

    QUIC_SETTINGS settings;
    memset(&settings, 0, sizeof(settings));
    settings.IdleTimeoutMs       = 60000;
    settings.IsSet.IdleTimeoutMs = TRUE;

    s = MsQuic->ConfigurationOpen(Registration, &Alpn, 1, &settings, sizeof(settings), NULL, &Configuration);
    if(s > QUIC_STATUS_SUCCESS) {
        fprintf(stderr, "ConfigurationOpen failed: %d\n", (int)s);
        return 1;
    }

    QUIC_CREDENTIAL_CONFIG cred;
    memset(&cred, 0, sizeof(cred));
    cred.Type = QUIC_CREDENTIAL_TYPE_NONE;
    cred.Flags = QUIC_CREDENTIAL_FLAG_CLIENT | QUIC_CREDENTIAL_FLAG_NO_CERTIFICATE_VALIDATION;

    s = MsQuic->ConfigurationLoadCredential(Configuration, &cred);
    if(s > QUIC_STATUS_SUCCESS) {
        fprintf(stderr, "ConfigurationLoadCredential failed: %d\n", (int)s);
        return 1;
    }

    HQUIC conn = NULL;
    s = MsQuic->ConnectionOpen(Registration, connection_callback, NULL, &conn);
    if(s > QUIC_STATUS_SUCCESS) {
        fprintf(stderr, "ConnectionOpen failed: %d\n", (int)s);
        return 1;
    }

    s = MsQuic->ConnectionStart(conn, Configuration, QUIC_ADDRESS_FAMILY_UNSPEC, argv[1], port);
    if(s > QUIC_STATUS_SUCCESS) {
        fprintf(stderr, "ConnectionStart failed: %d\n", (int)s);
        return 1;
    }

    sem_wait(&connected_sem);
    if(!g_connected) {
        return 1;
    }

    HQUIC stream = NULL;
    s = MsQuic->StreamOpen(conn, QUIC_STREAM_OPEN_FLAG_NONE, stream_callback, NULL, &stream);
    if(s > QUIC_STATUS_SUCCESS) {
        fprintf(stderr, "StreamOpen failed: %d\n", (int)s);
        return 1;
    }

    s = MsQuic->StreamStart(stream, QUIC_STREAM_START_FLAG_NONE);
    if(s > QUIC_STATUS_SUCCESS) {
        fprintf(stderr, "StreamStart failed: %d\n", (int)s);
        return 1;
    }

    fprintf(stdout, "Connected to %s:%d. Type a message and press Enter (Ctrl+D to quit).\n", argv[1], port);

    char input[BUF_SIZE];
    while(!g_closed && fgets(input, sizeof(input), stdin)) {
        size_t len = strlen(input);

        QUIC_BUFFER *qb = malloc(sizeof(QUIC_BUFFER) + len);
        if(qb == NULL) {
            break;
        }
        qb->Buffer = (uint8_t *)(qb + 1);
        qb->Length = (uint32_t)len;
        memcpy(qb->Buffer, input, len);

        s = MsQuic->StreamSend(stream, qb, 1, QUIC_SEND_FLAG_NONE, qb);
        if(s > QUIC_STATUS_SUCCESS) {
            free(qb);
            fprintf(stderr, "StreamSend failed: %d\n", (int)s);
            break;
        }
        sem_wait(&reply_sem);
    }

    MsQuic->StreamShutdown(stream, QUIC_STREAM_SHUTDOWN_FLAG_GRACEFUL, 0);
    MsQuic->ConnectionShutdown(conn, QUIC_CONNECTION_SHUTDOWN_FLAG_NONE, 0);
    usleep(200 * 1000);

    MsQuic->ConfigurationClose(Configuration);
    MsQuic->RegistrationClose(Registration);
    MsQuicClose(MsQuic);
    return 0;
}
