#include <msp430.h>
#include <libTimer.h>
#include "lcdutils.h"
#include "lcddraw.h"
//include libs from lab 2 for modules (events) and buzzer
#include "../../io-Azavala16/project/combinedFinalProject/modules.h"
#include "../../io-Azavala16/project/11-buzzer-change-tone/buzzer.h"


#define LEDS BIT6	/* note that bit zero req'd for display */

#define SW1 1
#define SW2 2
#define SW3 4
#define SW4 8

#define SWITCHES 15

char blue = 31, green = 0, red = 31;
unsigned char step = 0;


static char 
switch_update_interrupt_sense()
{
  char p2val = P2IN;
  P2IES |= (p2val & SWITCHES);
  P2IES &= (p2val | ~SWITCHES);	
  return p2val;
}

void 
switch_init()			
{  
  P2REN |= SWITCHES;		/* enables resistors for switches */
  P2IE |= SWITCHES;		/* enable interrupts from switches */
  P2OUT |= SWITCHES;		/* pull-ups for switches */
  P2DIR &= ~SWITCHES;		/* set switches' bits for input */
  switch_update_interrupt_sense();
}

int switches = 0;

void
switch_interrupt_handler()
{
  char p2val = switch_update_interrupt_sense();
  switches = ~p2val & SWITCHES;
}


short drawPos[2] = {1, 12}, controlPos[2] = {2, 12};
short colVelocity = 7, colLimits[2] = {1, screenWidth};

void
draw_ball(int col, int row, unsigned short color)
{
  if (switches & SW2){
	row=col;
	col=row;
  	fillRectangle((row-1)/2, (col-1)/2, 5, 5, color); //when SW2 pressed diagonal direction 
  	fillRectangle((screenHeight-row-1)/2-5, (screenWidth-col-1)/2, 5, 5, color); //ball2
  } 
  else{	
	fillRectangle(col-1, row-1, 6, 6, color); //creates ball horizontal
	fillRectangle(screenWidth-col-1, screenHeight-5-row-1, 6, 6, color); //horizontal inverted
 	int verticalDist = screenHeight - col - 1;
 	fillRectangle(row-10, verticalDist, 7, 7, color); //creates 2nd ball vertical
 	fillRectangle(screenWidth+5 -row-1, verticalDist, 7, 7, color);//2nd inverted
  } 
}

unsigned int warningColor; //purple

void
screen_update_ball()
{
  for (char axis = 0; axis < 2; axis ++) 
    if (drawPos[axis] != controlPos[axis]) /* position changed? */
      goto redraw;
  return;			/* nothing to do */
 redraw:
  draw_ball(drawPos[0], drawPos[1], COLOR_BLUE); /* erase */
  for (char axis = 0; axis < 2; axis ++) 
    drawPos[axis] = controlPos[axis];
    draw_ball(drawPos[0], drawPos[1], warningColor); /* draw */
}
  
short redrawScreen = 1;
u_int controlFontColor = COLOR_GREEN;

void wdt_c_handler()
{
  static int secCount = 0;
  int increaseStepGap = screenWidth/3;
  secCount ++;
  if (secCount >= 20) {
   
    {				// move ball 
      short oldCol = controlPos[0];
      short newCol = oldCol + colVelocity;
      if (newCol <= colLimits[0] || newCol >= colLimits[1]){
      	colVelocity = -colVelocity;}
      else
	controlPos[0] = newCol;
	
    }

    {	// update hourglass //
      if (switches & SW2){
	      blue = (red + 200);
	      red =  (blue + 200);
	      //displays middle light for siren
	      fillRectangle(screenHeight/2-22, screenWidth/2+8, 15, 15, warningColor);
      }
      if (switches & SW1) red = (red - 3) % 32;
      if (step <= increaseStepGap)
	step ++;
      else
	step = 0;
      secCount = 0;
    }
    if (switches & SW4) return;
    redrawScreen = 1;
  }
  
  
  //don't place modules inside the WDT_handler above
  if (switches & SW2){
	start_module11();
  }
  else{
	if(switches & SW3) buzzer_set_period(0);
	else buttonSound();
      	//start_module2();//bugs here
  }
  if(switches & SW1){
      	start_module2();
  }
  
}


void update_shape();

