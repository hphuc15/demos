# TCP Client–Server Demo
Demo TCP client/server.
> Demo chạy trên môi trường Linux.

## 1. Clone demo
```bash
git clone --branch tcp_demo --single-branch https://github.com/hphuc15/demos.git tcp_demo
cd tcp_demo
```

## 2. Compile
```bash
gcc server.c -o server
gcc client.c -o client
```

## 3. Chạy Server
Terminal 1:
```bash
# ./server <port>
./server 2005
```

Kết quả:
```text
Listening on port 2005...
```

## 4. Chạy Client
Terminal 2:
```bash
# ./client <server_ip> <port>
./client 127.0.0.1 2005
```

Nhập `Hello world.` rồi Enter.

Client nhận:
```text
Response: Done.
```

Server hiển thị:
```text
Client connected: 127.0.0.1:<client_port>
Received: Hello world.
```

## 5. Kết thúc
Nhấn `Ctrl+D` ở Client để đóng TCP connection (Server hiển thị `Client disconnected.`). Nhấn `Ctrl+C` ở Server để dừng.

> Dùng Wireshark với filter `tcp.port == 2005` để quan sát TCP 3-way handshake, trao đổi dữ liệu và connection close.