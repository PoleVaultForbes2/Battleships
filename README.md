JJ AI - Battleship AI
=====================


Overview
--------

JJ is a Battleship AI developed by Josh Forbes. The AI aims to intelligently place ships and choose shots to maximize its chances of winning.

Ship Placement Logic
--------------------

The AI's ship placement strategy considers the following:

- **Random Initialization**: Ships are initially placed in random valid locations and orientations (horizontal or vertical).

- **Avoiding Hotzones**: After the first 15 shots of a game, the AI analyzes where the opponent has been shooting on its own board. If a specific cell has been shot at multiple times  
  - (4 or more for board sizes greater than 9,  
  - 6 or more for board size 9),  
  the AI will try to avoid placing ships in those *"hotzones"* in subsequent games.  
  This is designed to make the AI's ship placement less predictable based on opponent's successful hits.

- **Adaptive Strategy (Early Game)**: In the very first game, the AI takes an initial shot at (0,0).  
  If this shot is a hit, it flags the opponent as "risky," which influences subsequent shot choices.

Shot Selection Logic
--------------------

The AI's shot selection strategy is based on a **"heat map"** approach, assigning values to each cell on the opponent's board.  
The goal is to choose the cell with the highest calculated value.

### Value Calculation Components:

The value of a cell is determined by a combination of factors:

### Hit-Based Targeting (Hunting Mode):

- If a previous shot was a **HIT**, the AI enters a *"hunting mode"* and prioritizes shooting around that hit.
- It recursively checks adjacent cells (right, left, down, up) to find **WATER** cells.
- The value of a potential shot increases significantly if it leads to a **WATER** cell that is adjacent to a **HIT**.
- It will also calculate how many *"moves"* away it needs to shoot to find a **WATER** cell next to a **HIT** (e.g., if there are multiple HITs in a row).
- If the AI detects that the opponent might be placing ships predominantly vertically (based on patterns of sunk ships), it will slightly prioritize vertical shots when in hunting mode.

### Pattern-Based Probability (General Shooting Mode):

- **Horizontal Value (hval)**: Calculates a value based on the number of consecutive **WATER** cells horizontally from the current cell.  
  Longer streaks of **WATER** cells contribute more value, especially considering the length of the remaining unsunk ships.

- **Vertical Value (vval)**: Similar to hval, but for vertical streaks of **WATER** cells.

- **Diagonal Value (dval)**: Provides a small positive value if the diagonal cells around the current cell are **WATER**.  
  This helps explore areas that might be less directly targeted by other patterns.

- **Edge Penalties**: Cells on the edges of the board receive a small penalty to discourage over-prioritizing them unless other factors make them highly valuable.

- **Early Game Randomness**: In the early stages of a game, a small random "coin flip" value is added to the horizontal and vertical values to introduce a slight element of unpredictability.

- **Shot History (Heat Map)**:  
  The AI maintains a `jj_shots` heat map, which tracks the history of shots.
  - If a shot was a **KILL**, the `ships_sunk` count for that cell increases, and its value slightly decreases.
  - If a shot was a **MISS**, its value slightly decreases.
  - If a cell was **WATER** (meaning it wasn't shot at), its value slightly increases for the next game.
  - If a cell has accumulated 6 or more `ships_sunk` over multiple games, its value significantly increases, and its `ships_sunk` count is reset.  
    This encourages the AI to revisit areas where it has had past success.

Adaptability
------------

- **Risky Opponent Detection**: If the AI detects a "risky" opponent (from the first shot being a hit at (0,0)), it gives a higher priority to shooting in the top rows (rows less than 3).

- **Periodic Heat Map Reset**:  
  Every 25 games, the `jj_shots` heat map is reset to prevent the AI from becoming too fixated on past patterns that might not be relevant anymore.  
  Similarly, the `enemy_shots` array (for ship placement) is reset every 23 games.

- **End Game Adjustment**:  
  As the `num_shots` increases (reaching 65 shots), the AI starts checking for shorter ship lengths (3) when calculating `hval` and `vval`,  
  assuming fewer large ships remain.

Internal Data Structures
------------------------

- `ship_board`: Stores the AI's own ship placements and records where the opponent has shot.

- `shot_board`: Stores the results of the AI's shots on the opponent's board (HIT, MISS, KILL, WATER).

- `enemy_shots`: A 2D array that tracks how many times the opponent has shot at each cell on the AI's board.  
  Used for adaptive ship placement.

- `jj_shots`: A 2D array of structs (`ShotInfo`) used as a heat map to track the success and failure of past shots, influencing future shot choices.  
  Each `ShotInfo` contains `ships_sunk` and `value`.

- `ships`: An array to keep track of the lengths of the AI's remaining unsunk ships.

Game Flow Handlers
------------------

The AI implements various `handle_` functions to manage the game flow:

- `handle_setup_match()`: Initializes boards and variables at the beginning of an entire match.

- `handle_start_game()`: Resets variables and clears boards at the beginning of each new game/round.

- `handle_shot_return()`: Updates `shot_board` (for AI's shots) or `ship_board` (for opponent's shots) based on the shot result.

- `handle_ship_dead()`: Updates the relevant board (`ship_board` or `shot_board`) to mark a ship as **KILLed**.  
  For the AI's own ships, it also removes the sunk ship's length from the `ships` array.

- `handle_game_over()`: Calculates and updates the `jj_shots` heat map at the end of each game,  
  unless the game was deemed "risky" (to avoid skewing the heat map based on an unusual opponent strategy).

- `handle_match_over()`: Cleans up dynamically allocated memory at the end of the entire match.
