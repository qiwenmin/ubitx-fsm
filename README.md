# ubitx-fms

The objective of this project is to implement a new firmware for
[ubitx](http://www.hfsignals.com/index.php/ubitx/), based on
[FsmOs](https://github.com/qiwenmin/fsmos). I am trying the FSM modal in this
project, to verify that FsmOs is useful or useless.

I learned a lot from [Ian Lee's ubitx project](https://github.com/phdlee/ubitx)
before I started this project. Thanks, [Ian Lee](https://github.com/phdlee)!

## Change logs

* 2018-01-26 Implemented the ICOM CI-V commands for ubitx. Supports HRD,
  OmniRig and WSJT-X (IC-7000).

  Because I have not received the preordered ubitx, I tested the codes on my
  STM32Arduino board. The codes can be compiled for Arduino Nano without any
  errors, warnings or hints. The implemented commands are listed
  [here](ICOM-CI-V-Note.md).
