/**
 * @file 	display.c
 * @brief 	Realtime OpenGL visualization.
 * @author 	Hanno Rein <hanno@hanno-rein.de>
 * @details 	These functions provide real time visualizations
 * using OpenGL. Screenshots can be saved with the reb_output_png() routine.
 * Tested under Mac OSX Snow Leopard and Linux. 
 * 
 * @section 	LICENSE
 * Copyright (c) 2011 Hanno Rein, Shangfei Liu
 *
 * This file is part of rebound.
 *
 * rebound is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * rebound is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with rebound.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifdef OPENGL
#ifdef MPI
#error OpenGL is not compatible with MPI.
#endif //MPI
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/stat.h>
#ifdef _APPLE
#include <GLUT/glut.h>
#else // _APPLE
#include <GL/glut.h>
#endif // _APPLE
#include "tools.h"
#include "zpr.h"
#include "rebound.h"
#include "particle.h"
#include "boundary.h"
#include "tree.h"
#include "display.h"
#include "output.h"
#include "integrator.h"

#ifdef _APPLE
GLuint display_dlist_sphere;	/**< Precalculated display list of a sphere. */
#endif // APPLE
int display_spheres = 1;	/**< Switches between point sprite and real spheres. */
int display_pause_sim = 0;	/**< Pauses simulation. */
int display_pause = 0;		/**< Pauses visualization, but keep simulation running */
int display_tree = 0;		/**< Shows/hides tree structure. */
int display_mass = 0;		/**< Shows/hides centre of mass in tree structure. */
int display_wire = 0;		/**< Shows/hides orbit wires. */
int display_clear = 1;		/**< Toggles clearing the display on each draw. */
int display_ghostboxes = 0;	/**< Shows/hides ghost boxes. */
int display_reference = -1;	/**< reb_particle used as a reference for rotation. */
double display_rotate_x = 0;	/**< Rotate everything around the x-axis. */
double display_rotate_z = 0;	/**< Rotate everything around the z-axis. */
#define DEG2RAD (M_PI/180.)

/**
 * This function is called when the user presses a key. 
 * @param key Character pressed.
 * @param x Position on screen.
 * @param y Position on screen.
 */

void display_exit(void){
	// Note that there is now general way to leave GlutMainLoop(). Sad but true.
	fprintf(stderr,"\n\033[1mWarning!\033[0m Exiting OpenGL visualization now. This will immediately terminate REBOUND and not return to your program. If you need to process data after the simulation is completed, disable the OpenGL vizualization.\n");
	exit(0);
}
double display_tmax;

void display_func(void){
	if (reb_check_exit(display_r,display_tmax)!=1){
		reb_step(display_r);
#ifdef OPENGL
		PROFILING_START()
		display();
		PROFILING_STOP(PROFILING_CAT_VISUALIZATION)
#endif // OPENGL
	}else{
		display_exit();
	}
}

void displayKey(unsigned char key, int x, int y){
	switch(key){
		case 'q': case 'Q':
			display_exit();
			break;
		case ' ':
			display_pause_sim=!display_pause_sim;
			if (display_pause_sim){
				printf("Pause.\n");
				glutIdleFunc(NULL);
			}else{
				printf("Resume.\n");
				glutIdleFunc(display_func);
			}
			break;
		case 's': case 'S':
			display_spheres = !display_spheres;
			break;
		case 'g': case 'G':
			display_ghostboxes = !display_ghostboxes;
			break;
		case 'r': case 'R':
			zprReset();
			break;
		case 't': case 'T':
			display_mass = 0;
			display_tree = !display_tree;
			break;
		case 'd': case 'D':
			display_pause = !display_pause;
			break;
		case 'm': case 'M':
			display_mass = !display_mass;
			break;
		case 'w': case 'W':
			display_wire = !display_wire;
			break;
		case 'c': case 'C':
			display_clear = !display_clear;
			break;
		case 'x': 
			display_reference++;
			if (display_reference>display_r->N) display_reference = -1;
			printf("Reference particle: %d.\n",display_reference);
			break;
		case 'X': 
			display_reference--;
			if (display_reference<-1) display_reference = display_r->N-1;
			printf("Reference particle: %d.\n",display_reference);
			break;
		case 'p': case 'P':
#ifdef LIBPNG
			reb_output_png_single("screenshot.png");
			printf("\nScreenshot saved as 'screenshot.png'.\n");
#else 	// LIBPNG
			printf("\nNeed LIBPNG to save screenshot.\n");
#endif 	// LIBPNG
			break;
	}
	display();
}

/**
 * Draws a cell and all its daughters.
 * @param node Cell to draw.
 */
