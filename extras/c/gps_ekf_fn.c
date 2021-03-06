/* gps_ekf: TinyEKF test case using You Chong's GPS example:
 * 
 *   http://www.mathworks.com/matlabcentral/fileexchange/31487-extended-kalman-filter-ekf--for-gps
 * 
 * Reads file gps.csv of satellite data and writes file ekf.csv of mean-subtracted estimated positions.
 *
 *
 * References:
 *
 * 1. R G Brown, P Y C Hwang, "Introduction to random signals and applied 
 * Kalman filtering : with MATLAB exercises and solutions",1996
 *
 * 2. Pratap Misra, Per Enge, "Global Positioning System Signals, 
 * Measurements, and Performance(Second Edition)",2006
 * 
 * Copyright (C) 2015 Simon D. Levy
 *
 * MIT License
 */

#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include <math.h>
#include "get_time.h"

#include "tinyekf_config.h"
#include "tiny_ekf.h"


// positioning interval
static const double T = 1;
#ifdef AWSM
extern double myatof(const char *s);
#endif

static void blkfill(ekf_t * ekf, const double * a, int off)
{
    off *= 2;

    ekf->Q[off]   [off]   = a[0]; 
    ekf->Q[off]   [off+1] = a[1];
    ekf->Q[off+1] [off]   = a[2];
    ekf->Q[off+1] [off+1] = a[3];
}


static void init(ekf_t * ekf)
{
    // Set Q, see [1]
    const double Sf    = 36;
    const double Sg    = 0.01;
    const double sigma = 5;         // state transition variance
    const double Qb[4] = {Sf*T+Sg*T*T*T/3, Sg*T*T/2, Sg*T*T/2, Sg*T};
    const double Qxyz[4] = {sigma*sigma*T*T*T/3, sigma*sigma*T*T/2, sigma*sigma*T*T/2, sigma*sigma*T};

    blkfill(ekf, Qxyz, 0);
    blkfill(ekf, Qxyz, 1);
    blkfill(ekf, Qxyz, 2);
    blkfill(ekf, Qb,   3);

    // initial covariances of state noise, measurement noise
    double P0 = 10;
    double R0 = 36;

    int i;

    for (i=0; i<8; ++i)
        ekf->P[i][i] = P0;

    for (i=0; i<4; ++i)
        ekf->R[i][i] = R0;

    // position
    ekf->x[0] = -2.168816181271560e+006;
    ekf->x[2] =  4.386648549091666e+006;
    ekf->x[4] =  4.077161596428751e+006;

    // velocity
    ekf->x[1] = 0;
    ekf->x[3] = 0;
    ekf->x[5] = 0;

    // clock bias
    ekf->x[6] = 3.575261153706439e+006;

    // clock drift
    ekf->x[7] = 4.549246345845814e+001;
}

static void model(ekf_t * ekf, double SV[4][3])
{ 

    int i, j;

    for (j=0; j<8; j+=2) {
        ekf->fx[j] = ekf->x[j] + T * ekf->x[j+1];
        ekf->fx[j+1] = ekf->x[j+1];
    }

    for (j=0; j<8; ++j)
        ekf->F[j][j] = 1;

    for (j=0; j<4; ++j)
        ekf->F[2*j][2*j+1] = T;

    double dx[4][3];

    for (i=0; i<4; ++i) {
        ekf->hx[i] = 0;
        for (j=0; j<3; ++j) {
            double d = ekf->fx[j*2] - SV[i][j];
            dx[i][j] = d;
            ekf->hx[i] += d*d;
        }
        ekf->hx[i] = pow(ekf->hx[i], 0.5) + ekf->fx[6];
    }

    for (i=0; i<4; ++i) {
        for (j=0; j<3; ++j) 
            ekf->H[i][j*2]  = dx[i][j] / ekf->hx[i];
        ekf->H[i][6] = 1;
    }   
}

static void readline(char * line, FILE * fp, int fd)
{
    fgets(line, 1000, fp);
    //int i = 0;

    //for (i = 0; i < 1000; i++) {
    //        int r = read(fd, line + i, 1);
    //        //int r = fread(line + i, 1, 1, fp);
    //        if (r <= 0) break;
    //        if (line[i] == '\n') break;
    //}
    //printf("%s, %d\n", line, i);
}

static void readdata(int fd, FILE * fp, double SV_Pos[4][3], double SV_Rho[4])
{
    char line[1000];

    readline(line, fp, fd);

    char * p = strtok(line, ",");

    int i, j;

    for (i=0; i<4; ++i)
        for (j=0; j<3; ++j) {
#ifdef AWSM
            SV_Pos[i][j] = myatof(p);
#else
            SV_Pos[i][j] = atof(p);
#endif
            p = strtok(NULL, ",");
        }

    for (j=0; j<4; ++j) {
#ifdef AWSM
        SV_Rho[j] = myatof(p);
#else
        SV_Rho[j] = atof(p);
#endif
        p = strtok(NULL, ",");
    }
}


static void skipline(int fd, FILE * fp)
{
    char line[1000];
    readline(line, fp, fd);
}

