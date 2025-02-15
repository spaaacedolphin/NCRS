#ifndef CELESTIAL_H
#define CELESTIAL_H

typedef struct vec3{
	double x;
	double y;
	double z;
} Vec3;

#define MAXNAMELEN 31
typedef struct celestial{
	char name[MAXNAMELEN];
	Vec3 pos;
	Vec3 vel;
	Vec3 acc;
	double mass;
} Celestial;

typedef struct celestial_list{
	Celestial * celest_pointer;
	int number;
} Celestial_list;

typedef struct simulation_settings{
	char filename[60];
	double max_t;
	double delta_t;
} Simulation_settings;

typedef struct cr_binary_system{
	Celestial primary;
	Celestial secondary;
	Vec3 L1;
	Vec3 L2;
	Vec3 L3;
	Vec3 L4;
	Vec3 L5;
	double angular_speed;
} cr_Binary_system;

typedef struct cr_system{
	cr_Binary_system binary_sys;
	Celestial_list inf_body;
} cr_System;

Vec3 makeVec(double x, double y, double z);

Vec3 scalar_multi(double sca, Vec3 vec);	//sca*vec

Vec3 scalar_div(Vec3 vec, double sca);	//vec/sca

Vec3 vec_add(Vec3 vec_1, Vec3 vec_2);	//vec_1 + vec_2

Vec3 vec_subtr(Vec3 vec_1, Vec3 vec_2);	//vec_1 - vec_2

double vec_mag(Vec3 vec);	//|vec|

Vec3 unit_vec(Vec3 vec);	//unit vector of vec

Vec3 acc_gravity(Celestial * c1, Celestial * c2);	//c1's acceleration by gravity of c2

double kepler_law3_period(double m1, double m2, double a);

double kepler_law3_semi_major_axis(double m1, double m2, double T);

double circular_orbit_speed(double m1, double m2, double r);

int is_exit_cl(Celestial_list cl);

int is_exit_ss(Simulation_settings ss);

#endif/*CELESTIAL_H*/
