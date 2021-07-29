#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <time.h>
#include "drawBox.h"

void flowerbox(int rows, int cols);
void createFlower(int flowerY, int flowerX, int state);

int debug; //debug switch
int flowers; //count these to detect mem leaks

int rows,cols;
extern int boxMarginBottom, boxMarginSide;

enum flowerState{none, seed, stem, leaf, bloom};

struct flowerListNode{ //simple linked list to store stuff
  int flowerY;
  int flowerX;
  int state;
  unsigned int gp;
  /*General purpose 32bit register, 
    for seed, stores char overlapped by seed
    for stem, stores direction of stem underneath
    for leaf, stores left/right direction of leaf
    for other states, reserved for future implementation
   */
  struct flowerListNode *next; 
} *head;  //head is always treated as dummy node to simplify code
struct flowerListNode *tail; //for optimization

int main(int argc, char* argv[]){
  debug = 0;
  flowers = 0;
  
  /* curses initialization */
  initscr(); //initialize ncurses
  halfdelay(1); //line buffering disabled, and after 1 ds give up
  noecho(); //don't echo user input
  keypad(stdscr, TRUE); //enable detection of special keys
  curs_set(0); //set cursor to be invisible

  if (has_colors() == FALSE){
    endwin();
    printf("Your terminal does not support color\n");
    exit(1);
  }
  
  start_color(); //enable terminal colors
  mousemask(ALL_MOUSE_EVENTS, NULL); //report all mouse events
  MEVENT event; //the mouse event container
  /* color setting */
  init_pair(1, COLOR_YELLOW, COLOR_BLACK);
  init_pair(2, COLOR_GREEN, COLOR_BLACK);
  init_pair(10, COLOR_RED, COLOR_BLACK);
  init_pair(11, COLOR_MAGENTA, COLOR_BLACK);
  init_pair(12, COLOR_WHITE, COLOR_BLACK);
  /* fill out dimensions of terminal window */
  getmaxyx(stdscr,rows,cols);
  if (rows < 12 || cols < 32){
    endwin();
    printf("Your terminal is too small (min 32x12)\n");
    exit(2);
  }
  /* create flowerbox */
  drawBox(rows,cols);
  
  srand((unsigned) time(NULL)); //seed PRNG
  int exited = 0; //exit with F1

  /* list initialization */
  struct flowerListNode *dummy = (struct flowerListNode *) malloc(sizeof(struct flowerListNode)); //always the start of the list
  dummy -> flowerY = 0;
  dummy -> flowerX = 0;
  dummy -> state = none;
  dummy -> next = NULL;
  dummy -> gp = 0;
  tail = head = dummy;
  
  while (!exited){ //until F1 pressed,
    rand(); //step the PRNG really fast for apparent entropy
    
    /* manage the flowers in the linked list one at a time */
    struct flowerListNode *prev = head, *current = head;


    while (current -> next != NULL){
      current = current -> next;
      int nowY = current -> flowerY;
      int nowX = current -> flowerX;
      int nowS = current -> state; //convenience
      
      switch (nowS){
      
      case seed:
	; //C wants a statement after labels, no declarations
	chtype pastchar = current -> gp; //the overlapped char
	char *pchstr = malloc(2); //convert pastchar to a string
	*pchstr = (char) (pastchar & A_CHARTEXT);
	*(pchstr + 1) = (char) 0; //null termination
	
	attrset(pastchar & A_COLOR | pastchar & A_ATTRIBUTES);
	mvprintw(nowY - 1, nowX, pchstr); //restore the char under the seed
	
	attrset(COLOR_PAIR(1) | A_BOLD);
	current -> gp = mvinch(nowY, nowX); //save overlapped char
	mvprintw(nowY, nowX, "."); //print out the seed
	attroff(COLOR_PAIR(1) | A_BOLD);

	if (nowY > rows - boxMarginBottom - 5){
	  //if landed in flowerbox
	  current -> state = stem;
	  current -> gp = 1; //set this to the neutral stem value
	  break;
	}
	current -> flowerY = nowY + 1; //fall down
	
	break;
	
      case stem:
	;
	int dx = (rand() % 5 + 2)/3 - 1 + current -> gp - 1;
	if (dx*dx == 4) dx /= -2;
	current -> gp = dx + 1;
	//0.6 chance of same heading, 0.2 chance each for other two directions
	//left-up, straight up, right-up
	//store the current direction in gp for the next stem to use
	
	attron(COLOR_PAIR(2));
	mvprintw(nowY, nowX, (dx == 0)? "|" : ((dx == 1)? "/" : "\\"));
	attroff(COLOR_PAIR(2));

	if (rand() % 5 == 0){ //0.2 chance to sprout a leaf, always
	  createFlower(nowY, nowX, leaf);
	}
	
	current -> flowerY = nowY - 1;
	current -> flowerX = nowX + dx;
	
	if (rand() % (nowY - 3) == 0){
	  //higher stems more likely to bloom, guaranteed at 3 rows from top
	  current -> state = bloom;
	  current -> gp = 0; //reset
	  }
	break;

      case leaf:
	;
	if (current -> gp == 0){ //gp == 0 means direction is undecided
	  attron(COLOR_PAIR(2) | A_BOLD);
	  mvprintw(nowY,nowX, "+"); //must be intersection of a leaf and stem
	  attroff(COLOR_PAIR(2) | A_BOLD);

	  current -> gp = 2*(rand() % 2) + 1; //pick a direction, 1 left 3 right
	  //those values are chosen because both are positive and subtracting 2 gives dx
	  current -> flowerX = nowX + (current -> gp) - 2; //make an initial straight side move
	}
	else{ //direction has already been decided
	  int dy = (rand() % 2) - 1;
	  //0.5 chance side-up, 0.5 side-straight,
	  //remember that going upwards decrements y

	  attron(COLOR_PAIR(2) | A_BOLD);
	  mvprintw(nowY,nowX, (dy == 0)? "-" : ((current -> gp == 1)? "\\" : "/"));
	  //correct symbol to print depends both on dy and on left/right direction, stored in gp
	  attroff(COLOR_PAIR(2) | A_BOLD);

	  current -> flowerX = nowX + (current -> gp) - 2; //advance the leaf
	  current -> flowerY = nowY + dy;

	  if (rand() % 3 == 0){ //0.33 chance to end the leaf growth, always
	    current -> state = none; 
	  }
	}
	break;
	
      case bloom:
	;
	int color = 10 + rand() % 3; //random color, labeled by 10, 11, or 12
	float size = (rows - nowY + rand() % 12) / 12.0; //the higher up, the larger the bloom

	attron(COLOR_PAIR(color) | A_BOLD & (unsigned int)(0 - rand() % 2));
	for (int i = -size; i <= size; i++){ //draw a roughened circle
	  for (int j = -size; j <= size; j++){
	    if (i*i + j*j < size*size + ((rand() % 17)/4.0 - 2))
	      mvprintw(nowY + i, nowX + j, "O");
	  }
	}
	attrset(COLOR_PAIR(1) | A_BOLD);
	mvprintw(nowY, nowX, "O"); //draw the bloom center in a diff color
	attroff(COLOR_PAIR(color) | A_BOLD);
	
	current -> state = none;
	break;

	
      case none:
	;
	struct flowerListNode *after; //can't reference a freed ptr
	prev -> next = current -> next;
	//mistyped == here, cost twenty minutes of time
	if (tail == current){
	  tail = prev;
	  after = prev;
	}
	else{
	  after = prev -> next;
	}
	free(current);
	flowers--; //count decrements
	current = after;
	break;
      }
      
      prev = current;
    }


    /* handle user input */
    int ch;
    switch (ch = getch()){ 
    case KEY_MOUSE:
      if (getmouse(&event) == OK){
	if (event.bstate & (BUTTON1_CLICKED | BUTTON1_PRESSED)){
	  //if primary mouse button pressed
	  if (event.x < cols - boxMarginSide && event.x > boxMarginSide
	      && event.y < rows - boxMarginBottom - 5){
	    //if seed can be placed there, place one
	    createFlower(event.y, event.x, seed);
	  }
	}
      }
      break;
    case KEY_F(1): //exit with F1
      exited = 1;
      break;

    case KEY_F(2): //automatically drop a ton of seeds with F2
      for (int i = boxMarginSide + 1; i < cols - boxMarginSide; i++){
	if (rand() % 2)
	  createFlower(0, i, seed);
      }
      break;

    case KEY_F(3): //toggle debugging data display
      debug = 1 - debug;
      break;
    }
    mvprintw(rows - 1, 0, "                        ");
    if (debug)
      mvprintw(rows - 1, 0, "flowers: %d", flowers); //show number of flowers
      
  }
  endwin();
  return 0;
}




void createFlower(int flowerY, int flowerX, int state){
  struct flowerListNode *newFlower = (struct flowerListNode *) malloc(sizeof(struct flowerListNode));
  newFlower -> flowerY = flowerY;
  newFlower -> flowerX = flowerX;
  newFlower -> state = state;
  newFlower -> next = NULL;
  newFlower -> gp = 0;
  tail = tail -> next = newFlower;

  flowers++; //count increments
}



//TODO
//style code
