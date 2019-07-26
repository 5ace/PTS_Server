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
 * BoundingBox.cpp
 *
 *  Created on: Nov 26, 2012
 *      Author: massimo
 */

#include "BoundingBox.h"
#include "CdvsException.h"
#include <string>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#ifdef _WIN32
    #define ISNAN _isnan
#elif _WIN64
    #define ISNAN _isnan
#elif __MACH__
	#define ISNAN std::isnan
# elif defined(__MINGW32__) || defined(__CYGWIN__)
#  define ISNAN(x) (std::isnan(x))
#else
    #define ISNAN std::isnan
#endif

const real_t Polygon::gamut = 500000000.0;
const real_t Polygon::mid = 500000000.0 / 2.0;
const real_t Polygon::bigReal = 1.E38;  	

Polygon::Polygon()	
{
	s = 0;
	B.max.x = -bigReal;		// these values will be replaced by real values
	B.max.y = -bigReal;
	B.min.x = bigReal;
	B.min.y = bigReal;
}

double Polygon::inter(int na, const point * a, int nb, const point * b)
{

	double ascale;

	if (na < 3 || nb < 3)
		return 0;

	vertex *ipa = new vertex[na + 1];
	vertex *ipb = new vertex[nb + 1];

	range(na, a);
	range(nb, b);

	// compute ascale
	{
		real_t rngx = B.max.x - B.min.x;
		real_t sclx = gamut / rngx;
		real_t rngy = B.max.y - B.min.y;
		real_t scly = gamut / rngy;
		{

			fit(na, a, ipa, 0, sclx, scly);
			fit(nb, b, ipb, 2, sclx, scly);
		}
		ascale = sclx * scly;
	}
	// compute s
	{
		s = 0;
		int j, k;

		for (j = 0; j < na; ++j)
			for (k = 0; k < nb; ++k)
				if (ovl(ipa[j].rx, ipb[k].rx) && ovl(ipa[j].ry, ipb[k].ry)) {
					hp a1 = -area(ipa[j].ip, ipb[k].ip, ipb[k + 1].ip), a2 =
							area(ipa[j + 1].ip, ipb[k].ip, ipb[k + 1].ip);
					{
						int o = a1 < 0;
						if (o == a2 < 0) {
							hp a3 = area(ipb[k].ip, ipa[j].ip, ipa[j + 1].ip),
									a4 = -area(ipb[k + 1].ip, ipa[j].ip,
											ipa[j + 1].ip);
							if (a3 < 0 == a4 < 0) {
								if (o)
									cross(&ipa[j], &ipa[j + 1], &ipb[k],
											&ipb[k + 1], a1, a2, a3, a4);
								else
									cross(&ipb[k], &ipb[k + 1], &ipa[j],
											&ipa[j + 1], a3, a4, a1, a2);
							}
						}
					}
				}
		{
			inness(na, ipa, nb, ipb);
			inness(nb, ipb, na, ipa);
		}
		delete [] ipa;
		delete [] ipb;
		return (double) s / ascale;
	}
}

BoundingBox::BoundingBox()
{
	for (int k=0; k<4; ++k)
	{
		bbox[k].x = 0. ;
		bbox[k].y = 0. ;
	}
}

bool BoundingBox::isValid()
{
	for (int k=0; k<4; ++k)
	{
		if ((ISNAN(bbox[k].x)) || (ISNAN(bbox[k].y)))
			return false;
	}
	return true;
}

void BoundingBox::read (const char *fn)
{
  int i; FILE *file; char buf[4*20];
  /* open file: */
  if ((file = fopen (fn, "rb")) == NULL) {
	  throw mpeg7cdvs::CdvsException(std::string("BoundingBox::read - can't open bounding box file: ").append(fn));
  }
  /* read bounding box coordinates: */
  char cc;
  for (i=0; i<4; i++) {
    if (fgets (buf, sizeof(buf), file) == NULL)
    	throw mpeg7cdvs::CdvsException(std::string("BoundingBox::read - can't read bounding box file: ").append(fn));

    if (sscanf(buf, "%f%c%f", &bbox[i].x, &cc, &bbox[i].y) != 3) {
    	throw mpeg7cdvs::CdvsException(std::string("BoundingBox::read - can't read coordinates in file: ").append(fn));
    }
  }
  /* close file and exit: */
  fclose (file);
}

mpeg7cdvs::CDVSPOINT * BoundingBox::getAddress() {
	return bbox;
}

double BoundingBox::getIntersection(BoundingBox &other)
{
	Polygon polygon;
	return fabs(polygon.inter(4, getAddress(), 4, other.getAddress()));
}

