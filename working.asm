; working.asm - Simple scout that moves fast and finds food

.const SNIFF_NEAR_FOOD 0x9000
.const DROP_PHER       0x9005
.const MOVE            0x9006
.const RAND            0x8010
.const MOVE_EAST 0x8180
.const NO_FOOD         0x6464

!main
    set rA, MOVE_EAST          ; direction = east
    set rB, 10         ; steps = 10
    jmp !loop

!loop
    ; ALWAYS move first
    str [MOVE], rA
    sub rB, 1

    ; Check for food while moving
    lod rD, [SNIFF_NEAR_FOOD]
    set rE, NO_FOOD
    cmp rD, rE
    jne !found_food

    ; Drop exploration pheromone
    set rE, 30
    str [DROP_PHER], rE

    ; Check if need new direction
    cmp rB, rZ
    jne !delay

    ; Pick new random direction for long scouting run
    lod rC, [RAND]
    mod rC, 4
    add rC, 4
    set rA, rC
    set rB, 15         ; Long scouting runs
    jmp !delay

!found_food
    ; Drop MAXIMUM beacon
    set rE, 255
    str [DROP_PHER], rE

    ; Move around food to mark it
    set rA, MOVE_EAST          ; Move east
    set rB, 2
    jmp !delay

!delay
    set rC, 50         ; VERY fast movement

!wait
    cmp rC, rZ
    je !loop
    sub rC, 1
    jmp !wait

!end
    jmp !end