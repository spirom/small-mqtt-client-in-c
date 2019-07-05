
# TODO

* proper logging API

* special send and receive queues
* deal with memory ownership issues

* message types
    * publish
    * subscribe
    * receive messages

## General Protocol Health

* worker thread needs to send a PING every once in a while (within keep-alive timeout)
* worker thread networking is a mess

# Design Notes

* never use the last free slot to send a message -- receive slots
will free in due course send ones may get stuck if we can't process
the responses due to a full buffer

## Buffers

* There is a single, dedicated read buffer
* there is a pool of buffers for use by clients for sending, and by the worker thread
for responding to server messages

## Queues

## Memory management
* The API copies input strings supplied byt he client and so is
responsible for its copies,
but the client is responsible for the originals, which can be deleted at
any tiem after the call
* The API hands response strings to the caller, show is
responsible for deleting them
and can do so at any time

## Threading

The processing thread:
* On startup, re-sends lost published messages
* sends pre-serialized (user-generated) outgoing messages
* listens for and reads incoming messages
* de-serializes incoming messages
* calls user-supplied callbacks when appropriate for acknowledgement or
subscription deliver
* takes appropriate protocol action on incoming messages, including generating,
serializing and sending messages