void error(const char * msg)
{
    fprintf(stderr, "%s\n", msg);
}

//#define MAX_SIZE 4096
//char inp_ekf_buf[MAX_SIZE] = { 0 };
//#define SV_POS_SZ (4*3*8)
//#define SV_RHO_SZ (4*8)
//#define POS_KF_SZ (3*8)
#define ENTRIES 1
#define DUMP_FILE "ekf_raw.dat"
//#define DUMP_STATE

#ifdef DUMP_STATE

int main(int argc, char ** argv)
{
    // Do generic EKF initialization
    ekf_t ekf;
    ekf_init(&ekf, Nsta, Mobs);

    // Do local initialization
    init(&ekf);

    // Open input data file
    FILE * ifp = fopen("gps.csv", "r");
    int ifd = -1;//open("gps.csv", O_RDONLY);

    // Skip CSV header
    skipline(ifd, ifp);

    // Make a place to store the data from the file and the output of the EKF
    double SV_Pos[4][3];
    double SV_Rho[4];
    double Pos_KF[ENTRIES][3];

    // Open output CSV file and write header
    //const char * OUTFILE = "ekf.csv";
    //FILE * ofp = fopen(OUTFILE, "w");
    //fprintf(ofp, "X,Y,Z\n");
    FILE *dfp = fopen(DUMP_FILE, "w+b");

    assert(ENTRIES == 1);
    int j, k;

    // Loop till no more data
    for (j=0; j<ENTRIES; ++j) {

        readdata(ifd, ifp, SV_Pos, SV_Rho);

        model(&ekf, SV_Pos);

        ekf_step(&ekf, SV_Rho);

        // grab positions, ignoring velocities
        for (k=0; k<3; ++k)
            Pos_KF[j][k] = ekf.x[2*k];
    }
    int r = fwrite(&ekf, 1, sizeof(ekf), dfp);
    assert(r == sizeof(ekf));
    r = fwrite(&SV_Pos, 1, sizeof(SV_Pos), dfp);
    assert(r == sizeof(SV_Pos));
    r = fwrite(&SV_Rho, 1, sizeof(SV_Rho), dfp);
    assert(r == sizeof(SV_Rho));
    r = fwrite(&Pos_KF[0], 1, sizeof(Pos_KF), dfp);
    assert(r == sizeof(Pos_KF));
    fclose(dfp);


    // Compute means of filtered positions
    double mean_Pos_KF[3] = {0, 0, 0};
    for (j=0; j<ENTRIES; ++j) 
        for (k=0; k<3; ++k)
            mean_Pos_KF[k] += Pos_KF[j][k];
    for (k=0; k<3; ++k)
        mean_Pos_KF[k] /= ENTRIES;

    en = get_time();
    print_time(st, en);
    // Dump filtered positions minus their means
    for (j=0; j<ENTRIES; ++j) {
        printf("%f,%f,%f\n", 
                Pos_KF[j][0]-mean_Pos_KF[0], Pos_KF[j][1]-mean_Pos_KF[1], Pos_KF[j][2]-mean_Pos_KF[2]);
        printf("%f %f %f\n\n", Pos_KF[j][0], Pos_KF[j][1], Pos_KF[j][2]);
    }
    
    // Done!
    fclose(ifp);
    //close(ifd);
    //fclose(ofp);
    //printf("Wrote file %s\n", OUTFILE);
    return 0;
}
#else
int main(int argc, char ** argv)
{
//	unsigned long long st = get_time(), en;	
	ekf_t ekf;
	double SV_Pos[4][3];
	double SV_Rho[4];
	double Pos_KF[3];

	//read state and input positions (binary)
	int r = read(0, &ekf, sizeof(ekf_t));
	assert(r == sizeof(ekf_t));
	r = read(0, &SV_Pos, sizeof(SV_Pos));
	assert(r == sizeof(SV_Pos));
	r = read(0, &SV_Rho, sizeof(SV_Rho));
	assert(r == sizeof(SV_Rho));
	r = read(0, &Pos_KF, sizeof(Pos_KF));
	assert(r == sizeof(Pos_KF));
	int k;

	//RUN EKF!!!
	/*************************************/
	model(&ekf, SV_Pos);
	ekf_step(&ekf, SV_Rho);
	// grab positions, ignoring velocities
	for (k=0; k<3; ++k)
		Pos_KF[k] = ekf.x[2*k];
	/*************************************/

	//write state..(binary)
	r = write(1, &ekf, sizeof(ekf));
	assert(r == sizeof(ekf));
	//write the input..
	r = write(1, &SV_Pos, sizeof(SV_Pos));
	assert(r == sizeof(SV_Pos));
	r = write(1, &SV_Rho, sizeof(SV_Rho));
	assert(r == sizeof(SV_Rho));
	//write the output positions (binary)
	r = write(1, &Pos_KF[0], sizeof(Pos_KF));
	assert(r == sizeof(Pos_KF));
//	en = get_time();
//	print_time(st, en);

	return 0;
}

#endif
