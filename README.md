## Flow

### 1. Setup
Build and install `msquic`, then generate a self-signed certificate in the directory where you will run the server:

```bash
openssl req -x509 -newkey rsa:2048 -nodes -keyout server.key -out server.crt -days 365 -subj "/CN=localhost"
```

Build the sources:

```bash
gcc server.c -o server -lmsquic
gcc client.c -o client -lmsquic
```

If `msquic` is not installed system-wide, add `-I<msquic/src/inc> -L<path to libmsquic> -lpthread`, and set `LD_LIBRARY_PATH` when running.

### 2. Run
Start Wireshark on the loopback interface (`lo0` on macOS, `lo` on Linux) and filter by the port you will use. For example, with port 2005:

```
udp.port == 2005
```

Terminal 1 (run it in the directory containing `server.crt` and `server.key`):

```bash
./server 2005
```

Terminal 2:

```bash
./client 127.0.0.1 2005
```

Type a message in Terminal 2 and press Enter. The server prints `Received: ...`, the client prints `Response: Done.`, and the packets show up in the Wireshark capture.