//  Public-domain function by Darel Rex Finley, 2006.
double BoundingBox::getArea() const {

  double  area=0. ;
  int points = 4;
  int     i, j=points-1  ;

  for (i=0; i<points; i++) {
    area += (bbox[j].x + bbox[i].x)*(bbox[j].y - bbox[i].y); j=i;
  }

  return fabs(area) * 0.5;
}

double BoundingBox::find_overlap(BoundingBox & other)
{
	/* verify input data */
	if (isValid() && other.isValid())
	{
		/* compute areas of these quadrilaterals: */
		double area1 = getArea();
		double area2 = other.getArea();
		double area3 = getIntersection(other);

		/* compute Jaccard index: */
		double jindex = area3 / (area1 + area2 - area3);
		return jindex;
	}
	else
		return 0.0;
}


/*
 * Compute ratio of intersection to union of two quadrilateral regions in an image (backward compatible version).
 */
double BoundingBox::cfind_overlap(int width, int height, mpeg7cdvs::CDVSPOINT *quad1, mpeg7cdvs::CDVSPOINT *quad2)
{
  mpeg7cdvs::CDVSPOINT p, q1[4], q2[4];
  int i, in1, in2, in1and2, in1or2;
  float delta = 1.0; // precision of area scan (1 = pixel-level, <1 - sub-pixel)

  /* get vertices: */
  memcpy (q1, quad1, sizeof(mpeg7cdvs::CDVSPOINT)*4);
  memcpy (q2, quad2, sizeof(mpeg7cdvs::CDVSPOINT)*4);

  /* check if bottom-up bitmap: */
  if (height < 0) {
    height = -height;
    for (i=0; i<4; i++) {
      q1[i].y = height - q1[i].y;
      q2[i].y = height - q2[i].y;
    }
  }

  /* sort vertices clock-wise: */
  arrange_cyclic (q1);
  arrange_cyclic (q2);

  /* compute areas of intersection and union of these quadrilaterals: */
  in1and2 = in1or2 = 0;
  for (p.y=0.; p.y<height; p.y+=delta) {
    for (p.x=0.; p.x<width; p.x+=delta) {
      /* does p belongs to q1, q2? */
      in1 = point_in_quad (&p, q1);
      in2 = point_in_quad (&p, q2);
      /* count # of points in intersection and union: */
      if (in1 && in2) in1and2++;
      if (in1 || in2) in1or2++;
    }
  }

  /* return Jaccard index: */
  return (double)(in1and2) / (double)(in1or2);
}

/*
 * Point + angle structure and comparison function:
 */
int BoundingBox::cmp_angles (PX *a, PX *b)
{
  double x = a->angle - b->angle;
  return (x > 0)? 1: (x < 0)? -1: 0;
}

/*
 * Arrange vertices in cyclic order:
 * @param quad quadrilateral
 */
void BoundingBox::arrange_cyclic (mpeg7cdvs::CDVSPOINT *quad)
{
  PX q[4]; float mean_x, mean_y; int i;

  /* compute center point: */
  mean_x = mean_y = 0.;
  for (i=0; i<4; i++) {mean_x += quad[i].x; mean_y += quad[i].y;}
  mean_x /= 4.; mean_y /= 4.;

  /* compute angles: */
  for (i=0; i<4; i++) {
    q[i].angle = atan2 (quad[i].y - mean_y, quad[i].x - mean_x);
    q[i].v.x = quad[i].x; q[i].v.y = quad[i].y;
  }

  /* sort vertices: */
  qsort(q, 4, sizeof(PX), (int (*)(const void*, const void*)) cmp_angles);

  /* store the results: */
  for (i=0; i<4; i++) {quad[i].x = q[i].v.x; quad[i].y = q[i].v.y;}
}

/*
 * Test point inclusion in quadrilateral.
 * 	Based on pnpoly() test:
 *  		  reference http://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
 * @param point point to test
 * @param quad vertices of quadrilateral
 * @return  0 - point is outside of the quadrilateral; !0 - point belogs to quadrilateral with coordinates vert[0..3]
*/
int BoundingBox::point_in_quad (mpeg7cdvs::CDVSPOINT *point, mpeg7cdvs::CDVSPOINT *quad)
{
  int i, j, c = 0;
  for (i = 0, j = 3; i < 4; j = i++) {
    if (((quad[i].y > point->y) != (quad[j].y > point->y)) &&
      (point->x < (quad[j].x - quad[i].x) * (point->y - quad[i].y) / (quad[j].y - quad[i].y) + quad[i].x))
      c = !c;
  }
  return c;
}

