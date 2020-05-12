# mafia cards giver
simple telegram bot with rooms and passwords that gives you roles for playing "mafia" game

# prerequisites 
* CMake 3.15 or something
* CLang 9.0.0 or G++ 8.0
this stuff:
```bash
sudo apt install g++ make binutils cmake libssl-dev libboost-system-dev zlib1g-dev libcurl4-openssl-dev
```

# building
```bash
  git submodule init
  git submodule update --recursive
  mkdir build
  cd build
  cmake ..
  make -j4
```

# running
>./server_exec [api_key]


# commands
```
/start - start bot
/create - make room
/create 1 - make room with password "1"
/join abcdef - join room with id "abcdef"
/join abcdef 1 - join room with id "abcdef" and pass 1
/shuffle - command for admin to give roles
```

# todo
* add cards images
* add buttons
* add history cleaning