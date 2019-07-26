/*
 * This software module was originally developed by:
 *
 *   Telecom Italia
 *
 * in the course of development of ISO/IEC 15938-13 Compact Descriptors for Visual
 * Search standard for reference purposes and its performance may not have been
 * optimized. This software module includes implementation of one or more tools as
 * specified by the ISO/IEC 15938-13 standard.
 *
 * ISO/IEC gives you a royalty-free, worldwide, non-exclusive, copyright license to copy,
 * distribute, and make derivative works of this software module or modifications thereof
 * for use in implementations of the ISO/IEC 15938-13 standard in products that satisfy
 * conformance criteria (if any).
 *
 * Those intending to use this software module in products are advised that its use may
 * infringe existing patents. ISO/IEC have no liability for use of this software module
 * or modifications thereof.
 *
 * Copyright is not released for products that do not conform to audiovisual and image-
 * coding related ITU Recommendations and/or ISO/IEC International Standards.
 *
 * Telecom Italia retain full rights to modify and use the code for their own
 * purposes, assign or donate the code to a third party and to inhibit third parties
 * from using the code for products that do not conform to MPEG-related
 * ITU Recommendations and/or ISO/IEC International Standards.
 *
 * This copyright notice must be included in all copies or derivative works.
 * Copyright (c) ISO/IEC 2012.
 *
 */

/*
 * BoundingBox.h
 *
 *  Created on: Nov 26, 2012
 *      Author: massimo
 */

#pragma once

#include "CdvsPoint.h"

typedef mpeg7cdvs::CDVSPOINT point;
typedef float real_t;


/**
 * A base class for all polygons;
 */
class Polygon {
private:

	static const real_t gamut;
	static const real_t mid;
	static const real_t bigReal;  	// FLT_MAX, DBL_MAX if double above

	typedef long long int hp;

	struct ipoint{
		long int x;
		long int y;
		ipoint(int _x, int _y) { x=_x; y=_y;};
		ipoint(long int _x, long int _y) { x=_x; y=_y;};
		ipoint(real_t _x, real_t _y) { x = (long int) _x; y = (long int) _y;};
	} ;

	struct rng{
		long int mn;
		long int mx;

		rng(long int n, long int x) { mn=n; mx=x;};
	} ;

	struct vertex{
		ipoint ip;
		rng rx;
		rng ry;
		short int in;

		vertex ():
			ip(0,0),
			rx(0,0),
			ry(0,0),
			in(0) 
			{};
	} ;

	typedef struct {
		point min;
		point max;
	} box;


	// member variables

	box B;
	hp s;

	// private methods

	void bd(real_t * X, real_t y)
	{
		*X = *X<y ? *X:y;
	}

	void bu(real_t * X, real_t y)
	{
		*X = *X>y ? *X:y;
	}

	void range(int c, const point *x) {
		while(c--)
		{
			bd(&B.min.x, x[c].x); bu(&B.max.x, x[c].x);
			bd(&B.min.y, x[c].y); bu(&B.max.y, x[c].y);
		}
	}

	void fit(int cx, const point * x, vertex * ix, int fudge, real_t sclx, real_t scly) {
		{	int c=cx; while(c--) {
				ix[c].ip.x = (((long)((x[c].x - B.min.x)*sclx - mid)&~7)|fudge|(c&1));
				ix[c].ip.y = (((long)((x[c].y - B.min.y)*scly - mid)&~7)|fudge);
			}
		}
		ix[0].ip.y += cx&1;
		ix[cx] = ix[0];
		{
			int c=cx; while(c--)
			{
				ix[c].rx = ix[c].ip.x < ix[c+1].ip.x ?
					rng(ix[c].ip.x,ix[c+1].ip.x):rng(ix[c+1].ip.x,ix[c].ip.x);
				ix[c].ry = ix[c].ip.y < ix[c+1].ip.y ?
					rng(ix[c].ip.y,ix[c+1].ip.y):rng(ix[c+1].ip.y,ix[c].ip.y);
				ix[c].in=0;
			}
		}
	}

