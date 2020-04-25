/* use Cartesian coordinates to find the city closest to 
   Beaverton, Oregon, and print the name and distance 
   from Beaverton */

#include <stdio.h>
#include <math.h>
#include <parallel/microtask.h> /* microtasking header */
#include <parallel/parallel.h>  /* parallel library 
                                   header */

#define NCITIES 10     /* number of cities */
#define BITE 1         /* bite of work for hungry puppy */

      /* Global shared memory data */

	shared float shortest;    /* distance to 
						nearest city */
	shared int closest;       /* index of 
						nearest city */
	struct location {
		char *name;
		float x, y;
	}; 
	shared struct location cities[NCITIES] = {
		{ "CHICAGO", 2000., 100. },
		{ "DENVER", 500., -550. },
		{ "NEW YORK", 150., 100. },
		{ "SEATTLE", 0., 200. },
		{ "MIAMI", 3500., -2000. },
		{ "SAN FRANCISCO", -100., -1000. },
		{ "RENO", 200., -600. },
		{ "PORTLAND", -17., 0.  },
		{ "WASHINGTON D.C.", 3000., -400. },
		{ "TILLAMOOK", -70., -50. },
	};
	shared struct location beaverton = { "BEAVERTON",
	    0., 0. };

main ()
{
  void get_cities(), find_dis(), m_fork(); 

  shortest = 999999999.;
  m_fork(find_dis, cities);
  printf("%s is closest to Beaverton.\n",
	  cities[closest].name);
  printf("%s is %3.2f miles from Beaverton.\n", 
	  cities[closest].name, shortest);
}
/* find distance to nearest city */

void 
find_dis(cities)
struct location cities[];
{
int i, base, top;  /* local loop index, start & end value */
float xsqdis, ysqdis, dist;

while ((base = BITE*(m_next( )-1)) < NCITIES) {
	top = base + BITE;	/* take a bite of work */
	if (top >= NCITIES)
		top = NCITIES-1;

		/* execute all iterations in bite of work */

		for (i = base; i <= top; i++) {
		    xsqdis = pow(fabs(beaverton.x - cities[i].x),2.);
		    ysqdis = pow(fabs(beaverton.y - cities[i].y),2.);
		    dist   = sqrt(xsqdis + ysqdis);
		    m_lock();
		    if (dist < shortest) {
				closest = i;
				shortest = dist;
				}
			m_unlock();
		}
	}
}
