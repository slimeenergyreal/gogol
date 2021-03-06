/*
 *--------------------------------------
 * Program Name: A Game Of A Game Of Life
 * Author: slimeenergy
 * License: None, please don't steal it.
 * Description: https://www.cemetech.net/forum/viewtopic.php?p=280363
 *--------------------------------------
*/

/* Keep these headers */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>

/* Standard headers (recommended) */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <graphx.h>
#include <keypadc.h>
#include <debug.h>

#define GAME_NAME "Game of Game of Life"
#define FONT_HEIGHT 8
#define EMPTY_FUNC_MENU_OPTION "\0"
#define CUSTOM_PALETTE_SIZE 13
#define FANCY_SPEED 4 //The speed at which tiles fade in and out with a fancy background
#define FANCY_TILE_COUNT 6 //How many fancy tiles should there be in splash screen
#define TIME_LIMIT 60 //The counter will count upwards until it reaches TIME_LIMIT. It is displayed as TIME_LIMIT-counter
#define ALIVE_CELLS_PER_LEVEL 1
#define ALIVE_CELLS_BASE 6
#define KEY_DEBOUNCE_DELAY 4 //How long since key has been held to activate debounce
#define KEY_DEBOUNCE 2 //How long betweeen each debounce

//All of the timer code was stolen from the "second_counter2" example because I barely have an idea on how it works.
#define SECOND 32768/1

#define WIN_TEXT_LESS_FIVE "I believe this is below average, level %d!"
#define WIN_TEXT_LESS_TEN "Nice, you did well! You made it to level %d."
#define WIN_TEXT_MORE_TEN "Amazing job! Level %d, double factorial!!"
#define WIN_TEXT_HIGHSCORE "You beat your highscore with level %d!"

typedef struct {
    uint8_t width;
    uint8_t height;
    bool* cells;
    bool* selected_cells;
} level_t;

typedef struct {
    bool running; //Whether or not the main game loop should loop
    bool timing; //Whether or not a timer should be counting the number of seconds that have gone by since it started.
    level_t* playing_level; //Current level structure being played.
    uint8_t scene; //Current game.scene. game.scenes: 0 Main Menu, 1 Level Start Screen, 2 Play, 3 Win/Lose, 4 Credits, 5 How To Play
    uint8_t cursor_pos; //Y cursor position
    uint8_t cursor_pos2; //X cursor position
    uint8_t level; //The current level being played.
    uint24_t timer_seconds; //Amount of seconds counted by timer so far.
    uint8_t highscore;
    uint8_t key_debounce_activation_timer;
} game_t;

typedef struct {
    uint8_t pos_x;
    uint8_t pos_y;
    uint8_t progress;
} fancy_tile_t;

/* Put your function prototypes here */
void update();
void update_scene();

void render();
void render_scene();
void render_background(bool fancy);
void render_centered_string(char* str, uint24_t x, uint8_t y);
void render_centered_string_outline(char* str,uint24_t x, uint8_t y, uint8_t fill,uint8_t stroke);
void render_level();

void add_cells_to_level(uint24_t cell_count);
level_t* generate_level(uint8_t width, uint8_t height, uint24_t cell_count, bool one_birth);
void create_new_level(uint8_t width, uint8_t height, uint24_t cell_count);
void destroy_level();
void next_level();
void iterate_level();
void clear_level(bool cells,bool selected);
bool compare_selection_level();

void update_and_render_fancy_tile(fancy_tile_t* tile);

bool key_up_pressed();
bool key_down_pressed();
bool key_left_pressed();
bool key_right_pressed();
bool key_enter_pressed();
bool key_second_pressed();

void start_timer();
void update_timer();
void end_timer();
void time_up();

void set_default_stats();
void reset_stats();
void save_stats();

//uint8_t random_number(uint8_t minimum,uint8_t maximum);

int24_t min(int24_t a, int24_t b);
int24_t max(int24_t a, int24_t b);
/* Put all your globals here */
/*bool game.running = true; //Whether or not the main loop should keep game.running.
level_t* game.playing_level = NULL; //The game.level that is currently being played.
uint8_t game.scene = 0; //game.scenes: 0 Main Menu, 1 game.level Start Screen, 2 Play, 3 Win, 4 Credits, 5 How To
uint8_t game.cursor_pos = 0;
uint8_t game.cursor_pos2 = 0;
uint8_t game.level = 0;*/

