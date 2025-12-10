; filepath: c:\Users\thunt\CS520\TeenyAT\TeenyAnts\working.asm
; TeenyAT ant program compatible with our simulation system

; Port definitions matching our simulation
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

; Initialize ant
SET rA, rZ          ; Clear register A (general purpose)
SET rB, rZ          ; Clear register B (food sensor data)
SET rC, rZ          ; Clear register C (movement direction)
SET rD, rZ           ; Step counter
SET rE, 10          ; Pheromone strength

!main_loop
    ; Drop a pheromone trail
    STR [DROP_PHER], rE
    
    ; Look for nearby food
    LOD rB, [SNIFF_NEAR_FOOD]
    
    ; Check if food was found (0xFF means no food)
    SET rA, 0xFF
    CMP rB, rA
    JE !no_food_found   ; If rB == 0xFF, no food nearby
    
    ; Food found! rB contains direction (0-7)
    JMP !move_toward_food

!no_food_found
    ; No food nearby, look for pheromone trails
    LOD rB, [SNIFF_NEAR_PHER]
    
    ; Check if pheromone trail found
    SET rA, rZ  
    CMP rB, rA
    JE !random_explore  ; If rB == 0xFF, no pheromones nearby
    
    ; Follow pheromone trail
    JMP !move_toward_pheromone

!move_toward_food
    ; Convert direction (0-7) to movement command
    SET rC, rB          ; Copy direction to rC
    JMP !execute_movement

!move_toward_pheromone  
    ; Convert direction (0-7) to movement command
    SET rC, rB          ; Copy direction to rC
    JMP !execute_movement

!random_explore
    ; Generate pseudo-random direction (0-7)
    INC rD          ; Increment step counter
    SET rC, rD          ; Copy step counter
    MOD rC, rZ + 7          ; rC = rD & 7 (gives 0-7)
    JMP !execute_movement

!execute_movement
    ; Convert direction number (0-7) to actual movement command
    CMP rC, 0
    JE !move_n
    CMP rC, 1  
    JE !move_ne
    CMP rC, 2
    JE !move_e
    CMP rC, 3
    JE !move_se
    CMP rC, 4
    JE !move_s
    CMP rC, 5
    JE !move_sw
    CMP rC, 6
    JE !move_w
    JMP !move_nw        ; Default case (7)

!move_n
    SET rA, MOVE_NORTH
    JMP !do_move
!move_ne
    SET rA, MOVE_NE
    JMP !do_move
!move_e  
    SET rA, MOVE_EAST
    JMP !do_move
!move_se
    SET rA, MOVE_SE
    JMP !do_move
!move_s
    SET rA, MOVE_SOUTH
    JMP !do_move
!move_sw
    SET rA, MOVE_SW
    JMP !do_move
!move_w
    SET rA, MOVE_WEST
    JMP !do_move
!move_nw
    SET rA, MOVE_NW

!do_move
    ; Execute the movement
    STR [MOVE], rA
    
    ; Wait a bit before next action using rA (reuse since we're done with movement)
    SET rA, 1000

!wait_loop
    CMP rA, rZ
    JE !main_loop       ; If counter is zero, continue main loop
    SUB rA, 1           ; Decrement counter  
    JMP !wait_loop      ; Keep waiting

; Should never reach here
!end
    JMP !end