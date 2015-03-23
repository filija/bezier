/******************************************************************************
 * Projekt - Zaklady pocitacove grafiky - IZG Lab 04
 * ihulik@fit.vutbr.cz
 *
 *
 *
 * *) Ovladani programu:
 *      "Leve tlacitko mysi"		- Zadavani controlnich bodu
 *      "Esc"						- ukonceni programu
 *
 * Opravy a modifikace:
 * isvoboda@fit.vutbr.cz	- Lab01 to Lab04
 * 							- Prepracovan system zpracovani udalosti
 * 							- Clear uzivatelskeho bufferu - frame_buffer
 */

/******************************************************************************
******************************************************************************
 * Includes
 */

#ifdef _WIN32
    #include <windows.h>
#endif

/* knihovna SDL + system */
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <float.h>

/* nase veci... */
#include "color.h"
#include "student.h"
#include "globals.h"
#include "vector.h"

/******************************************************************************
 ******************************************************************************
 * Globalni konstanty a promenne
 */

/* titulek hlavniho okna */
const char     * PROGRAM_TITLE  = "IZG Lab 04 Curves";

/* defaultni velikost okna */
const int          DEFAULT_WIDTH    = 800;
const int          DEFAULT_HEIGHT   = 600;

/* kreslici buffer knihovny SDL */
SDL_Surface         * screen        = NULL;

/* kreslici buffer IZG cviceni */
S_RGBA				* frame_buffer	= NULL;

/* pomocna promenna pro ukonceni aplikace */
int                 quit            = 0;
int					width			= 800;
int					height			= 600;

/* aktivni bod */
Point2d				*active_point	= NULL;
const int			snipeDistance	= 10;

/* Kvalita generovane krivky - pocet segmentu - usecek, ze kterych je vykreslena */
int					quality			= 100;

//Viz hlavicka vector.h
USE_VECTOR_OF(Point2d, point2d_vec)
#define point2d_vecGet(pVec, i) (*point2i_vecGetPtr((pVec), (i)))

/*Initialization has to be done in the scope of function (Limitation of C), see the main function.*/
S_Vector			* control_points	= NULL;
S_Vector			* curve_points		= NULL;

/*Ukaztel na funkci, pomoci ktere chceme pocitat Bezierovu krivku*/
void (*bezier)(int, const S_Vector*, S_Vector*) = bezierBernsteinPolynom;

/**
 * Funkce vlozi pixel na pozici bodu [x;y]. Je nutne hlidat frame_buffer, pokud
 * je dana souradnice mimo hranice, funkce neprovadi zadnou zmenu.
 * @param x Souradnice bodu x
 * @param y Souradnice bodu y
 * @param color Barva bodu
 */
void putPixel(int x, int y, const S_RGBA *color)
{
	if (x > 0 && y > 0 && x < width && y < height)
			frame_buffer[y * width + x] = *color;
}

void putPixelPoint2d(const Point2d *p, const S_RGBA *color)
{
	putPixel(ROUND(p->x), ROUND(p->y), color);
}

/**
 * Funkce DrawLine kresli usecku mezi dvema body [x1,y1] a [x2,y2]
 * pomoci Bressenhamova algoritmu.
 * @param x1 pocatecni souradnice x
 * @param y1 pocatecni souradnice y
 * @param x2 koncova souradnice x
 * @param y2 koncova souradnice y
 * @param col Barva, kterou usecku vykreslujeme
 */
void drawLine(int x1, int y1, int x2, int y2, const S_RGBA *col)
{
	bool steep = (abs( y2 - y1 ) > abs( x2 - x1 )) ? true : false;
	if( steep )
	{
		SWAP(x1, y1);
		SWAP(x2, y2);
	}
	if( x1 > x2 )
	{
		SWAP(x1, x2);
		SWAP(y1, y2);
	}

	const int dx = x2 - x1, dy = abs(y2 - y1);
	const int P1 = 2*dy, P2 = P1 - 2*dx;
	int P = 2*dy - dx;
	int y = y1;
	int ystep = 1;
	if ( y1 > y2 ) ystep = -1;

	for (int x = x1; x <= x2; x++ )
	{
		if( steep )
			putPixel( y, x, col );
		else
			putPixel( x, y, col );

		if( P >= 0 )
		{
			P += P2;
			y += ystep;
		} else
			P += P1;
	}


	return;
}

void drawLinePoint2d (const Point2d *a, const Point2d *b, const S_RGBA *col)
{
	drawLine(ROUND(a->x), ROUND(a->y), ROUND(b->x), ROUND(b->y), col);
}

