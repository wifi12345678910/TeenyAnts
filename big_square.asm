.const SNIFF_NEAR_FOOD 0x9000
.const SNIFF_NEAR_PHER 0x9001  
.const SNIFF_STRONG_PHER 0x9002
.const DROP_PHER 0x9005
.const MOVE 0x9006

; Movement encoding: 0x80 + offset where 0x80 = no movement
; 0x7F = -1, 0x80 = 0, 0x81 = +1
.const MOVE_NORTH 0x807F     ; dx=0, dy=-1
.const MOVE_EAST 0x8180      ; dx=+1, dy=0  
.const MOVE_SOUTH 0x8081     ; dx=0, dy=+1
.const MOVE_WEST 0x7F80      ; dx=-1, dy=0
.const MOVE_NE 0x817F        ; dx=+1, dy=-1
.const MOVE_SE 0x8181        ; dx=+1, dy=+1
.const MOVE_SW 0x7F81        ; dx=-1, dy=+1
.const MOVE_NW 0x7F7F        ; dx=-1, dy=-1