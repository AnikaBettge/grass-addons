#include<stdio.h>
#include<stdlib.h>
#include<math.h>

#define PI 3.14159265358979323846 

double fixed_deltat(double u2m, double roh_air,double cp,double dt,double disp,double z0m,double z0h,double tempk){
	int i;
	double ublend;
	double length;
	double xm,xh;
	double psim, psih;
	double ustar;
	double rah;
	double h_in;
	
	ublend=u2m*(log(100-disp)-log(z0m))/(log(2-disp)-log(z0m));

	psim=0.0;
	psih=0.0;

	for(i=0;i<10;i++){
		ustar = 0.41*ublend/(log((100-disp)/z0m)-psim);
		rah   = (log((2-disp)/z0h)-psih)/(0.41*ustar);
		h_in  = roh_air*cp*dt/rah;
		if(h_in < 0.0){
			h_in = 0.01;
		}
		length= -roh_air*cp*pow(ustar,3)*tempk/(0.41*9.81*h_in);
		xm    = pow(1.0-16.0*((100-disp)/length),0.25);
		xh    = pow(1.0-16.0*((2-disp)/length),0.25);
		psim  = 2.0*log((1.0+xm)/2.0)+log((1+xm*xm)-2*atan(xm)+0.5*PI);
		psih  = 2.0*log((1.0+xh*xh)/2.0);
	}

	return rah;
}
