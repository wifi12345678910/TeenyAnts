; filepath: c:\Users\thunt\CS520\TeenyAT\TeenyAnts\simple_ant.asm
; Very simple ant that just moves in a circle and drops pheromones

.const DROP_PHER 0x9005
.const MOVE 0x9006

; Movement commands
.const MOVE_NORTH 0x807F
.const MOVE_EAST 0x8180  
.const MOVE_SOUTH 0x8081
.const MOVE_WEST 0x7F80

; Initialize
SET rA, 0           ; Direction counter (0,1,2,3 = N,E,S,W)
SET rB, 0           ; Step counter
SET rC, 25          ; Pheromone strength

!main_loop
    ; Drop pheromone
    STR [DROP_PHER], rC
    
    ; Move based on current direction
    CMP rA, 0
    JE !go_north
    CMP rA, 1
    JE !go_east  
    CMP rA, 2
    JE !go_south
    JMP !go_west        ; Default: west

!go_north
    SET rD, MOVE_NORTH
    JMP !do_move
!go_east
    SET rD, MOVE_EAST
    JMP !do_move
!go_south  
    SET rD, MOVE_SOUTH
    JMP !do_move
!go_west
    SET rD, MOVE_WEST

!do_move
    ; Execute movement
    STR [MOVE], rD
    
    ; Increment step counter
    ADD rB, 1
    
    ; Change direction every 5 steps
    SET rE, rB
    SET rD, 5           ; Reuse rD for modulo calculation
    ; Manual modulo: rE = rB % 5
!mod_loop
    CMP rE, rD
    JL !mod_done
    SUB rE, rD
    JMP !mod_loop
!mod_done
    
    ; If rE == 0, change direction
    CMP rE, rZ
    JNE !wait
    
    ; Change direction: rA = (rA + 1) % 4
    ADD rA, 1
    SET rE, rA
    SET rD, 4           ; Reuse rD for modulo calculation
!mod_loop2
    CMP rE, rD  
    JL !mod_done2
    SUB rE, rD
    JMP !mod_loop2
!mod_done2
    SET rA, rE

!wait
    ; Simple delay using rD
    SET rD, 500
!wait_loop
    CMP rD, rZ
    JE !main_loop
    SUB rD, 1
    JMP !wait_loop

!end
    JMP !end