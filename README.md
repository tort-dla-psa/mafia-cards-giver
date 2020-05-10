# mafia cards giver
simple server with rooms and passwords that gives you roles for playing "mafia" game

# prerequisites 
* CMake 3.15 or something
* CLang 9.0.0 or G++ 8.0

# building
```bash
  mkdir build
  cd build
  cmake ..
  make -j3
```

# running
for server:

>./server_exec

for client:

>./client [ip] [port]

# commands
```
/start - make room without password
/start 1 - make room with password "1"
/join abcdef - join room with id "abcdef"
/join abcdef 1 - join room with id "abcdef" and pass 1
/shuffle - command for admin to give roles
```

# todo
* send all roles and nicks to admin
* fix joining to rooms without password
* add nicknames
