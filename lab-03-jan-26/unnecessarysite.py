from socket import *
import json

server_port = 12001
server_socket = socket(AF_INET, SOCK_STREAM)
server_socket.bind(('', server_port))
server_socket.listen(1)
print(f"www.unnecessarysite.com started listening on port {server_port}")

def process_request(request):
    response = {
        "type": "hello",
        "data": "i am fine."
    }
    return response

while True:
    connection_socket, client_addr = server_socket.accept()
    print(f"RECEIVED Request from {client_addr}")
    message = connection_socket.recv(1024)
    request = json.loads(message.decode())
    response = process_request(request)
    message = json.dumps(response)
    connection_socket.send(message.encode())
    print(f"SENT Response to the client")
    connection_socket.close()
    print(f"CLOSED Connection")
