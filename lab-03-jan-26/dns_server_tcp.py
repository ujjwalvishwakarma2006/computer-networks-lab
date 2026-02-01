import json
from socket import *
from dns_init import *

# Initialise Socket
dns_socket_tcp = socket(AF_INET, SOCK_STREAM)
dns_socket_tcp.bind(('', DNS_PORT_TCP))
dns_socket_tcp.listen(1)

print(f"DNS Server [TCP] started on port {DNS_PORT_TCP}")

while True:

    # Accept all connections requesting
    connection_socket, client_addr = dns_socket_tcp.accept()
    message = connection_socket.recv(1048)
    request = json.loads(message.decode())
    hostname = request["hostname"]

    print(f"RECEIVED {request['type']} from {client_addr}")

    if request["type"] == "ip-resolve-req":
        response = {
            "type": "ip-resolve-res",
            "hostname": hostname,
            "ipaddr": ip_addr[hostname]
        }
        sent = json.dumps(response)
        connection_socket.send(sent.encode())
        print("RESOLVED and sent to the client")
                                                                                                        
    connection_socket.close()