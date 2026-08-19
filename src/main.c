#include "raylib.h"
#include "stdio.h"
#include "string.h"
#include "memory.h"
#include <stdlib.h>
#include <time.h>

#define SCREEN_WIDTH 1200
#define SCREEN_HEIGHT 900

#define FPS 60
#define DEFAULT_TIME_PER_UPDATE 0.5 // seconds
#define DROP_INTERVAL_DECREMENT_PER_LEVEL 0.05 // seconds
#define MINIMUM_DROP_INTERVAL 0.1 // seconds
#define CLEARS_PER_LEVEL 5

#define SCORE_SLOW_FALL 1
#define SCORE_FAST_FALL 2
#define SCORE_1_LINE 100
#define SCORE_2_LINE 200
#define SCORE_3_LINE 400
#define SCORE_4_LINE 800

#define PIECE_GRID_SIZE 4

#define GAME_GRID_WIDTH 8
#define GAME_GRID_HEIGHT 10

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

enum {
    PIECE_GRID_LENGTH = PIECE_GRID_SIZE*PIECE_GRID_SIZE + 1, // +1 for nullterminator
    GAME_GRID_LENGTH = GAME_GRID_WIDTH * GAME_GRID_HEIGHT + 1,
    BRICK_SIZE_PIXELS = MIN((int)(SCREEN_WIDTH / GAME_GRID_WIDTH), (int)(SCREEN_HEIGHT / GAME_GRID_HEIGHT)),
};

enum {
  GAME_PLAYING,
  GAME_LOST
};

typedef int Gamestate;
typedef char Brick;

#define PIECE_I    "00000000IIII0000"
#define PIECE_O    "00000OO00OO00000"
#define PIECE_S    "00000SS000SS0000"
#define PIECE_J    "0000J000JJJ00000"
#define PIECE_L    "000000L0LLL00000"
#define PIECE_Z    "0000ZZ000ZZ00000"
#define PIECE_T    "00000T00TTT00000"
#define PIECE_NULL "0000000000000000"

const Brick *piece_shapes[] = {
    PIECE_I,
    PIECE_O,
    PIECE_T,
    PIECE_S,
    PIECE_Z,
    PIECE_J,
    PIECE_L
};

typedef struct Piece {
  int x, y;
  Brick type;
  Brick grid[PIECE_GRID_LENGTH];
} Piece;

typedef struct GameStats {
  int score;
  float drop_interval;
  int level;
  int lines_cleared;
} GameStats;

GameStats game_stats;

Brick game_grid[GAME_GRID_LENGTH];

Piece controlled_piece = {.x=2, .y=-3, .type = 'S', .grid = PIECE_S};

// position of the tetris playing area in pixels
int game_viewport_x;
int game_viewport_y;

int text_anchor_x;
int text_anchor_y;

void set_piece_shape(Piece *piece, const Brick *shape){
    strcpy(piece->grid, shape);
}

void randomize_piece_shape(Piece* p) {
    int index = rand() % 7;

    p->type = "IOTSZJL"[index];
    set_piece_shape(p, piece_shapes[index]);
    set_piece_shape(p, PIECE_I);
}

void zero_grid(Brick* grid, int length) {
  for (int i = 0; i < length; i++) {
    grid[i] = '0';
  }
}

Gamestate gamestate;

Color color_from_id(Brick id)
{
    Color color = MAGENTA;

    switch (id) {
        case '0':
            color = (Color){0, 0, 0, 0};
            break;
        case 'I':
            color = SKYBLUE;
            break;
        case 'O':
            color = YELLOW;
            break;
        case 'T':
            color = PURPLE;
            break;
        case 'S':
            color = GREEN;
            break;
        case 'Z':
            color = RED;
            break;
        case 'J':
            color = BLUE;
            break;
        case 'L':
            color = ORANGE;
            break;
    }

    return color;
}

// ============================================================
// SCORE & STATS
// ============================================================

void update_game_stats() {
  game_stats.level = (int)(game_stats.lines_cleared / CLEARS_PER_LEVEL);
  game_stats.drop_interval = DEFAULT_TIME_PER_UPDATE - DROP_INTERVAL_DECREMENT_PER_LEVEL*((float)game_stats.level);
  game_stats.drop_interval = MAX(MINIMUM_DROP_INTERVAL, game_stats.drop_interval);
}

void score_slow_fall(void) {
  game_stats.score += SCORE_SLOW_FALL;
}

void score_fast_fall(void) {
  game_stats.score += SCORE_FAST_FALL;
}

void score_line_clear(const int lines_cleared) {
  game_stats.lines_cleared += lines_cleared;
  update_game_stats();
  
  switch(lines_cleared) {
    case 0:
      break;
    case 1:
      game_stats.score+=SCORE_1_LINE;
    case 2:
      game_stats.score+=SCORE_2_LINE;
      break;
    case 3:
      game_stats.score+=SCORE_3_LINE;
      break;
    case 4:
      game_stats.score+=SCORE_4_LINE;
      break;
    default:
      break;
  }

}

// ============================================================
// LOGIC & MECHANICS
// ============================================================

