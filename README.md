# CS176A-HW5-HangmanGame

Project Description
=====================
This project implements a simple two-part Hangman game using C: a game server and a player client. 
The server hosts the game and manages all game logic, including word selection, guess checking, and game state tracking. 
The client connects to the server and allows a player to guess letters and receive feedback through a terminal interface.

The client is kept lightweight, acting purely as a means to send player input and display the server's responses. 
This project demonstrates basic socket programming, inter-process communication, and game logic in C.

How to Compile
=====================
To compile the project, simply run the following command in the root directory:

    make

This will generate two executables:
- server
- client

How to Run
=====================
1. Open a terminal and run the server on a port (e.g., 1234):

    ./server 1234

2. Open another terminal and run the client to connect to the server:

    ./client 127.0.0.1 1234

(Replace `127.0.0.1` with the appropriate server IP address if running across different machines.)

Notes
=====================
- Make sure your code compiles and runs correctly on Gradescope.
- The Makefile provided should compile your code without errors.
- If your code only runs locally but fails on Gradescope, it will not receive credit.
- Follow all turn-in instructions provided with the assignment.
