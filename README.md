## 🐜 TeenyAnts 🐜

A real-time ant colony simulation built with the TeenyAT virtual processor. Watch as different ant types work together using pheromone trails to forage for food and return it to their nest.

### Building

To build TeenyAnts use the following:
```bash
g++ main.cpp graphics.c tigr.c ../teenyat.c -lopengl32 -lgdi32 -std=c++11 -o teenyants.exe
```

### Running

Run with different ant programs:
```bash
teenyants.exe simple.bin 1 retriever.bin 1 working.bin 1
```

### Ant Types

- **Retriever Ant** - Follows pheromone trails to collect food and return to nest  
- **Working Ant** - Scouts the world to find food sources and marks them with strong pheromone beacons
- **Big Square Ant** - Creates geometric pheromone patterns while moving
- **Simple Ant** - Moves in a circle and returns food if found

### Assembly Programming

Each ant behavior is programmed in TeenyAT assembly language (.asm files). To create new ant programs:

1. Write your ant behavior in assembly (see existing .asm files for examples)
2. Assemble with: `tnasm.exe your_ant.asm` 
3. Run your ant: `teenyants.exe your_ant.bin 1`

### Controls

The simulation runs automatically. Close the window or press esc to exit.

Food appears as black dots, the nest is the brown square, and pheromone trails glow yellow/orange.
