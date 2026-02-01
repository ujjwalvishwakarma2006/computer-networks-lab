import csv

DNS_PORT_TCP = 11999
DNS_PORT_UDP = 12000

def load_ip_addresses(filepath='./ip_addresses.csv'):
    ip_addr = {}
    with open(filepath, mode='r') as ips:
        for line in csv.reader(ips):
            if len(line) == 2:
                ip_addr[line[0]] = line[1]
    return ip_addr

ip_addr = load_ip_addresses()