game_t game = {true,false,NULL,0,0,0,0,0,0};

uint16_t custom_palette[CUSTOM_PALETTE_SIZE] = {
    gfx_RGBTo1555(0,0,0),       //00, Black
    gfx_RGBTo1555(0,0,255),     //01, Full Blue Tile
    gfx_RGBTo1555(30,30,255),   //02, Slightly Less Blue Tile
    gfx_RGBTo1555(60,60,255),   //03, Slightly Slightly Less Blue Tile
    gfx_RGBTo1555(90,90,255),   //04, Slightly Slightly Slightly Less Blue Tile
    gfx_RGBTo1555(120,120,255), //05, Slightly Slightly Slightly Slightly Less Blue Tile
    gfx_RGBTo1555(150,150,255), //06, Slightly Slightly Slightly Slightly Slightly Less Blue Tile
    gfx_RGBTo1555(180,180,255), //07, Slightly Slightly Slightly Slightly Slightly Slightly Less Blue Tile
    gfx_RGBTo1555(210,210,255), //08, Slightly Slightly Slightly Slightly Slightly Slightly Slightly Less Blue Tile
    gfx_RGBTo1555(240,240,255), //09, Slightly Slightly Slightly Slightly Slightly Slightly Slightly Slightly Less Blue Tile
    gfx_RGBTo1555(255,255,255), //10, White
    gfx_RGBTo1555(255,43,43),   //11, Red
    gfx_RGBTo1555(155,24,24)   //12, Dark Red
};

void main() {
    timer_1_MatchValue_1 = SECOND;
    srandom(rtc_Time());
    ti_CloseAll();
    gfx_Begin();
    gfx_SetPalette(custom_palette,CUSTOM_PALETTE_SIZE*2,0);

    set_default_stats();
    while(game.running) {
        kb_Scan();

        update();
        render();

        gfx_SwapDraw();
    }
    save_stats();
    destroy_level();
    gfx_End();
}

/* Put other functions here */
void update() {
    if(game.timing) {
        update_timer();
    }
    if(kb_Data[6] == kb_Clear) {
        game.running = false;
    }
    update_scene();
}
void update_scene() {
    static bool timer_finished = false;
    if(game.scene == 0) {
        //Main menu options are "Play" "Credits" "Exit"
        if(key_down_pressed()) {
            game.cursor_pos = min(4,game.cursor_pos+1);
        }
        if(key_up_pressed()) {
            game.cursor_pos = max(0,game.cursor_pos-1);
        }
        if(key_enter_pressed()) {
            if(game.cursor_pos == 0) {
                game.scene = 1;
            }
            if(game.cursor_pos == 1) {
                game.scene = 4;
            }
            if(game.cursor_pos == 2) {
                game.running = false;
            }
            if(game.cursor_pos == 3) {
                game.scene = 5;
            }
            if(game.cursor_pos == 4) {
                reset_stats();
            }
        }
    } else if(game.scene == 1) {
        if(key_enter_pressed()) {
            gfx_SetTextScale(2,2);
            render_centered_string("Generating Level",LCD_WIDTH/2,LCD_HEIGHT/2-FONT_HEIGHT);
            render_centered_string("Please Wait...",LCD_WIDTH/2,LCD_HEIGHT/2+FONT_HEIGHT);
            gfx_SetTextScale(1,1);
            gfx_SwapDraw();
            next_level();
            game.scene = 2;
            game.cursor_pos = 0;
            game.cursor_pos2 = 0;
            start_timer();
            timer_finished = false;
        }
    } else if(game.scene == 2) {
        if(!timer_finished) {
            if(key_up_pressed()) {
                game.cursor_pos = max(0,game.cursor_pos-1);
            }
            if(key_left_pressed()) {
                game.cursor_pos2 = max(0,game.cursor_pos2-1);
            }
            if(key_right_pressed()) {
                game.cursor_pos2 = min(game.playing_level->width-1,game.cursor_pos2+1);
            }
            if(key_down_pressed()) {
                game.cursor_pos = min(game.playing_level->height-1,game.cursor_pos+1);
            }
            if(key_second_pressed()) {
                game.playing_level->selected_cells[game.cursor_pos*game.playing_level->width+game.cursor_pos2] = !game.playing_level->selected_cells[game.cursor_pos*game.playing_level->width+game.cursor_pos2];
            }
            if(game.timer_seconds > TIME_LIMIT) {
                time_up();
                timer_finished = true;
                //loser
            }
        }
        if(key_enter_pressed()) {
            if(!timer_finished) {
                time_up();
                timer_finished = true;
            } else {
                if(compare_selection_level()) {
                    //Win. Your selection is all on alive cells, not on dead.
                    game.scene = 1;
                } else {
                    //Lose.
                    game.scene = 3;
                    if(game.level > game.highscore) {
                        game.highscore = game.level;
                        game.level = -1;
                        save_stats();
                    }
                }
            }
        }
    } else if(game.scene == 3) {
        if(key_enter_pressed()) {
            //Set the scene to main menu
            game.scene = 0;
            //Set the cursor position to 0.
            game.cursor_pos = 0;
            game.level = 0;
        }
    } else if(game.scene == 4) {
        if(key_enter_pressed()) {
            game.cursor_pos = 0;
            game.scene = 0;
        }
    } else if(game.scene == 5) {
        if(key_enter_pressed()) {
            game.cursor_pos = 0;
            game.scene = 0;
        }
    }
}

