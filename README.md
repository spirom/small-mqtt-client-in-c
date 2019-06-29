
# Introduction

This is a very simple C99 implementation of the MQTT
protocol from the client side only, for the purpose of
exploring just how light-weight such an implementation can
be made, for embedded use on small devices, eventually including
even micro-controllers.

It has lots of limitations, including:
* It only implements version 3.1 of the MQTT protocol, and
any failures to produce or understand messages
of that version should be treated as a bug, except that:
    * Although the protocols support UTF-8 messages, this client only supports ASCII.
    * Very long messages are not supported and the limit is currently ill-defined.
* It doesn't yet implement the QoS aspects -- doesn't retry and doesn't deal with
the server retrying.
* The approach to networking is currently very naive, assuming that connections
are reliable.
* There is no support for SSL/TLS.
* In addition to the unit tests, it has been tested against the Mosquitto server. Testing
is very far from complete, especially in edge cases like long messages.

# Compromises

To keep this a small, portable library across a range of devices, it is written in C,
does not rely on threading, and avoids external libraries. Currently it *does* rely on
the C library, and avoids multi-threading in an unnecessarily simple minded way --
each publisher and each subscriber requires a different connection, and all operations
on a connection are assumed to take place in a single thread.

# Protocol Versions

Note that the protocol level declared in a message packet is an unsigned integer, so
its relationship with the official protocol version is not completely obvious.

| Version | Protocol Level |
| ------- | -------------- |
| [3.1](http://public.dhe.ibm.com/software/dw/webservices/ws-mqtt/mqtt-v3r1.html) | 3 |
| [3.1.1](http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html) | 4 |
| [5](http://docs.oasis-open.org/mqtt/mqtt/v5.0/cs02/mqtt-v5.0-cs02.html) | 5 |

Differences between 3.1 and 3.1.1 are described
[here](https://github.com/mqtt/mqtt.github.io/wiki/Differences-between-3.1.0-and-3.1.1).