void main()
{
  
  P1DIR |= LEDS;		/**< Green led on when CPU on */
  P1OUT &= ~LEDS;
  configureClocks();
  lcd_init();
  switch_init();
  buzzer_init();
  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */
 
  P1DIR |= LEDS;

  clearScreen(COLOR_BLUE);
  while (1) {			/* forever */
    if (redrawScreen) {
      redrawScreen = 0;
      update_shape();
    }
    P1OUT &= ~LEDS;
    or_sr(0x10);	/**< CPU OFF */
    P1OUT |= LEDS;
  }
}

//display normal messages
void printMessage(){	
   drawString5x7(40, 3, "CAUTION!!", warningColor, COLOR_BLUE); 
   drawString5x7(40, screenHeight-10, "WARNING!!", warningColor, COLOR_BLUE); 
}


int switchGlass =0;
void
screen_update_hourglass()
{
  static unsigned char row = screenHeight/2 , col = screenWidth /2;
  static char lastStep = 0;
  
  if (step == 0 || (lastStep > step)) {
    clearScreen(COLOR_BLUE);
    lastStep = 0;
  } 
  else {
    for (; lastStep <= step; lastStep++) {
      int startCol = col - lastStep;
      int endCol = col + lastStep;
      int width = 1+ endCol - startCol;
     

     int centerX = screenWidth /2; 
     int centerY = screenHeight /2;
	
     int white =15;
     // a color in this BGR encoding is BBBB BGGG GGGR RRRR
     unsigned int color1 = (red << 11) | (green << 5) | blue; //purple
     unsigned int color2 = (green << 11) | (red << 5) | blue; //yellow
     unsigned int color3 = (green << 11) | (blue << 5) | red; //green
     warningColor = (white << 11) | (red << 5) | blue;        //purple/cyon/white
     

     if(switches & SW2){ 
     	//flipped 
      	fillRectangle(centerX - lastStep, centerY - width/2, 1, width, color1);
      	fillRectangle(centerX + lastStep, centerY - width/2, 1, width, color1);

      	//normal hr glass
      	fillRectangle(startCol, row+lastStep, width, 1, color2);
      	fillRectangle(startCol, row-lastStep, width, 1, COLOR_BLUE);
      
      	drawString5x7(40, 3, "EMERGENCY!!", warningColor, COLOR_BLUE); 
   	drawString5x7(40, screenHeight-10, "ALERT!!", warningColor, COLOR_BLUE); 
     }
     else{ //display flipped and normal hr glass along with caution msg if not pressed
	printMessage();
	if(switchGlass % 2 == 0){ //prints one hrglass 1sec then the other the next sec 
	
      		fillRectangle(centerX - lastStep, centerY - width/2, 1, width, color1);
      		fillRectangle(centerX + lastStep, centerY - width/2, 1, width, color1);
	}else{
	
      		fillRectangle(startCol, row+lastStep, width, 1, color2);
      		fillRectangle(startCol, row-lastStep, width, 1, color2);
	}
	switchGlass++;
      	//fillRectangle(centerX - lastStep, centerY - width/2, 1, width, color1);
      	//fillRectangle(centerX + lastStep, centerY - width/2, 1, width, color1);
      	
	//fillRectangle(startCol, row+lastStep, width, 1, COLOR_YELLOW);
     } 
    }  
  }
}


int size = 30;

void f1(){
	int startX = (screenWidth- size) / 2;
	int startY = (screenHeight - size) /2;
	
	if(switches & SW2){
	   for(int i=size-1; i >= 0; i--){
		for(int j =0; j <= i; ++j){
      			unsigned int color12 = red; 
      			unsigned int color2 = blue;
			//columns and rows are switched in drawpixel funct j is cols i is rows
 			//drawPixel(startX  + (size /2) - i/2 +j , startY +i, COLOR_WHITE);
 			drawPixel(startY -i , startX + (size) - i/2 + j, color12);
 			drawPixel(startY +i , startX + (size) - i/2 + j, color2);
	 	}
 	   }
	}
}	    

void
update_shape()
{ 
  screen_update_ball();
  screen_update_hourglass();
  f1();
}
   
void
__interrupt_vec(PORT2_VECTOR) Port_2(){
  if (P2IFG & SWITCHES) {	      /* did a button cause this interrupt? */
    P2IFG &= ~SWITCHES;		      /* clear pending sw interrupts */
    switch_interrupt_handler();	/* single handler for all switches */
  }
}