	hp area(ipoint a, ipoint p, ipoint q)
	{
		return (hp)p.x*q.y - (hp)p.y*q.x +
				(hp)a.x*(p.y - q.y) + (hp)a.y*(q.x - p.x);
	}

	void cntrib(ipoint f, ipoint t, short w)
	{
		s += (hp)w*(t.x-f.x)*(t.y+f.y)/2;
	}

	int ovl(rng p, rng q)
	{
		return p.mn < q.mx && q.mn < p.mx;
	}

	void cross(vertex * a, vertex * b, vertex * c, vertex * d,
				double a1, double a2, double a3, double a4)
	{
		real_t r1=a1/((real_t)a1+a2), r2 = a3/((real_t)a3+a4);
		cntrib( ipoint(a->ip.x + r1*(b->ip.x-a->ip.x),
					a->ip.y + r1*(b->ip.y-a->ip.y)),
				b->ip, 1);
		cntrib(d->ip, ipoint(
					c->ip.x + r2*(d->ip.x - c->ip.x),
					c->ip.y + r2*(d->ip.y - c->ip.y)), 1);
		++a->in; --c->in;
	}

	void inness(int cP, vertex * P, int cQ, vertex * Q)
	{
		int s=0, c=cQ; ipoint p = P[0].ip;
		while(c--)if(Q[c].rx.mn < p.x && p.x < Q[c].rx.mx)
		{	int sgn = 0 < area(p, Q[c].ip, Q[c+1].ip);
			s += sgn != Q[c].ip.x < Q[c+1].ip.x ? 0 : (sgn?-1:1);
		}
		{	int j; for(j=0; j<cP; ++j)
			{	if(s) cntrib(P[j].ip, P[j+1].ip, s);
				s += P[j].in;
			}
		}
	}


public:
	// constructor
	Polygon();

	/**
	 * Compute the intersection area of two polygons.
	 * @param na number of points of polygon A
	 * @param a points of polygon A
	 * @param nb number of points of polygon B
	 * @param b points of polygon B
	 * @return the intersection area.
	 */
	double inter(int na, const point * a, int nb, const point * b);

};



/**
 * A class containing four points which identify the object in the image.
 */
class BoundingBox {
private:
	  typedef struct {mpeg7cdvs::CDVSPOINT v; double angle;} PX;
	  mpeg7cdvs::CDVSPOINT bbox[4];

	  bool isValid();
	  double getArea() const;	// compute area of the bounding box
	  double getIntersection(BoundingBox &other); // compute area of intersection
	  static int cmp_angles (PX *a, PX *b);
	  static void arrange_cyclic (mpeg7cdvs::CDVSPOINT *quad);
	  static int point_in_quad (mpeg7cdvs::CDVSPOINT *point, mpeg7cdvs::CDVSPOINT *quad);

public:
	  BoundingBox();

	  /**
	   * Read the four points from a bbox file.
	   */
	  void read (const char *fn);

	  /**
	   * Get the bounding box base address.
	   * @return the address of the bbox vector
	   */
	  mpeg7cdvs::CDVSPOINT * getAddress();

	  /**
	   * Compute ratio of intersection to union of two quadrilateral regions in an image.
	   * This is the new version which computes the Jaccard index correctly.
	   * @param other the other bounding box
	   * @return area of intersection / area of union (~Jaccard Index)
	   */
	  double find_overlap (BoundingBox & other);

	  /**
	   * Compute ratio of intersection to union of two quadrilateral regions in an image (backward compatible version).
	   * This is the first version of find_overlap, which limits the area of union and intersection to the query image area.
	   * @param width width of the image
	   * @param height height of the image; height < 0 implies bottom-up ordering of rows (as in .bmp files)
	   * @param quad1 vertices of first quadrilateral defined as points in image space (can be outside boundaries)
	   * @param quad2 vertices of second quadrilateral defined as points in image space (can be outside boundaries)
	   * @return area of intersection / area of union (~Jaccard Index)
	   */
	  static double cfind_overlap(int width, int height, mpeg7cdvs::CDVSPOINT *quad1, mpeg7cdvs::CDVSPOINT *quad2);

};
