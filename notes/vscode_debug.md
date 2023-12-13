# Debug sudo application with vscode

  1. create /home/gamer/gdb with following content:
```
sudo /usr/bin/gdb "$@"
```

  2. add this line to /etc/sudoers
```
gamer ALL=(ALL) NOPASSWD:/usr/bin/gdb
```

then copy the launch.json in this folder into .vscode folder