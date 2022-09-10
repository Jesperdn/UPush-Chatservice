# UPush-Chatservice

Multi-client chat service over UDP written in C for the home exam in IN2140 - Introduction to operating systems and data communication.

## Run the program

Open at least 3 different terminal windows. The chat service uses 1 instance of the server and minimum 2 instances of clients to function properly.
Use the makefile to compile and run the different parts of the program with the desired parameters or start manually: 

The loss probability of packets is a number representing percentage between 0 and 100.

### For the server:
```bash
./upush_server <port> <loss_probability>
```

### For the clients:
```bash
./push_client <nickname> <server_address> <server_port> <timeout> <loss_probability>
```

Make sure all instances are running on the same local network and asign them on different ports!
