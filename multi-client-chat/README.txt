Compile:
    1. gcc -o s server.c -lpthread
    2. gcc -o c client.c -lpthread

2. Run ./s in one terminal
3. Run ./c in atleast two terminals.

4. Enter command to interact with the server.
    Supported Commands:
        1.RS - To get the list of available clients
        2.CHAT 'id' - Chat with the client of given id.
        3.CLOSE - Close the connection
        
5. After establishing the chat both clients can send messeage to their chat partners.

6. To end the chat send 'GOODBYE!'