# CS118-Project2
## Purpose
The purpose of this project is to implement a basic version of reliable data transfer protocol, including connection management and congestion control. (In other words, to recreate parts of TCP protocol on top of UPD)

The project contains two parts: a server and a client.
* The server opens UDP socket and implements incoming connection management from clients. For each of the connection, the server saves all the received data from the client in a file.
* The client opens UDP socket, implements outgoing connection management, and connects to the server. Once connection is established, it sends the content of a file to the server.

Both client and server must implement reliable data transfer using unreliable UDP transport, including data sequencing, cumulative acknowledgments, and a basic version of congestion control.

## Quality Evaluation
This project received full credit (including extra credit portion of the project description).

## Contributors
Kameron Carr (@kamalca) and Kenna Wang (@Kennawang6)

## Further Reading
There is a report file, `report.tex` that outlines how the work was divided as well as our implementation details.

## Disclaimer
This repository is to showcase my work, not to be used for the purposes of cheating in the course.
