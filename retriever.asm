.const SNIFF_NEAR_FOOD 0x9000
.const SNIFF_NEAR_PHER 0x9001  
.const SNIFF_STRONG_PHER 0x9002
.const DROP_PHER 0x9005
.const MOVE 0x9006
.const SET_SNIFF_DIR 0x9007
.const SNIFF_PHER_DIR 0x9003
.const CHECK_CARRYING 0x9010
.const CHECK_NEST 0x900A
.const CAN_CARRY 0x900B

; Movement encoding: 0x80 + offset where 0x80 = no movement
; e.g 0x7F = -1, 0x80 = 0, 0x81 = +1
.const MOVE_N 0x807F     ; dx=0, dy=-1
.const MOVE_E 0x8180      ; dx=+1, dy=0  
.const MOVE_S 0x8081     ; dx=0, dy=+1
.const MOVE_W 0x7F80      ; dx=-1, dy=0
.const MOVE_NE 0x817F        ; dx=+1, dy=-1
.const MOVE_SE 0x8181        ; dx=+1, dy=+1
.const MOVE_SW 0x7F81        ; dx=-1, dy=+1
.const MOVE_NW 0x7F7F 
    
    STR [CAN_CARRY], rZ ; track distance from starting point
    LOD rD, [0x8010]; randomize search square starting direction

!findnest
    set rB, MOVE_N
    str [MOVE], rB
    LOD rC, [CHECK_NEST]
    CMP rC, rZ
    JE !main
    set rB, MOVE_W
    str [MOVE], rB
    LOD rC, [CHECK_NEST]
    CMP rC, rZ
    JE !main
    set rB, MOVE_S
    str [MOVE], rB
    LOD rC, [CHECK_NEST]
    CMP rC, rZ
    JE !main
    set rB, MOVE_S
    str [MOVE], rB
    LOD rC, [CHECK_NEST]
    CMP rC, rZ
    JE !main
    set rB, MOVE_E
    str [MOVE], rB
    LOD rC, [CHECK_NEST]
    CMP rC, rZ
    JE !main
    set rB, MOVE_E
    str [MOVE], rB
    LOD rC, [CHECK_NEST]
    CMP rC, rZ
    JE !main
    set rB, MOVE_N
    str [MOVE], rB
    LOD rC, [CHECK_NEST]
    CMP rC, rZ
    JE !main
    set rB, MOVE_N
    str [MOVE], rB
    LOD rC, [CHECK_NEST]
    CMP rC, rZ
    JE !main   
    JMP !findnest

!main
    LOD rE, [CHECK_CARRYING]
    CMP rE, rZ
    jne !findnest
    set rA, 0x8080
    JMP !scouting

!scouting
    set rC, 32
!corner
    MOD rD, 4
    CMP rD, 0
    JE !case0
    CMP rD, 1
    JE !case1
    CMP rD, 2
    JE !case2
    set rB, MOVE_E
    JMP !line
!case0
    set rB, MOVE_N
    JMP !line
!case1
    set rB, MOVE_W
    JMP !line
!case2
    set rB, MOVE_S
!line
    CAL !move
    set rE, MOVE_N
    str [SET_SNIFF_DIR],rE
    LOD rE, [SNIFF_PHER_DIR]
    CMP rE, 0x0064
    JNE !northret
!northfail
    set rE, MOVE_E
    str [SET_SNIFF_DIR],rE
    LOD rE, [SNIFF_PHER_DIR]
    CMP rE, 0x0064
    JNE !eastret
!eastfail
    set rE, MOVE_S
    str [SET_SNIFF_DIR],rE
    LOD rE, [SNIFF_PHER_DIR]
    CMP rE, 0x0064
    JNE !southret
!southfail
    set rE, MOVE_W
    str [SET_SNIFF_DIR],rE
    LOD rE, [SNIFF_PHER_DIR]
    CMP rE, 0x0064
    JNE !westret
!westfail
    LUP rC, !corner
    LOD rE, [CHECK_CARRYING]
    CMP rE, rZ
    JNE !found_food
    LOD rD, [0x8010]
    JMP !scouting

!northret
    SHL rE,8
    SHR rE,8
    PSH rE
!northloop1
    set rB, MOVE_N
    CAL !move
    LUP rE, !northloop1
    LOD rE, [CHECK_CARRYING]
    CMP rE, rZ
    JNE !found_food
    POP rE
!northloop2
    set rB, MOVE_S
    CAL !move
    LUP rE, !northloop2
    JMP !northfail

!eastret
    SHL rE,8
    SHR rE,8
    PSH rE
!eastloop1
    set rB, MOVE_E
    CAL !move
    LUP rE, !eastloop1
    LOD rE, [CHECK_CARRYING]
    CMP rE, rZ
    JNE !found_food
    POP rE
!eastloop2
    set rB, MOVE_W
    CAL !move
    LUP rE, !eastloop2
    JMP !eastfail

!southret
    SHL rE,8
    SHR rE,8
    PSH rE
!southloop1
    set rB, MOVE_S
    CAL !move
    LUP rE, !southloop1
    LOD rE, [CHECK_CARRYING]
    CMP rE, rZ
    JNE !found_food
    POP rE
!southloop2
    set rB, MOVE_N
    CAL !move
    LUP rE, !southloop2
    JMP !southfail

!westret
    SHL rE,8
    SHR rE,8
    PSH rE
!westloop1
    set rB, MOVE_W
    CAL !move
    LUP rE, !westloop1
    LOD rE, [CHECK_CARRYING]
    CMP rE, rZ
    JNE !found_food
    POP rE
!westloop2
    set rB, MOVE_E
    CAL !move
    LUP rE, !westloop2
    JMP !westfail    

!move
    STR [MOVE], rB
    CMP rB, 0x8180
    JE !east
    CMP rB, 0x7F80
    JE !west
    CMP rB, 0x8081
    JE !south
    set rB, 0x0001
    JMP !incs
!south
    set rB, 0x0001
    JMP !decs
!west
    set rB, 0x0100
    JMP !incs
!east
    set rB, 0x0100
!decs
    SUB rA, rB
    RET
!incs
    ADD rA, rB
    RET

!found_food
    ;;STR [MOVE], rA
    set rC, rA
    SHL rC, 8
    SHR rC, 8
    CMP rC, 0x0080
    jl !yless
    jg !ygreat
    jmp !cmpx
!ygreat
    SUB rC, 0x0080
!yglp
    set rB, 0x8081
    cal !move
    DLY 3
    lup rC, !yglp
    jmp !cmpx
!yless
    set rD, 0x0080
    sub rD, rC
!yllp
    set rB, 0x807F
    cal !move
    DLY 3
    lup rD, !yllp
!cmpx
    set rC, rA
    SHR rC, 8
    CMP rC, 0x0080
    jl !xless
    jg !xgreat
    jmp !reset
!xgreat
    SUB rC, 0x0080
!xglp
    set rB, 0x8180
    CAL !move
    DLY 3
    lup rC, !xglp
    jmp !reset
!xless
    set rD, 0x0080
    sub rD, rC
!xllp
    set rB, 0x7F80
    cal !move
    DLY 3
    lup rD, !xllp
!reset
    JMP !main