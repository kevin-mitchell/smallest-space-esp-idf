# smallest.space

_Warning: this project is currently a work in progress and very very rough._

This project is the elctronics component of a lending-library inspired instalation in front of my house. I'm calling the project `Smallest Space`, and there are different "modules" I can swap out that add different fun little bits of interest to my community.

(this link does not work yet but will) https://smallest.space 

## Smallest Remote Control House

A minature version of my house, in front of my house, that will allow people to turn the lights in our house on and off. 

### Orientation

`components/hue_http` - this is intended to contain all of the heavy lifting with the Hue API. Primarily this code should handle all of the HTTP requests and parsing out response JSON. Cleverly `hue_http` is the HTTP handling side of things, including the long running _server-sent events_ handling functionality. Parsing of the response JSON happens with cJSON inside of `hue_data_parsers`.

`wifi_connect` - this is largly what I came up with by following along with the excellent [learnesp32.com](https://learnesp32.com). It handles most of the initial wifi station connection logic and error handling (more needed).

`house_control` - this is a small layer on top of `TCA9523`, basically things specifically related to I/O Port Expander and i2c. Reading the paddle switches and turning on / off the LEDs on the board via i2c.

`TCA9534` - a library for handling i2c with the port expanded. This is slightly modified from the version in `docs`

`main` - everything else


## TODO

* the main task stack size seems to be fixed to something around 3500-4000 bytes. It would be nice to be able to incraese this for the main task / thread.