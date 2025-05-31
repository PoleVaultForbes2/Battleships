/**
 * @file jj.cpp (original name of AI)
 * @author Matthew Getgen
 * @author Josh Forbes
 * @brief The starter file for making your own AI.
 * @date 2023-11-27
 */

#include "jj.h"

// Write your AI's name here. Please don't make it more than 64 bytes.
#define AI_NAME "JJ WATT"

// Write your name(s) here. Please don't make it more than 64 bytes.
#define AUTHOR_NAMES "Josh Forbes & Jackson Dahlin"


/*================================================================================
 * Starts up the entire match. Do NOT change anything here unless you really understand
 * what you are doing.
 *================================================================================*/
int main(int argc, char *argv[]) {
    // player must have the socket path as an argument.
    if ( argc != 2 ) {
        printf("%s Error: Requires socket name! (line: %d)\n", AI_NAME, __LINE__);
        return -1;
    }
    char *socket_path = argv[1];

    // set random seed
    srand(getpid());
    srand(time(NULL));

    jj my_player = jj();
    return my_player.play_match(socket_path, AI_NAME, AUTHOR_NAMES);
}

jj::jj():Player() {
    return;
}

jj::~jj() {
    return;
}

/*================================================================================
 * This is like a constructor for the entire match.
 * You probably don't want to make changes here unless it is something that is done once at the beginning 
 * of the entire match..
 *================================================================================*/
void jj::handle_setup_match(PlayerNum player, int board_size) {
    this->player = player;
    this->board_size = board_size;

    //declare the our shot heat map at beg of match
    for(int r=0; r<this->board_size; r++){
        for(int c=0; c<this->board_size; c++){
            this->enemy_shots[r][c] = 0;
            this->jj_shots[r][c].ships_sunk = 0;
            this->jj_shots[r][c].value = 0;
        }
    }

    this->num_games = 0;

    create_boards();
    return;
}

/*================================================================================
 * This is like a constructor for one game/round within the entire match.
 * Add anything here that you need to do at the beginning of each game.
 *================================================================================*/
void jj::handle_start_game() {
    //set the ships array to 0
    for(int i=0; i<10; i++){
        this->ships[i] = 0;
    }
    //define variables that track through each game
    this->num_shots = 0;
    this->num_games++;
    this->risky = false;

    clear_boards();
    return;
}

/*================================================================================
 * Example of how to decide where to place a ship of length ship_length and
 * inform the contest controller of your decision.
 *
 * If you place your ship even a bit off the board or collide with a previous ship
 * that you placed in this round, you instantly forfeit the round.
 *
 * TLDR: set ship.len, ship.row, and ship.col to good values and return ship.
 *================================================================================*/
Ship jj::choose_ship_place(int ship_length) {
    Ship ship;
    ship.row = 0;
    ship.col = 0;
    Direction dir;

    //gives the ship a random direction
    int dirc = rand()%2;
    if(dirc==0){
        dir = HORIZONTAL;
    }
    else dir = VERTICAL;

    bool ship_okay = false;

    int row=0;
    int col=0;
    int i=0;
    while(ship_okay == false){
        //get random location
        row = rand() % this->board_size;
        col = rand() % this->board_size;

        if(canPlaceShip(row, col, dir, ship_length)){
            //if we can't find a coordinate to place a ship, stop checking if we want... only can
            if(i<30){
                if(wantPlaceShip(row, col, dir)){
                    ship_okay = true;
                }
            }
            else{
                ship_okay = true;
            }
        }
        //else if failed, then try other direction
        else if(dir == HORIZONTAL){
            dir = VERTICAL;
            if(canPlaceShip(row, col, dir, ship_length)){
                if(i<30){
                    if(wantPlaceShip(row, col, dir)){
                        ship_okay = true;
                    }
                }
                else{
                    ship_okay = true;
                }
            }
        }
        else if(dir == VERTICAL){
            dir = HORIZONTAL;
            if(canPlaceShip(row, col, dir, ship_length)){
                if(i<30){
                    if(wantPlaceShip(row, col, dir)){
                        ship_okay = true;
                    }
                }
                else{
                    ship_okay = true;
                }
            }
        }
        i++;
    }
    markShip(row, col, dir, ship_length);
    ship.row = row;
    ship.col = col;
    ship.dir = dir;
    ship.len = ship_length;

    for(int i=0; i<10; i++){
        if(this->ships[i] == 0){
            this->ships[i] = ship_length;
            i=10;
        }
    }
    
    return ship;
}

