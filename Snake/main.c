#include <curses.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#define MIN_Y 2
#define CONTROLS 3

const double DELAY = 0.1;

enum {LEFT=1, UP, RIGHT, DOWN, STOP_GAME=KEY_F(10)};
enum {MAX_TAIL_SIZE=100, START_TAIL_SIZE=3, MAX_FOOD_SIZE=20, FOOD_EXPIRE_SECONDS=10, SEED_NUMBER=5};

typedef struct control_buttons
{
    int down;
    int up;
    int left;
    int right;
} control_buttons;

typedef struct snake_t
{
    int x;
    int y;
    int direction;
    size_t tsize;
    struct tail_t *tail;
    struct control_buttons *controls;
} snake_t;

typedef struct tail_t
{
    int x;
    int y;
} tail_t;

typedef struct food
{
    int x;
    int y;
    time_t put_time;
    char point;
    uint8_t enable;
} food[MAX_FOOD_SIZE];

struct control_buttons default_controls[CONTROLS] = {{KEY_DOWN, KEY_UP, KEY_LEFT, KEY_RIGHT},
                                                     {'S', 'W', 'A', 'D'},
                                                     {'s', 'w', 'a', 'd'}};

void initTail(struct tail_t t[], size_t size)
{
    struct tail_t init_t = {0, 0};
    for (size_t i = 0; i < size; i++)
    {
        t[i] = init_t;
    }
}

void initHead(struct snake_t *head, int x, int y)
{
    head->x = x;
    head->y = y;
    head->direction = RIGHT;
}

void initSnake(snake_t *head, size_t size, int x, int y)
{
    tail_t *tail = (tail_t *) malloc(MAX_TAIL_SIZE*sizeof(tail_t));
    initTail(tail, MAX_TAIL_SIZE);
    initHead(head, x, y);
    head->tail = tail;
    head->tsize = size + 1;
    head->controls = default_controls;
}

void initFood(struct food f[], size_t size)
{
    struct food init = {0, 0, 0, 0, 0};
    int max_y = 0, max_x = 0;
    getmaxyx(stdscr, max_y, max_x);
    for (size_t i = 0; i < size; i++)
    {
        f[i] = init;
    }
}

void putFoodSeed (struct food *fp)
{
    int max_x = 0, max_y = 0;
    char spoint[2] = {0};
    getmaxyx(stdscr, max_y, max_x);
    mvprintw(fp->y, fp->x, " ");
    fp->x = rand() % (max_x - 1);
    fp->y = rand() % (max_y - 2) + 1;
    fp->put_time = time(NULL);
    fp->point = '$';
    fp->enable = 1;
    spoint[0] = fp->point;
    mvprintw(fp->y, fp->x, "%s", spoint);
}

void putFood(struct food f[], size_t number_seeds)
{
    for (size_t i = 0; i < number_seeds; i++)
    {
        putFoodSeed(&f[i]);
    }
}

void refreshFood(struct food f[], int nfood)
{
    int max_x = 0, max_y = 0;
    char spoint[2] = {0};
    getmaxyx(stdscr, max_y, max_x);
    for (size_t i = 0; i < nfood; i++)
    {
        if (f[i].put_time)
        {
            if (!f[i].enable || (time(NULL) - f[i].put_time) > FOOD_EXPIRE_SECONDS)
            {
                putFoodSeed(&f[i]);
            }
        }        
    }    
}

void go(struct snake_t *head)
{
    char ch = '@';
    int max_x = 0, max_y = 0;
    getmaxyx(stdscr, max_y, max_x);
    mvprintw(head->y, head->x, " ");
    switch (head->direction)
    {
    case LEFT:
        if (head->x <= 0)
        {
            head->x = max_x;
        }
        mvprintw(head->y, --(head->x), "%c", ch);
        break;
    case RIGHT:
        if (head->x >= max_x)
        {
            head->x = 0;
        }
        mvprintw(head->y, ++(head->x), "%c", ch);
        break;
    case UP:
        if (head->y <= MIN_Y)
        {
            head->y = max_y;
        }
        mvprintw(--(head->y), head->x, "%c", ch);
        break;
    case DOWN:
        if (head->y >= max_y)
        {
            head->y = MIN_Y;
        }
        mvprintw(++(head->y), head->x, "%c", ch);
        break;
    default:
        break;
    }
    refresh();
}

