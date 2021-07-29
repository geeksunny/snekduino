#include <stdint.h>
#include <time.h>

/** Snake & Queue data types, values, constants **/
#define GRID_HEIGHT 5
#define GRID_WIDTH 10

#define MAX_QUEUE_SIZE GRID_HEIGHT * GRID_WIDTH
#define MAX_LIST_SIZE MAX_QUEUE_SIZE

#define TICK_MS_RUNNING 1000
#define TICK_MS_RESET 5000

enum Direction {
  UP, DOWN, LEFT, RIGHT
};

enum GameState {
  RESET, MOVE, WIN, LOSE
};

typedef struct PACKED snake_t {
  uint8_t x : 5;  // value: 0-16
  uint8_t y : 4;  // value: 0-8
  uint8_t is_enabled : 1;
  uint8_t is_food : 1;
  // todo: convert padding bit into needs_update/state_changed flag for rendering
  uint8_t : 1;  // needed for padding. 12 bits in total.
} snake_t;

typedef struct queue_t {
  snake_t** array;
  uint8_t front;
  uint8_t rear;
  uint8_t count;
} queue_t;

typedef struct list_node_t {
  snake_t* data;
  list_node_t* next;
  list_node_t* prev;
} list_node_t;

typedef struct list_t {
  list_node_t* node_pool;
  list_node_t* head;
  list_node_t* tail;
  uint8_t count;
} list_t;


/** Forward declarations of functions **/
// todo: generate these from find-all with a regex
//point_t index_to_grid_point(uint8_t index);
//uint8_t xy_to_index(uint8_t x, uint8_t y);
//queue_t sn_queue_init(snake_t** array_ptr);
//bool queue_is_empty(queue_t *queue);
//bool sn_queue_is_full(queue_t *queue);
//bool sn_queue_insert(queue_t *queue, snake_t* element);


/** Snake game variables **/
static snake_t grid_[GRID_WIDTH][GRID_HEIGHT];  // grid_[x][y]
static snake_t* snake_arr_[MAX_QUEUE_SIZE];
static queue_t snake_queue_;
// TODO: Remove this led_map_ for ergodox/moonlander, replace with macros for automated mapping with vertical|horizontal and wrapping options
static const snake_t* led_map_[48] = {
    // Right side
    &grid_[9][0], &grid_[8][0], &grid_[7][0], &grid_[6][0], &grid_[5][0],
    &grid_[9][1], &grid_[8][1], &grid_[7][1], &grid_[6][1], &grid_[5][1],
    &grid_[9][2], &grid_[8][2], &grid_[7][2], &grid_[6][2], &grid_[5][2],
    &grid_[9][3], &grid_[8][3], &grid_[7][3], &grid_[6][3], &grid_[5][3],
    &grid_[9][4], &grid_[8][4], &grid_[7][4], &grid_[6][4],
    // Left side
    &grid_[0][0], &grid_[1][0], &grid_[2][0], &grid_[3][0], &grid_[4][0],
    &grid_[0][1], &grid_[1][1], &grid_[2][1], &grid_[3][1], &grid_[4][1],
    &grid_[0][2], &grid_[1][2], &grid_[2][2], &grid_[3][2], &grid_[4][2],
    &grid_[0][3], &grid_[1][3], &grid_[2][3], &grid_[3][3], &grid_[4][3],
    &grid_[0][4], &grid_[1][4], &grid_[2][4], &grid_[3][4]
};
/** Empty node list **/
static list_node_t empty_cell_pool_[MAX_LIST_SIZE];
static list_t empty_cell_list_;

static enum Direction direction_;
static enum GameState game_state_ = RESET;

static unsigned long last_tick_ = 0;
static unsigned long tick_length_ = TICK_MS_RUNNING;


/* Decimal Index -> X/Y Grid Location */
point_t index_to_grid_point(uint8_t index) {
  return (point_t){(uint8_t)(index % GRID_WIDTH), (uint8_t)(index / GRID_WIDTH)};
}

