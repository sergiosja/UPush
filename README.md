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

### Note
For some reason the files have massive intendation faults when posted to Github, despite looking completely fine locally. For instance, `server.c` is double indented (8 rather than 4) and `client.c` is all over the place really. Not sure how to fix this, and would love some feedback on the matter😄