void display_cell(struct reb_treecell* node){
	if (node == NULL) return;
	glColor4f(1.0,0.5,1.0,0.4);
	glTranslatef(node->mx,node->my,node->mz);
	glScalef(0.04*node->w,0.04*node->w,0.04*node->w);
	if (display_mass) {
#ifdef _APPLE
		glCallList(display_dlist_sphere);
#else
		glutSolidSphere(1,40,10);
#endif
	}
	glScalef(25./node->w,25./node->w,25./node->w);
	glTranslatef(-node->mx,-node->my,-node->mz);
	glColor4f(1.0,0.0,0.0,0.4);
	glTranslatef(node->x,node->y,node->z);
	glutWireCube(node->w);
	glTranslatef(-node->x,-node->y,-node->z);
	for (int i=0;i<8;i++) {
		display_cell(node->oct[i]);
	}
}

/**
 * Draws the entire tree structure.
 */
void display_entire_tree(void){
	for(int i=0;i<display_r->root_n;i++){
		display_cell(display_r->tree_root[i]);
	}
}

struct reb_simulation* display_r = NULL;

void display(void){
	const struct reb_particle* particles = display_r->particles;
	if (display_pause) return;
	if (display_tree){
		reb_tree_update(display_r);
		if (display_r->gravity==RB_GT_TREE){
			reb_tree_update_gravity_data(display_r);
		}
	}
	if (display_clear){
	        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}
	glEnable(GL_POINT_SMOOTH);
	glVertexPointer(3, GL_DOUBLE, sizeof(struct reb_particle), particles);
	//int _N_active = ((N_active==-1)?N:N_active);
	if (display_reference>=0){
		glTranslatef(-particles[display_reference].x,-particles[display_reference].y,-particles[display_reference].z);
	}
	glRotatef(display_rotate_x,1,0,0);
	glRotatef(display_rotate_z,0,0,1);
	for (int i=-display_ghostboxes*display_r->nghostx;i<=display_ghostboxes*display_r->nghostx;i++){
	for (int j=-display_ghostboxes*display_r->nghosty;j<=display_ghostboxes*display_r->nghosty;j++){
	for (int k=-display_ghostboxes*display_r->nghostz;k<=display_ghostboxes*display_r->nghostz;k++){
		struct reb_ghostbox gb = reb_boundary_get_ghostbox(display_r, i,j,k);
		glTranslatef(gb.shiftx,gb.shifty,gb.shiftz);
		if (!(!display_clear&&display_wire)){
			// Drawing Points
			glEnableClientState(GL_VERTEX_ARRAY);
			glPointSize(3.);
			glColor4f(1.0,1.0,1.0,0.5);
			//glDrawArrays(GL_POINTS, _N_active, N-_N_active);
			glColor4f(1.0,1.0,0.0,0.9);
			glPointSize(5.);
			glDrawArrays(GL_POINTS, 0, display_r->N-display_r->N_var);
			glDisableClientState(GL_VERTEX_ARRAY);
			if (display_r->collision != RB_CT_NONE && display_spheres){
				glDisable(GL_BLEND);                    
				glEnable(GL_DEPTH_TEST);
				glEnable(GL_LIGHTING);
				glEnable(GL_LIGHT0);
				GLfloat lightpos[] = {0, display_r->boxsize_max, display_r->boxsize_max, 0.f};
				glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
				// Drawing Spheres
				glColor4f(1.0,1.0,1.0,1.0);
				for (int i=0;i<display_r->N-display_r->N_var;i++){
					struct reb_particle p = particles[i];
					if (p.r>0){
						glTranslatef(p.x,p.y,p.z);
						glScalef(p.r,p.r,p.r);
#ifdef _APPLE
						glCallList(display_dlist_sphere);
#else //_APPLE
						glutSolidSphere(1,40,10);
#endif //_APPLE
						glScalef(1./p.r,1./p.r,1./p.r);
						glTranslatef(-p.x,-p.y,-p.z);
					}
				}
				glEnable(GL_BLEND);                    
				glDisable(GL_DEPTH_TEST);
				glDisable(GL_LIGHTING);
				glDisable(GL_LIGHT0);
			}
		}
		// Drawing wires
		if (display_wire){
			if(display_r->integrator!=RB_IT_SEI){
				double radius = 0;
				struct reb_particle com = particles[0];
				for (int i=1;i<display_r->N-display_r->N_var;i++){
					struct reb_particle p = particles[i];
					if (display_r->N_active>0){
						// Different colors for active/test particles
						if (i>=display_r->N_active){
							glColor4f(0.9,1.0,0.9,0.9);
						}else{
							glColor4f(1.0,0.9,0.0,0.9);
						}
					}else{
						// Alternating colors
						if (i%2 == 1){
							glColor4f(0.0,1.0,0.0,0.9);
						}else{
							glColor4f(0.0,0.0,1.0,0.9);
						}
					}
					if (display_r->integrator==RB_IT_WHFAST && display_r->ri_whfast.is_synchronized==0){
						double m = p.m;
						p = display_r->ri_whfast.p_j[i];
						p.m = m;
						// Note: need also update to com.
						// TODO
					}
					struct orbit o = reb_tools_p2orbit(display_r->G, p,com);
					glPushMatrix();
					
					glTranslatef(com.x,com.y,com.z);
					glRotatef(o.Omega/DEG2RAD,0,0,1);
					glRotatef(o.inc/DEG2RAD,1,0,0);
					glRotatef(o.omega/DEG2RAD,0,0,1);
					
					glBegin(GL_LINE_LOOP);
					for (double trueAnom=0; trueAnom < 2.*M_PI; trueAnom+=M_PI/100.) {
						//convert degrees into radians
						radius = o.a * (1. - o.e*o.e) / (1. + o.e*cos(trueAnom));
						glVertex3f(radius*cos(trueAnom),radius*sin(trueAnom),0);
					}
					glEnd();
					glPopMatrix();
					com = reb_get_com(p,com);
				}
			}else{
				for (int i=1;i<display_r->N;i++){
					struct reb_particle p = particles[i];
					glBegin(GL_LINE_LOOP);
					for (double _t=-100.*display_r->dt;_t<=100.*display_r->dt;_t+=20.*display_r->dt){
						double frac = 1.-fabs(_t/(120.*display_r->dt));
						glColor4f(1.0,(_t+100.*display_r->dt)/(200.*display_r->dt),0.0,frac);
						glVertex3f(p.x+p.vx*_t, p.y+p.vy*_t, p.z+p.vz*_t);
					}
					glEnd();
				}
			}
		}
		// Drawing Tree
		glColor4f(1.0,0.0,0.0,0.4);
		if (display_tree && display_r->tree_root!=NULL){
			glColor4f(1.0,0.0,0.0,0.4);
			display_entire_tree();
		}
		glTranslatef(-gb.shiftx,-gb.shifty,-gb.shiftz);
	}
	}
	}
	glColor4f(1.0,0.0,0.0,0.4);
	glScalef(display_r->boxsize.x,display_r->boxsize.y,display_r->boxsize.z);
	glutWireCube(1);
	glScalef(1./display_r->boxsize.x,1./display_r->boxsize.y,1./display_r->boxsize.z);
	glRotatef(-display_rotate_z,0,0,1);
	glRotatef(-display_rotate_x,1,0,0);
	if (display_reference>=0){
		glTranslatef(particles[display_reference].x,particles[display_reference].y,particles[display_reference].z);
	}
	glutSwapBuffers();
}

