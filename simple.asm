; filepath: c:\Users\thunt\CS520\TeenyAT\TeenyAnts\simple_ant.asm
; Very simple ant that just moves in a circle and drops pheromones

.const MOVE_CMD  0x9006
.const CHECK_CARRYING  0x9010
.const TRY_PICKUP_FOOD 0x9011
.const RAND      0x8011
; Movement encoding: 0x80 + offset where 0x80 = no movement
; 0x7F = -1, 0x80 = 0, 0x81 = +1
.const MOVE_N 0x807F     ; dx=0, dy=-1
.const MOVE_E 0x8180      ; dx=+1, dy=0  
.const MOVE_S 0x8081     ; dx=0, dy=+1
.const MOVE_W 0x7F80      ; dx=-1, dy=0

; MOVE_CMD commands: 4=N, 5=E, 6=S, 7=W

    STR [0x900B], rZ

!main
    set rA, 0          ; direction (0=E, 1=S, 2=W, 3=N)
    set rB, 0          ; step counter
    jmp !circle_loop

!circle_loop
    ; Check if we picked up food - if so, return to nest
    lod rC, [CHECK_CARRYING]
    cmp rC, rZ
    jne !return_to_nest

    ; NO pheromone dropping - just move in circle

    ; Move in current direction around nest perimeter
    cmp rA, 0
    je  !go_east
    cmp rA, 1
    je  !go_south
    cmp rA, 2
    je  !go_west
    jmp !go_north

!go_east
    set rC, MOVE_E          ; step east
    jmp !do_move

!go_south
    set rC, MOVE_S          ; step south
    jmp !do_move

!go_west
    set rC, MOVE_W          ; step west
    jmp !do_move

!go_north
    set rC, MOVE_N          ; step north

!do_move
    str [MOVE_CMD], rC

    ; Try to pick up food if we accidentally step on it
    lod rE, [TRY_PICKUP_FOOD]

    ; Turn every 32 steps to make a large circle around nest perimeter
    add rB, 1
    set rC, rB
    mod rC, 32         ; turn every 32 steps for wide circle AROUND nest
    cmp rC, rZ
    jne !delay

    ; Turn clockwise: E→S→W→N→E
    add rA, 1
    mod rA, 4
    jmp !delay

!return_to_nest
    ; Simple nest return when carrying food
    cmp rB, rZ
    jne !nest_move
    
    ; Random direction toward center
    lod rD, [RAND]
    mod rD, 4
    set rA, rD
    set rB, 4
    cmp rA, 0
    je  !goback_east
    cmp rA, 1
    je  !goback_south
    cmp rA, 2
    je  !goback_west
    jmp !goback_north

!goback_east
    set rC, MOVE_E          ; step east
    jmp !nest_move

!goback_south
    set rC, MOVE_S          ; step south
    jmp !nest_move

!goback_west
    set rC, MOVE_W          ; step west
    jmp !nest_move

!goback_north
    set rC, MOVE_N          ; step north

!nest_move
    str [MOVE_CMD], r
    sub rB, 1

    ; Faster return speed
    set rC, 1
    jmp !wait_loop

!delay
    ; Steady circling speed
    set rC, 2        ; Slightly slower for wider circle

!wait_loop
    cmp rC, rZ
    je  !circle_loop
    sub rC, 1
    jmp !wait_loop

!end
    jmp !end