# ICOM CI-V Implementation Notes

## Online resources

* http://www.plicht.de/ekki/civ/index.html

## About the frame

The command frame structure:

```
[$FE][$FE][to-addr][fm-addr]...[$FD]
```

* `$FE$FE` - beginning, `$FD` - end.
* `to-addr` and `fm-addr` are the destination address and the source address.
* The content of the frame is the data (command, subcommand, parameters…). The data size may be up to 50+ bytes.

The response frame is simular. If responses OK or NG (Not Good), the content should be `$FB` and `$FA`.

There are NO responses for the command `$00` and the command `$01` for communicating speed.

## Command table

* http://www.plicht.de/ekki/civ/civ-p4.html

## The basic commands of old rigs

* http://www.plicht.de/ekki/civ/civ-p52.html

This project will implement the commands used by Ham Radio Deluxe 5.24.0.38. The commands used by wsjt-x and by OmniRig will be supported also.

## CatTask Class

The CatTask is implemented based on the following FSM:

```
https://textik.com/#d495c6d9e1f82836

+-----------+  +-----------+  +---------+  +------------+
|Initialized|->|frame begin|->|addresses|->|to frame end|
+-----------+  +-----------+  +---------+  +------------+
                     ^                            |      
                     |                            v      
                     |        +---------+    +--------+  
                     +--------|send resp|<---|exec cmd|  
                              +---------+    +--------+  
```

## Commands used by HRD

* ~~$00 - Send frequency - DONE~~
* ~~$01 - Send mode - DONE~~
* ~~$02 - Read band edge frequencies - DONE~~
* ~~$03 - Read operating frequency - DONE~~
* ~~$04 - Read operating mode - DONE~~
* ~~$05 - Set operating frequency - DONE~~
* ~~$06 - Set operating mode - DONE~~
* ~~$07 - VFO mode - DONE~~
* ~~$08 - Memory mode - DONE~~
* ~~$09 - Memory write - DONE~~
* ~~$0A - Memory to VFO - DONE~~
* ~~$0B - Memory clear - DONE~~
* ~~$0F - Split - DONE~~
* ~~$11 - Select/read attenuator (0=OFF, 12=ON (12 dB)) - NG~~
* ~~$14 $01 - AF Level - DONE~~
* ~~$14 $02 - RF Level - DONE~~
* ~~$14 $06 - NR Level - NG~~
* ~~$14 $0A - RF Power - DONE~~
* ~~$14 $0B - MIC Gain - DONE~~
* ~~$14 $0D - NOTCH (NF1) frequency - NG~~
* ~~$14 $0E - COMP Level - NG~~
* ~~$14 $12 - NB Level - NG~~
* ~~$14 $16 - VOX Gain - NG~~
* ~~$14 $17 - Anti VOX Gain - NG~~
* ~~$14 $1A - NOTCH (NF2) frequency - NG~~
* ~~$15 $02 - Read S-meter level - NG~~
* ~~$15 $11 Read RF power meter - DONE~~
* ~~$15 $12 Read SWR meter - NG~~
* ~~$15 $13 Read ALC meter - NG~~
* ~~$16 $02 - Preamp - NG~~
* ~~$16 $12 - AGC selection - NG~~
* ~~$16 $22 - Noise Blanker - NG~~
* ~~$16 $40 - Noise Reducer - NG~~
* ~~$16 $41 - Auto Notch - NG~~
* ~~$16 $42 - Repeater Tone - NG~~
* ~~$16 $43 - Tone Squeltch - NG~~
* ~~$16 $44 - Speech compressor - NG~~
* ~~$16 $45 - Monitor - NG~~
* ~~$16 $46 - VOX function - NG~~
* ~~$16 $48 - Manual Notch (NF1) - NG~~
* ~~$16 $4C - VSC - NG~~
* ~~$16 $4F - Twin Peak Filter - NG~~
* ~~$16 $50 - Dial lock function (0=OFF, 1=ON) - DONE~~
* ~~$16 $51 - Manual Notch (NF2) - NG~~
* ~~$19 - Get rig id - DONE~~
* ~~$1C $01 - Set/read antenna tuner condition - NG~~

## Commands used by OmniRig

* ~~$1A $05 $00 $92 $00 - Send CI-V transceive set (OFF) - OK~~

## Commands used by WSJT-X

All commands used by WSJT-X is the subset of the commands use by HRD.
