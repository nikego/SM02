#ifndef __DRAW_H__
#define __DRAW_H__

// work modes
enum { MODE_NOSYNC, MODE_INPUT_CODE, MODE_INPUT_LEFT, MODE_INPUT_RIGHT, MODE_INPUT_REFEREE, MODE_FIGHT, MODE_PAUSED, MODE_STOPPED, MODE_TEST,};

// rectangles
enum { WHITE_LEFT, RED_LEFT, WHITE_RIGTH, GREEN_RIGHT, YELLOW_LEFT, YELLOW_RIGHT, RECT_NUMS };

// weapons
enum { FOIL, EPEE, SABRE };

typedef struct fighter_t {
	char* name;
	int score;
	int card;
	int priority;
} fighter_t;

enum { F_LEFT, F_RIGHT};

// fight parameters
typedef struct fight_state_t {
	int mode;
	long duration;
	int weapon;
	int period;
	char* referee;
	struct fighter_t fighter[2];
} fight_state_t;

// draw parameters
typedef struct draw_state_t {
	int w, h;
	struct figure_t* rectangles[RECT_NUMS];
	struct image_t *logo, *logo2;
	struct fight_state_t* fight;
	char* dev_no;

	struct figure_t* circ;
	long cnt;
	long mask;
	double angle;
	int pause;
} draw_state_t;

void init(struct draw_state_t* s);

void draw(struct draw_state_t* s);

#endif // __DRAW_H__
