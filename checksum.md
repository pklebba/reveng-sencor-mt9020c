# Checksum counting

**SEEMS NOT BE CORRECT!**

For example: `[1000 0100 1001 0010 0100 0100 1000 0000 0000 0000] [0010 0100] [1000 0000] [1010 0000] [0000 0000] [0000 0000 0000 0000 0000 0000] [0000 0000] [1001 0010]` (first packet in the table)
1. Take everything except header and checksum, split for each 8 bits
*Got*: `[0010 0100] [1000 0000] [1010 0000] [0000 0000] [0000 0000] [0000 0000] [0000 0000] [0000 0000]` or in HEX: 0x24 0x80 0xA0 0x00 0x00 0x00
2. Reverse order of bits
*Got*: `[0010 0100] [0000 0001] [0000 0101] [0000 0000] [0000 0000] [0000 0000] [0000 0000] [0000 0000]` or in HEX: 0x24 0x01 0x05 0x00 0x00 0x00
3. Sum all bytes and modulo 0xff
*Got*: (0x24 + 0x01 + 0x05 + 0x00 + 0x00 + 0x00) % 0xff = 0x2A % 0xff = 0x2A
4. Add 0x1F, reverse again
*Got*: 0x2A + 0x1F = 0x49. In BIN: `[0100 1001]`. After reverse the order: `[1001 0010]`



































































