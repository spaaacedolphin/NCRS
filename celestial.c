#define _USE_MATH_DEFINES
#include <math.h>
#include <string.h>
#include "celestial.h"

const double G = 6.67430e-11;

Vec3 makeVec(double x, double y, double z){
	Vec3 result;
	result.x=x;
	result.y=y;
	result.z=z;
	return result;
}

Vec3 scalar_multi(double sca, Vec3 vec){	//sca*vec
	Vec3 result = vec;
	result.x *= sca;
	result.y *= sca;
	result.z *= sca;
	return result;
}

Vec3 scalar_div(Vec3 vec, double sca){	//vec/sca
	Vec3 result = vec;
	result.x /= sca;
	result.y /= sca;
	result.z /= sca;
	return result;
}

Vec3 vec_add(Vec3 v1, Vec3 v2){	//v1 + v2
	Vec3 result = v1;
	result.x += v2.x;
	result.y += v2.y;
	result.z += v2.z;
	return result;
}

Vec3 vec_subtr(Vec3 v1, Vec3 v2){	//v1 - v2
	Vec3 result = v1;
	result.x -= v2.x;
	result.y -= v2.y;
	result.z -= v2.z;
	return result;
}

double vec_mag_squared(Vec3 vec){	//|vec|^2
	return (vec.x * vec.x) + (vec.y * vec.y) + (vec.z * vec.z);
}

double vec_mag(Vec3 vec){	//|vec|
	return sqrt(vec_mag_squared(vec));
}

Vec3 unit_vec(Vec3 vec){	//unit vector of vec
	return scalar_div(vec,vec_mag(vec));
}

Vec3 acc_gravity(Celestial * c1, Celestial * c2){	//c1's acceleration by gravity of c2
	Vec3 r = vec_subtr(c2->pos,c1->pos);
	return scalar_multi( (G*c2->mass)/vec_mag_squared(r), unit_vec(r) );
}

double kepler_law3_period(double m1, double m2, double a){
	double mu = G*(m1+m2);
	return 2*M_PI*pow(pow(a,3)/mu, 0.5);
}

double kepler_law3_semi_major_axis(double m1, double m2, double T){
	double mu = G*(m1+m2);
	return pow(mu*pow(T/(2*M_PI),2),(1/3));
}

double circular_orbit_speed(double m1, double m2, double r){
	double mu = G*(m1+m2);
	//printf("mu: %f\n",mu);
	return pow(mu/r,0.5);
}

