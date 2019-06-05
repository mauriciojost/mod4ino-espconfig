# README

This project is a base for Arduino modules built on top of `main4ino` framework. 

It allows you to write your code:
- in an extremely modular way (based in components)
- abstracting from modules configuration / synchronization with a server (already resolved)
- abstracting from time keeping (already resolved)

## Overview

Here the to-be functional behavior. 

1. The device boots and a Module is instantiated
2. Invoke `module.setup(...)`. 
 a. It will initialize `propSync`, `clockSync`
 b. It will also launch `setupArchitecture(...)`.
3. Launch `module.loop()` in the `loop()` section
4. The first time the `clockSync` will act, it will synchronize the clock (via GET /time?timezone=<tz>)
5. The first time the `propSync` will act (see `pullPushActors(...)`), it will pull & push properties (server property value has priority over device): 
 a. Retrieve oldest pull id (targets that are to be consumed) via GET /devices/<dev>/targets?status=C&ids=true
 b. Do a round of pull actors per actor:
  i. Restore via pull from server if not restored: /devices/actors/%s/reports/last
  ii. Consume pull id via pull API_URL_GET_TARGET /devices/%s/%l/actors/%s/targets
 c. Create a push id via POST /devices/<dev>/targets/ 
 d. Do a round of push actors per actor: 
  i. Push properties: API_URL_POST_CURRENT /devices/%s/%l/reports/actors/%s
 c. Mark the pull id as consumed (so it's not consumed again) via PUT /devices/<dev>/targets/<request_id>?status=X
 d. Mark the push id as closed (so it cannot be modified and can be used as last status) via PUT /devices/<dev>/reports/<request_id>?status=C


## Why it uses `log4ino` and `main4ino`?

These frameworks underneath provide:

- Simple and powerful logging framework (thanks to `log4ino`)
- Modular approach based on Actors (components) inherited from `main4ino`
  - Each actor has properties that can be changed to change its behavior
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

