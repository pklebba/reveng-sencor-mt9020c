# Summary

Packet Frame: 112 bits
Ordering: dunno, LE??
LOW pulse width: ~500us
HIGH pulse width (bin 0): ~350us
HIGH pulse width (bin 1): ~1.17ms

## Known packets

Turn on 23'C - D4 E3 64 80 00 24 d0 b0 00 00 00 00 00 92
Turn off 23'C - D4 E3 64 80 00 04 d0 b0 00 00 00 00 00 A2

=== 

## Sample number 1
Turn on 23'C

```
1 1 0 0 0xD
0 1 0 0 0x4
1 1 0 1 0xE
0 0 1 1 0x3

0 1 1 0 0x6
0 1 0 0 0x4
1 0 0 0 0x8
0 0 0 0 0x0

0 0 0 0 0x0
0 0 0 0 0x0
0 0 1 0 0x2
0 1 0 0 0x4

1 1 0 0 0xD
0 0 0 0 0x0
1 0 1 1 0xB
0 0 0 0 0x0

0 0 0 0 0x0
0 0 0 0 0x0
0 0 0 0 0x0
0 0 0 0 0x0

0 0 0 0 0x0
0 0 0 0 0x0
0 0 0 0 0x0
0 0 0 0 0x0

0 0 0 0 0x0
0 0 0 0 0x0
1 0 0 1 0x9
0 0 1 0 0x2
```

## Sample number 2
Turn off 23'C

```
1 1 0 0 0xD
0 1 0 0 0x4
1 1 0 1 0xE
0 0 1 1 0x3

0 1 1 0 0x6 ]
0 1 0 0 0x4 ] <- 23'C (72'F) somewhere here?  
1 0 0 0 0x8 ] <- 72 = b01001000 
0 0 0 0 0x0 ] <- Dunno LE or BE

0 0 0 0 0x0
0 0 0 0 0x0
0 0 0 0 0x0 ] <- 3 bit - turn on/off
0 1 0 0 0x4

1 1 0 0 0xD
0 0 0 0 0x0
1 0 1 1 0xB
0 0 0 0 0x0

0 0 0 0 0x0
0 0 0 0 0x0
0 0 0 0 0x0
0 0 0 0 0x0

0 0 0 0 0x0
0 0 0 0 0x0
0 0 0 0 0x0
0 0 0 0 0x0

0 0 0 0 0x0
0 0 0 0 0x0
1 0 1 0 0xA ] <- CRC?
0 0 1 0 0x2 ] <- Which one? Length?
```