/*================================================================================
 * Example of how to decide where to shoot and inform the contest controller
 * of your decision.
 *================================================================================*/
Shot jj::choose_shot() {
    Shot shot;

    shot.row = 0;
    shot.col = 0;
    
    int row = 0;
    int col = 0;

    //if its the first game, shoot in corner to see if they are playing risky
    if(this->num_games == 1 and this->num_shots == 0){
        this->num_shots++;
        return shot;
    }
    //if the first shot was a hit, then keep shooting at the top
    else if(this->num_games == 1 and this->num_shots == 1){
        if(this->shot_board[0][0] == HIT){
            this->risky = true;
        }
    }

    //check if there has been 15 shots
    //if yes then check where the opponent has been shooting
    if(this->num_shots == 15){
        for(int r=0; r<this->board_size; r++){
            for(int c=0; c<this->board_size; c++){
                //if the spot isn't water, then it must have been shot at
                if(this->ship_board[r][c] != WATER){
                    //if they shot there, subtrack value to place ships there
                    this->enemy_shots[r][c] += 10;
                }
            }
        }
    }
    //after 23 games reset the enemy_shot pattern
    if(this->num_games % 23 == 0){
        for(int r=0; r<this->board_size; r++){
            for(int c=0; c<this->board_size; c++){
                this->enemy_shots[r][c] = 0;
            }
        }
    }

    //gives a value to each position and saves the best coordinate
    int best_position = -1;
    int value = -1;

    if(this->num_games % 25 == 0){
        //every 25 games, reset the Heat map
        for(int r=0; r<this->board_size; r++){
            for(int c=0; c<this->board_size; c++){
                this->jj_shots[r][c].ships_sunk = 0;
                this->jj_shots[r][c].value = 0;
            }
        }
    }

    for(int r=0; r<this->board_size; r++){
        for(int c=0; c<this->board_size; c++){
            if(this->shot_board[r][c] == HIT){
                row = r;
                col = c;
                goto jump;  //if there is a hit, then do a differnet value system
            }
            value = hval(r, c) + vval(r, c) + dval(r, c);
            if(this->risky == true){
                if(r < 3) value += 200;
            }
            //check history of that coordiante
            value += this->jj_shots[r][c].value;
            if(value > best_position){
                best_position = value;
                row = r;
                col = c;
            }
        }
    }
    
jump:
    if(this->shot_board[row][col] == HIT){
        int right_value = 0;
        int left_value = 0;
        int up_value = 0;
        int down_value = 0;
        int times_need_move =0;
        int direction = 0;

        //find out if shot is valid that direction and how valuable it is
        //the value is based on if there is a hit near the hit, water near the hit, or a miss or killed ship
        if(can_shoot_right(row, col, right_value) == false) right_value = -1500;
        if(can_shoot_left(row, col, left_value) == false) left_value = -1500;
        if(can_shoot_up(row, col, up_value) == false ) up_value = -1500;
        if(can_shoot_down(row, col, down_value) == false) down_value = -1500;

        //if the gambler is placing all their ships vertical then shoot up and down first
        if(placing_vertical() == true){
            down_value += 10;
            up_value += 9;
        }

        //check which has the best value and go that direction
        if(right_value>=left_value and right_value>=up_value and right_value>=down_value) direction = 1;
        else if(left_value>=right_value and left_value>=up_value and left_value>=down_value) direction = 2;
        else if(down_value>=right_value and down_value>=left_value and down_value>=up_value) direction = 3;
        else if(up_value>=right_value and up_value>=left_value and up_value>=down_value) direction = 4;
        //if there is a hit next to another hit then we need to move an extra move away from that hit
        times_need_move =  find_need_move(row, col, direction); 

        //move that many times
        if(direction == 1) col+=times_need_move;
        else if(direction == 2) col-=times_need_move;
        else if(direction == 3) row+=times_need_move;
        else if(direction == 4) row-=times_need_move;
        else col+=times_need_move;

        shot.row = row;
        shot.col = col;
        this->num_shots++;

        return shot;

    }
    
    shot.row = row;
    shot.col = col;
    this->num_shots++;

    return shot;
}

