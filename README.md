# Building

Build all:
```
make
```

Build only the `ddfs` module:
```
make module
```

# Testing
**IMPORTANT! Tests should be run in a virtual machine. They involve operation on modules and direct kernel interaction by the ddfs module.**

Tests include inserting/removing module thus they need to be run as root.
```
sudo make test
```

