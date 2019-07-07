
# TODO

* do-while macro fixup
* get actual messages using two connections
    * QoS0 messagea
    * QoS1 messagea
    * QoS2 messages
    * Mixed QoS
* unsubscribe
    * simple
    * full
    * partial
* test multiple subscribers
* test multiple publishers
* test multiple subscribers and publishers
* maybe just need to have one (or 0) message CB per client
* MT safety problem for multiple subscribers vs publishers
* maybe merge session state back with connection

* off-by-1 in test count*
* proper logging API
* callbacks should always be optional so check for NULL before calling

* deal with memory ownership issues

* BUG: header isn't read correctly if the length field is long
* add tests for long messages

* re-examine buffesr and memory management

## General Protocol Health

* worker thread needs to send a PING every once in a while (within keep-alive timeout)
* worker thread networking is a mess

# Design Notes

* never use the last free slot to send a message -- receive slots
will free in due course send ones may get stuck if we can't process
the responses due to a full buffer

## Buffers

* There is a single, dedicated read buffer
* There is a pool of buffers for use by clients for sending
* When the worker thread is reponding directly to a message it has just received,
it goes ahead and sends it back through the *receive* buffer to avoid deadlock
when the buffer pool is full
    * Note: this requires disciplined reading of messages to make sure there
    aren't parts of another message in the receive buffer beyond the one being processed

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