/*================================================================================
 * This function is called to inform your AI of the result of a previous shot,
 * as well as where the opponent has shot.
 *================================================================================*/
void jj::handle_shot_return(PlayerNum player, Shot &shot) {
    // Results of your AI's shot was returned, store it
    if ( player == this->player ) {
        this->shot_board[shot.row][shot.col] = shot.value;
    }

    // Your AI is informed of where the opponent AI shot, store it
    // NOTE: Opponent shots are stored in ship_board, not shot_board
    else {
        this->ship_board[shot.row][shot.col] = shot.value;
    }
    return;
}

/*================================================================================
 * This function is called to update your shot_board (results of your shots at
 * opponent) when an opponent ship has been killed, OR to update your ship_board
 * (where you keep track of your ships) to show that your ship was killed.
 *================================================================================*/
void jj::handle_ship_dead(PlayerNum player, Ship &ship) {
    // store the ship that was killed
    for (int i = 0; i < ship.len; i++) {
        if ( player == this->player ) { // your ship is dead
            if      (ship.dir == HORIZONTAL) this->ship_board[ship.row][ship.col+i] = KILL;
            else if (ship.dir == VERTICAL)   this->ship_board[ship.row+i][ship.col] = KILL;
        } else {             // their ship is dead
            if      (ship.dir == HORIZONTAL) this->shot_board[ship.row][ship.col+i] = KILL;
            else if (ship.dir == VERTICAL)   this->shot_board[ship.row+i][ship.col] = KILL;
            //if their ship is dead, subtract the length from the ships array
            for(int j=0; j<10; j++){
                if(ship.len == this->ships[j]){
                    //after finding the ship length in the array, make it 0 and break
                    ships[j] = 0;
                    j=10;
                }
            }
        }
    }
    return;
}

/*================================================================================
 * This function runs at the end of a particular game/round.
 * Do anything here that needs to be done at the end of a game/round in the match.
 *================================================================================*/
void jj::handle_game_over() {
    //don't add values if the map was risky
    if(this->risky == false){
        heat_map_cal();
    }
    return;
}

/*================================================================================
 * This function is called by the AI's destructor and runs at the end of the entire match.
 *================================================================================*/
void jj::handle_match_over() {
    delete_boards();
    return;
}

/*================================================================================
 * This function sets up all boards at the beginning of the whole match.
 * Add setup here for any boards you create.
 *================================================================================*/
void jj::create_boards() {
    int size = this->board_size;

    // dynamically create an array of pointers.
    this->ship_board = new char*[size];
    this->shot_board = new char*[size];
    //this->int_board = new int*[size];

    // dynamically allocate memory of size `board_size` for each row.
    for (int i = 0; i < size; i++) {
        this->ship_board[i] = new char[size];
        this->shot_board[i] = new char[size];
    }
    return;
}

/*================================================================================
 * This function resets boards between rounds.
 *================================================================================*/
void jj::clear_boards() {
    // assign WATER to the boards
    for (int i = 0; i < this->board_size; i++) {
        for (int j = 0; j < this->board_size; j++) {
            this->ship_board[i][j] = WATER;
            this->shot_board[i][j] = WATER;
        }
    }
    return;
}

/*================================================================================
 * This function is called by the AI's destructor and runs at the end of the entire match.
 *================================================================================*/
void jj::delete_boards() {
    // deallocates memory using the delete operator

    for (int i = 0; i < this->board_size; i++) {
        delete[] this->ship_board[i];
        delete[] this->shot_board[i];
    }
    delete[] this->ship_board;
    delete[] this->shot_board;
    return;
}