//time is up. End timer. Iterate level.
void time_up() {
    end_timer();
    iterate_level();
}

void render() {
    render_scene();
}
void render_scene () {
    if(game.scene == 0) {
        uint8_t i;
        char *fsa = malloc(strlen("Highscore: Level 255"));
        render_background(true);
        gfx_SetTextScale(2,2);
        gfx_SetTextFGColor(10);
        render_centered_string_outline(GAME_NAME,LCD_WIDTH/2,LCD_HEIGHT/4,0,10);

        for(i = 0; i < 5; i++) {
            uint8_t clr;
            if(i == game.cursor_pos) {
                clr = 11;
            } else {
                clr = 0;
            }
            gfx_SetTextFGColor(clr);
            if(i == 0) {
                render_centered_string("Play",LCD_WIDTH/2,LCD_HEIGHT/4+FONT_HEIGHT*4);
            }
            if(i == 1) {
                render_centered_string("Credits",LCD_WIDTH/2,LCD_HEIGHT/4+FONT_HEIGHT*6.5);
            }
            if(i == 2) {
                render_centered_string("Exit",LCD_WIDTH/2,LCD_HEIGHT/4+FONT_HEIGHT*9);
            }
            if(i == 3) {
                render_centered_string("How To Play",LCD_WIDTH/2,LCD_HEIGHT/4+FONT_HEIGHT*11.5);
            }
            if(i == 4) {
                render_centered_string("!RESET STATS!",LCD_WIDTH/2,LCD_HEIGHT/4+FONT_HEIGHT*14);
            }
        }
        sprintf(fsa,"Highscore: Level %d",game.highscore);
        gfx_SetTextFGColor(0);
        gfx_SetTextScale(1,1);
        render_centered_string_outline(fsa,LCD_WIDTH/2,15,0,10);
        free(fsa);
    } else if(game.scene == 1) {
        char* fsa = malloc(strlen("Level 255"));
        render_background(false);
        gfx_SetTextScale(2,2);
        sprintf(fsa,"Level %d",game.level+1);
        render_centered_string(fsa,LCD_WIDTH/2,LCD_HEIGHT/4);
        free(fsa);
        render_centered_string("Press Enter To Start",LCD_WIDTH/2,LCD_HEIGHT-LCD_HEIGHT/4);
        gfx_SetTextScale(1,1);
    } else if(game.scene == 2) {
        uint24_t w,h;
        char* seconds;
        render_background(false);
        if(game.timing)  {
            //If the timer ever gets past 9999s then something must have gone severely wrong in this game's development.
            seconds = malloc(strlen("1337s"));
            sprintf(seconds,"%d",TIME_LIMIT-game.timer_seconds);
        } else {
            seconds = "ENTER";
        }
        w = LCD_WIDTH/game.playing_level->width;
        h = LCD_HEIGHT/game.playing_level->height;
        render_level();
        gfx_SetColor(11);
        gfx_Rectangle(game.cursor_pos2*w,game.cursor_pos*h,w,h);
        gfx_Rectangle(game.cursor_pos2*w-2,game.cursor_pos*h-2,w+4,h+4);
        gfx_SetColor(12);
        gfx_Rectangle(game.cursor_pos2*w-3,game.cursor_pos*h-3,w+6,h+6);

        gfx_SetColor(0);        
        gfx_PrintStringXY(seconds,3,3);
        if(game.timing) {
            free(seconds);
        }
    } else if(game.scene == 3) {
        char* fsa;
        render_background(false);
        //if 255 then it's a highscore, because I set the level to -1. If not then it's not a highscore
        if(game.level != 0xFF) {
            if(game.level <= 5) {
                //Plus one because the level could be more than two digits, it will be at most three.
                fsa = malloc(strlen(WIN_TEXT_LESS_FIVE)+1);
                sprintf(fsa,WIN_TEXT_LESS_FIVE,game.level);
            } else if(game.level <= 10) {
                fsa = malloc(strlen(WIN_TEXT_LESS_TEN)+1);
                sprintf(fsa,WIN_TEXT_LESS_TEN,game.level);
            } else { // if(game.level > 10). Just did an else rather than an else if to avoid compiler warnings
                fsa = malloc(strlen(WIN_TEXT_MORE_TEN)+1);
                sprintf(fsa,WIN_TEXT_MORE_TEN,game.level);
            }
        } else { //It's a highscore.
            fsa = malloc(strlen(WIN_TEXT_HIGHSCORE)+1);
            sprintf(fsa,WIN_TEXT_HIGHSCORE,game.highscore);
        }
        render_centered_string(fsa,LCD_WIDTH/2,LCD_HEIGHT/4);
        render_centered_string("Press Enter",LCD_WIDTH/2,LCD_HEIGHT*3/4);
        free(fsa);
    } else if(game.scene == 4) {
        render_background(true);
        gfx_SetTextFGColor(0);
        gfx_SetTextScale(2,2);
        render_centered_string("Game by slimeenergy",LCD_WIDTH/2,LCD_HEIGHT/2-FONT_HEIGHT*1.5);
        render_centered_string("Help by MateoC",LCD_WIDTH/2,LCD_HEIGHT/2+FONT_HEIGHT*1.5);
        gfx_SetTextScale(1,1);
    } else if(game.scene == 5) {
        render_background(false);
        gfx_SetTextFGColor(0);
        //If anyone wants to rewrite these directions to make sense that would be good.
        gfx_PrintStringXY("Use the arrow keys to navigate menus.",2,1);
        gfx_PrintStringXY("Press enter to leave the How To Play menu.",2,FONT_HEIGHT*1.5);
        gfx_PrintStringXY("Press enter to select an option.",2,FONT_HEIGHT*3);
        gfx_PrintStringXY("When in game, arrow keys move the cursor.",2,FONT_HEIGHT*4.5);
        gfx_PrintStringXY("Press 2nd on cells to mark/unmmark them.",2,FONT_HEIGHT*6);
        gfx_PrintStringXY("The goal is to mark cells that will be.",2,FONT_HEIGHT*7.5);
        gfx_PrintStringXY("alive in the next iteration. Cells that.",2,FONT_HEIGHT*9);
        gfx_PrintStringXY("are alive will carry onto the next level.",2,FONT_HEIGHT*10.5);
        gfx_PrintStringXY("Also, more cells appear per level.",2,FONT_HEIGHT*12);
        gfx_PrintStringXY("Press enter to skip the timer.",2,FONT_HEIGHT*13.5);
        gfx_PrintStringXY("Press enter once the timer runs out to",2,FONT_HEIGHT*15);
        gfx_PrintStringXY("go to the next level.",2,FONT_HEIGHT*16.5);
        gfx_PrintStringXY("When the timer runs out the world iterates",2,FONT_HEIGHT*18);
        gfx_PrintStringXY("so you may see if you guessed correctly.",2,FONT_HEIGHT*19.5);
        gfx_PrintStringXY("Press clear to quit at any time.",2,FONT_HEIGHT*22);
    }
}
//If fancy is true, a fancy and distracting background will be made. TODO: Make the fancy background. My idea would be a grid with tiles fading in and out.
void render_background(bool fancy) {
    //iterators
    uint24_t i;
    gfx_FillScreen(10); //white

    //The fancy background could probably be rewritten to be more efficient
    if(fancy) {
        //The fancy tile array
        static fancy_tile_t tiles[FANCY_TILE_COUNT] = 0;
        //Whether or not the fancy tile array has been initialized
        static bool initialized = false;
        if(!initialized) {
            //Give new tiles random positions and progresses
            for(i = 0; i < FANCY_TILE_COUNT; i++) {
                tiles[i].pos_x = randInt(0,7);
                tiles[i].pos_y = randInt(0,5);
                tiles[i].progress = randInt(0,255);
            }
            initialized = true;
        }
        //Update and render all fancy tiles
        for(i = 0; i < FANCY_TILE_COUNT; i++) {
            update_and_render_fancy_tile(&tiles[i]);
        }

        //Render grid
        gfx_SetColor(0);
        for(i = 0; i < 8; i++) {
            gfx_Rectangle(i*40,0,40,240);
        }
        for(i = 0; i < 6; i++) {
            gfx_Rectangle(0,i*40,320,40);
        }
    }
}
void render_centered_string(char* str,uint24_t x, uint8_t y) {
    gfx_PrintStringXY(str,x - (gfx_GetStringWidth(str)/2),y - (FONT_HEIGHT/2));
}
//Warning: This method is slow
void render_centered_string_outline(char* str,uint24_t x, uint8_t y, uint8_t fill,uint8_t stroke) {
    gfx_SetTextFGColor(stroke);
    gfx_PrintStringXY(str,x - (gfx_GetStringWidth(str)/2)-1,y - (FONT_HEIGHT/2));
    gfx_PrintStringXY(str,x - (gfx_GetStringWidth(str)/2)+1,y - (FONT_HEIGHT/2));
    gfx_PrintStringXY(str,x - (gfx_GetStringWidth(str)/2),y - (FONT_HEIGHT/2)-1);
    gfx_PrintStringXY(str,x - (gfx_GetStringWidth(str)/2),y - (FONT_HEIGHT/2)+1);
    gfx_SetTextFGColor(fill);
    gfx_PrintStringXY(str,x - (gfx_GetStringWidth(str)/2),y - (FONT_HEIGHT/2));
}
//This does one generation on the game.level currently being played. <!> It may not work <!>
void iterate_level() {
    //A bunch of iterators
    int24_t i,j,xp,yp;
    //A buffer to be read for counting neighbors
    bool* cells_buffer = malloc(game.playing_level->width * game.playing_level->height * sizeof(bool));

    //Copying cells to buffer
    //Hopefully this method works, I'm paranoid of memory overflows now.
    for(i = 0; i < game.playing_level->width*game.playing_level->height; i++) {
        cells_buffer[i] = game.playing_level->cells[i];
    }
    //Looping through cells
    for(i = 0; i < game.playing_level->height; i++) {
        for(j = 0; j < game.playing_level->width; j++) {
            uint8_t neighbors = 0;
            //Counting neighbors
            for(yp = max(0,i-1); yp < min(game.playing_level->height,i+2); yp++) {
                for(xp = max(0,j-1); xp < min(game.playing_level->width,j+2); xp++) {
                    if(!(xp == j && yp == i)) {
                        if(cells_buffer[yp*game.playing_level->width+xp] == true) {
                            neighbors++;
                        }
                    }
                }
            }
            //Killing underpopulated/overpopulated areas
            if(neighbors < 2 || neighbors > 3) {
                game.playing_level->cells[i*game.playing_level->width+j] = false;
            }
            //New births, thanks to three parents!
            if(neighbors == 3) {
                game.playing_level->cells[i*game.playing_level->width+j] = true;
            }
        }
    }
    //Free the buffer that was used.
    free(cells_buffer);
}
void clear_level(bool cells, bool selected) {
    uint24_t i;
    for(i = 0; i < game.playing_level->height*game.playing_level->width; i++) {
        if(cells) {
            game.playing_level->cells[i] = false;
        }
        if(selected) {
            game.playing_level->selected_cells[i] = false;
        }
    }
}
//Checks if all of the selected cells are alive, and that no dead cells are selected. 
bool compare_selection_level() {
    uint24_t i;
    for(i = 0; i < game.playing_level->height*game.playing_level->width; i++) {
        if(game.playing_level->cells[i] != game.playing_level->selected_cells[i]) {
            return false;
        }
    }
    return true;
}
//Returns a new game.level.
level_t* generate_level(uint8_t width, uint8_t height, uint24_t cell_count, bool one_birth) {
    uint24_t i,j;
    level_t* out = malloc(sizeof(level_t));
    out->width = width;
    out->height = height;
    out->cells = malloc(width*height*sizeof(bool));
    out->selected_cells = malloc(width*height*sizeof(bool));

    for(i = 0; i < width*height; i++) {
        out->cells[i] = false;
        out->selected_cells[i] = false;
    }
    if(one_birth) {
        uint24_t xp,yp;
        bool doneone=false;
        while(!doneone) {
            for(i = 0; i < width*height; i++) {
                out->cells[i] = false;
            }
            for(i = 0; i < min(255,cell_count); i++) {
                //Whether or not the cell has generated yet. The program will try to generate cells at random locations but won't generate if there's already a cell there. If done is false then there is already a cell there.
                bool done = false;

                while(!done) {
                    uint8_t x,y;
                    x = randInt(0,width);
                    y = randInt(0,height);
                    if(!out->cells[y*(width-1)+x]) {
                        out->cells[y*(width-1)+x] = true;
                        done = true;
                    }
                }
            }
            for(i = 0; i < height; i++) {
                for(j = 0; j < width; j++) {
                    uint8_t neighbors = 0;
                    //Counting neighbors
                    for(yp = max(0,i-1); yp < min(height,i+2); yp++) {
                        for(xp = max(0,j-1); xp < min(width,j+2); xp++) {
                            if(!(xp == j && yp == i)) {
                                if(out->cells[yp*width+xp] == true) {
                                    neighbors++;
                                }
                            }
                        }
                    }
                    if(neighbors == 3) {
                        doneone = true;
                        break;
                    }
                }
            }
        }
    } else {
        for(i = 0; i < min(255,cell_count); i++) {
            //Whether or not the cell has generated yet. The program will try to generate cells at random locations but won't generate if there's already a cell there. If done is false then there is already a cell there.
            bool done = false;
            while(!done) {
                uint8_t x,y;
                x = randInt(0,width);
                y = randInt(0,height);
                if(!out->cells[y*(width-1)+x]) {
                    out->cells[y*(width-1)+x] = true;
                    done = true;
                }
            }
        }
    }
    return out;
}
void add_cells_to_level(uint24_t cell_count) {
    uint24_t i;
    for(i = 0; i < cell_count; i++) {
        //Whether or not the cell has generated yet. The program will try to generate cells at random locations but won't generate if there's already a cell there. If done is false then there is already a cell there.
        //Yes, this code was copy and pasted from the generate_level function
        bool done = false;
        uint16_t tries = 0;
        while(!done && tries < 15) {
            uint8_t x,y;
            x = randInt(0,game.playing_level->width);
            y = randInt(0,game.playing_level->height);
            if(!game.playing_level->cells[y*(game.playing_level->width-1)+x]) {
                game.playing_level->cells[y*(game.playing_level->width-1)+x] = true;
                done = true;
            } else {
                tries++;
            }
        }
    }
}
//Frees the memory of the current playing level.
void destroy_level() {
    if(game.playing_level != NULL) {
        free(game.playing_level->selected_cells);
        free(game.playing_level->cells);
        free(game.playing_level);
    }
}
//Frees the memory of the current game.level, then generates a new game.level and sets "game.playing_level" to that new game.level.
void create_new_level(uint8_t width, uint8_t height, uint24_t cell_count) {
    destroy_level();
    game.playing_level = generate_level(width,height,cell_count,true);
}
void render_level() {
    uint24_t tile_width, j;
    uint8_t tile_height, i;
    tile_width = LCD_WIDTH/game.playing_level->width;
    tile_height = LCD_HEIGHT/game.playing_level->height;
    for(i = 0; i < game.playing_level->height; i++) {
        for(j = 0; j < game.playing_level->width; j++) {
            bool state = game.playing_level->cells[i*game.playing_level->width+j];
            bool state2 = game.playing_level->selected_cells[i*game.playing_level->width+j];
            //If state is true, then the cell is alive.
            if(state) {
                gfx_SetColor(1);
                if(state2) {
                    gfx_SetColor(12);
                }
                gfx_FillRectangle(j*tile_width,i*tile_height,tile_width,tile_height);
            } else if(state2) {
                gfx_SetColor(11);
                gfx_FillRectangle(j*tile_width,i*tile_height,tile_width,tile_height);
            }
        }
    }
    
    gfx_SetColor(0);
    for(i = 0; i < game.playing_level->width; i++) {
        gfx_Rectangle(i*tile_width,0,tile_width,240);
    }
    for(i = 0; i < game.playing_level->height; i++) {
        gfx_Rectangle(0,i*tile_height,320,tile_height);
    }
}

