# README

This project is a base for Arduino modules built on top of `main4ino` framework. 

It allows you to write your code:
- in an extremely modular way (based in components)
- abstracting from modules configuration / synchronization with a server (already resolved)
- abstracting from time keeping (already resolved)

## Why it uses `log4ino` and `main4ino`?

These frameworks underneath provide:

- Simple and powerful logging framework (thanks to `log4ino`)
- Modular approach based on Actors (components) inherited from `main4ino`
  - Each actor has properties that can be changed to change its behaviour
  - Each actor can act with a given frequency and perform its duty

## What is `mod4ino` useful for? 

It provides a `Module` implementation with the following features:

- API for management of multiple Actors
- Time-keeping for Actors acting
- Automatic synchronization of all Actors properties with the main4ino server
- Automatic synchronization of clock given a timezone and timing management
- Extensible user commands API, including some already implemented commands
  - set properties
  - get properties
  - write/read to FS
  - change mode
  - run HW tests
  - change log level
  - connect/initialize wifi
  - upgrade firmware
  - among others proper to the module
  - among other you can add for your implementation!

