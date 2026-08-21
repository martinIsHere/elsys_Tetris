#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "raylib.h"
#include "stdio.h"
#include "string.h"

/*
 * TODO:
 * add animation *
 * add music
 * add soundfx
 *
 */

#define SCREEN_WIDTH 1100
#define SCREEN_HEIGHT 900

#define ENABLE_SHAKE true
#define ENABLE_LIGHT true
#define ENABLE_LINE_CLEAR_ANIMATION true
#define ENABLE_FALL_LINE true
#define ENABLE_PREDICTION true

#define FPS 60
#define DEFAULT_TIME_PER_UPDATE 0.5            // seconds
#define DEFAULT_TIME_PER_ANIMATION_UPDATE 0.05 // seconds
#define DROP_INTERVAL_DECREMENT_PER_LEVEL 0.05 // seconds
#define MINIMUM_DROP_INTERVAL 0.1              // seconds
#define CLEARS_PER_LEVEL 5

#define SCORE_SLOW_FALL 1
#define SCORE_FAST_FALL 2
#define SCORE_1_LINE 100
#define SCORE_2_LINE 200
#define SCORE_3_LINE 400
#define SCORE_4_LINE 800

#define AMOUNT_OF_SWITCHES_PER_DROP 1

#define PIECE_GRID_SIZE 4

#define GAME_GRID_WIDTH 10
#define GAME_GRID_HEIGHT 20

#define DEFAULT_GGVIEWPORT_X 300
#define DEFAULT_GGVIEWPORT_Y 0

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

enum {
  PIECE_GRID_LENGTH =
      PIECE_GRID_SIZE * PIECE_GRID_SIZE + 1, // +1 for nullterminator
  GAME_GRID_LENGTH = GAME_GRID_WIDTH * GAME_GRID_HEIGHT + 1,
  BRICK_SIZE_PIXELS = MIN((int)(SCREEN_WIDTH / GAME_GRID_WIDTH),
                          (int)(SCREEN_HEIGHT / GAME_GRID_HEIGHT)),
};

enum {
  GAME_PLAYING,
  GAME_LOST,
  GAME_ANIMATION_LINE_CLEAR1,
  GAME_ANIMATION_SMASH,
};

typedef int Gamestate;
typedef char Brick;

#define PIECE_I "00000000IIII0000"
#define PIECE_O "00000OO00OO00000"
#define PIECE_S "00000SS000SS0000"
#define PIECE_J "0000J000JJJ00000"
#define PIECE_L "000000L0LLL00000"
#define PIECE_Z "000000ZZ0ZZ00000"
#define PIECE_T "00000T00TTT00000"
#define PIECE_NULL "0000000000000000"

const Brick *piece_shapes[] = {PIECE_I, PIECE_O, PIECE_T, PIECE_S,
                               PIECE_Z, PIECE_J, PIECE_L};

