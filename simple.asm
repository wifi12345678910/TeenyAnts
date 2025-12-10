; filepath: c:\Users\thunt\CS520\TeenyAT\TeenyAnts\simple_ant.asm
; Very simple ant that just moves in a circle and drops pheromones

.const MOVE_CMD  0x9006
.const CHECK_CARRYING  0x9010
.const TRY_PICKUP_FOOD 0x9011
.const RAND      0x8010

; MOVE_CMD commands: 4=N, 5=E, 6=S, 7=W

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
    set rC, 5          ; step east
    jmp !do_move

!go_south
    set rC, 6          ; step south
    jmp !do_move

!go_west
    set rC, 7          ; step west
    jmp !do_move

!go_north
    set rC, 4          ; step north

!do_move
    str [MOVE_CMD], rC

    ; Try to pick up food if we accidentally step on it
    lod rE, [TRY_PICKUP_FOOD]

    ; Turn every 12 steps to make a large circle around nest perimeter
    add rB, 1
    set rC, rB
    mod rC, 12         ; turn every 12 steps for wide circle AROUND nest
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
    add rD, 4
    set rA, rD
    set rB, 4
    jmp !nest_move

!nest_move
    str [MOVE_CMD], rA
    sub rB, 1

    ; Faster return speed
    set rC, 120
    jmp !wait_loop

!delay
    ; Steady circling speed
    set rC, 250        ; Slightly slower for wider circle

!wait_loop
    cmp rC, rZ
    je  !circle_loop
    sub rC, 1
    jmp !wait_loop

!end
    jmp !end