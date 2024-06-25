#include <curses.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#define MIN_Y 2
#define CONTROLS 3
#define PLAYERS 2

enum {LEFT=1, UP, RIGHT, DOWN, STOP_GAME=KEY_F(10), PAUSE_GAME = 'p'};
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

struct control_buttons default1_controls[CONTROLS] = {{'S', 'W', 'A', 'D'},
                                                     {'S', 'W', 'A', 'D'},
                                                     {'s', 'w', 'a', 'd'}};

struct control_buttons default2_controls[CONTROLS] = {{KEY_DOWN, KEY_UP, KEY_LEFT, KEY_RIGHT},
                                                     {KEY_DOWN, KEY_UP, KEY_LEFT, KEY_RIGHT},
                                                     {KEY_DOWN, KEY_UP, KEY_LEFT, KEY_RIGHT}};

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

void initSnake(snake_t *head[], size_t size, int x, int y, int i)
{
    head[i] = (snake_t *) malloc(sizeof(snake_t));
    tail_t *tail = (tail_t *) malloc(MAX_TAIL_SIZE*sizeof(tail_t));
    initTail(tail, MAX_TAIL_SIZE);
    initHead(head[i], x, y);
    head[i]->tail = tail;
    head[i]->tsize = size;
    //head[i]->controls = default_controls;
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

void repairSeed(struct food f[], size_t nfood, struct snake_t *head)
{
    // Если хвост совпадает с зерном
    for (size_t i = 0; i < head->tsize; i++)
    {
        for (size_t j = 0; j < nfood; j++)
        {
            if (f[j].x == head->tail[i].x && f[j].y == head->tail[i].y && f[i].enable)
            {
                mvprintw(1, 0, "Repair tail seed %zu", j);
                putFoodSeed(&f[j]);
            }
        }
    }
    // Если два зерна на одной точке
    for (size_t i = 0; i < nfood; i++)
    {
        for (size_t j = 0; j < nfood; j++)
        {
            if (i != j && f[i].enable && f[j].enable && f[j].x == f[i].x && f[j].y == f[i].y && f[i].enable)
            {
                mvprintw(1, 0, "Repair same seed %zu", j);
                putFoodSeed(&f[j]);
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
        mvprintw(1, 0, "Can't add tail.");
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

_Bool isCrush(snake_t *snake)
{
    for (size_t i = 1; i < snake->tsize; i++)
    {
        if (snake->x == snake->tail[i].x && snake->y == snake->tail[i].y)
        {
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

void printLevel(struct snake_t *head) {
    int max_x = 0, max_y = 0;
    getmaxyx(stdscr, max_y, max_x);
    int level = head->tsize - START_TAIL_SIZE;
    mvprintw(0, max_x - 10, "Level: %d", level);
}

void printExit(struct snake_t *head)
{
    int max_x = 0, max_y = 0;
    getmaxyx(stdscr, max_y, max_x);
    int level = head->tsize - START_TAIL_SIZE;
    clear();
    mvprintw(max_y/2, max_x/2 - 5, "Level: %d", level);
    refresh();
    getchar();
}

void pause(void)
{
    int max_x = 0, max_y = 0;
    getmaxyx(stdscr, max_y, max_x);
    mvprintw(max_y/2, max_x/2 - 5, "Press P to continue");
    while (getch() != PAUSE_GAME)
    {
    }
    mvprintw(max_y/2, max_x/2 - 5, "                   ");
}

int distance(const snake_t snake, const struct food food)
{
    return (abs(snake.x - food.x) + abs(snake.y - food.y));
}

void autoChangeDirection(snake_t *snake, struct food food[], int foodSize)
{
    int pointer = 0;
    for (int i = 1; i < foodSize; i++)
    {
        pointer = (distance(*snake, food[i]) < distance(*snake, food[pointer])) ? i : pointer;
    }
    if ((snake->direction == RIGHT || snake->direction == LEFT) && (snake->y != food[pointer].y))
    {
        snake->direction = (food[pointer].y > snake->y) ? DOWN : UP;
    }
    else if ((snake->direction == DOWN || snake->direction == UP) && (snake->x != food[pointer].x))
    {
        snake->direction = (food[pointer].x > snake->x) ? RIGHT : LEFT;
    }
}

void update(snake_t *head, struct food f[], const int32_t key, double delay, int ai)
{
    if (ai == 1)
    {
        autoChangeDirection(head, f, SEED_NUMBER);
    }
    
    go(head);
    goTail(head);
    changeDirection(head, key);
    refreshFood(f, SEED_NUMBER);
    if (haveEat(head, f))
    {
        addTail(head);
        printLevel(head);
        delay -= 0.001;
    }
}

int main(int argc, char const *argv[])
{
    double delay = 0.1;
    int isFinish = 0;

    snake_t *snakes[PLAYERS];
    for (int i = 0; i < PLAYERS; i++)
    {
        initSnake(snakes, START_TAIL_SIZE, 10 + i * 10, 10 + i * 10, i);
    }
    snakes[0]->controls = default1_controls;
    snakes[1]->controls = default2_controls;
    

    food *seed = (food *) malloc(sizeof(food));

    initFood(seed, MAX_FOOD_SIZE);
    initScreen();

    int key_pressed = 0;
    putFood(seed, SEED_NUMBER);
    while (key_pressed != STOP_GAME && !isFinish)
    {
        clock_t begin = clock();
        key_pressed = getch();
        if (key_pressed == PAUSE_GAME)
        {
            pause();
        }
        update(snakes[0], seed, key_pressed, delay, 0);
        update(snakes[1], seed, key_pressed, delay, 1);
        if (isCrush(snakes[0]))
        {
            printExit(snakes[0]);
            isFinish = 1;
        }
        for (int i = 0; i < PLAYERS; i++)
        {
            repairSeed(seed, SEED_NUMBER, snakes[i]);
        }
        refresh();
        while (((double)(clock() - begin) / CLOCKS_PER_SEC) < delay)
        {
        }
    }

    for (int i = 0; i < PLAYERS; i++)
    {
        free(snakes[i]->tail);
        free(snakes[i]);
    }
    free(seed);
    endwin();
    return 0;
}