void goTail(struct snake_t *head)
{
    char ch = '*';
    mvprintw(head->tail[head->tsize - 1].y, head->tail[head->tsize - 1].x, " ");
    for (size_t i = head->tsize - 1; i > 0; i--)
    {
        head->tail[i] = head->tail[i - 1];
        if (head->tail[i].y || head->tail[i].x)
        {
            mvprintw(head->tail[i].y, head->tail[i].x, "%c", ch);
        }
    }
    head->tail[0].x = head->x;
    head->tail[0].y = head->y;
}

void changeDirection(snake_t *snake, const int32_t key)
{
    for (int i = 0; i < CONTROLS; i++)
    {
        if (key == snake->controls[i].down)
        {
            if (checkDirection(snake, key, i))
            {
                snake->direction = DOWN;
            }
        }
        else if (key == snake->controls[i].up)
        {
            if (checkDirection(snake, key, i))
            {
                snake->direction = UP;
            }
        }
        else if (key == snake->controls[i].right)
        {
            if (checkDirection(snake, key, i))
            {
                snake->direction = RIGHT;
            }
        }
        else if (key == snake->controls[i].left)
        {
            if (checkDirection(snake, key, i))
            {
                snake->direction = LEFT;
            }
        }    
    }
}

int checkDirection(snake_t *snake, int32_t key, int i)
{

    if (snake->direction == RIGHT && key == snake->controls[i].left)
    {
        snake->direction = RIGHT;
        return 0;
    }
    else if (snake->direction == UP && key == snake->controls[i].down)
    {
        snake->direction = UP;
        return 0;
    }
    else if (snake->direction == LEFT && key == snake->controls[i].right)
    {
        snake->direction = LEFT;
        return 0;
    }
    else if (snake->direction == DOWN && key == snake->controls[i].up)
    {
        snake->direction = DOWN;
        return 0;
    }
    else
    {
        return 1;
    }
}

void addTail(struct snake_t *head)
{
    if (head == NULL || head->tsize > MAX_TAIL_SIZE)
    {
        mvprintw(0, 0, "Can't add tail.");
        return;
    }
    head->tsize++;
}

_Bool haveEat(struct snake_t *head, struct food f[])
{
    for (size_t i = 0; i < MAX_FOOD_SIZE; i++)
    {
        if (f[i].enable && head->x == f[i].x && head->y == f[i].y)
        {
            f[i].enable = 0;
            return 1;
        }
    }
    return 0;
}

void initScreen(){
    initscr();
    keypad(stdscr, TRUE);
    raw();
    noecho();
    curs_set(FALSE);
    mvprintw(0, 0, " Use arrows for control. Press 'F10' for EXIT.");
    timeout(0);
}

int main(int argc, char const *argv[])
{
    int delay = 200000;
    int level = 1;

    snake_t *snake = (snake_t *) malloc(sizeof(snake_t));
    food *seed = (food *) malloc(sizeof(food));

    initFood(seed, MAX_FOOD_SIZE);
    initSnake(snake, START_TAIL_SIZE, 10, 10);
    initScreen();

    int key_pressed = 0;
    putFood(seed, SEED_NUMBER);
    while (key_pressed != STOP_GAME)
    {
        key_pressed = getch();
        go(snake);
        goTail(snake);
        timeout(100);
        changeDirection(snake, key_pressed);
        refreshFood(seed, SEED_NUMBER);
        if (haveEat(snake, seed))
        {
            addTail(snake);
        }              
    }
    free(snake->tail);
    free(snake);
    free(seed);
    endwin();
    return 0;
}