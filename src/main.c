#include "raylib.h"
#include "stdio.h"
#include "string.h"

#define SCREEN_WIDTH 1200
#define SCREEN_HEIGHT 900

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

Piece controlled_piece = {.x=1, .y=1, .grid = PIECE_T};

void set_piece_shape(Piece *piece, const char *shape){
    strcpy(piece->grid, shape);
}

Color color_from_id(char id)
{
    Color color = MAGENTA;

    switch (id) {
        case '0':
            color = BLACK;
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

void rotate_piece(Piece* p) {
  char rotated_piece_shape[PIECE_GRID_LENGTH];
  for (int j = 0; j < PIECE_GRID_SIZE; j++) {
    for (int i = 0; i < PIECE_GRID_SIZE; i++) {
      int x = i;
      int y = j;
      rotated_piece_shape[y*PIECE_GRID_SIZE + x] = p->grid[j*PIECE_GRID_SIZE + i];
    }
  }
}

int main(void)
{
    const int screenWidth = SCREEN_WIDTH;
    const int screenHeight = SCREEN_HEIGHT;

    InitWindow(screenWidth, screenHeight, "Elsys Tetris");
    SetTargetFPS(60);

    float x = 100.0f;

    while (!WindowShouldClose())
    {
        // Update
        x += 2.0f;

        if (x > screenWidth + 50)
            x = -50;

        // Draw
        BeginDrawing();

        draw_game_grid(game_grid);

        draw_piece(controlled_piece);

        ClearBackground(RAYWHITE);

        // // Rectangle
        // DrawRectangle(50, 50, 200, 100, BLUE);
        //
        // // Circle
        // DrawCircle(400, 100, 50, RED);
        //
        // // Line
        // DrawLine(50, 200, 750, 200, DARKGRAY);
        //
        // // Triangle
        // DrawTriangle(
        //     (Vector2){600, 50},
        //     (Vector2){550, 150},
        //     (Vector2){650, 150},
        //     GREEN
        // );
        //
        // // Moving circle
        // DrawCircle((int)x, 300, 30, PURPLE);
        //
        // // Text
        // DrawText("Hello, raylib!", 50, 350, 30, DARKGRAY);
        DrawText("Press ESC to exit", 50, 390, 20, GRAY);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}