bool jj::canPlaceShip(int row, int col, Direction dir, int length){
    if(dir == HORIZONTAL){
        if(row<0 or row >= this->board_size) return false;
        if(col<0 or col > this->board_size - length) return false;

        //check to see if ship is out of bounds or on top of another ship
        for(int c=col; c<col+length; c++){
            if(c+1 < this->board_size and (this->ship_board[row][c] == SHIP or this->ship_board[row][c+1] == SHIP)){
                return false;
            }
        }
    }
    else if(dir == VERTICAL){
        if(row<0 or row > this->board_size - length) return false;
        if(col<0 or col >= this->board_size) return false;

        //check to see if ship is out of bounds or on top of another ship
        for(int r=row; r<row+length; r++) {
            if(r+1 < this->board_size and (this->ship_board[r][col] == SHIP or this->ship_board[r+1][col] == SHIP)){
                return false;
            }
        }
    }

    //else
    return true;
}

bool jj::wantPlaceShip(int row, int col, Direction dir){
    if(dir == HORIZONTAL){
        if(this->board_size > 9){
            for(int r=0; r<this->board_size; r++){
                for(int c=0; c<this->board_size; c++){
                    if(this->enemy_shots[r][c] >= 40){  //if the enemy has shot their 4 times then avoid
                        if(row == r and col == c) return false;
                    }
                }
            }
        }
        //value system differnet for a board size of 9
        else if(this->board_size == 9){
            for(int r=0; r<this->board_size; r++){
                for(int c=0; c<this->board_size; c++){
                    if(this->enemy_shots[r][c] >= 60){  //if the enemy has shot their 6 times then avoid
                        if(row == r and col == c) return false;
                    }
                }
            }

        }
    }
    else if(dir == VERTICAL){
        if(this->board_size > 9){
            for(int r=0; r<this->board_size; r++){
                for(int c=0; c<this->board_size; c++){
                    if(this->enemy_shots[r][c] >= 40){  //if the enemy has shot their 4 times then avoid
                        if(row == r and col == c) return false;
                    }
                }
            }
        }
        //value system different for a board size of 9
        else if(this->board_size == 9){
            for(int r=0; r<this->board_size; r++){
                for(int c=0; c<this->board_size; c++){
                    if(this->enemy_shots[r][c] >= 60){  //if the enemy has shot their 6 times then avoid
                        if(row == r and col == c) return false;
                    }
                }
            }
        }
    }
    //else
    return true;
}


void jj::markShip(int row, int col, Direction dir, int length){
    if(dir == HORIZONTAL){
        for(int c=col; c<col+length; c++){
            this->ship_board[row][c] = SHIP;
        }
    }
    if(dir == VERTICAL){
        for(int r=row; r<row+length; r++){
            this->ship_board[r][col] = SHIP;
        }
    }
}

int jj::hval(int row, int col){
    int ship_length = 3;
    
    for(int i=0; i<10; i++){
        if(this->ships[i] > ship_length){
            ship_length = this->ships[i];
        }
    }

    //near the end of the game, start checking for less spots
    if(this->num_shots >= 65) ship_length = 3;
    
    int final_counter=0;
    int right_counter =0;
    int left_counter =0;
    int add_value = 16;
    int sub_value = 10;
    int coin = -1;

    //if the spot we are checking is not water, don't need to check
    if(this->shot_board[row][col] != WATER){
        final_counter = -1000;
        return final_counter;
    }

    //***CHECKING EDGE CASES VALUE***
    if(col == 0 or col == this->board_size -1) final_counter -= sub_value; 

    //check how many water spaces are right from the given spot
    for(int i=1; col+i < this->board_size and i<ship_length; i++){
       if(this->shot_board[row][col+i] == WATER) right_counter += add_value*i;
       //else if(this->shot_board[row][col+i] == KILL) right_counter -= sub_value/i+2;
       else {
           right_counter -= sub_value/i;
           break;
       }
    }
    //check how many water spaces are left from the given spot
    for(int j=1; col-j >= 0 and j<ship_length; j++){
        if(this->shot_board[row][col-j] == WATER) left_counter += add_value*j;
        //else if(this->shot_board[row][col+-j] == KILL) left_counter -= sub_value/j+2;
        else {
            left_counter -= sub_value/j;
            break;
        }
    }

    final_counter = right_counter + left_counter;

    
    //coin value to make the beg more interesting
    coin = rand() % 2;
    if(coin == 0) final_counter++;

    return final_counter;
}

