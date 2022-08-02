Mediatronix IP Camera Stream Client
-----------------------------------

```
Usage:
./CameraImageRx [-c config-file] [-h]

All arguments are optional.
   -c config-file	:	a JSON configuration file. Default: ./config.json
   -h            	:	this help/usage messase
```

Config File
-----------
See `config.json` for example. It is a simple JSON file, and is self-explainable. `./config.json` is the default configuration. It must be provided for the program to work. The directory to save image streams is configurable through `config.json`.

Requirements
------------
1. yajl (libyajl >= 2.0): To parse configs
   - On Ubuntu (or Debian or other Debian-derived dist: `sudo apt install libyajl-dev`. Required Makefile flags already updated, so just running `make` would work fine after yajl is installed.

Journal
-------
1. Added parsing configuration from the config file, so that the source
   is not changed over and over again.
2. The client attempts a maximum of 10 times with the cameras -- failing
   which exits the client process.

TODO
----
1. Provide the maximum number of connection attempts through `config.json`.
2. Provide the `MAX_CLIENT_CONNECTIONS` through `config.json`.
3. Add debug flags to avoid redundant print statements (so many as of now) -- so that the information is spit to STDOUT only if the debug flags are set.
4. IMPORTANT: Check for any potential memory leaks (since the new code was added in just 40 minutes to *just make it work* without focusing on the performance of the code).

