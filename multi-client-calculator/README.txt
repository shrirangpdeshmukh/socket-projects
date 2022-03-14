
  Compile: 
  1. gcc -o server server.c -lpthread
  2. gcc -o client client.c
  
  Run: 
  1. ./server
  2. ./client
  
  Instructions supported: 
    ADD X Y => X + Y
    SUB X Y => X - Y
    MUL X Y => X * Y
    DIV X Y => X / Y
    FINISH  => Terminate the connection.
    
  Note:
    Please ensure that server is running before any client is executed.
    Run multiple client programs in different terminals to observe the server serving the clients simultaneously.