# Sample Chatting Program

## Prerequisite

* CMake (version 3.10 or later)
* SPDLOG

## How to compile

*  Clone the repository

   $ git clone https://github.com/gbkim/chat

   $ cd chat

* Compile

  $ mkdir build

  $ cd build

  $ cmake ..

  $ make -j

## How to use
  * server

    $ cd {PROJECT_DIR}/build/server

    $ ./server {PORT}

  * client

    $ cd {PROJECT_DIR}/build/client

    $ ./client {SERVER_IP} {PORT}


## TODO

  * Add client id and nickname