void drawLines(const S_Vector *points, const S_RGBA *color)
{
	for(int i = 1; i < point2d_vecSize(points); ++i)
	{
		drawLinePoint2d(point2d_vecGetPtr(points, i-1), point2d_vecGetPtr(points, i), color);
	}
}

int pointSnap(const Point2d *p, int range, const S_Vector *points)
{
  int index = -1;
  float min_range = FLT_MAX;

  // nalezeni nejblizsiho bodu
  for (int i = 0; i < point2d_vecSize(points); ++i)
  {
    float dx = ROUND(p->x) - ROUND(point2d_vecGetPtr(points,i)->x);
    float dy = ROUND(p->y) - ROUND(point2d_vecGetPtr(points,i)->y);
//    Point *p = (Point *)(vecGetPtr(points,i));
    float temp = dx * dx + dy * dy;

    if (temp < min_range)
    {
	    min_range = temp;
	    index = i;
	  }
  }

  // lezi bod v pozadovanem okoli?
  if (min_range < range * range)
    return index;
  else
    return -1;
}

void drawCrossPoint(const Point2d *p, int size, const S_RGBA *color)
{
  putPixelPoint2d(p, color);
  for (int i = 0; i < size; ++i)
  {
    putPixel(ROUND(p->x) - i, ROUND(p->y), color);
    putPixel(ROUND(p->x) + i, ROUND(p->y), color);
    putPixel(ROUND(p->x), ROUND(p->y) + i, color);
    putPixel(ROUND(p->x), ROUND(p->y) - i, color);
  }
}

void drawCrossPoints(const S_Vector *points, int size, const S_RGBA *color)
{
  for (int i = 0; i < point2d_vecSize(points); ++i)
  {
    drawCrossPoint(point2d_vecGetPtr(points, i), size, color);
  }
}

/******************************************************************************
 ******************************************************************************
 * funkce zajistujici prekresleni obsahu okna programu
 */
void onDraw(void)
{
	/* Spojnice mezi kontrolnimi body */
	drawLines(control_points, &COLOR_GREY);
	/* Vykreslime vsechny kontrolni body */
	drawCrossPoints(control_points, 5, &COLOR_GREEN);
	/* Kreslime samotnou krivku */
	drawLines(curve_points, &COLOR_WHITE);

    /* Test existence frame bufferu a obrazove pameti */
	IZG_ASSERT(frame_buffer && screen);

    /* Test typu pixelu */
    IZG_ASSERT(screen->format->BytesPerPixel == 4);
    /* Kopie bufferu do obrazove pameti */
    SDL_LockSurface(screen);

	/* Test, pokud kopirujeme rozdilnou velikost frame_bufferu a rozdilne pameti, musime pamet prealokovat */
	if (width != screen->w || height != screen->h)
	{
		SDL_SetVideoMode(width, height, 32, SDL_SWSURFACE);
		/*SDL_FreeSurface(screen);
		if (!(screen = SDL_SetVideoMode(width, height, 32, SDL_SWSURFACE)))
		{
			IZG_ERROR("Cannot realocate screen buffer");
			SDL_Quit();
		}*/
	}
	MEMCOPY(screen->pixels, frame_buffer, sizeof(S_RGBA) * width * height);
    SDL_UnlockSurface(screen);

    /* Vymena zobrazovaneho a zapisovaneho bufferu */
    SDL_Flip(screen);

    /* Vynulovani framebufferu, do ktereho vykreslujeme */
    memset(frame_buffer, 0 , sizeof(S_RGBA)*width*height);

}

void changeWeight(Point2d* point, float increment)
{
	if(point != NULL)
		active_point->weight *= increment;
}

/******************************************************************************
 ******************************************************************************
 * funkce reagujici na stisknuti klavesnice
 * key - udalost klavesnice
 */
void onKeyboard(SDL_KeyboardEvent *key)
{
    /* test existence rendereru */
	IZG_ASSERT(frame_buffer);
    /* vetveni podle stisknute klavesy */
    switch( key->keysym.sym )
    {
        /* ukonceni programu - klavesa Esc*/
        case SDLK_ESCAPE:
            quit = 1;
            break;
        case SDLK_b:
        	bezier = &bezierBernsteinPolynom;
        	bezier(quality, control_points, curve_points);
        	printf("Using Bernstein polynom to compute the Bézier curve\n");
        	break;
        case SDLK_c:
			bezier = &bezierDeCasteljau;
			bezier(quality, control_points, curve_points);
			printf("Using De Casteljau algorithm to compute the Bézier curve\n");
			break;
        case SDLK_UP:
        	printf("Increasing the weight...\n");
        	changeWeight(active_point, 1.2);
        	bezier(quality, control_points, curve_points);
        	break;
        case SDLK_DOWN:
			printf("Decreasing the weight...\n");
			changeWeight(active_point, 0.8);
			bezier(quality, control_points, curve_points);
			break;
        default:
            break;
    }
}

