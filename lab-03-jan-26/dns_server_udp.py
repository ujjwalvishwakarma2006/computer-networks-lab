import json
from socket import *
from dns_init import *

# Initialise Socket
dns_socket_udp = socket(AF_INET, SOCK_DGRAM)
dns_socket_udp.bind(('', DNS_PORT_UDP))

print(f"DNS Server started on port {DNS_PORT_UDP}")

while True:

    # Receive requests from the socket
    message, client_addr = dns_socket_udp.recvfrom(2048)
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
        dns_socket_udp.sendto(sent.encode(), client_addr)
        print("RESOLVED and sent to the client")