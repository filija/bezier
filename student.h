/******************************************************************************
 * Laborator 04 - Zaklady pocitacove grafiky - IZG
 * ihulik@fit.vutbr.cz
 *
 * $Id:$
 *
 * Popis: Hlavicky funkci pro funkce studentu
 *
 * Opravy a modifikace:
 * - isvoboda@fit.vutbr.cz -
 */

#ifndef Student_H
#define Student_H

/*****************************************************************************
 * Includes
 */
#include "color.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/* mocnina */
double Power(double a, int b);

/* vypocet faktorialu */
double Factorial(int n);

/* vypocet kombinacniho cisla 'n' nad 'k' */
double BinomialCoefficient(int n, int k);

/* Vypocet Bernsteinova polynomu */
double BernsteinPolynom(int n, int i, double t);


//------------------------Zde dopiste Vas kod-------------------------
// generovani bodu Bezierovy krivky
void    bezierBernsteinPolynom(int quality, const S_Vector *points, S_Vector *line_points);
void	bezierRecursivePure(int quality, const S_Vector *points, S_Vector *line_points);
void	bezierDeCasteljau(int quality, const S_Vector *points, S_Vector *line_points);

#ifdef __cplusplus
}
#endif

#endif /* Student_H */

/*****************************************************************************/
/*****************************************************************************/