int jj::vval(int row, int col){
    int ship_length = 3;
    
    for(int i=0; i<10; i++){
        if(this->ships[i] > ship_length){
            ship_length = this->ships[i];
        }
    }
    
    //near the end of the game, start checking for less spots
    if(this->num_shots >= 65) ship_length = 3;

    int final_counter = 0;
    int up_counter = 0;
    int down_counter = 0;
    int add_value = 16;
    int sub_value = 10;
    int coin = -1;

    //if the spot we are checking is not water, don't need to check
    if(this->shot_board[row][col] != WATER) {
        final_counter = -1000;
        return final_counter;
    }

    //***CHECKING EDGE CASES VALUE***
    if(row == 0 or row == this->board_size-1) final_counter -= sub_value;

    //check how many water spaces are down from the given spot
    for(int i=1; row+i < this->board_size and i<ship_length; i++){
       if(this->shot_board[row+i][col] == WATER) down_counter += add_value*i;
       //else if(this->shot_board[row+i][col] == KILL) down_counter -= sub_value/i+2;
       else {
           down_counter -= sub_value/i;
           break;
       }
    }
    //check how many water spaces are up from the given spot
    for(int j=1; row-j >= 0 and j<ship_length; j++){
        if(this->shot_board[row-j][col] == WATER) up_counter += add_value*j;
        //else if(this->shot_board[row-j][col] == KILL) up_counter -= sub_value/j+2;
        else {
            up_counter -= sub_value/j;
            break;
        }
    }

    final_counter = up_counter + down_counter;

    
    //coin value to make the beg more interesting
    coin = rand() % 2;
    if(coin == 0) final_counter++;
    

    return final_counter;
}

int jj::dval(int row, int col){
    int up_right = 0;
    int up_left = 0;
    int down_right = 0;
    int down_left = 0;
    int final_count = 0;

    //if the coordinate is on the edge don't check diagonal
    if(row == 0 or row == this->board_size-1 or col == 0 or col == this->board_size-1) return final_count;

    //check up_right
    if(row-1 >= 0 and col+1 < this->board_size and this->shot_board[row-1][col+1] == WATER) up_right+=3;

    //check down_right
    if(row+1 < this->board_size and col+1 < this->board_size and this->shot_board[row+1][col+1] == WATER) down_right+=3;

    //check up_left
    if(row-1 >=0 and col-1 >=0 and this->shot_board[row-1][col-1] == WATER) up_left+=3;

    //check down_left
    if(row+1 < this->board_size and col-1 >= 0 and this->shot_board[row+1][col-1] == WATER) down_left+=3;

    final_count = up_right + up_left + down_left + down_right;
    return final_count;
}

int jj::find_need_move(int row, int col, int direction){
    //track how many spaces we need to move for a valid shot
    int tracker=0;
    if(direction == 1){
        for(int i=0; col+i < this->board_size and this->shot_board[row][col+i] == HIT; i++){
            tracker++;  //if there is a hit then we need to move 1 more away from it
        }
    }
    else if(direction == 2){
        for(int i=0; col-i >= 0 and this->shot_board[row][col-i] == HIT; i++){
            tracker++;
        }
    }
    else if(direction == 3){
        for(int i=0; row+i < this->board_size and this->shot_board[row+i][col] == HIT; i++){
            tracker++;
        }
    }
    else if(direction == 4){
        for(int i=0; row-i >= 0 and this->shot_board[row-i][col-i] == HIT; i++){
            tracker++;
        }
    }

    return tracker;
}

bool jj::can_shoot_right(int row, int col, int& value){
    //base case that means we can't shoot if there is a miss or killed ship
    if(col+1 >= this->board_size or (this->shot_board[row][col] != WATER and this->shot_board[row][col] != HIT)) {
        return false;
    }
    //base case that means we can shoot at water
    if(this->shot_board[row][col+1] == WATER) {
        if(col-1 >= 0 and this->shot_board[row][col-1] == HIT){
            value += 80;
        }
        value++;
        return true;
    }

    //recursion case
    if(can_shoot_right(row, col+1, value)){
        value += 100;
        return true;
    }
    return false;
}

