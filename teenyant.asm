; filepath: c:\Users\thunt\CS520\TeenyAT\TeenyAnts\teenyant.asm
; TeenyAT ant program - moves randomly and looks for food

; Port definitions for ant communication
.const GET_POSITION 0x1000
.const GET_SURROUNDINGS 0x1001  
.const MOVE 0x2000
.const PICKUP_FOOD 0x2001

; Direction constants (0-7 for 8 directions)
.const NORTH 0
.const NORTHEAST 1
.const EAST 2
.const SOUTHEAST 3
.const SOUTH 4
.const SOUTHWEST 5
.const WEST 6
.const NORTHWEST 7

; Initialize ant
SET rA, rZ          ; Clear register A
SET rB, rZ          ; Clear register B  
SET rC, rZ          ; Clear register C (direction counter)
SET rD, rZ          ; Clear register D (temp/counter)

!main_loop
    ; Get my current position
    LOD rA, [GET_POSITION]
    
    ; Get what's around me
    LOD rB, [GET_SURROUNDINGS] 
    
    ; Check if there's food nearby (bits 0-7 indicate food)
    SET rD, 0x00FF      ; Mask for food bits
    SET rE, rB          ; Copy surroundings to rE
    AND rE, rD          ; Check food bits (rE = rB & rD)
    CMP rE, rZ
    JNE !found_food     ; If not zero, there's food nearby
    
    ; No food nearby, move randomly
    JMP !random_move

!found_food
    ; There's food nearby! Find which direction
    SET rC, rZ          ; Direction counter (0-7)
    SET rD, 1           ; Bit mask starting at bit 0
    
!find_direction_loop
    ; Check if food is in direction rC
    SET rE, rB          ; Copy surroundings to rE
    AND rE, rD          ; Test specific bit (rE = rB & rD)
    CMP rE, rZ
    JNE !move_to_food   ; Found food in this direction!
    
    ; Try next direction
    ADD rC, 1           ; Next direction
    SHL rD, 1           ; Shift bit mask left
    SET rA, 8
    CMP rC, rA          ; Check if we've tried all 8 directions
    JL !find_direction_loop
    
    ; Shouldn't reach here, but fallback to random move
    JMP !random_move

!move_to_food
    ; Move toward the food in direction rC
    STR [MOVE], rC
    
    ; Try to pick up food at new location
    SET rA, 1
    STR [PICKUP_FOOD], rA
    
    ; Small delay before next action
    SET rD, 200
    JMP !wait_loop

!random_move
    ; Generate pseudo-random direction (0-7)
    LOD rA, [GET_POSITION]  ; Use position as seed
    SET rB, 7
    SET rC, rA              ; Copy position to rC
    AND rC, rB              ; Apply mask (rC = rA & 7)
    
    ; Move in random direction
    STR [MOVE], rC
    
    ; Try to pick up food (in case we step on some)
    SET rA, 1
    STR [PICKUP_FOOD], rA
    
    ; Longer delay for random movement
    SET rD, 500

!wait_loop
    ; Simple delay loop
    CMP rD, rZ
    JE !main_loop       ; If counter is zero, continue
    SUB rD, 1           ; Decrement counter
    JMP !wait_loop      ; Keep waiting

; Should never reach here
!infinite_loop
    JMP !infinite_loop