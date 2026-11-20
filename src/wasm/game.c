#include "platform_api.h"

#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

static char logf_buf[4096] = {0};
#define LOGF(...)                                                              \
  do {                                                                         \
    stbsp_snprintf(logf_buf, sizeof(logf_buf), __VA_ARGS__);                   \
    platform_log(logf_buf);                                                    \
  } while (0)

#define CHECK(x)                                                               \
  if (!(x)) {                                                                  \
    LOGF("Fatal Error: Expected %p to not be false/nullptr", (x));             \
    return;                                                                    \
  }

#define CHECK_R(x, r)                                                          \
  if (!(x)) {                                                                  \
    LOGF("Fatal Error: Expected %p to not be false/nullptr", (x));             \
    return r;                                                                  \
  }

// WASM is little endian
#define RGBA(r, g, b, a) (u32)(((a) << 24) | ((b) << 16) | ((g) << 8) | (r))
#define WHITE RGBA(255, 255, 255, 255)
#define BLACK RGBA(0, 0, 0, 255)

// add measuments use screen height as reference
#define PADDLE_HEIGHT_PERC 0.3
#define PADDLE_WIDTH_PERC 0.05
#define PADDLE_WIDTH_OFFSET_PERC 0.05f
#define MAX_PADDLE_VELOCITY 200

#define BALL_SIZE_PERC 0.05 // radis is 5% of the height

#define MAX_BALL_VELOCITY 0.1 // 0.5% of the screen height

typedef enum {
  NONE = 0,
  LINE,   // Nothing
  RECT,   // Length and width
  CIRCLE, // Radius
} Shape;

// ### DEPRECATED
typedef struct dimension2D {
  i32 _1;
  i32 _2;
  Shape shape;
} dimension2D;

// ### DEPRECATED
typedef struct motion2D {
  double velocity;
  double angle;
} motion2D;

// :Math
typedef struct Vector2f {
  f32 x;
  f32 y;
} Vector2f;

typedef struct Vector2i {
  i32 x;
  i32 y;
} Vector2i;

#define V2F_ZERO ((Vector2f){0.0f, 0.0f})
#define V2I_ZERO ((Vector2f){0, 0})

// A = B
b32 v2f_eq(Vector2f a, Vector2f b) { return a.x == b.x && a.y == b.y; }

// A + B
Vector2f v2f_add(Vector2f a, Vector2f b) {
  return (Vector2f){.x = a.x + b.x, .y = a.y + b.y};
}

// A - B
Vector2f v2f_sub(Vector2f a, Vector2f b) {
  return (Vector2f){.x = a.x - b.x, .y = a.y - b.y};
}

// Component wise multiply by constant
Vector2f v2f_scale(Vector2f a, f32 scale) {
  return (Vector2f){.x = scale * a.x, .y = scale * a.y};
}

// Lerp
Vector2f v2f_lerp(Vector2f a, Vector2f b, f32 t) {
  return (Vector2f){
      .x = a.x * t + b.x * (1.0f - t),
      .y = a.y * t + b.y * (1.0f - t),
  };
}

// Dot Product
f32 v2f_dot(Vector2f a, Vector2f b) { return (a.x * b.x) + (a.y * b.y); }

// Floored conversion from float vector to int vector
Vector2i v2f_to_v2i(Vector2f a) {
  return (Vector2i){.x = (i32)a.x, .y = (i32)a.y};
}

// :Inputs
typedef enum InputAction { MOVE_UP = 0, MOVE_DOWN, ACTION_COUNT } InputAction;

typedef struct InputMapping {
  InputAction action;
  i32 key_code;
  i32 input_value;
} InputMapping;

typedef struct PlayerInput {
  InputMapping mappings[ACTION_COUNT];
  u8 pressed_keys[ACTION_COUNT];
} PlayerInput;

// :Colliders
typedef struct CircleCollider {
  f32 radius;
} CircleCollider;

typedef union RectangleCollider {
  struct {
    f32 length;
    f32 width;
  };
  Vector2f size;
} RectangleCollider;

typedef struct Collider {
  Shape shape;
  union {
    // Line doesn't need anything.
    CircleCollider circle;
    RectangleCollider rectangle;
  } data;
} Collider;

// :Entity
typedef enum EntityType {
  ET_UNKNOWN, // Skips by default
  ET_TEXT,
  ET_LINE,
  ET_BALL,
  ET_PADDLE,
} EntityType;

typedef struct Entity {
  EntityType type;
  Vector2f curr_position;
  Vector2f target_position;
  Vector2f velocity;
  Collider collider;
} Entity;

