#include <ncurses.h>
#include "drawBox.h"

int boxMarginBottom = 3; 
int boxMarginSide = 7;	
  
void drawBox(int rows, int cols){ //draws the actual box
  /* constructing the flowerbox */
  
  attron(COLOR_PAIR(1));
  mvprintw(rows - boxMarginBottom,boxMarginSide,""); //bottom
  for (int i = boxMarginSide; i < cols - boxMarginSide; i++){
    printw("-");
  }
  for (int i = 3;i--;){ //walls
    mvprintw(rows - boxMarginBottom - i,boxMarginSide - i, "\\");
    mvprintw(rows - boxMarginBottom - i,cols - boxMarginSide + i, "/");
  }
  for (int i = 5;(i--) > 3;){ //top side lip
    mvprintw(rows - boxMarginBottom - i,boxMarginSide - 3, "|");
    mvprintw(rows - boxMarginBottom - i,cols - boxMarginSide + 3, "|");
  }
  for (int i = boxMarginSide - 2; i < cols - boxMarginSide + 3; i++){  //top front/back lip
    mvprintw(rows - boxMarginBottom - 4,i,"-");
    mvprintw(rows - boxMarginBottom - 3,i,"-");
  }

  mvprintw(rows - boxMarginBottom - 2, cols/2 - 8, "Click to drop seed");
  mvprintw(rows - boxMarginBottom - 1, cols/2 - 7, "F1-exit  F2-auto");
  attroff(COLOR_PAIR(1));
    
  refresh();
}