/******************************************************************************
 ******************************************************************************
 * funkce reagujici na udalost mysi
 * mouse - udalost mysi
 */

void onMouseDown(SDL_MouseMotionEvent *mouse)
{
	Point2d p = {mouse->x, mouse->y, 1};
	int indexSnap = -1;

	switch(mouse->state)
    {
    case SDL_BUTTON_LMASK:
		if(active_point != NULL)
		{
			active_point->x += mouse->xrel;
			active_point->y += mouse->yrel;
		}
		else
		{
			indexSnap = pointSnap(&p, snipeDistance, control_points);
			if(indexSnap != -1)
			{
				active_point = point2d_vecGetPtr(control_points, indexSnap);
				active_point->x += mouse->xrel;
				active_point->y += mouse->yrel;
			}
			else
			{
				point2d_vecPushBack(control_points, p);
				/*printf("Info: Click on coordinates %d %d)\n", mouse->x, mouse->y);*/
			}
		}
		if(point2d_vecSize(control_points) >= 2 )
			(*bezier)(100, control_points, curve_points);
    	break;
    default:
    	break;
    }
}

void onMouseUp(SDL_MouseMotionEvent *mouse)
{
	switch(mouse->state)
	{
	case SDL_BUTTON_LMASK:
		if(active_point != NULL)
		{
			active_point->x = mouse->x;
			active_point->y = mouse->y;
			active_point = NULL;
		}
		break;
	}
}

/******************************************************************************
 ******************************************************************************
 * funkce tiskne napovedu na stdout
 */
void printHelpText()
{
	IZG_INFO("Application loaded - IZG LAB 04 - Curves.\n\n Controls:\n\n"
			 "Left mouse click: \n"
			 "    Defines the control point\n"
			 "    Drag and move with control points\n"
			 "Key b: \n"
			 "    Using Bernstein polynom to compute the Bézier Curve\n"
			 "Key c: \n"
			 "    Using de Casteljau algorithm - to compute the Bézier Curve\n"
			"Key arrow_up and mouse holding the control point: \n"
			 "    Increasing its weight - Rational Bézier curve\n"
      "Key arrow_down and mouse holding the control point: \n"
			 "    Decreasing its weight - Rational Bézier curve\n"
			 "========================================================================\n\n")
}

/******************************************************************************
 ******************************************************************************
 * hlavni funkce programu
 * argc - pocet vstupnich parametru
 * argv - pole vstupnich parametru
 */

int main(int argc, char *argv[])
{
	SDL_Event event;

	/* Inicializace SDL knihovny */
    if( SDL_Init(SDL_INIT_VIDEO) == -1 )
    {
        IZG_SDL_ERROR("Could not initialize SDL library");
    }

    /* Nastaveni okna */
    SDL_WM_SetCaption(PROGRAM_TITLE, 0);

	/* Alokace frame bufferu (okno + SW zapisovaci */
	if (!(screen = SDL_SetVideoMode(width, height, 32, SDL_SWSURFACE)))
    {
        SDL_Quit();
        return 1;
    }

	if (!(frame_buffer = (S_RGBA *)malloc(sizeof(S_RGBA) * width * height)))
	{
		SDL_Quit();
        return 1;
	}

	control_points = vecCreateEmpty(sizeof(Point2d));
	curve_points = vecCreateEmpty(sizeof(Point2d));

	printHelpText();

	/* Kreslime, dokud nenarazime na SDL_QUIT event */
	while(!quit)
    {

		/*Udalost se vyridi, az prijde*/
		SDL_WaitEvent(&event);
		switch( event.type )
		{
			/* Udalost klavesnice */
			case SDL_KEYDOWN:
				onKeyboard(&event.key);
				break;

			/* Udalost mysi */
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEMOTION:
				onMouseDown(&event.motion);
				break;
			case SDL_MOUSEBUTTONUP:
				onMouseUp(&event.motion);
				break;
			/* SDL_QUIT event */
			case SDL_QUIT:
				quit = 1;
				break;

			default:
				break;
		}

		/* Provedeme preklopeni zapisovaciho frame_bufferu na obrazovku*/
		onDraw();
    }

	/* Uvolneni pameti */
	point2d_vecRelease(&control_points);
	point2d_vecRelease(&curve_points);

	SDL_FreeSurface(screen);
	free(frame_buffer);
    SDL_Quit();

	IZG_INFO("Bye bye....\n\n");
    return 0;
}


/*****************************************************************************/
/*****************************************************************************/