#define MAX_FIELD_LINES 5

#define PLAYER_1_IDX 0
#define PLAYER_2_IDX 1
#define MAX_PLAYERS 2

typedef struct GameState {
  // Field Dimentions
  Vector2i field_dim;

  // Inputs
  PlayerInput inputs[MAX_PLAYERS];

  // Entities
  // Lines
  Entity border;
  Entity center_line;

  // Player Paddles
  Entity player_paddles[MAX_PLAYERS];

  // Ball
  Entity ball;
} GameState;

static GameState game_state;

// :Inputs
i32 input_get_value(PlayerInput *player_input) {
  CHECK_R(player_input, 0)

  i32 result = 0;
  for (i32 i = 0; i < ACTION_COUNT; ++i) {
    result += (player_input->pressed_keys[i] ? 1 : 0) *
              player_input->mappings[i].input_value;
  }
  return result;
}

void input_clear(PlayerInput *player_input) {
  CHECK(player_input)

  for (i32 i = 0; i < ACTION_COUNT; ++i) {
    player_input->pressed_keys[i] = FALSE;
  }
}

void input_handle_key(PlayerInput *player_input, u8 key_code, u8 is_pressed) {
  CHECK(player_input)

  for (i32 i = 0; i < ACTION_COUNT; ++i) {
    if (player_input->mappings[i].key_code == key_code) {
      player_input->pressed_keys[i] = is_pressed;
    }
  }
}

// :Collider
void init_rect_collider(Collider *collider, Vector2f size) {
  CHECK(collider)

  collider->shape = RECT;
  collider->data.rectangle =
      (RectangleCollider){.length = size.x, .width = size.y};
}

void init_circle_collider(Collider *collider, f32 radius) {
  CHECK(collider)

  collider->shape = CIRCLE;
  collider->data.circle = (CircleCollider){.radius = radius};
}

// :Paddles
void init_paddle(Entity *entity, Vector2f position, Vector2f size) {
  CHECK(entity)

  entity->type = ET_PADDLE;
  entity->curr_position = position;
  entity->target_position = position;
  entity->velocity = (Vector2f){0.0f, 0.0f};
  init_rect_collider(&entity->collider, size);
}

void draw_paddle(Entity *entity, u32 color, f32 alpha) {
  CHECK(entity)

  Vector2f lerp_pos;
  if (v2f_eq(entity->curr_position, entity->target_position)) {
    lerp_pos = entity->target_position;
  } else {
    lerp_pos = v2f_lerp(entity->curr_position, entity->target_position, alpha);
  }

  Vector2f origin =
      v2f_sub(lerp_pos, v2f_scale(entity->collider.data.rectangle.size, 0.5f));
  Vector2i origin_i = v2f_to_v2i(origin);
  Vector2i size_i = v2f_to_v2i(entity->collider.data.rectangle.size);

  platform_fill_rect(origin_i.x, origin_i.y, size_i.x, size_i.y, color);
}

void update_paddle(Entity *entity, PlayerInput *player_input, f32 dt) {
  CHECK(entity)
  CHECK(player_input)
  i32 input_value = input_get_value(player_input);
  entity->velocity.y = input_value * MAX_PADDLE_VELOCITY;
  entity->curr_position = entity->target_position;
  entity->target_position.y = entity->curr_position.y + entity->velocity.y * dt;

  // constraint it height wise
  f32 half_height = 0.5f * entity->collider.data.rectangle.size.y;
  if (entity->target_position.y - half_height <= 0.0f) {
    entity->target_position.y = half_height;
  }

  f32 fheight = (f32)game_state.field_dim.y;
  if (entity->target_position.y + half_height >= fheight) {
    entity->target_position.y = fheight - half_height;
  }
}

void init_line(Entity *entity, Vector2f start, Vector2f end) {
  entity->type = ET_LINE;
  entity->curr_position = start;
  entity->target_position = end;
  entity->collider = (Collider){
      .shape = LINE,
      .data = {0},
  };
}

void draw_line(Entity *entity, u32 color, f32 alpha) {

  Vector2i point_1 = v2f_to_v2i(entity->curr_position);
  Vector2i point_2 = v2f_to_v2i(entity->target_position);
  platform_stroke_line(point_1.x, point_1.y, point_2.x, point_2.y, RGBA(200, 200, 200,155));
}

