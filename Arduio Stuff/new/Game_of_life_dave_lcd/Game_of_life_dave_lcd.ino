//#include <ShiftRegister.h>


#include <SPI.h>
#include <Adafruit_GFX.h>
//#include <Max72xxPanel.h>
#include <Adafruit_PCD8544.h>

#define NUM_CYCLES_UNTIL_RESTART 60

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * These #defines declare the size of the overall display in panels  *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#define NUM_PANELS_PER_DISPLAY_ROW       (1)
#define NUM_PANELS_PER_DISPLAY_COL       (1)
#define NUM_COLUMNS_PER_FRAME_ROW        (8)
#define NUM_ROWS_PER_FRAME_COL           (8)
#define PIXELS_PER_MEMORY_WORD           (16) 
#define NUM_MEMORY_WORDS_IN_DISPLAY      ( NUM_COLUMNS_PER_FRAME_ROW     \
                                           * NUM_ROWS_PER_FRAME_COL      \
                                           * NUM_PANELS_PER_DISPLAY_ROW  \
                                           * NUM_PANELS_PER_DISPLAY_COL  \
                                           / PIXELS_PER_MEMORY_WORD )                                                                                   
                                          
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * These #defines are just aliases for the above for less verbosity  * 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#define M (NUM_COLUMNS_PER_FRAME_ROW)
#define N (NUM_ROWS_PER_FRAME_COL)
#define U (NUM_PANELS_PER_DISPLAY_ROW)
#define V (NUM_PANELS_PER_DISPLAY_COL)
#define W (PIXELS_PER_MEMORY_WORD)

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * These #defines are just useful derivations of the above           * 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#define NUM_DISPLAY_COLUMNS_PER_ROW   (M * U)
#define NUM_DISPLAY_ROWS_PER_COL      (N * V)

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * game_board is a flattened representation of the gameboard         *
 * where the index represents the row number, and the bit-           *
 * position within the word represents the column number. So,        *
 * for example, if rows and columns start at 0, and coordinate       *
 * (0, 0) is the upper left corner, and x increases to the right,    *
 * and y increases going down (this is normal display convention),   *
 * then coordinate (7, 12) is represented by game_board[i][7] bit 12 *
 *                                                                   *
 * The reason for it being flattened in this way is in order to most *
 * naturally interface with the shift register hardware to avoid     *
 * added software complexity.                                        *
 *                                                                   *
 * If the grid is larger than a single panel (16x16) then the array  *
 * can be simply be made larger and some math applied to figure out  * 
 * the effective coordinates of a cell based on the raster pattern.  * 
 * This software assumes the bottom-right corner of a panel is       *
 * logically connected to the top-left corner of the panel to its    * 
 * right, and that the left-most panel of a row connects analogously *
 * to the right-most panel of the row above it.                      * 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 *  PIXELS_PER_GAME_BOARD_WORD must be consistent with               *
 *  game_board data-type (e.g. uint16_t)                             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */                                                                       
uint16_t game_board[2][NUM_MEMORY_WORDS_IN_DISPLAY]; 
uint8_t  game_board_current_index = 0;
#define CURRENT_GAMEBOARD  (game_board_current_index)
#define PREVIOUS_GAMEBOARD (1 - game_board_current_index)

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * These macros and functions make the code more readable for        *
 * translating between flattened and grid-based coordinates.         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#define FPIXEL(R,C)(                   \
  (N * R) + C                          \
) 

#define DPIXEL(R,C)(                   \
  (U * N * R) + C                      \
) 

#define NORMR(R)(                      \
  R / N                                \
)

#define NORMC(C)(                      \
  C / M                                \
)

#define FRAME(R,C)(                    \
  NORMR(R) * U + NORMC(C)              \
)

#define R0(R,C)(                       \
  N * (FRAME(R,C) /  U)                \
)

#define C0(R,C)(                       \
  M * (FRAME(R,C) % U)                 \
)

#define RPRIME(R,C)(                   \
  R - R0(R,C)                          \
)

#define CPRIME(R,C)(                   \
  C - C0(R,C)                          \
)

#define GAME_BOARD_WORD_INDEX(R,C)(     \
  (FPIXEL(RPRIME(R,C),CPRIME(R,C)) / W) \
    + (FRAME(R,C) * M * N  / W) \
)

#define GAME_BOARD_PIXEL_OFFSET(R, C)( \
  FPIXEL(RPRIME(R,C),CPRIME(R,C)) % W  \
)

/* Functions that 'read' from the game board affect the 'foreground' memory*/

uint8_t get_board_value(uint16_t row, uint16_t col){
  return ( game_board[CURRENT_GAMEBOARD][ GAME_BOARD_WORD_INDEX(row, col) ]
    & _BV( GAME_BOARD_PIXEL_OFFSET(row, col) ) ) 
    >> GAME_BOARD_PIXEL_OFFSET(row, col);
}

/* Functions that 'write' to the game board affect the 'background' memory*/

void clear_board_location(uint16_t row, uint16_t col){
  game_board[PREVIOUS_GAMEBOARD][ GAME_BOARD_WORD_INDEX(row, col) ] &= 
    ~_BV(GAME_BOARD_PIXEL_OFFSET(row, col));
}