int24_t min(int24_t a, int24_t b) {
    if(a < b) {
        return a;
    }
    return b;
}
int24_t max(int24_t a, int24_t b) {
    if(a > b) {
        return a;
    }
    return b;
}

//These key_x_pressed functions probably need to be rewritten into one master function.
bool key_up_pressed() {
    static bool up = false;
    static uint8_t debounce = 0;
    static uint8_t individual_debounce = 0;
    if(kb_Data[7] == kb_Up) {
        debounce++;
        individual_debounce++;
        if(!up || (debounce >= KEY_DEBOUNCE_DELAY && individual_debounce >= KEY_DEBOUNCE)) {
            up = true;
            individual_debounce = 0;
            return true;
        }
        if(individual_debounce >= KEY_DEBOUNCE) {
            individual_debounce = 0;
        }
        //The reason I'm returning false before it reaches the end, is that I don't want UP to be false while the key is pressed.
        return false;
    }
    debounce = 0;
    individual_debounce = 0;
    up = false;
    return false;
}
bool key_down_pressed() {
    static bool down = false;
    static uint8_t debounce = 0;
    static uint8_t individual_debounce = 0;
    if(kb_Data[7] == kb_Down) {
        debounce++;
        individual_debounce++;
        if(!down || (debounce >= KEY_DEBOUNCE_DELAY && individual_debounce >= KEY_DEBOUNCE)) {
            down = true;
            individual_debounce = 0;
            return true;
        }
        if(individual_debounce >= KEY_DEBOUNCE) {
            individual_debounce = 0;
        }
        return false;
    }
    debounce = 0;
    individual_debounce = 0;
    down = false;
    return false;
}
bool key_left_pressed() {
    static bool left = false;
    static uint8_t debounce = 0;
    static uint8_t individual_debounce = 0;
    if(kb_Data[7] == kb_Left) {
        debounce++;
        individual_debounce++;
        if(!left || (debounce >= KEY_DEBOUNCE_DELAY && individual_debounce >= KEY_DEBOUNCE)) {
            left = true;
            individual_debounce = 0;
            return true;
        }
        if(individual_debounce >= KEY_DEBOUNCE) {
            individual_debounce = 0;
        }
        return false;
    }
    debounce = 0;
    individual_debounce = 0;
    left = false;
    return false;
}
bool key_right_pressed() {
    static bool right = false;
    static uint8_t debounce = 0;
    static uint8_t individual_debounce = 0;
    if(kb_Data[7] == kb_Right) {
        debounce++;
        individual_debounce++;
        if(!right || (debounce >= KEY_DEBOUNCE_DELAY && individual_debounce >= KEY_DEBOUNCE)) {
            right = true;

            individual_debounce = 0;
            return true;
        }
        if(individual_debounce >= KEY_DEBOUNCE) {
            individual_debounce = 0;
        }
        return false;
    }
    debounce = 0;
    individual_debounce = 0;
    right = false;
    return false;
}
bool key_enter_pressed() {
    static bool enter = false;
    if(kb_Data[6] == kb_Enter) {
        if(!enter) {
            enter = true;
            return true;
        }
        return false;
    }
    enter = false;
    return false;
}
bool key_second_pressed() {
    static bool second = false;
    if(kb_Data[1] == kb_2nd) {
        if(!second) {
            second = true;
            return true;
        }
        return false;
    }
    second = false;
    return false;
}
void next_level() {
    game.level++;
    if(game.level == 1) {
        create_new_level(16,12,game.level*ALIVE_CELLS_PER_LEVEL + ALIVE_CELLS_BASE);
    } else {
        uint24_t i;
        for(i = 0; i < game.playing_level->width*game.playing_level->height; i++) {
            game.playing_level->selected_cells[i] = false;
        }
        add_cells_to_level(game.level*ALIVE_CELLS_PER_LEVEL + ALIVE_CELLS_BASE);
    }
}