/*
// fix this to check the bounding box
bool is_out_of_width(i32 w) { return w > width || w < 0; }

// fix this to check the bounding box
bool is_out_of_height(i32 h) { return h > height || h < 0; }

// fix this to check the bounding box
bool is_out_of_bounds(Vector2f pos) {
return is_out_of_height(pos.y) || is_out_of_width(pos.x);
}

// fix this to check the bounding box
bool is_ball_touching_paddle(Vector2f new_ball_pos, Paddle &paddle) {
i32 collision_distance_x = (ball->dim._1 / 2 + paddle.dim._2 / 2);
i32 collision_distance_y = (ball->dim._1 / 2 + paddle.dim._1 / 2);

bool is_paddle_p1 = paddle.position.x < width / 2;
bool is_approach_top = paddle.position.y >= new_ball_pos.y;

if ((paddle.position.y - new_ball_pos.y) * (is_approach_top ? 1 : -1) >
        collision_distance_y) {
  return false;
}

return (paddle.position.x - new_ball_pos.x) * (is_paddle_p1 ? -1 : 1) <=
           collision_distance_x;
}

// decouple height as a reference for pixels
Vector2f move_entity(Vector2f prevPos, motion2D &motion) {
Vector2f pos = {
        prevPos.x + i32(motion.velocity * std::cos(motion.angle) * height),
        prevPos.y + i32(motion.velocity * std::sin(motion.angle) * height),
};
return pos;
}

double bounce_entity(double angle, bool is_vertical_collision) {
return 2 * PI + (2 * (is_vertical_collision ? 0 : PI / 2)) - angle;
}

void handle_ball_collision() {
// pri32f("before :: (%d %d), (%f %f)\n", ball->position.x,
// ball->position.y,
//        ball->motion.angle, ball->motion.velocity);
auto new_pos = move_entity(ball->position, ball->motion);
// pri32f("before :: [%d] (%d %d)\n", ball->dim._1, new_pos.x, new_pos.y);

// pri32f("before :: (%d, %d, %d)\n",
// is_out_of_height(new_pos.y + ball->dim._1),
// is_out_of_width(new_pos.y + ball->dim._1),
// is_ball_touching_paddle(*ball, *player1) ||
//     is_ball_touching_paddle(*ball, *player2));

if (is_out_of_height(new_pos.y + (ball->dim._1 / 2)) ||
        is_out_of_height(new_pos.y - (ball->dim._1 / 2))) {
  ball->motion.angle = bounce_entity(ball->motion.angle, true);
  return;
}
if (is_out_of_width(new_pos.x + (ball->dim._1 / 2)) -
        is_out_of_width(new_pos.x - (ball->dim._1 / 2))) {
  ball->motion.angle = bounce_entity(ball->motion.angle, false);
  return;
}
if (is_ball_touching_paddle(new_pos, *player1) ||
        is_ball_touching_paddle(new_pos, *player2)) {
  ball->motion.angle = bounce_entity(ball->motion.angle, false);
}

ball->position.x = new_pos.x;
ball->position.y = new_pos.y;
}

void handle_paddle_collision(Paddle &paddle) {

auto new_pos = move_entity(paddle.position, paddle.motion);

if (is_out_of_height(new_pos.y + (paddle.dim._1 / 2)) ||
        is_out_of_height(new_pos.y - (paddle.dim._1 / 2))) {
  return;
}
paddle.position.x = new_pos.x;
paddle.position.y = new_pos.y;
}

void draw_frame(Frame *frame) {
frame->wipe();
frame->draw_rect(player1->dim, player1->position);
frame->draw_rect(player2->dim, player2->position);
frame->draw_rect(ball->dim, ball->position);
frame->draw_line(middle_net->dim, middle_net->start, middle_net->end);
frame->draw_line(top_wall->dim, top_wall->start, top_wall->end);
frame->draw_line(bottom_wall->dim, bottom_wall->start, bottom_wall->end);
frame->draw_line(right_wall->dim, right_wall->start, right_wall->end);
frame->draw_line(left_wall->dim, left_wall->start, left_wall->end);
frame->render();
}

void update_entity_positions(Frame *frame) {
handle_paddle_collision(*player1);
handle_paddle_collision(*player2);
handle_ball_collision();

draw_frame(frame);
// do random stuff
}

// :Emscripten
extern "C" {
EMSCRIPTEN_KEEPALIVE
void init(i32 canvas_width, i32 canvas_height) {
  start_game(canvas_width, canvas_height);
}

EMSCRIPTEN_KEEPALIVE
void player1_input(i32 input) {
  auto globalField = GlobalField::getInstance();
  auto field = globalField.getField();

  if (input == 0) {
    field->player1->motion.velocity = 0;
  } else if (input > 0) {
    field->player1->motion.angle = PI / 2;
    field->player1->motion.velocity = PADDLE_VELOCITY;
  } else if (input < 0) {
    field->player1->motion.angle = -PI / 2;
    field->player1->motion.velocity = PADDLE_VELOCITY;
  }
}

EMSCRIPTEN_KEEPALIVE
void player2_input(i32 input) {
  auto globalField = GlobalField::getInstance();
  auto field = globalField.getField();

  if (input == 0) {
    field->player2->motion.velocity = 0;
  } else if (input > 0) {
    field->player2->motion.angle = PI / 2;
    field->player2->motion.velocity = PADDLE_VELOCITY;
  }
  if (input < 0) {
    field->player2->motion.angle = -PI / 2;
    field->player2->motion.velocity = PADDLE_VELOCITY;
  }
}
}

std::mt19937 get_rand_gen() {
  std::random_device rd;
  std::mt19937 gen(rd());

  return gen;
}

Field *init_field(i32 width, i32 height, std::mt19937 &gen) {
  std::uniform_real_distribution<double> rand_distr(0.05, 0.4);

  Field *field = new Field();
  field->width = width;
  field->height = height;

  Line *left_wall = new Line();
  left_wall->start = {0, 0};
  left_wall->end = {0, height - 1};
  left_wall->hasCollision = true;
  field->left_wall = left_wall;

  Line *top_wall = new Line();
  top_wall->start = {0, 0};
  top_wall->end = {width - 1, 0};
  top_wall->hasCollision = true;
  field->top_wall = top_wall;

  Line *right_wall = new Line();
  right_wall->start = {width - 1, 0};
  right_wall->end = {width - 1, height - 1};
  right_wall->hasCollision = true;
  field->right_wall = right_wall;

  Line *bottom_wall = new Line();
  bottom_wall->start = {0, height - 1};
  bottom_wall->end = {width - 1, height - 1};
  bottom_wall->hasCollision = true;
  field->bottom_wall = bottom_wall;

  Line *middle_net = new Line();
  middle_net->start = {width / 2, 0};
  middle_net->end = {width / 2, height - 1};
  middle_net->dim._1 = height;
  middle_net->dim.shape = LINE;
  middle_net->hasCollision = true;
  field->middle_net = middle_net;

  Paddle *player1 = new Paddle();
  player1->dim._1 = height * PADDLE_HEIGHT_PERC;
  player1->dim._2 = width * PADDLE_WIDTH_PERC;
  player1->dim.shape = RECT;
  player1->position = {i32(width * PADDLE_WIDTH_OFFSET_PERC), height / 2};
  player1->hasCollision = true;
  field->player1 = player1;

  Paddle *player2 = new Paddle();
  player2->dim._1 = height * PADDLE_HEIGHT_PERC;
  player2->dim._2 = width * PADDLE_WIDTH_PERC;
  player2->dim.shape = RECT;
  player2->position = {i32(width * (1 - PADDLE_WIDTH_OFFSET_PERC)), height / 2};
  player2->hasCollision = true;
  field->player2 = player2;

  Ball *ball = new Ball();
  ball->motion = {0.1 * MAX_BALL_VELOCITY, rand_distr(gen) * 2 * PI + (PI / 2)};
  ball->position = {width / 2, height / 2};
  ball->dim._1 = height * BALL_SIZE_PERC;
  ball->dim._2 = height * BALL_SIZE_PERC;
  ball->dim.shape = RECT;
  ball->hasCollision = true;
  field->ball = ball;

  return field;
}

void start_game_loop(Frame *frame, Field *field) {
  bool is_init = false;
  while (1) {
    auto start = chrono::steady_clock::now();

    if (!is_init) {
      field->draw_frame(frame);
      is_init = true;
      continue;
    }

    field->update_entity_positions(frame);

    auto after = chrono::steady_clock::now();
    auto diff = chrono::duration_cast<chrono::milliseconds>(after - start);

    i32 ms = static_cast<i32>((FRAME_BUDGET * 1000) - diff.count());
    if (ms > 0) {
      emscripten_sleep(ms);
      // this_thread::sleep_for(chrono::milliseconds(ms));
    }
  }
}

void start_game(i32 canvas_width, i32 canvas_height) {
  GlobalField &globalField = GlobalField::getInstance();
  globalField.init(canvas_width, canvas_height);
  Field *field = globalField.getField();

  Frame *frame = new Frame();
  frame->construct(canvas_width, canvas_height);

  printf("created frame: %d x %d\n", canvas_width, canvas_height);
  start_game_loop(frame, field);
  printf("end flush\n");
}
*/