void set_board_location(uint16_t row, uint16_t col){
  game_board[PREVIOUS_GAMEBOARD][ GAME_BOARD_WORD_INDEX(row, col) ] |= 
    _BV(GAME_BOARD_PIXEL_OFFSET(row, col));       
}


/* function prototypes */
void print_current_game_board(void);
void calculate_next_game_board(void);
uint8_t get_neighbors(uint8_t row, uint8_t col);
uint8_t num_ones(uint8_t value);
void swap_buffers(void);

/* global parameter variables */
typedef enum{
  WASTELAND, /* everything outside the board is dead */
  PARADISE,  /* everything outside the board is alive */
  TOROIDAL,  /* board wraps around  through the edges */
  MIRROR,    /* board perimeter gets copied past the edges */
} boundary_condition_t;

boundary_condition_t boundary_condition = TOROIDAL;


/* * * * * * * * * * * * * * * * * * *
 * pin 5 = ShiftRegister.SER (data)  *
 * pin 7 = ShiftRegister.CLK (clock) *
 * pin 9 = ShiftRegister.RCK (latch) *
 * * * * * * * * * * * * * * * * * * */
// DAVE ShiftRegister sr;

// pin 12 (MISO)  is connected to the DataIn on the display
// pin 13 (MOSI) is connected to the CLK on the display
// pin 10 is connected to LOAD / CS on the display
int pinCS = 10; // Attach CS to this pin, DIN to MOSI and CLK to SCK (cf http://arduino.cc/en/Reference/SPI )

//Max72xxPanel matrix = Max72xxPanel(pinCS, NUM_PANELS_PER_DISPLAY_ROW, NUM_PANELS_PER_DISPLAY_COL);
Adafruit_PCD8544 matrix = Adafruit_PCD8544(3, 4, 5, 7, 6);

int cycle_count = 0;
int pattern_index = 0;

void setup(){
//matrix.setIntensity(1); // Set brightness between 0 and 15


  Serial.begin(115200);
  Serial.print(F("Display Columns/Row: "));
  Serial.println(NUM_DISPLAY_COLUMNS_PER_ROW);
  Serial.print(F("Display Rows/Col: "));
  Serial.println(NUM_DISPLAY_ROWS_PER_COL);  
  Serial.print(F(" # MEMORY WORDS: "));
  Serial.println(NUM_MEMORY_WORDS_IN_DISPLAY); 
  Serial.print(F("Sizeof(Gameboard)"));
  Serial.println(sizeof(game_board));

  randomSeed(analogRead(0));
  
  // initialize the game board
  memset(game_board, 0, sizeof(game_board)); 
 
  glider();
  
  swap_buffers();
  print_current_game_board();  
}

void loop(){    
  calculate_next_game_board(); 

  cycle_count++;
  if(NUM_CYCLES_UNTIL_RESTART == cycle_count){
    cycle_count = 0;
    memset(game_board, 0, sizeof(game_board)); 
    pattern_index = (pattern_index + 1) % 4;
    switch(pattern_index){
    case 0:
      Serial.println(F("GLI  DER"));
      glider();
      swap_buffers();
      glider();
      break;
    case 1:
      Serial.println(F("PUSLAR"));
      pulsar();
      swap_buffers();
      pulsar();
      break;
    case 2:
      Serial.println(F("BLINKER"));    
      blinker();
      swap_buffers();
      blinker();
      break;
    case 3: 
      Serial.println(F("RANDOM"));    
      random_board();
      swap_buffers();
      copy_buffer();
      break;
    }     
  }
  
  swap_buffers();              
  Serial.println();  
  delay(250);    
  print_current_game_board();   

  long time0 = micros();  
  // DAVE sr.loadValues(game_board[CURRENT_GAMEBOARD], 16);
  
  long time1 = micros();
  Serial.print(time1-time0);
  Serial.println(" microseconds");

}

uint8_t get_neighbors(uint8_t row, uint8_t col){
  uint8_t return_value = 0;
  int8_t r = 0, c = 0;
  uint8_t neighbor_value = 0;
  for(int8_t rr = -1; rr <= 1; rr++){
    for(int8_t cc = -1; cc <= 1; cc++){
      if(rr != 0 || cc != 0){ // you can't be your own neighbor     
        r = row + rr;
        c = col + cc;
    
        if( (r < 0) || (r >= NUM_DISPLAY_ROWS_PER_COL) || (c < 0) || (c >= NUM_DISPLAY_COLUMNS_PER_ROW)){
          switch(boundary_condition){
          case WASTELAND:
            neighbor_value = 0;
            break;
          case PARADISE:
            neighbor_value = 1;
            break;
          case TOROIDAL:
            if(r < 0) r = NUM_DISPLAY_ROWS_PER_COL - 1;
            else if(r >= NUM_DISPLAY_ROWS_PER_COL) r = 0;
            if(c < 0) c = NUM_DISPLAY_COLUMNS_PER_ROW - 1;
            else if(c >= NUM_DISPLAY_COLUMNS_PER_ROW) c = 0;
            neighbor_value = get_board_value(r, c); 
            break;
          case MIRROR:
            neighbor_value = get_board_value(rr, cc);       
            break;
          }      
        }
        else{
          neighbor_value = get_board_value(r, c);
        }
        
        return_value <<= 1; 
        return_value |= neighbor_value;                  
      }
    }   
  }
  
  return return_value;
}

