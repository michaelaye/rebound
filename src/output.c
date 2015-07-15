/**
 * @file 	output.c
 * @brief 	Output routines.
 * @author 	Hanno Rein <hanno@hanno-rein.de>
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include "particle.h"
#include "rebound.h"
#include "tools.h"
#include "output.h"
#include "integrator_sei.h"
#include "input.h"
#ifdef OPENGL
#include "display.h"
#ifdef LIBPNG
#include <png.h>
#endif // LIBPNG
#ifdef _APPLE
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif  // _APPLE
#endif  // OPENGL
#ifdef MPI
#include "communication_mpi.h"
#include "mpi.h"
#endif // MPI

// Check if output is needed

int reb_output_check(struct reb_simulation* r, double interval){
	return reb_output_check_phase(r, interval,0);
}

int reb_output_check_phase(struct reb_simulation* r, double interval,double phase){
	double shift = r->t+interval*phase;
	if (floor(shift/interval)!=floor((shift-r->dt)/interval)){
		return 1;
	}
	// Output at beginning 
	if (r->t==0){
		return 1;
	}
	return 0;
}


/**
 * 3D vector struct.
 */
struct vec3 {
	double x;
	double y;
	double z;
};

#ifdef PROFILING
double profiling_time_sum[PROFILING_CAT_NUM];
double profiling_time_initial 	= 0;
double profiling_time_final 	= 0;
void profiling_start(void){
	struct timeval tim;
	gettimeofday(&tim, NULL);
	profiling_time_initial = tim.tv_sec+(tim.tv_usec/1000000.0);
}
void profiling_stop(int cat){
	struct timeval tim;
	gettimeofday(&tim, NULL);
	profiling_time_final = tim.tv_sec+(tim.tv_usec/1000000.0);
	profiling_time_sum[cat] += profiling_time_final - profiling_time_initial;
}
#endif // PROFILING

void reb_output_timing(struct reb_simulation* r, const double tmax){
	const int N = r->N;
#ifdef MPI
	int N_tot = 0;
	MPI_Reduce(&N, &N_tot, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD); 
	if (mpi_id!=0) return;
#else
	int N_tot = N;
#endif
	struct timeval tim;
	gettimeofday(&tim, NULL);
	double temp = tim.tv_sec+(tim.tv_usec/1000000.0);
	if (r->reb_output_timing_last==-1){
		r->reb_output_timing_last = temp;
	}else{
		printf("\r");
#ifdef PROFILING
		fputs("\033[A\033[2K",stdout);
		for (int i=0;i<=PROFILING_CAT_NUM;i++){
			fputs("\033[A\033[2K",stdout);
		}
#endif // PROFILING
	}
	printf("N_tot= %- 9d  ",N_tot);
	if (r->integrator==RB_IT_SEI){
		printf("t= %- 9f [orb]  ",r->t*r->ri_sei.OMEGA/2./M_PI);
	}else{
		printf("t= %- 9f  ",r->t);
	}
	printf("dt= %- 9f  ",r->dt);
	if (r->integrator==RB_IT_HYBRID){
		printf("INT= %- 1d  ",r->ri_hybrid.mode);
	}
	printf("cpu= %- 9f [s]  ",temp-r->reb_output_timing_last);
	if (tmax>0){
		printf("t/tmax= %5.2f%%",r->t/tmax*100.0);
	}
#ifdef PROFILING
	printf("\nCATEGORY       TIME \n");
	double _sum = 0;
	for (int i=0;i<=PROFILING_CAT_NUM;i++){
		switch (i){
			case PROFILING_CAT_INTEGRATOR:
				printf("Integrator     ");
				break;
			case PROFILING_CAT_BOUNDARY:
				printf("Boundary check ");
				break;
			case PROFILING_CAT_GRAVITY:
				printf("Gravity/Forces ");
				break;
			case PROFILING_CAT_COLLISION:
				printf("Collisions     ");
				break;
#ifdef OPENGL
			case PROFILING_CAT_VISUALIZATION:
				printf("Visualization  ");
				break;
#endif // OPENGL
			case PROFILING_CAT_NUM:
				printf("Other          ");
				break;
		}
		if (i==PROFILING_CAT_NUM){
			printf("%5.2f%%",(1.-_sum/(profiling_time_final - timing_initial))*100.);
		}else{
			printf("%5.2f%%\n",profiling_time_sum[i]/(profiling_time_final - timing_initial)*100.);
			_sum += profiling_time_sum[i];
		}
	}
#endif // PROFILING
	fflush(stdout);
	r->reb_output_timing_last = temp;
}


void reb_output_append_ascii(struct reb_simulation* r, char* filename){
	const int N = r->N;
#ifdef MPI
	char filename_mpi[1024];
	sprintf(filename_mpi,"%s_%d",filename,mpi_id);
	FILE* of = fopen(filename_mpi,"a"); 
#else // MPI
	FILE* of = fopen(filename,"a"); 
#endif // MPI
	if (of==NULL){
		printf("\n\nError while opening file '%s'.\n",filename);
		return;
	}
	for (int i=0;i<N;i++){
		struct reb_particle p = r->particles[i];
		fprintf(of,"%e\t%e\t%e\t%e\t%e\t%e\n",p.x,p.y,p.z,p.vx,p.vy,p.vz);
	}
	fclose(of);
}

