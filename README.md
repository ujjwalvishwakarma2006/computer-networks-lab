# Computer Networks Laboratory

Writing codes to practice computer networking. 
These codes are intentionally over-engineered to follow best practices.

Here are brief descriptions of each folder in the order. 
Please follow the recommended order to get the best experience. 
I am using the Top-Down approach based on the Kurose and Ross book. 

Initial lab sessions were utilized to setup 2 VirtualBox VMs so that experiments can be performed. 
Therefore, the description starts with the 3rd lab session.

> **Note**: Codes are not written from a performance point of view. They are solely written from an understanding point of view. Error handling is minimal or non-existent - this is intentional to keep things simple and focused on networking concepts.

## 1. DNS Server Implementation

[**Jan 26 Lab Session**](./lab-03-jan-26/) focuses on implementing a simple DNS server using socket programming in Python. I chose Python for better understanding and ease of implementation. This lab demonstrates both TCP and UDP server implementations for DNS resolution.

**Files Overview:**

**Client Side (runs on client VM):**
- **`client.py`** - A client program that can query either the TCP or UDP DNS server to resolve hostnames to IP addresses, then make requests to the resolved IP

**Server Side (runs on server VM):**
- **`dns_init.py`** - Shared configuration module that loads the `ip_addr` dictionary from CSV for fast hostname lookups and defines DNS port constants (`DNS_PORT_TCP` and `DNS_PORT_UDP`)
- **`dns_server_tcp.py`** - TCP-based DNS server listening on port `11999`, handles `ip-resolve-req` requests using connection-oriented communication
- **`dns_server_udp.py`** - UDP-based DNS server listening on port `12000`, handles `ip-resolve-req` requests using connectionless communication
- **`ip_addresses.csv`** - Simple CSV file containing `hostname,ip_address` pairs for DNS resolution
- **`unnecessarysite.py`** - A completely unnecessary server for a completely unnecessary site that demonstrates what happens after DNS resolution. Handles only one type of request as of now because why not

**How it works:**
1. Client requests hostname resolution from DNS server (TCP or UDP)
2. DNS server looks up hostname in the loaded dictionary
3. Server responds with IP address
4. Client can then connect to the resolved IP address

Fun fact: Both servers can run simultaneously since they use different ports!