# UPush Chat Service

To create executable binary files, run
```
make all
```

To start a server, run
```
./upush_server <port> <packet loss probability>
```

To start a client, run
```
./upush_client <nick> <server address> <server port> <packet loss probability> <timeout>
```

## Features

### Alternating bit implementation

### Heartbeat
Clients will automatically send a registration message to the server every 10 seconds. To prevent an abundance of inactive clients, servers will remove clients who have not sent a registration message in the last 30 seconds, since clients have no formal way of letting the server know they are disconnecting.
