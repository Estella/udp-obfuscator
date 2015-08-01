# Usage #

```
udpobfuscator -b [BIND_ADDRESS]:BIND_PORT -f FORWARD_ADDRESS:FORWARD_PORT [-k KEY | -c]
```

udpobfuscator listens to BIND\_ADDRESS:BIND\_PORT and forwards UDP packets to FORWARD\_ADDRESS:FORWARD\_PORT. The packets are obfuscated by taking the xor of the KEY. udpobfuscator can handle multi-clients, but each client will expire if it is idle for 5 minutes.

**BIND\_ADDRESS** the address udpobfuscator listens to. It can be a hostname (e.g. localhost, ip6-localhost) or an IPv4/v6 address. If it is missing, udpobfuscator listens to 0.0.0.0.

**BIND\_ADDRESS** the port udpobfuscator listens to. It can be a number or a service name (e.g. daytime).

**FORWARD\_ADDRESS** the address udpobfuscator forwards to. It can be a hostname (e.g. localhost, ip6-localhost) or an IPv4/v6 address.

**FORWARD\_ADDRESS** the port udpobfuscator forwards to. It can be a number or a service name (e.g. daytime).

**KEY** The forwarding traffic is the xor of the incoming traffic and the KEY. If KEY is missing, there is no obfuscation. The two ends of the tunnel must use the same KEY.

**-c** Flip every bit. Equivalent to setting KEY to 0xff.

# Example #

For example, there are two machines: a server and a client, and you run OpenVPN in UDP mode on them. The server listens at UDP port 1194. The client connects to the server at UDP port 1194.

The OpenVPN configuration file on the server is as below.
```
...
port 1194
proto udp
server 10.8.0.0 255.255.255.0
...
```

The OpenVPN configuration file on the client is as below.
```
...
client
proto udp
remote S.S.S.S 1194
...
```
where S.S.S.S is the address of the server.

But some firewall blocks the UDP connection. Then, you can run
```
udpobfuscator -b :11940 -f localhost:1194 -k asdfghjkl
```
on the server.

And run
```
udpobfuscator -b localhost:1194 -f S.S.S.S:11940 -k asdfghjkl
```
on the client.

Then, the client connects localhost:1194 instead of S.S.S.S:1194. So the OpenVPN configuration file on the client becomes
```
...
client
proto udp
remote localhost 1194
...
```

If the client routes all the traffic via the OpenVPN interface, please make sure the traffic to the server goes through the original gateway.
```
...
route S.S.S.S 255.255.255.255 net_gateway 
redirect-gateway def1
...
```