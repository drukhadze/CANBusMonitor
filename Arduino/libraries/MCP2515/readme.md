MCP2515 library derived from repository https://github.com/franksmicro/Arduino/tree/master/libraries/MCP2515 with small modifications

* bit for RTR transmission has been corrected
* while receiving an RTR message, the data field will not evaluated, because these are generally not included. The same in reverse direction for RTR transmission
* Timeout parameter for reception and transmission deactivated