typedef struct Piece {
  int x, y;
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
Brick game_modification_grid[GAME_GRID_LENGTH];

Piece controlled_piece;
Brick next_piece_grid[PIECE_GRID_LENGTH];

// position of the tetris playing area in pixels
int game_viewport_x;
int game_viewport_y;

int text_anchor_x;
int text_anchor_y;

int next_piece_anchor_x;
int next_piece_anchor_y;

double time_since_update;
double time_since_animation_update;
double time_since_animation_start;

int animation_lines_being_cleared;

int switch_count = 0;

typedef struct SoundFX {
  Sound move;
  Sound rotate;
  Sound place;
  Sound line_clear;
  Sound tetris;
  Sound game_over;
} SoundFX;

typedef struct Musica {
  Music track1;
} Musica;

SoundFX soundfx;
Musica playlist;

bool has_audio_output;

void set_piece_shape(Piece *piece, const Brick *shape) {
  strcpy(piece->grid, shape);
}

void randomize_shape(Brick *grid) {
  int index = rand() % 7;

  strcpy(grid, piece_shapes[index]);
}

void zero_grid(Brick *grid, int length) {
  for (int i = 0; i < length - 1; i++) { // -1 to not destroy \0
    grid[i] = '0';
  }
}

Gamestate gamestate;

Color color_wt(int r, int g, int b, float intensity) {
  return (Color){(unsigned char)(r * intensity), (unsigned char)(g * intensity),
                 (unsigned char)(b * intensity), 255};
}

Color color_from_id(Brick id, float intensity) {
  Color color = MAGENTA;

  switch (id) {
  case '0':
    color = (Color){0, 0, 0, 0};
    break;
  case 'i':
    color = SKYBLUE;
    break;
  case 'o':
    color = YELLOW;
    break;
  case 't':
    color = PURPLE;
    break;
  case 's':
    color = GREEN;
    break;
  case 'z':
    color = RED;
    break;
  case 'j':
    color = BLUE;
    break;
  case 'l':
    color = ORANGE;
    break;
  case 'I':
    color = color_wt(100, 220, 255, intensity);
    break;
  case 'O':
    color = color_wt(255, 255, 100, intensity);
    break;
  case 'T':
    color = color_wt(220, 100, 255, intensity);
    break;
  case 'S':
    color = color_wt(120, 255, 100, intensity);
    break;
  case 'Z':
    color = color_wt(255, 100, 100, intensity);
    break;
  case 'J':
    color = color_wt(100, 150, 255, intensity);
    break;
  case 'L':
    color = color_wt(255, 180, 70, intensity);
    break;
  }

  return color;
}

// ============================================================
// MATH
// ============================================================

// output: pixel offset of game_grid
// intensity = 1 is default for 1 line
int game_grid_shake_offset(double time_seconds, double intensity) {
  // constants
  double omega = 4000 * intensity + 4000;
  double phi = 0;
  double A = 8 * intensity + 5;
  double c = 0;

  return (int)(A * sin(omega * time_seconds + phi) + c);
}

double time_per_animation_update(int amount_of_lines) {
  return DEFAULT_TIME_PER_ANIMATION_UPDATE +
         amount_of_lines * DEFAULT_TIME_PER_ANIMATION_UPDATE * 0.1;
}

// ============================================================
// SHADER
// ============================================================

Shader glow_shader;
RenderTexture2D game_target;

const char *glow_shader_code =
    "#version 330\n"

    "in vec2 fragTexCoord;\n"
    "in vec4 fragColor;\n"

    "uniform sampler2D texture0;\n"
    "uniform vec2 resolution;\n"

    "out vec4 finalColor;\n"

    "void main()\n"
    "{\n"
    "    vec2 texel = 1.0 / resolution;\n"

    "    vec4 center = texture(texture0, fragTexCoord);\n"

    "    vec4 glow = vec4(0.0);\n"
    "    const float GLOW_RADIUS = 6.0;\n"
    "    glow += texture(texture0, fragTexCoord + texel * vec2(-GLOW_RADIUS, "
    "-GLOW_RADIUS));\n"
    "    glow += texture(texture0, fragTexCoord + texel * vec2( 0.0,        "
    "-GLOW_RADIUS));\n"
    "    glow += texture(texture0, fragTexCoord + texel * vec2( GLOW_RADIUS, "
    "-GLOW_RADIUS));\n"
    "    glow += texture(texture0, fragTexCoord + texel * vec2(-GLOW_RADIUS,  "
    "0.0));\n"
    "    glow += texture(texture0, fragTexCoord + texel * vec2( GLOW_RADIUS,  "
    "0.0));\n"
    "    glow += texture(texture0, fragTexCoord + texel * vec2(-GLOW_RADIUS,  "
    "GLOW_RADIUS));\n"
    "    glow += texture(texture0, fragTexCoord + texel * vec2( 0.0,         "
    "GLOW_RADIUS));\n"
    "    glow += texture(texture0, fragTexCoord + texel * vec2( GLOW_RADIUS,  "
    "GLOW_RADIUS));\n"

    "    glow /= 8.0;\n"

    "    finalColor = center + glow * 0.35;\n"
    "}\n";

// ============================================================
// SCORE & STATS
// ============================================================

void update_game_stats() {
  game_stats.level = (int)(game_stats.lines_cleared / CLEARS_PER_LEVEL);
  game_stats.drop_interval =
      DEFAULT_TIME_PER_UPDATE -
      DROP_INTERVAL_DECREMENT_PER_LEVEL * ((float)game_stats.level);
  game_stats.drop_interval =
      MAX(MINIMUM_DROP_INTERVAL, game_stats.drop_interval);
}

void score_slow_fall(void) { game_stats.score += SCORE_SLOW_FALL; }

void score_fast_fall(void) { game_stats.score += SCORE_FAST_FALL; }

void score_line_clear(const int lines_cleared) {
  game_stats.lines_cleared += lines_cleared;
  update_game_stats();

  switch (lines_cleared) {
  case 0:
    break;
  case 1:
    game_stats.score += SCORE_1_LINE;
    PlaySound(soundfx.line_clear);
    break;
  case 2:
    game_stats.score += SCORE_2_LINE;
    PlaySound(soundfx.line_clear);
    break;
  case 3:
    game_stats.score += SCORE_3_LINE;
    PlaySound(soundfx.line_clear);
    break;
  case 4:
    game_stats.score += SCORE_4_LINE;
    PlaySound(soundfx.tetris);
    break;
  default:
    break;
  }
}

// ============================================================
// LOGIC & MECHANICS
// ============================================================
// for. dec.
void lose();

bool check_overlap(Piece p, Brick game_grid[GAME_GRID_LENGTH]) {
  for (int j = 0; j < PIECE_GRID_SIZE; j++) {
    for (int i = 0; i < PIECE_GRID_SIZE; i++) {
      if (p.grid[j * PIECE_GRID_SIZE + i] == '0')
        continue;
      int xprime = i + p.x;
      int yprime = j + p.y;
      if (xprime < 0)
        return true;
      if (xprime >= GAME_GRID_WIDTH)
        return true;
      if (yprime >= GAME_GRID_HEIGHT)
        return true;
      if (yprime < 0)
        continue;
      if (game_grid[(yprime)*GAME_GRID_WIDTH + (xprime)] != '0')
        return true;
    }
  }
  return false;
}

void rotate_piece(Piece *p, Brick game_grid[GAME_GRID_LENGTH]) {
  float rotcen_x = 1.5f; // rotation center
  float rotcen_y = 1.5f; // rotation center
  Piece rotated_piece = {.x = p->x, .y = p->y};
  for (int j = 0; j < PIECE_GRID_SIZE; j++) {
    for (int i = 0; i < PIECE_GRID_SIZE; i++) {
      float x = j - rotcen_y;
      float y = -i + rotcen_x;
      rotated_piece
          .grid[(int)(y + rotcen_y) * PIECE_GRID_SIZE + (int)(x + rotcen_x)] =
          p->grid[j * PIECE_GRID_SIZE + i];
    }
  }
  if (check_overlap(rotated_piece, game_grid))
    return;
  strcpy(p->grid, rotated_piece.grid);
}

// true if piece is finished
// false if falling
bool check_next_frame(Piece p, Brick game_grid[GAME_GRID_LENGTH]) {
  p.y += 1;
  return check_overlap(p, game_grid);
}

void mark_row_for_deletion(int row, Brick game_grid[GAME_GRID_LENGTH],
                           Brick game_modification_grid[GAME_GRID_LENGTH]) {
  for (int i = 0; i < GAME_GRID_WIDTH; i++) {
    game_modification_grid[row * GAME_GRID_WIDTH + i] =
        'X'; // indicates that the corrosponding brick is to be removed
  }
}

void move_rows_down(Brick game_grid[GAME_GRID_LENGTH],
                    Brick in_game_modification_grid[GAME_GRID_LENGTH]) {
  /*
   * WARNING! RECURSIVE DESIGN
   *
   */
  Brick copy[GAME_GRID_LENGTH];
  strcpy(copy, game_grid);

  Brick game_modification_grid[GAME_GRID_LENGTH];
  strcpy(game_modification_grid, in_game_modification_grid);
  int amount_of_removed_bricks = 0;
  int y_of_lowest_removed_brick = -1;
  for (int j = 0; j < GAME_GRID_HEIGHT; j++) {
    if (game_modification_grid[j * GAME_GRID_WIDTH] != 'X') {
      if (amount_of_removed_bricks == 0)
        continue;
      break;
    }
    amount_of_removed_bricks++;
    y_of_lowest_removed_brick = j;
  }
  for (int j = 0; j < y_of_lowest_removed_brick + 1 - amount_of_removed_bricks;
       j++) {
    for (int i = 0; i < GAME_GRID_WIDTH; i++) {
      copy[(j + amount_of_removed_bricks) * GAME_GRID_WIDTH + i] =
          game_grid[j * GAME_GRID_WIDTH + i];
      game_modification_grid[(j + amount_of_removed_bricks) * GAME_GRID_WIDTH +
                             i] = '0';
    }
  }

  // apply changes
  strcpy(game_grid, copy);

  // move remaining rows
  // TODO: remove shitty recursion solution
  if (amount_of_removed_bricks != 0)
    move_rows_down(game_grid, game_modification_grid);
}

int clear_lines(Brick game_grid[GAME_GRID_LENGTH],
                Brick game_modification_grid[GAME_GRID_LENGTH]) {
  int lines_cleared = 0;
  for (int row = 0; row < GAME_GRID_HEIGHT; row++) {
    for (int i = 0; i < GAME_GRID_WIDTH; i++) {
      // check if space contains a brick, or if a brick has been removed (i.e.
      // empty)
      if (game_grid[row * GAME_GRID_WIDTH + i] == '0' ||
          game_modification_grid[row * GAME_GRID_WIDTH + i] == 'X')
        break; // break if not a brick
      if (i == GAME_GRID_WIDTH - 1) {
        mark_row_for_deletion(row, game_grid, game_modification_grid);
        // move rows above down
        lines_cleared++;
        row = 0;
        i = 0; // restart
      } // delete row if the last tile is a brick
    }
  }
  // change to animation state
  return lines_cleared;
}

// true if lost, false if nothing
bool add_piece_to_gamegrid(Piece p, Brick game_grid[GAME_GRID_LENGTH]) {
  for (int j = 0; j < PIECE_GRID_SIZE; j++) {
    for (int i = 0; i < PIECE_GRID_SIZE; i++) {
      if (p.grid[j * PIECE_GRID_SIZE + i] == '0')
        continue;
      if (j + p.y < 0)
        return true;
      if (i + p.x < 0)
        continue;
      if (i + p.x >= GAME_GRID_WIDTH)
        continue;
      if (j + p.y >= GAME_GRID_HEIGHT)
        continue;
      game_grid[(j + p.y) * GAME_GRID_WIDTH + (i + p.x)] =
          p.grid[j * PIECE_GRID_SIZE + i];
    }
  }
  int lines_cleared = clear_lines(game_grid, game_modification_grid);
  if (lines_cleared != 0) {
    gamestate = GAME_ANIMATION_LINE_CLEAR1;
    animation_lines_being_cleared = lines_cleared;
  }
  score_line_clear(lines_cleared);
  return false;
}

void reset_piece_position(Piece *p) {
  p->x = 2;
  p->y = -3;
}

// true if lost, false if nothing
bool move_to_next_piece(Piece *controlled_piece,
                        Brick game_grid[GAME_GRID_LENGTH]) {
  if (add_piece_to_gamegrid(*controlled_piece, game_grid))
    return true;
  reset_piece_position(controlled_piece);
  randomize_shape(controlled_piece->grid);
  switch_count = 0;
  return false;
}

void smash(Piece *p, Brick game_grid[GAME_GRID_LENGTH]) {
  while (!check_next_frame(*p, game_grid)) {
    p->y++;
    score_fast_fall();
  }
  if (move_to_next_piece(p, game_grid))
    lose();
}

void switch_controlled_piece() {
  if (switch_count >= AMOUNT_OF_SWITCHES_PER_DROP)
    return;
  switch_count++;
  Brick buffer[PIECE_GRID_LENGTH];
  strcpy(buffer, controlled_piece.grid);
  strcpy(controlled_piece.grid, next_piece_grid);
  strcpy(next_piece_grid, buffer);
  reset_piece_position(&controlled_piece);
}

// ============================================================
// DRAW
// ============================================================

void draw_brick_on_grid(const int x_in, const int y_in, const Brick id,
                        bool ghost) {
  int padding = 2; // pixels
  int x = x_in * BRICK_SIZE_PIXELS + game_viewport_x + padding;
  int y = y_in * BRICK_SIZE_PIXELS + game_viewport_y + padding;
  int width = BRICK_SIZE_PIXELS - padding;
  int height = BRICK_SIZE_PIXELS - padding;
  if (ghost) {
    if (id == '0')
      return;
    DrawRectangle(x, y, width, height, (Color){255, 255, 255, 100});
    return;
  }
  DrawRectangle(x, y, width, height, color_from_id(id, 1));
}

void draw_brick(const int x, const int y, const int width, const int height,
                const Brick id) {
  DrawRectangle(x, y, width, height, color_from_id(id, 1));
}

void draw_grid_on_grid(const Brick *const grid, const size_t width,
                       const size_t height, const int x, const int y,
                       bool ghost) {
  for (int j = 0; j < height; j++) {
    for (int i = 0; i < width; i++) {
      draw_brick_on_grid(i + x, j + y, grid[width * j + i], ghost);
    }
  }
}

void draw_grid(const Brick *const grid, const size_t width, const size_t height,
               const size_t size_pixels, const int x, const int y) {
  for (int j = 0; j < height; j++) {
    for (int i = 0; i < width; i++) {
      draw_brick(i * size_pixels + x, j * size_pixels + y, size_pixels,
                 size_pixels, grid[width * j + i]);
    }
  }
}

void draw_piece_on_grid(const Piece p, bool ghost) {
  draw_grid_on_grid(p.grid, PIECE_GRID_SIZE, PIECE_GRID_SIZE, p.x, p.y, ghost);
}

void draw_game_grid(const Brick game_grid[GAME_GRID_LENGTH]) {
  draw_grid_on_grid(game_grid, GAME_GRID_WIDTH, GAME_GRID_HEIGHT, 0, 0, false);
}

void draw_drop_line(const Piece p) {
  int x_grid = p.x + (int)(PIECE_GRID_SIZE / 2);
  int y_grid = p.y + (int)(PIECE_GRID_SIZE / 2);
  int x1 = x_grid * BRICK_SIZE_PIXELS + game_viewport_x;
  int y1 = y_grid * BRICK_SIZE_PIXELS + game_viewport_y;
  int x2 = x1;
  int y2 = GAME_GRID_HEIGHT * BRICK_SIZE_PIXELS + game_viewport_y;
  DrawLine(x1, y1, x2, y2, WHITE);
}

void draw_borders() {
  DrawRectangleLinesEx((Rectangle){(float)game_viewport_x,
                                   (float)game_viewport_y,
                                   GAME_GRID_WIDTH * BRICK_SIZE_PIXELS,
                                   GAME_GRID_HEIGHT * BRICK_SIZE_PIXELS},
                       4, WHITE);
}

void draw_prediction_piece(Piece p) {
  while (!check_next_frame(p, game_grid)) {
    p.y++;
  }
  draw_piece_on_grid(p, true);
}

void draw_next_piece() {
  draw_grid(next_piece_grid, PIECE_GRID_SIZE, PIECE_GRID_SIZE,
            (int)(BRICK_SIZE_PIXELS * 1.25), next_piece_anchor_x,
            next_piece_anchor_y);
}

// ============================================================
// ANIMATION
// ============================================================

void game_animation_line_clear1_update(
    Brick game_grid[GAME_GRID_LENGTH],
    Brick game_modification_grid[GAME_GRID_LENGTH],
    int amount_of_cleared_lines) {
  int blocks_removed = 0;
  for (int col = 0; col < GAME_GRID_WIDTH; col++) {
    for (int j = 0; j < GAME_GRID_HEIGHT; j++) {
      if (game_grid[j * GAME_GRID_WIDTH + col] == '0')
        continue;
      if (game_modification_grid[j * GAME_GRID_WIDTH + col] == 'X') {
        game_grid[j * GAME_GRID_WIDTH + col] = '0';
        blocks_removed++;
      }
    }
    if (blocks_removed != 0)
      break;
  }

  game_viewport_x =
      DEFAULT_GGVIEWPORT_X +
      game_grid_shake_offset(time_since_animation_start,
                             (double)animation_lines_being_cleared);

  if (blocks_removed == 0) {
    game_viewport_x = DEFAULT_GGVIEWPORT_X;
    time_since_animation_start = 0;
    move_rows_down(game_grid, game_modification_grid);
    zero_grid(game_modification_grid, GAME_GRID_LENGTH);
    gamestate = GAME_PLAYING;
  }
}

// ============================================================
// update & main
// ============================================================

void update() {
  if (check_next_frame(controlled_piece, game_grid)) {
    if (move_to_next_piece(&controlled_piece, game_grid))
      lose();
  } else
    controlled_piece.y++;
  score_slow_fall();
}

void draw() {
  BeginTextureMode(game_target);

  // general
  ClearBackground((Color){20, 20, 20, 255});

  draw_game_grid(game_grid);

  draw_borders();

  if (ENABLE_FALL_LINE)
    draw_drop_line(controlled_piece);

  if (ENABLE_PREDICTION)
    draw_prediction_piece(controlled_piece);

  draw_piece_on_grid(controlled_piece, false);

  // text
  if (gamestate == GAME_LOST)
    DrawText("YOU LOST", text_anchor_x + 50, text_anchor_y + 350, 30, RED);

  DrawText("Press ESC to exit", text_anchor_x + 50, text_anchor_y + 10, 20,
           GRAY);

  DrawText(TextFormat("Score: %d", game_stats.score), text_anchor_x + 50,
           text_anchor_y + 50, 30, YELLOW);

  DrawText(TextFormat("LEVEL: %d", game_stats.level), text_anchor_x + 50,
           text_anchor_y + 80, 30, YELLOW);
  DrawText("left/right = move", text_anchor_x + 50, text_anchor_y + 110, 30,
           GRAY);
  DrawText("down = smash", text_anchor_x + 50, text_anchor_y + 140, 30, GRAY);
  DrawText("c = hold", text_anchor_x + 50, text_anchor_y + 170, 30, GRAY);
  DrawText("r = reset", text_anchor_x + 50, text_anchor_y + 200, 30, GRAY);
  DrawText("up = rotate", text_anchor_x + 50, text_anchor_y + 230, 30, GRAY);

  // next piece
  draw_next_piece();

  EndTextureMode();

  // draw texture with shader to window
  BeginDrawing();

  ClearBackground(BLACK);

  if (ENABLE_LIGHT) {
    BeginShaderMode(glow_shader);

    DrawTextureRec(game_target.texture,
                   (Rectangle){0, 0, (float)game_target.texture.width,
                               -(float)game_target.texture.height},
                   (Vector2){0, 0}, WHITE);

    EndShaderMode();
  } else {
    DrawTextureRec(game_target.texture,
                   (Rectangle){0, 0, (float)game_target.texture.width,
                               -(float)game_target.texture.height},
                   (Vector2){0, 0}, WHITE);
  }

  EndDrawing();
}

void lose() {
  gamestate = GAME_LOST;
  PlaySound(soundfx.game_over);
}

void init_globals(void) {
  zero_grid(game_grid, GAME_GRID_LENGTH);
  zero_grid(game_modification_grid, GAME_GRID_LENGTH);

  controlled_piece.x = 2;
  controlled_piece.y = -3;

  animation_lines_being_cleared = 0;

  time_since_animation_start = 0;
  time_since_update = 0;
  gamestate = GAME_PLAYING;
  game_stats.score = 0;
  game_stats.level = 0;
  game_stats.lines_cleared = 0;
  game_stats.drop_interval = DEFAULT_TIME_PER_UPDATE;

  game_viewport_x = DEFAULT_GGVIEWPORT_X;
  game_viewport_y = DEFAULT_GGVIEWPORT_Y;

  text_anchor_x = game_viewport_x + GAME_GRID_WIDTH * BRICK_SIZE_PIXELS;
  text_anchor_y = game_viewport_y;

  next_piece_anchor_x = 20;
  next_piece_anchor_y = 100;

  randomize_shape(controlled_piece.grid);
  randomize_shape(next_piece_grid);
}

void shader_init_function(void) {
  game_target = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
  glow_shader = LoadShaderFromMemory(NULL, glow_shader_code);

  int resolution_location = GetShaderLocation(glow_shader, "resolution");

  Vector2 resolution = {(float)SCREEN_WIDTH, (float)SCREEN_HEIGHT};

  SetShaderValue(glow_shader, resolution_location, &resolution,
                 SHADER_UNIFORM_VEC2);
}

void reset(void) { init_globals(); }

void handle_input(void) {
  if (IsKeyPressed(KEY_UP)) {
    rotate_piece(&controlled_piece, game_grid);
    PlaySound(soundfx.rotate);
  }
  if (IsKeyPressed(KEY_DOWN)) {
    smash(&controlled_piece, game_grid);
    PlaySound(soundfx.place);
  }

  if (IsKeyPressed(KEY_LEFT)) {
    controlled_piece.x--;
    if (check_overlap(controlled_piece, game_grid))
      controlled_piece.x++;
    PlaySound(soundfx.move);
  }
  if (IsKeyPressed(KEY_RIGHT)) {
    controlled_piece.x++;
    if (check_overlap(controlled_piece, game_grid))
      controlled_piece.x--;
    PlaySound(soundfx.move);
  }
  if (IsKeyPressed(KEY_C)) {
    switch_controlled_piece();
  }
  if (IsKeyPressed(KEY_R)) {
    reset();
  }
}

int main(void) {
  srand(time(NULL)); // seed time for RNG

  init_globals();

  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Elsys Tetris");
  SetTargetFPS(FPS);

  shader_init_function();

  InitAudioDevice();

  has_audio_output = IsAudioDeviceReady();

  soundfx = (SoundFX){
      .move = LoadSound("assets/fx/move.wav"),
      .rotate = LoadSound("assets/fx/rotate.wav"),
      .place = LoadSound("assets/fx/place.wav"),
      .line_clear = LoadSound("assets/fx/line_clear.wav"),
      .tetris = LoadSound("assets/fx/tetris.wav"),
      .game_over = LoadSound("assets/fx/lose.wav"),
  };

  playlist = (Musica){.track1 = LoadMusicStream("assets/music/track1.wav")};
  playlist.track1.looping = true;
  PlayMusicStream(playlist.track1);

  while (!WindowShouldClose()) {
    UpdateMusicStream(playlist.track1);
    switch (gamestate) {
    case GAME_PLAYING: {
      time_since_update += GetFrameTime();

      if (time_since_update >= game_stats.drop_interval) {
        time_since_update -= game_stats.drop_interval;
        update();
      }

      handle_input();
      break;
    }
    case GAME_LOST: {
      handle_input();
      break;
    }
    // animations use a different timer
    case GAME_ANIMATION_LINE_CLEAR1: {
      // should remove blocks to be cleared.
      // Blocks to be removed should have ID 'X'
      // Two copies of game_grid. One drawn, and one
      // indicating bricks to be removed
      double frametime = GetFrameTime();
      time_since_animation_update += frametime;
      time_since_animation_start += frametime;

      if (time_since_animation_update >=
          time_per_animation_update(animation_lines_being_cleared)) {
        time_since_animation_update -=
            time_per_animation_update(animation_lines_being_cleared);
        game_animation_line_clear1_update(game_grid, game_modification_grid,
                                          animation_lines_being_cleared);
      }
      break;
    }
    case GAME_ANIMATION_SMASH: {
      break;
    }
    }

    // Draw
    draw();
  }

  UnloadShader(glow_shader);
  UnloadRenderTexture(game_target);
  UnloadSound(soundfx.move);
  UnloadSound(soundfx.rotate);
  UnloadSound(soundfx.place);
  UnloadSound(soundfx.line_clear);
  UnloadSound(soundfx.tetris);
  UnloadSound(soundfx.game_over);
  UnloadMusicStream(playlist.track1);
  CloseAudioDevice();
  CloseWindow();

  return 0;
}