bool check_overlap(Piece p, Brick game_grid[GAME_GRID_LENGTH]) {
  for (int j = 0; j < PIECE_GRID_SIZE; j++) {
    for (int i = 0; i < PIECE_GRID_SIZE; i++) {
      if (p.grid[j*PIECE_GRID_SIZE + i] == '0') continue;
      int xprime = i+p.x;
      int yprime = j+p.y;
      if (xprime < 0) return true;
      if (xprime >= GAME_GRID_WIDTH) return true;
      if (yprime >= GAME_GRID_HEIGHT) return true;
      if (yprime < 0) continue;
      if (game_grid[(yprime)*GAME_GRID_WIDTH + (xprime)] != '0') return true;
      
    }
  }
  return false;
}

void rotate_piece(Piece* p, Brick game_grid[GAME_GRID_LENGTH]) {
  float rotcen_x = 1.5f; // rotation center
  float rotcen_y = 1.5f; // rotation center
  Piece rotated_piece = {.x = p->x, .y = p->y};
  for (int j = 0; j < PIECE_GRID_SIZE; j++) {
    for (int i = 0; i < PIECE_GRID_SIZE; i++) {
      float x = j - rotcen_y;
      float y = -i + rotcen_x;
      rotated_piece.grid[(int)(y + rotcen_y) * PIECE_GRID_SIZE + (int)(x + rotcen_x)] = p->grid[j * PIECE_GRID_SIZE + i];
    }
  }
  if(check_overlap(rotated_piece, game_grid)) return;
  strcpy(p->grid, rotated_piece.grid);
}

// true if piece is finished
// false if falling
bool check_next_frame(Piece p, Brick game_grid[GAME_GRID_LENGTH]) {
  p.y += 1;
  return check_overlap(p, game_grid);
}

void delete_row(int row, Brick game_grid[GAME_GRID_LENGTH]) {
  for (int i = 0; i < GAME_GRID_WIDTH; i++) {
    game_grid[row*GAME_GRID_WIDTH + i] = '0';
  }
}

void move_rows_down(int row, Brick game_grid[GAME_GRID_LENGTH]) {
  Brick copy [GAME_GRID_LENGTH];
  strcpy(copy, game_grid);
  for (int i = 0; i < GAME_GRID_WIDTH; i ++) {
    game_grid[i] = '0';
  }
  for (int j = 0; j < row; j++) {
    for (int i = 0; i < GAME_GRID_WIDTH; i++) {
      game_grid[(j+1) * GAME_GRID_WIDTH + i] = copy[j * GAME_GRID_WIDTH + i];
    }
  }
}

int clear_lines(Brick game_grid[GAME_GRID_LENGTH]) {
  int lines_cleared = 0;
  for (int row = 0; row < GAME_GRID_HEIGHT; row++) {
    for (int i = 0; i < GAME_GRID_WIDTH; i ++) {
      if(game_grid[row*GAME_GRID_WIDTH + i] == '0') break; // break if not a brick
      if (i == GAME_GRID_WIDTH-1) {
        delete_row(row, game_grid);
        move_rows_down(row, game_grid); // move rows above down
        lines_cleared++;
        row = 0; i = 0; // restart
      } // delete row if the last tile is a brick
    }
  }
  return lines_cleared;
}

// true if lost, false if nothing
bool add_piece_to_gamegrid(Piece p, Brick game_grid[GAME_GRID_LENGTH]) {
  for (int j = 0; j < PIECE_GRID_SIZE; j++) {
    for (int i = 0; i < PIECE_GRID_SIZE; i++) {
      if (p.grid[j*PIECE_GRID_SIZE + i] == '0') continue;
      if (j+p.y < 0) return true;
      if (i+p.x < 0) continue;
      if (i+p.x >= GAME_GRID_WIDTH) continue;
      if (j+p.y >= GAME_GRID_HEIGHT) continue;
      game_grid[(j+p.y)*GAME_GRID_WIDTH + (i+p.x)] = p.grid[j*PIECE_GRID_SIZE + i];
    }
  }
  int lines_cleared = clear_lines(game_grid);
  score_line_clear(lines_cleared);
  return false;
}

// true if lost, false if nothing
bool move_to_next_piece(Piece* controlled_piece, Brick game_grid[GAME_GRID_LENGTH]) {
    if(add_piece_to_gamegrid(*controlled_piece, game_grid)) return true;
    controlled_piece->x = 2;
    controlled_piece->y = -3;
    randomize_piece_shape(controlled_piece);
    return false;
}

void smash(Piece *p, Brick game_grid[GAME_GRID_LENGTH]){
  while (!check_next_frame(*p, game_grid)) {
    p->y++;
    score_fast_fall();
  }
  if (move_to_next_piece(p, game_grid)) gamestate = GAME_LOST;
}

// ============================================================
// DRAW
// ============================================================