bool jj::can_shoot_left(int row, int col, int& value){
    //base case that means we can't shoot if there is a miss or killed ship
    if(col-1 < 0 or (this->shot_board[row][col] != WATER and this->shot_board[row][col] != HIT)) {
        return false;
    }
    //base case that means we can shoot at water
    if(this->shot_board[row][col-1] == WATER) {
        if(col+1 < this->board_size and this->shot_board[row][col+1] == HIT){
            value += 80;
        }
        value++;
        return true;
    }

    //recursion case
    if(can_shoot_left(row, col-1, value)){
        value += 100;
        return true;
    }
    return false;
}

bool jj::can_shoot_down(int row, int col, int& value){
    //base case that means we can't shoot if there is a miss or killed ship
    if(row+1 >= this->board_size or (this->shot_board[row][col] != WATER and this->shot_board[row][col] != HIT)) {
        return false;
    }
    //base case that means we can shoot at water
    if(this->shot_board[row+1][col] == WATER) {
        if(row-1 >=0 and this->shot_board[row-1][col] == HIT){
            value+=80;
        }
        value++;
        return true;
    }

    //recursion case
    if(can_shoot_down(row+1, col, value)){
        value += 100;
        return true;
    }
    return false;
}

bool jj::can_shoot_up(int row, int col, int& value){
    //base case that means we can't shoot if there is a miss or killed ship
    if(row-1 < 0 or  (this->shot_board[row][col] != WATER and this->shot_board[row][col] != HIT)) {
        return false;
    }
    //base case that means we can shoot at water
    if(this->shot_board[row-1][col] == WATER) {
        if(row+1 < this->board_size and this->shot_board[row+1][col] == HIT){
            value =+ 80;
        }
        value++;
        return true;
    }

    //recursion case
    if(can_shoot_up(row-1, col, value)){
        value += 100;
        return true;
    }

    //if we hit a miss or killed ship before water then shot is invalid
    return false;
}

bool jj::placing_vertical(){
    //variables to track if the ship is vertical
    int up_value = 0;
    int down_value = 0;
    int num_ver_ship = 0;

    //make a copy of board so that we can erase the sunken ship after checking it
    char copy_board[this->board_size][this->board_size];
    for(int row = 0; row<this->board_size; row++){
        for(int col=0; col<this->board_size; col++){
            if(this->shot_board[row][col] == KILL){
                copy_board[row][col] = 'K';
            }
            else{
                copy_board[row][col] = 'N';
            }
        }
    }

    //looking for sunken ships and what direction it was
    for(int r=0; r<this->board_size; r++){
        for(int c=0; c<this->board_size; c++){
            if(copy_board[r][c] == 'K'){
                //after finding the sunken ship, get rid of it
                copy_board[r][c] = 'N';
                for(int i=1; i<3; i++){
                    if(r-i >= 0 and copy_board[r-i][c] == 'K'){
                        up_value++;
                        copy_board[r-i][c] = 'N';
                    }
                    else if(r+i < this->board_size and copy_board[r+i][c] == 'K') {
                        down_value++;
                        copy_board[r+i][c] = 'N';
                    }
                }
                if(up_value + down_value >= 2) num_ver_ship++;
                up_value = 0;
                down_value = 0;
            }
        }
    }


    if(num_ver_ship >= 2) return true;
    return false;
}

void jj::heat_map_cal(){
    //add ships_sunk for each ship sunk in that spot
    for(int r=0; r<this->board_size; r++){
        for(int c=0; c<this->board_size; c++){
            if(this->shot_board[r][c] == KILL){
                this->jj_shots[r][c].ships_sunk++;
                this->jj_shots[r][c].value -= 4;
            }
            //if i did shoot but there wasn't a ship
            else if(this->shot_board[r][c] == MISS){
                this->jj_shots[r][c].value -= 4;
            }
            //if i didn't shoot there, value goes up some
            else if(this->shot_board[r][c] == WATER){
                this->jj_shots[r][c].value += 6;
            }
        }
    }
    //if that spot has sunk 6 ships add to its value
    for(int r=0; r<this->board_size; r++){
        for(int c=0; c<this->board_size; c++){
            if(this->jj_shots[r][c].ships_sunk >= 6){
                this->jj_shots[r][c].value += 50;
                this->jj_shots[r][c].ships_sunk = 0;
            }
        }
    }


}