uint8_t num_ones(uint8_t v){
  uint8_t c; // c accumulates the total bits set in v
  for (c = 0; v; c++)
  {
    v &= v - 1; // clear the least significant bit set
  }  
  return c; 
}

void print_current_game_board(void){
  for(uint16_t row = 0; row < NUM_DISPLAY_ROWS_PER_COL; row++){
    for(uint16_t col = 0; col < NUM_DISPLAY_COLUMNS_PER_ROW; col++){
      if(get_board_value(row, col)){
         Serial.print(F("*"));
         matrix.drawPixel(col, row, 1);
      }
      else{
         Serial.print(F("-"));        
         matrix.drawPixel(col, row, 0);
      }
      //Serial.print(F("   "));
    }
    Serial.println();
  }
      //matrix.write();
      matrix.display();
}



void calculate_next_game_board(void){
  for(uint16_t row = 0; row < NUM_DISPLAY_ROWS_PER_COL; row++){
    for(uint16_t col = 0; col < NUM_DISPLAY_COLUMNS_PER_ROW; col++){
      uint8_t neighbors = get_neighbors(row, col);
      uint8_t num_living_neighbors = num_ones(neighbors);
      uint8_t cell_is_alive = get_board_value(row, col);
      
      if(cell_is_alive){
        if(num_living_neighbors < 2 || num_living_neighbors > 3){ 
          clear_board_location(row, col); /* death */
        }              
        else{
          set_board_location(row, col); /* survival */      
        }
      }
      else{
        if(num_living_neighbors == 3){
          set_board_location(row, col); /* birth */    
        }
        else{
          clear_board_location(row, col); /* desolation */       
        }
      }
      
    }
  }  
}

void swap_buffers(void){
  CURRENT_GAMEBOARD = PREVIOUS_GAMEBOARD;
}

  /* block */
void block(){
  set_board_location(3,4);
  set_board_location(4,3);
  set_board_location(3,3);  
  set_board_location(4,4);  
}

  /* blinker */
void blinker(){
  set_board_location(6, 2);  
  set_board_location(6, 3);
  set_board_location(6, 4);  
}

/* glider */
void glider(){
  set_board_location(6, 2);  
  set_board_location(6, 3);
  set_board_location(6, 4);
  set_board_location(5, 4);  
  set_board_location(4, 3);    
}  

/* pulsar */
void pulsar(){
  set_board_location(1, 3);
  set_board_location(1, 4);
  set_board_location(1, 5);
  set_board_location(1, 9);
  set_board_location(1, 10);
  set_board_location(1, 11);
  set_board_location(3, 1);
  set_board_location(3, 6);
  set_board_location(3, 8);
  set_board_location(3, 13);
  set_board_location(4, 1);
  set_board_location(4, 6);
  set_board_location(4, 8);
  set_board_location(4, 13);
  set_board_location(5, 1);
  set_board_location(5, 6);
  set_board_location(5, 8);
  set_board_location(5, 13);
  set_board_location(6, 3);
  set_board_location(6, 4);
  set_board_location(6, 5);
  set_board_location(6, 9);
  set_board_location(6, 10);
  set_board_location(6, 11);  
  set_board_location(8, 3);
  set_board_location(8, 4);
  set_board_location(8, 5);
  set_board_location(8, 9);
  set_board_location(8, 10);
  set_board_location(8, 11);
  set_board_location(9, 1);
  set_board_location(9, 6);
  set_board_location(9, 8);
  set_board_location(9, 13);
  set_board_location(10, 1);
  set_board_location(10, 6);
  set_board_location(10, 8);
  set_board_location(10, 13);
  set_board_location(11, 1);
  set_board_location(11, 6);
  set_board_location(11, 8);
  set_board_location(11, 13);  
  set_board_location(13, 3);
  set_board_location(13, 4);
  set_board_location(13, 5);
  set_board_location(13, 9);
  set_board_location(13, 10);
  set_board_location(13, 11);  
}

void random_board(){
  for(uint16_t row = 0; row < NUM_DISPLAY_ROWS_PER_COL; row++){
    for(uint16_t col = 0; col < NUM_DISPLAY_COLUMNS_PER_ROW; col++){
      if(random(2)){
        set_board_location(row, col);
      }
      else{
        clear_board_location(row, col); 
      }
    }
  }
}

void copy_buffer(){
  for(uint16_t row = 0; row < NUM_DISPLAY_ROWS_PER_COL; row++){
    for(uint16_t col = 0; col < NUM_DISPLAY_COLUMNS_PER_ROW; col++){
      if(get_board_value(row, col)){
        set_board_location(row, col);
      }
      else{
        clear_board_location(row, col); 
      }
    }
  }    
}