/* X/Y Grid Location -> Decimal Index */
uint8_t xy_to_index(uint8_t x, uint8_t y) {
  return x + (y * GRID_WIDTH);
}

/** Queue functions **/
queue_t sn_queue_init(snake_t** array_ptr) {
  queue_t result;
  result.array = array_ptr;
  result.front = 0;
  result.rear = 0;
  result.count = 0;
  return result;
}

bool sn_queue_is_empty() {
  return snake_queue_.front == snake_queue_.rear && snake_queue_.count == 0;
}

bool sn_queue_is_full() {
  return snake_queue_.front == snake_queue_.rear && snake_queue_.count != 0;
}

bool sn_queue_insert(snake_t* element) {
  if (sn_queue_is_full()) {
    return false;
  }
  // Insert
  *(snake_queue_.array + snake_queue_.rear) = element;
  snake_queue_.rear = (snake_queue_.rear + 1) % MAX_QUEUE_SIZE;
  ++snake_queue_.count;
  return true;
}

// front = dequeue = read
snake_t* sn_queue_remove() {
  snake_t* result = *(snake_queue_.array + snake_queue_.front);
  snake_queue_.front = (snake_queue_.front + 1) % MAX_QUEUE_SIZE;
  --snake_queue_.count;
  return result;
}

snake_t* sn_queue_peek_front() {
  return *snake_queue_.array + snake_queue_.front;
}

snake_t* sn_queue_peek_rear() {
  return *snake_queue_.array + snake_queue_.rear;
}


/* Array-List functions */
void ec_list_clear(list_t* list) {
  list->head = nullptr;
  list->tail = nullptr;
  list->count = 0;
  // Clear out the links on the list nodes.
  for (int i = 0; i < MAX_LIST_SIZE; ++i) {
    (list->node_pool + i)->prev = nullptr;
    (list->node_pool + i)->next = nullptr;
  }
}

list_node_t* ec_list_get_node(uint8_t list_index) {
  if (list_index > empty_cell_list_.count) {
    // list_index is out-of-range of the list contents.
    return nullptr;
  }
  list_node_t* focused_node;
  // todo: share search logic from both directions below (using inline function(s)); also above version
  if (list_index / empty_cell_list_.count > 0.5) {
    // list_index is closer to empty_cell_list_.tail; Traverse backwards.
    focused_node = empty_cell_list_.tail;
    for (uint8_t i = empty_cell_list_.count-1; i >= 0; --i) {
      focused_node = focused_node->prev;
      if (focused_node == nullptr) {
        // Previous link is null. Error.
        return nullptr;
      } else if (i == list_index) {
        return focused_node;
      }
    }
  } else {
    // list_index is closer to empty_cell_list_.head; Traverse forwards.
    focused_node = empty_cell_list_.head;
    for (uint8_t i = 0; i < empty_cell_list_.count; ++i) {
      focused_node = focused_node->next;
      if (focused_node == nullptr) {
        // Next link is null. Error.
        return nullptr;
      } else if (i == list_index) {
        return focused_node;
      }
    }
  }
  return nullptr;
}

snake_t* ec_list_get(uint8_t list_index) {
  list_node_t* node = ec_list_get_node(list_index);
  return (node != nullptr) ? node->data : nullptr;
}

bool ec_list_add_cell_node(list_node_t* cell_node) {
  if (!cell_node) {
      // Null node; nothing to insert.
      return false;
  }
  if (cell_node->next != nullptr || cell_node->prev != nullptr) {
      // The cell_node is already linked within the list. Cannot add.
      return false;
  }
  if (empty_cell_list_.tail == nullptr) {
      // If tail is null, then head will also be null.
      empty_cell_list_.head = cell_node;
      empty_cell_list_.tail = cell_node;
  } else {
      // todo: check if list->tail has a next already? auto-repair list in this case ??
      cell_node->prev = empty_cell_list_.tail;
      empty_cell_list_.tail->next = cell_node;
      empty_cell_list_.tail = cell_node;
  }
  ++empty_cell_list_.count;
  return true;
}

