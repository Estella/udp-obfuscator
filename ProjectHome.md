This project is a UDP port forwarding program with an obfuscator. It can handle multiple UDP clients.

Some firewalls (e.g. GFW) blocks some UDP applications (e.g. OpenVPN). This project creates a UDP tunnel to break through the firewalls.

An typical example is the OpenVPN. When the OpenVPN in UDP mode is blocked by firewalls, one can use pre-shared static key to break through it, but pre-shared static key cannot be used in multi-clients mode. This project can solve this problem.

More at [Install](Install.md) and [Run](Run.md).