void draw_brick (const int x_in, const int y_in, const Brick id, bool ghost) {
  int padding = 2; // pixels
  int x = x_in * BRICK_SIZE_PIXELS + game_viewport_x + padding;
  int y = y_in * BRICK_SIZE_PIXELS + game_viewport_y + padding;
  int width = BRICK_SIZE_PIXELS - padding;
  int height = BRICK_SIZE_PIXELS - padding;
  if (ghost) {
    if (id == '0') return;
    DrawRectangle(x, y, width,height, (Color){255,255,255,100});
    return;
  }
  DrawRectangle(x, y, width,height, color_from_id(id));
}

void draw_grid(const Brick* const grid, const size_t width, const size_t height, const int x, const int y, bool ghost) {
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
          draw_brick(i+x,j+y,grid[width*j+i], ghost);
      }
  }
}

void draw_piece(const Piece p, bool ghost) {
  draw_grid(p.grid, PIECE_GRID_SIZE, PIECE_GRID_SIZE, p.x, p.y, ghost);
}

void draw_game_grid(const Brick game_grid[GAME_GRID_LENGTH]) {
  draw_grid(game_grid, GAME_GRID_WIDTH, GAME_GRID_HEIGHT,0,0, false);
}

void draw_drop_line(const Piece p) {
  int x_grid = p.x+(int)(PIECE_GRID_SIZE/2);
  int y_grid = p.y+(int)(PIECE_GRID_SIZE/2);
  int x1 = x_grid*BRICK_SIZE_PIXELS + game_viewport_x;
  int y1 = y_grid*BRICK_SIZE_PIXELS + game_viewport_y;
  int x2 = x1;
  int y2 = GAME_GRID_HEIGHT*BRICK_SIZE_PIXELS + game_viewport_y;
  DrawLine(x1,y1,x2,y2, WHITE);
}

void draw_borders() {
  DrawRectangleLinesEx(
      (Rectangle){
          (float)game_viewport_x,
          (float)game_viewport_y,
          GAME_GRID_WIDTH * BRICK_SIZE_PIXELS,
          GAME_GRID_HEIGHT * BRICK_SIZE_PIXELS
      },
      4,
      WHITE
  );
}

void draw_prediction_piece(Piece p) {
  while (!check_next_frame(p, game_grid)) {
    p.y++;
  }
  draw_piece(p, true);
}


// ============================================================
// update & main
// ============================================================

void update(){
  if (check_next_frame(controlled_piece, game_grid)) {
    if (move_to_next_piece(&controlled_piece, game_grid)) gamestate = GAME_LOST;
  }
  else controlled_piece.y ++;
  score_slow_fall();
}

int main(void)
{
    srand(time(NULL)); // seed time for RNG
    const int screenWidth = SCREEN_WIDTH;
    const int screenHeight = SCREEN_HEIGHT;

    zero_grid(game_grid, GAME_GRID_LENGTH);

    double time_since_update = 0;
    gamestate = GAME_PLAYING;
    game_stats.score = 0;
    game_stats.level = 0;
    game_stats.lines_cleared = 0;
    game_stats.drop_interval = DEFAULT_TIME_PER_UPDATE;


    game_viewport_x = 100;
    game_viewport_y = 0;

    text_anchor_x = game_viewport_x + GAME_GRID_WIDTH * BRICK_SIZE_PIXELS;
    text_anchor_y = game_viewport_y;

    randomize_piece_shape(&controlled_piece);

    InitWindow(screenWidth, screenHeight, "Elsys Tetris");
    SetTargetFPS(FPS);

    while (!WindowShouldClose())
    {
        if (gamestate == GAME_PLAYING) {


          time_since_update += GetFrameTime();

          if (time_since_update >= game_stats.drop_interval && gamestate == GAME_PLAYING)
          {
              time_since_update -= game_stats.drop_interval;
              update();
          }

          if (IsKeyPressed(KEY_UP)) {
              rotate_piece(&controlled_piece, game_grid);
          }
          if (IsKeyPressed(KEY_DOWN)) {
              smash(&controlled_piece, game_grid);
          }

          if (IsKeyPressed(KEY_LEFT)) {
              controlled_piece.x --;
              if(check_overlap(controlled_piece, game_grid)) controlled_piece.x ++;
          }
          if (IsKeyPressed(KEY_RIGHT)) {
              controlled_piece.x ++;
              if(check_overlap(controlled_piece, game_grid)) controlled_piece.x --;
          }
        }

        // Draw
        BeginDrawing();

        draw_game_grid(game_grid);

        draw_borders();

        draw_drop_line(controlled_piece);

        draw_prediction_piece(controlled_piece);

        draw_piece(controlled_piece, false);

        ClearBackground(BLACK);

        // Text
        if (gamestate == GAME_LOST) DrawText("YOU LOST", text_anchor_x + 50, text_anchor_y + 350, 30, RED);
        DrawText("Press ESC to exit", text_anchor_x + 50, text_anchor_y + 10, 20, GRAY);
        DrawText(TextFormat("Score: %d", game_stats.score), text_anchor_x + 50, text_anchor_y + 50, 30, YELLOW);
        DrawText(TextFormat("LEVEL: %d", game_stats.level), text_anchor_x + 50, text_anchor_y + 80, 30, YELLOW);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}