bool ec_list_add_cell_index(uint8_t* cell_index) {
  if (*cell_index >= MAX_LIST_SIZE) {
    // Out of range; Cannot insert.
    return false;
  }
  list_node_t* added_cell = &empty_cell_pool_[*cell_index];
  return ec_list_add_cell_node(added_cell);
}

bool ec_list_remove_cell_node(list_node_t* cell_node) {
    if (!cell_node) {
        // Null node; nothing to remove.
        return false;
    }
    // TODO: Add in checks&set for head/tail...
    if (cell_node->next == nullptr) {
        if (cell_node->prev == nullptr) {
            // The cell_node is not linked within the list. Cannot remove.
            return false;
        } else {
            // Cell is at the end of the list.
            // todo: should this also compare cell_node to list->tail ???
            cell_node->prev->next = nullptr;
            empty_cell_list_.tail = cell_node->prev;
        }
    } else {
        // Join the two cells surrounding cell_node.
        cell_node->prev->next = cell_node->next;
        cell_node->next->prev = cell_node->prev;
    }
    --empty_cell_list_.count;
    return true;
}

bool ec_list_remove_cell_index(uint8_t* cell_index) {
  if (*cell_index >= MAX_LIST_SIZE) {
    // Out of range; Cannot remove.
    return false;
  }
  list_node_t* removed_cell = &empty_cell_pool_[*cell_index];
  return ec_list_remove_cell_node(removed_cell);
}

bool ec_list_remove(uint8_t* list_index) {
  list_node_t* node = ec_list_get_node(*list_index);
  if (node == nullptr) {
    return false;
  }
  return ec_list_remove_cell_node(node);
}


/** Snake game logic functions **/
void snake_init() {
  // Init rand()
  srand(time(nullptr));
  // Init snake cell queue struct
  snake_queue_ = sn_queue_init(snake_arr_);
  // todo : segment these blocks out into inline functions for organization?
  // todo :: or.... combine them into the same loop(s)!!
  // Init grid contents
  for (uint8_t x = 0; x < GRID_WIDTH; ++x) {
    for (uint8_t y = 0; y < GRID_HEIGHT; ++y) {
      grid_[x][y] = (snake_t){x, y, 0, 0};
    }
  }
  // Init empty cell list
  point_t xy;
  for (int i = 0; i < MAX_LIST_SIZE; ++i) {
    xy = index_to_grid_point(i);
    empty_cell_pool_[i] = {&grid_[xy.x][xy.y], nullptr, nullptr};
  }
  empty_cell_list_ = {empty_cell_pool_, nullptr, nullptr};
}

bool place_segment(snake_t *segment, uint8_t x, uint8_t y) {
  // TODO: NEEDS REVAMP AFTER FIFO QUEUE IS INTRODUCED
  // todo: swap empty-cell from list to snake queue

//    // test the placement
//    if ((x < 0 || x >= GRID_WIDTH) || (y < 0 || y >= GRID_HEIGHT)) {
//      // invalid placement; bail.
//      return false;
//    }
//    // set x/y on segment
//    segment->x = x;
//    segment->y = y;
//    // place segment into the grid
//    grid_[x][y] = segment;
    return false;
}

snake_t* pick_next_food_loc() {
  uint8_t index = rand() % empty_cell_list_.count;
  // TODO: Test if the node at `index` is too close to snake??
  list_node_t* node = ec_list_get_node(index);
  ec_list_remove_cell_node(node);
  return node->data;
}

bool create_snake(uint8_t x, uint8_t y, uint8_t length, Direction direction) {
  // TODO: Create snake segments, return false if doesn't fit in bounds.
  //  ... Work out a wrapped snake if near the corner
  return true;
}

static inline void place_snake_node() {

}

static inline void reset_tick_length() {
  tick_length_ = TICK_MS_RUNNING;
}

static inline void extend_tick_length() {
  tick_length_ = TICK_MS_RESET;
}