static void setup_level() {
  game_state.border =
      (Entity){.type = ET_LINE,
               .curr_position = V2F_ZERO,
               .target_position = V2F_ZERO,
               .velocity = V2F_ZERO,
               .collider = (Collider){.shape = RECT,
                                      .data.rectangle = (RectangleCollider){
                                          .length = game_state.field_dim.x,
                                          .width = game_state.field_dim.y}}};

  init_line(&game_state.center_line,
            (Vector2f){0.5f * game_state.field_dim.x, 0.0f},
            (Vector2f){0.5f * game_state.field_dim.x, game_state.field_dim.y});
}

static void draw_level(f32 alpha) {

  (void)alpha;
  Vector2i origin_i = v2f_to_v2i(game_state.border.curr_position);
  Vector2i size_i = v2f_to_v2i(game_state.border.collider.data.rectangle.size);
  platform_stroke_rect(origin_i.x, origin_i.y, size_i.x, size_i.y, WHITE);

  draw_line(&game_state.center_line, WHITE, 0.0);
}

static void setup_input_mappings() {
  game_state.inputs[0] = (PlayerInput){
      .mappings =
          {
              (InputMapping){
                  .action = MOVE_UP, .key_code = 87, .input_value = -1},
              (InputMapping){
                  .action = MOVE_DOWN, .key_code = 83, .input_value = 1},
          },
      .pressed_keys = {0},
  };

  game_state.inputs[1] = (PlayerInput){
      .mappings =
          {
              (InputMapping){
                  .action = MOVE_UP, .key_code = 38, .input_value = -1},
              (InputMapping){
                  .action = MOVE_DOWN, .key_code = 40, .input_value = 1},
          },
      .pressed_keys = {0},
  };
}