void display_init(int argc, char* argv[], double tmax){
	display_tmax = tmax;
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH );
	glutInitWindowSize(700,700);
	glutCreateWindow("rebound");
	zprInit();
	glutDisplayFunc(display);
	glutIdleFunc(display_func);
	glutKeyboardFunc(displayKey);
	glDepthMask(GL_TRUE);
	glEnable(GL_BLEND);                    
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);  
	
	// Sphere
#ifdef _APPLE
	display_dlist_sphere = glGenLists(1);
	GLUquadricObj *sphere;
	glNewList(display_dlist_sphere, GL_COMPILE);
	sphere = gluNewQuadric();
	gluSphere(sphere, 1.f, 20, 20);
	gluDeleteQuadric(sphere);
	glEndList();
#endif // _APPLE
  	
	// Setup lights

	glCullFace(GL_BACK);
	glShadeModel ( GL_SMOOTH );
	glEnable( GL_NORMALIZE );
	glEnable(GL_COLOR_MATERIAL);
	static GLfloat light[] = {0.7f, 0.7f, 0.7f, 1.f};
	static GLfloat lightspec[] = {0.2f, 0.2f, 0.2f, 1.f};
	static GLfloat lmodel_ambient[] = { 0.15, 0.14, 0.13, 1.0 };

	glLightfv(GL_LIGHT0, GL_DIFFUSE, light );
	glLightfv(GL_LIGHT0, GL_SPECULAR, lightspec );
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);

	static GLfloat sphere_mat[] = {0.8f, 0.8f, 0.8f, 1.f};
	static GLfloat sphere_spec[] = {1.0f, 1.0f, 1.0f, 1.f};
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, sphere_mat);
	glMaterialfv(GL_FRONT, GL_SPECULAR, sphere_spec);
	glMaterialf(GL_FRONT, GL_SHININESS, 80);

	// Enter glut run loop and never come back.
	glutMainLoop();
}

#endif // OPENGL