void start_timer() {
    game.timing = true;
    game.timer_seconds = 0;
    timer_Control = TIMER1_DISABLE;
    timer_1_Counter = 0;
    timer_Control = TIMER1_ENABLE | TIMER1_32K | TIMER1_NOINT | TIMER1_UP;
}
void update_timer() {
    if(timer_IntStatus & TIMER1_MATCH1) {
        game.timer_seconds++;
        timer_IntStatus = TIMER1_MATCH1;
        timer_Control = TIMER1_DISABLE;
        timer_1_Counter = 0;
        timer_Control = TIMER1_ENABLE | TIMER1_32K | TIMER1_NOINT | TIMER1_UP;
    }
}
void end_timer() {
    game.timing = false;
    timer_Control = TIMER1_DISABLE;
    timer_1_Counter = 0;
}

void update_and_render_fancy_tile(fancy_tile_t* tile) {
    //Progress will be 0-255. 128 will be when it's fully blue, going up from 0-128 will be becoming blue, then 128-255 will be going to white.
    //There are 10 different states of the gradient/animation. gfx_SetColor(01-10);
    //if progress <= 128, calculate the color with something similar to 10-round(progress/128*9+1). Otherwise, calculate the color with round((progress)/128*9+1).
    //If progress >= 255, then set a new position.
    uint8_t color_index = 0;
    if(tile->progress <= 128) {
        color_index = ceil(10-((tile->progress)/128.0f*9.0f));
    } else {
        color_index = floor((tile->progress-128)/128.0f*9.0f+1);
    }
    gfx_SetColor(color_index);
    gfx_FillRectangle(tile->pos_x*40,tile->pos_y*40,40,40);

    if(color_index == 10) {
        tile->pos_x = randInt(0,7);
        tile->pos_y = randInt(0,5);
    }

    tile->progress += FANCY_SPEED;
}
/*
Obselete; Function "randInt" provided by C libs.
uint8_t random_number(uint8_t minimum,uint8_t maximum) {
    return (random() % (maximum-minimum)) + minimum;
}*/

void set_default_stats() {
    unsigned int index, file, i;
    ti_CloseAll();
    
    file = ti_Open("agogolstt","a+");

    if(file) {
        ti_Rewind(file);
        ti_Read(&game.highscore,sizeof(uint16_t),1,file);
    } else {
        reset_stats();
    }
    ti_Close(file);
}
void save_stats() {
    unsigned int index, file, i;
    ti_CloseAll();

    file = ti_Open("agogolstt","a+");
    if(file) {
        ti_Rewind(file);
        ti_Write(&game.highscore,sizeof(uint16_t),1,file);
    }
    ti_Close(file);
}
void reset_stats() {
    unsigned int index, file, i;
    game.highscore = 0;
    ti_CloseAll();

    file = ti_Open("agogolstt","a+");
    if(file) {
        ti_Rewind(file);
        ti_Write(0,sizeof(uint16_t),1,file);
    }
    ti_Close(file);
}