/**
 * Handle key down event
 */
void game_keydown(u8 key_code) {
  for (i32 i = 0; i < MAX_PLAYERS; ++i) {
    input_handle_key(&game_state.inputs[i], key_code, TRUE);
  }
}

/**
 * Handle key up event
 */
void game_keyup(u8 key_code) {
  for (i32 i = 0; i < MAX_PLAYERS; ++i) {
    input_handle_key(&game_state.inputs[i], key_code, FALSE);
  }
}

void game_init(i32 width, i32 height) {
  game_state.field_dim = (Vector2i){.x = width, .y = height};

  setup_input_mappings();

  setup_level();

  // TODO: Call inits for all types of entities
  init_paddle(&game_state.player_paddles[PLAYER_1_IDX],
              (Vector2f){PADDLE_WIDTH_OFFSET_PERC * (f32)game_state.field_dim.x,
                         (f32)game_state.field_dim.y * 0.5f},
              (Vector2f){PADDLE_WIDTH_PERC * (f32)game_state.field_dim.x,
                         PADDLE_HEIGHT_PERC * (f32)game_state.field_dim.y});

  init_paddle(&game_state.player_paddles[PLAYER_2_IDX],
              (Vector2f){(1.0f - PADDLE_WIDTH_OFFSET_PERC) *
                             (f32)game_state.field_dim.x,
                         (f32)game_state.field_dim.y * 0.5f},
              (Vector2f){PADDLE_WIDTH_PERC * (f32)game_state.field_dim.x,
                         PADDLE_HEIGHT_PERC * (f32)game_state.field_dim.y});
}

/**
 * Runs at a fixed delta time.
 * Process inputs and clear them here.
 */
void game_update(f32 fixed_dt) {

  // TODO: Update Ball

  for (i32 i = 0; i < MAX_PLAYERS; ++i) {
    update_paddle(&game_state.player_paddles[i], &game_state.inputs[i],
                  fixed_dt);
  }

  // TODO: Handle Paddle Collisions
  // TODO: Handle Ball Collisions
  // TODO: Check for Scores
  // TODO: Clear Inputs
}

/**
 * Runs at a variable delta time.
 * Also provides the alpha to interpolate between the two physics states.
 */
void game_render(f32 dt, f32 alpha) {
  (void)dt;

  // Clear Background
  platform_fill_rect(0, 0, game_state.field_dim.x, game_state.field_dim.y,
                     RGBA(0, 0, 0, 255));

  draw_level(alpha);

  // Draw Paddles
  for (i32 i = 0; i < MAX_PLAYERS; ++i) {
    draw_paddle(&game_state.player_paddles[i], WHITE, alpha);
  }
}
