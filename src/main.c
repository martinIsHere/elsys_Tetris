#include "raylib.h"
#include "stdio.h"
#include "string.h"
#include "memory.h"

#define SCREEN_WIDTH 1200
#define SCREEN_HEIGHT 900

#define FPS 60
#define TIME_PER_UPDATE 0.5 // seconds

#define PIECE_GRID_SIZE 4

#define GAME_GRID_WIDTH 8
#define GAME_GRID_HEIGHT 20

#define MIN(a, b) ((a) < (b) ? (a) : (b))

enum {
    PIECE_GRID_LENGTH = PIECE_GRID_SIZE*PIECE_GRID_SIZE + 1, // +1 for nullterminator
    GAME_GRID_LENGTH = GAME_GRID_WIDTH * GAME_GRID_HEIGHT + 1,
    BRICK_SIZE_PIXELS = MIN((int)(SCREEN_WIDTH / GAME_GRID_WIDTH), (int)(SCREEN_HEIGHT / GAME_GRID_HEIGHT)),
};

#define PIECE_I    "00000000IIII0000"
#define PIECE_O    "00000OO00OO00000"
#define PIECE_S    "00000SS00SS00000"
#define PIECE_J    "0000J000JJJ00000"
#define PIECE_L    "000000L0LLL00000"
#define PIECE_Z    "0000ZZ000ZZ00000"
#define PIECE_T    "00000T00TTT00000"
#define PIECE_NULL "0000000000000000"

typedef struct Piece {
  int x, y;
  char type;
  char grid[PIECE_GRID_LENGTH];
} Piece;

char game_grid[GAME_GRID_LENGTH] =
    "00000000"
    "00000000"
    "00000000"
    "00000000"
    "00000000"
    "00000000"
    "00000000"
    "00000000"
    "00000000"
    "00000000"
    "00000000"
    "00000000"
    "00000000"
    "00000000"
    "00000000"
    "000TT000"
    "000TT000"
    "0OO00LL0"
    "0OOJJLL0"
    "SSSZZZZ0";

Piece controlled_piece = {.x=1, .y=1, .type = 'Z', .grid = PIECE_Z};

void set_piece_shape(Piece *piece, const char *shape){
    strcpy(piece->grid, shape);
}

Color color_from_id(char id)
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


void draw_brick (const int x, const int y, const char id) {
  int padding = 2; // pixels
  DrawRectangle(x * BRICK_SIZE_PIXELS + padding, y * BRICK_SIZE_PIXELS + padding, BRICK_SIZE_PIXELS-padding, BRICK_SIZE_PIXELS - padding, color_from_id(id));
}

void draw_grid(const char* const grid, const size_t width, const size_t height, const int x, const int y) {
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
          draw_brick(i+x,j+y,grid[width*j+i]);
      }
  }
}

void draw_piece(const Piece p) {
  draw_grid(p.grid, PIECE_GRID_SIZE, PIECE_GRID_SIZE, p.x, p.y);
}

void draw_game_grid(const char game_grid[GAME_GRID_LENGTH]) {
  draw_grid(game_grid, GAME_GRID_WIDTH, GAME_GRID_HEIGHT,0,0);
}

bool check_overlap(Piece p, char game_grid[GAME_GRID_LENGTH]) {
  for (int j = 0; j < PIECE_GRID_SIZE; j++) {
    for (int i = 0; i < PIECE_GRID_SIZE; i++) {
      if (p.grid[j*PIECE_GRID_SIZE + i] == '0') continue;
      int xprime = i+p.x;
      int yprime = j+p.y;
      if (game_grid[(yprime)*GAME_GRID_WIDTH + (xprime)] != '0') return true;
      if (xprime < 0) return true;
      if (yprime < 0) return true;
      if (xprime >= GAME_GRID_WIDTH) return true;
      if (yprime >= GAME_GRID_HEIGHT) return true;
      
    }
  }
  return false;
}

void rotate_piece(Piece* p, char game_grid[GAME_GRID_LENGTH]) {
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
bool check_next_frame(Piece p, char game_grid[GAME_GRID_LENGTH]) {
  p.y += 1;
  return check_overlap(p, game_grid);
}

bool piece_above_screen(Piece p, char game_grid[GAME_GRID_LENGTH]){

}

void clear_lines(char game_grid[GAME_GRID_LENGTH]) {

}

void move_rows_down(char game_grid[GAME_GRID_LENGTH]) {

}

void add_piece_to_gamegrid(Piece p, char game_grid[GAME_GRID_LENGTH]) {
  for (int j = 0; j < PIECE_GRID_SIZE; j++) {
    for (int i = 0; i < PIECE_GRID_SIZE; i++) {
      if (p.grid[j*PIECE_GRID_SIZE + i] == '0') continue;
      game_grid[(j+p.y)*GAME_GRID_WIDTH + (i+p.x)] = p.grid[j*PIECE_GRID_SIZE + i];
    }
  }
}

void move_to_next_piece(Piece* controlled_piece, char game_grid[GAME_GRID_LENGTH]) {
    add_piece_to_gamegrid(*controlled_piece, game_grid);
    controlled_piece->x = 0;
    controlled_piece->y = 0;
    controlled_piece->type = 'I';
    set_piece_shape(controlled_piece, PIECE_I);
}

void update(){
  if (check_next_frame(controlled_piece, game_grid)) {
    move_to_next_piece(&controlled_piece, game_grid);
  }
  else controlled_piece.y ++;
}

int main(void)
{
    const int screenWidth = SCREEN_WIDTH;
    const int screenHeight = SCREEN_HEIGHT;

    double time_since_update = 0;

    InitWindow(screenWidth, screenHeight, "Elsys Tetris");
    SetTargetFPS(FPS);

    while (!WindowShouldClose())
    {

        time_since_update += GetFrameTime();
    printf("%f\n", time_since_update);

        if (time_since_update >= TIME_PER_UPDATE)
        {
            time_since_update -= TIME_PER_UPDATE;
            update();
        }

        if (IsKeyPressed(KEY_UP)) {
            rotate_piece(&controlled_piece, game_grid);
        }
        if (IsKeyPressed(KEY_DOWN)) {
            // smash piece down
        }

        if (IsKeyPressed(KEY_LEFT)) {
            controlled_piece.x --;
            if(check_overlap(controlled_piece, game_grid)) controlled_piece.x ++;
        }
        if (IsKeyPressed(KEY_RIGHT)) {
            controlled_piece.x ++;
            if(check_overlap(controlled_piece, game_grid)) controlled_piece.x --;
        }

        // Draw
        BeginDrawing();

        draw_game_grid(game_grid);

        draw_piece(controlled_piece);

        ClearBackground(BLACK);

        // Text
        DrawText("Hello, raylib!", 50, 350, 30, DARKGRAY);
        DrawText("Press ESC to exit", 50, 390, 20, GRAY);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}