void reset() {
  //  reset the grid nodes
  for (uint8_t x = 0; x < GRID_WIDTH; ++x) {
    for (uint8_t y = 0; y < GRID_HEIGHT; ++y) {
      grid_[x][y].is_enabled = 0;
      grid_[x][y].is_food = 0;
    }
  }
  // TODO: NEEDS REVAMP AFTER FIFO QUEUE IS INTRODUCED
  // place food
//    place_segment(&food_, 1, 5);
//    // reset snake length
//    length_ = 3;
//    // determine snake placement
//    uint8_t x = 0;
//    uint8_t y = 0;
//    // place initial snake segments
//    for (uint8_t i = length_ - 1; i >= 0; --i) {
//      place_segment(&snake_[i], x, y + i);
//    }
  // initial direction
  direction_ = RIGHT;
  // game state
  game_state_ = MOVE;
  // Reset tick length to RUNNING duration
  reset_tick_length();
}

void move_snake(snake_t* new_loc) {
  // Pop the old tail off the queue
  snake_t* old_tail = sn_queue_remove();
  old_tail->is_enabled = false;
  // TODO: Old tail needs to be inserted back into the empty-cell list
  // Add the new head to the snake
  new_loc->is_enabled = true;
  sn_queue_insert(new_loc);
}

void eat_food(snake_t* food_loc) {
  // Disable the is_food flag
  food_loc->is_food = false;
  // Add the new head to the snake
  sn_queue_insert(food_loc);
}

bool handle_move() {
  // Is the next move valid?
  snake_t* head = sn_queue_peek_rear();
  uint8_t nextX, nextY;
  switch (direction_) {
    case UP:
      nextX = head->x;
      nextY = head->y - 1;
      break;
    case DOWN:
      nextX = head->x;
      nextY = head->y + 1;
      break;
    case LEFT:
      nextX = head->x - 1;
      nextY = head->y;
      break;
    case RIGHT:
      nextX = head->x + 1;
      nextY = head->y;
      break;
  }
  // Out-of-bounds check
  if ((nextX < 0 || nextX >= GRID_WIDTH) || (nextY < 0 || nextY >= GRID_HEIGHT)) {
    game_state_ = LOSE;
    return false;
  }
  // Collision with snake check
  if (grid_[nextX][nextY].is_enabled) {
    if (grid_[nextX][nextY].is_food) {
      // Eating the food!
      eat_food(&grid_[nextX][nextY]);
    } else if (&grid_[nextX][nextY] != sn_queue_peek_front()) {
      // Not the food and not the end of the tail; collision with snake body.
      game_state_ = LOSE;
      return false;
    }
  }
  // Move is valid.
  move_snake(&grid_[nextX][nextY]);
  // Is the game won?
  // TODO: should game win check be handled at eating stage???
  if (sn_queue_is_full()) {
    // TODO: Using sn_queue_is_full() includes two spots on the Ergodox-EZ grid that do not have LEDs...
    //  should this be modified back to old LED_COUNT check logic?
    // Snake length has reached the total amount of LEDs available without encountering an invalid move.
    // Game is won. No further work to be done.
    game_state_ = WIN;
  }

  // TODO: Is there any work we need to do here related to directional changes / player input ?
  return true;
}


/** Basic game loop functions **/
void handle_tick() {
  bool running;
  switch (game_state_) {
    case MOVE:
      // TODO: Is using result of `running` here OK??
      //  It would make the subsequent WIN/LOSE state take longer.....
      running = handle_move();
      break;
    case RESET:
      reset();
      running = true;
      break;
    case WIN:
    case LOSE:
      game_state_ = RESET;
      running = false; //TODO: Or is this TRUE ?? False comes from handle_move()....
      break;
  }
  if (!running) {
    // Set tick_length_ to RESET duration
    extend_tick_length();
  }
  last_tick_ = millis();
}

void snake_exec() {
  if (millis() - last_tick_ < tick_length_) {
    handle_tick();
  }
  // todo: render LEDs out from led_map_ as a normal RGB effect.
  //  * look into logic to reduce redundant color calls... if a color is already displayed, skip it.(right?)
}


/** RGB Animation code **/
// Right now this lives in main.cpp::SnakeEffect