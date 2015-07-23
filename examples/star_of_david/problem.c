/**
 * Star of David
 * 
 * This example uses the IAS15 integrator
 * to integrate the "Star od David", a four body system consisting of two
 * binaries orbiting each other. Note that the time is running backwards,
 * which illustrates that IAS15 can handle both forward and backward in time
 * integrations. The initial conditions are by Robert Vanderbei.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include "rebound.h"


int main(int argc, char* argv[]){
	struct reb_simulation* r = reb_create_simulation();
	r->integrator = REB_INTEGRATOR_IAS15;
	r->dt = -1;

	struct reb_particle p = {.m = 1., .z = 0., .vz = 0.};
	
	p.x =  -1.842389804706855; p.y =  -1.063801316823613; 
	p.vx =  -0.012073765486548; p.vy =   0.021537467220014; 
	reb_add(r, p);

	p.x =  -0.689515464218133; p.y =  -0.398759403276399; 
	p.vx =   0.637331229856386; p.vy =  -1.103822313621890; 
	reb_add(r, p);
	
	p.x =   0.689515464218133; p.y =   0.398759403276399; 
	p.vx =  -0.637331229856386; p.vy =   1.103822313621890; 
	reb_add(r, p);
	
	p.x =   1.842389804706855; p.y =   1.063801316823613; 
	p.vx =   0.012073765486548; p.vy =  -0.021537467220014; 
	reb_add(r, p);
	
	reb_move_to_com(r);

	reb_integrate(r, INFINITY);
}
