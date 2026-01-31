from socket import *
import json

DNS_ADDRESS = '10.0.2.4'
DNS_PORT = {
    "TCP": 11999,
    "UDP": 12000 
}
SERVER_PORT = 12001

client_socket = socket(AF_INET, SOCK_DGRAM)
# client_socket.bind('', (

def resolve_ip(hostname: str, protocol:str="UDP"):
    dns_socket = socket(AF_INET, SOCK_DGRAM)
    request = {
        "type": "ip-resolve-req",
        "hostname": hostname
        }
    message = json.dumps(request)
    dns_socket.sendto(message.encode(), (DNS_ADDRESS, DNS_PORT[protocol]))
    print(f"IP Resolution request sent to DNS Server at {DNS_ADDRESS}:{DNS_PORT[protocol]}")
    received, _ = dns_socket.recvfrom(2048)
    dns_socket.close()
    response = json.loads(received.decode())
    if response["type"] != "ip-resolve-res":
        print(f"Hostname {hostname} not resolved")
        return None
    print(f"RESOLVED\tHostname: {hostname}\tIP Address: {response['ipaddr']}")
    return response["ipaddr"]

def make_request(server_addr: str, request):
    connection_socket = socket(AF_INET, SOCK_STREAM)
    connection_socket.connect((server_addr, SERVER_PORT))
    message = json.dumps(request)
    connection_socket.send(message.encode())
    print(f"SENT Request to {server_addr}:{SERVER_PORT}")
    response = connection_socket.recv(1024).decode()
    connection_socket.close()
    return response

if __name__ == "__main__":
    hostname = "www.unnecessarysite.com"
    ip_addr = resolve_ip(hostname)
    request = {
        "type": "hi",
        "data": "how are you?"
    }
    response = make_request(ip_addr, request)
    print(response)
