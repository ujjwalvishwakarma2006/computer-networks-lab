from socket import *
import json

DNS_PORT = 12000

dns_socket = socket(AF_INET, SOCK_DGRAM)
dns_socket.bind(('', DNS_PORT))
print(f"DNS Server started on port {DNS_PORT}")

ip_addr = {
    "www.unnecessarysite.com": "10.0.2.4",
    "www.anotherrandomsite.com": "10.0.2.6"
}

while True:
    message, client_addr = dns_socket.recvfrom(2048)
    request = json.loads(message.decode())
    print(f"RECEIVED {request['type']} from {client_addr}")
    hostname = request["hostname"]
    if request["type"] == "ip-resolve-req":
        response = {
            "type": "ip-resolve-res",
            "hostname": hostname,
            "ipaddr": ip_addr[hostname]
        }
        sent = json.dumps(response)
        dns_socket.sendto(sent.encode(), client_addr)
        print("RESOLVED and sent to the client")