void reb_output_ascii(struct reb_simulation* r, char* filename){
	const int N = r->N;
#ifdef MPI
	char filename_mpi[1024];
	sprintf(filename_mpi,"%s_%d",filename,mpi_id);
	FILE* of = fopen(filename_mpi,"w"); 
#else // MPI
	FILE* of = fopen(filename,"w"); 
#endif // MPI
	if (of==NULL){
		printf("\n\nError while opening file '%s'.\n",filename);
		return;
	}
	for (int i=0;i<N;i++){
		struct reb_particle p = r->particles[i];
		fprintf(of,"%e\t%e\t%e\t%e\t%e\t%e\n",p.x,p.y,p.z,p.vx,p.vy,p.vz);
	}
	fclose(of);
}

void reb_output_append_orbits(struct reb_simulation* r, char* filename){
	const int N = r->N;
#ifdef MPI
	char filename_mpi[1024];
	sprintf(filename_mpi,"%s_%d",filename,mpi_id);
	FILE* of = fopen(filename_mpi,"a"); 
#else // MPI
	FILE* of = fopen(filename,"a"); 
#endif // MPI
	if (of==NULL){
		printf("\n\nError while opening file '%s'.\n",filename);
		return;
	}
	struct reb_particle com = r->particles[0];
	for (int i=1;i<N;i++){
		struct orbit o = reb_tools_p2orbit(r->G, r->particles[i],com);
		fprintf(of,"%e\t%e\t%e\t%e\t%e\t%e\t%e\t%e\t%e\n",r->t,o.a,o.e,o.inc,o.Omega,o.omega,o.l,o.P,o.f);
		com = reb_get_com(com,r->particles[i]);
	}
	fclose(of);
}

void reb_output_orbits(struct reb_simulation* r, char* filename){
	const int N = r->N;
#ifdef MPI
	char filename_mpi[1024];
	sprintf(filename_mpi,"%s_%d",filename,mpi_id);
	FILE* of = fopen(filename_mpi,"w"); 
#else // MPI
	FILE* of = fopen(filename,"w"); 
#endif // MPI
	if (of==NULL){
		printf("\n\nError while opening file '%s'.\n",filename);
		return;
	}
	struct reb_particle com = r->particles[0];
	for (int i=1;i<N;i++){
		struct orbit o = reb_tools_p2orbit(r->G, r->particles[i],com);
		fprintf(of,"%e\t%e\t%e\t%e\t%e\t%e\t%e\t%e\t%e\n",r->t,o.a,o.e,o.inc,o.Omega,o.omega,o.l,o.P,o.f);
		com = reb_get_com(com,r->particles[i]);
	}
	fclose(of);
}


void reb_output_binary(struct reb_simulation* r, char* filename){
#ifdef MPI
	char filename_mpi[1024];
	sprintf(filename_mpi,"%s_%d",filename,mpi_id);
	FILE* of = fopen(filename_mpi,"wb"); 
#else // MPI
	FILE* of = fopen(filename,"wb"); 
#endif // MPI
	if (of==NULL){
		printf("\n\nError while opening file '%s'.\n",filename);
		return;
	}
	fwrite(r,sizeof(struct reb_simulation),1,of);
	fwrite(r->particles,sizeof(struct reb_particle),r->N,of);
	fclose(of);
}

void reb_output_binary_positions(struct reb_simulation* r, char* filename){
	const int N = r->N;
#ifdef MPI
	char filename_mpi[1024];
	sprintf(filename_mpi,"%s_%d",filename,mpi_id);
	FILE* of = fopen(filename_mpi,"wb"); 
#else // MPI
	FILE* of = fopen(filename,"wb"); 
#endif // MPI
	if (of==NULL){
		printf("\n\nError while opening file '%s'.\n",filename);
		return;
	}
	for (int i=0;i<N;i++){
		struct vec3 v;
		v.x = r->particles[i].x;
		v.y = r->particles[i].y;
		v.z = r->particles[i].z;
		fwrite(&(v),sizeof(struct vec3),1,of);
	}
	fclose(of);
}

