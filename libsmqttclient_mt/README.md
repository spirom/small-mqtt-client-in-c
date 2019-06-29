
# TODO

* proper signaling
* thread state holds connection info
* Networking status codes

* message types
    * ping
    * publish
    * subscribe
    * receive messages


* buffer pool

# Design Notes

* never use the last free slot to send a message -- receive slots
will free in due course send ones may get stuck if we can't process
the responses due to a full buffer