void reb_output_append_velocity_dispersion(struct reb_simulation* r, char* filename){
	const int N = r->N;
	// Algorithm with reduced roundoff errors (see wikipedia)
	struct vec3 A = {.x=0, .y=0, .z=0};
	struct vec3 Q = {.x=0, .y=0, .z=0};
	for (int i=0;i<N;i++){
		struct vec3 Aim1 = A;
		struct reb_particle p = r->particles[i];
		A.x = A.x + (p.vx-A.x)/(double)(i+1);
		if (r->integrator==RB_IT_SEI){
			A.y = A.y + (p.vy+1.5*r->ri_sei.OMEGA*p.x-A.y)/(double)(i+1);
		}else{
			A.y = A.y + (p.vy-A.y)/(double)(i+1);
		}
		A.z = A.z + (p.vz-A.z)/(double)(i+1);
		Q.x = Q.x + (p.vx-Aim1.x)*(p.vx-A.x);
		if (r->integrator==RB_IT_SEI){
			Q.y = Q.y + (p.vy+1.5*r->ri_sei.OMEGA*p.x-Aim1.y)*(p.vy+1.5*r->ri_sei.OMEGA*p.x-A.y);
		}else{
			Q.y = Q.y + (p.vy-Aim1.y)*(p.vy-A.y);
		}
		Q.z = Q.z + (p.vz-Aim1.z)*(p.vz-A.z);
	}
#ifdef MPI
	int N_tot = 0;
	struct vec3 A_tot = {.x=0, .y=0, .z=0};
	struct vec3 Q_tot = {.x=0, .y=0, .z=0};
	MPI_Reduce(&N, &N_tot, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD); 
	MPI_Reduce(&A, &A_tot, 3, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD); 
	MPI_Reduce(&Q, &Q_tot, 3, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD); 
	if (mpi_id!=0) return;
#else
	int N_tot = N;
	struct vec3 A_tot = A;
	struct vec3 Q_tot = Q;
#endif
	Q_tot.x = sqrt(Q_tot.x/(double)N_tot);
	Q_tot.y = sqrt(Q_tot.y/(double)N_tot);
	Q_tot.z = sqrt(Q_tot.z/(double)N_tot);
	FILE* of = fopen(filename,"a"); 
	if (of==NULL){
		printf("\n\nError while opening file '%s'.\n",filename);
		return;
	}
	fprintf(of,"%e\t%e\t%e\t%e\t%e\t%e\t%e\n",r->t,A_tot.x,A_tot.y,A_tot.z,Q_tot.x,Q_tot.y,Q_tot.z);
	fclose(of);
}

	
#ifdef OPENGL
#ifdef LIBPNG
unsigned char* 	imgdata = NULL;
int reb_output_png_num = 0;
void reb_output_png(char* dirname){
	char filename[1024];
	sprintf(filename,"%s%09d.png",dirname,reb_output_png_num);
	reb_output_png_num++;
	reb_output_png_single(filename);
}

void reb_output_png_single(char* filename){
	// Read Image
	if (display_init_done==0) return;
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	int width = viewport[2];
	int height = viewport[3];
	glReadBuffer(GL_BACK);
	//glReadBuffer(GL_FRONT);
	if (imgdata==NULL){
		imgdata = calloc(width*height*3,sizeof(unsigned char));
	}
	png_byte* row_pointers[height];
	for (int h = 0; h < height; h++) {
		row_pointers[height-h-1] = (png_bytep) &imgdata[width*3*h];
	}

	glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,imgdata);

	/* open the file */
	FILE *fp;
	png_structp png_ptr;
	png_infop info_ptr;
	fp = fopen(filename, "wb");
	if (fp==NULL){
		printf("\n\nError while opening file '%s'.\n",filename);
		return;
	}

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (png_ptr == NULL) {
		fclose(fp);
		return;
	}

	/* Allocate/initialize the image information data.  REQUIRED */
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		fclose(fp);
		png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
		return;
	}

	/* Set error handling.  REQUIRED if you aren't supplying your own
	* error hadnling functions in the png_create_write_struct() call.
	*/
	/*
	if (setjmp(png_ptr->jmpbuf))
	{
	fclose(fp);
	png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
	return;
	}
	*/

	/* I/O initialization functions is REQUIRED */
	/* set up the output control if you are using standard C streams */
	png_init_io(png_ptr, fp);

	/* Set the image information here.  Width and height are up to 2^31,
	* bit_depth is one of 1, 2, 4, 8, or 16, but valid values also depend on
	* the color_type selected. color_type is one of PNG_COLOR_TYPE_GRAY,
	* PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COLOR_TYPE_PALETTE, PNG_COLOR_TYPE_RGB,
	* or PNG_COLOR_TYPE_RGB_ALPHA.  interlace is either PNG_INTERLACE_NONE or
	* PNG_INTERLACE_ADAM7, and the compression_type and filter_type MUST
	* currently be PNG_COMPRESSION_TYPE_BASE and PNG_FILTER_TYPE_BASE. REQUIRED
	*/
	png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB,
	PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	/* Write the file  */
	png_write_info(png_ptr, info_ptr);
	png_write_image(png_ptr, row_pointers);
	png_write_end(png_ptr, info_ptr);

	/* if you allocated any text comments, free them here */

	/* clean up after the write, and free any memory allocated */
	png_destroy_write_struct(&png_ptr, (png_infopp)NULL);

	/* close the file */
	fclose(fp);
}
#endif
#endif

