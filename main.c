#define _XOPEN_SOURCE_EXTENDED 

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <locale.h>
#include <curses.h>
#include "inputbox.h"

extern char * mass_unit_options[5];
extern char * pos_unit_options[3];
extern char * vel_unit_options[2];
extern signed char selected_default_units[3];

#define UNICODE_BBLOCK  L"\u2584"
#define UNICODE_UBLOCK  L"\u2580"
#define UNICODE_FBLOCK  L"\u2588"

const wchar_t BBLOCK[] = UNICODE_BBLOCK;
const wchar_t UBLOCK[] = UNICODE_UBLOCK;
const wchar_t FBLOCK[] = UNICODE_FBLOCK;
const int BLOCK_LEN = sizeof(FBLOCK)/sizeof(wchar_t);

const char time_unit_types[5] = {'y','d','h','m','s'};
const char * file_size_units[4] = {"bytes","kB","MB","GB"};
#define KILO 1000
#define MEGA 1000000
#define GIGA 1000000000

typedef struct commands
{
	const char *text;
	void (*function)(WINDOW *);
} COMMAND;

int wch_cmpn(const wchar_t * wch1, const wchar_t * wch2, int n){
	int i;
	for(i=0;i<n;i++){
		if(wch1[i] != wch2[i])
			return 0;
	}
	return 1;
}

void update_progress_bar(double t,double max_t){
	int progress = (int)((t/max_t)*100);
	mvprintw(10,10,"%d%%",progress);
	int i;
	cchar_t ch;
	setcchar(&ch,FBLOCK,A_NORMAL,2,NULL);
	for(i=0;i<(progress/5);i++)
		mvadd_wch(10,15+i,&ch);
	setcchar(&ch,FBLOCK,A_NORMAL,0,NULL);
	for(;i<20;i++)
		mvadd_wch(10,15+i,&ch);
	refresh();
}

#define _USE_MATH_DEFINES
#include <math.h>

const double G = 6.67430e-11;

typedef struct vector{
	double x;
	double y;
	double z;
} Vector;

#define MAXNAMELEN 31
typedef struct celestial{
	char name[MAXNAMELEN];
	Vector pos;
	Vector vel;
	Vector acc;
	double mass;
} Celestial;

Vector makeVec(double x, double y, double z){
	Vector result;
	result.x=x;
	result.y=y;
	result.z=z;
	return result;
}

Vector scalar_multi(double sca, Vector vec){	//sca*vec
	Vector result = vec;
	result.x *= sca;
	result.y *= sca;
	result.z *= sca;
	return result;
}

Vector scalar_div(Vector vec, double sca){	//vec/sca
	return scalar_multi(1/sca,vec);
}

Vector vec_add(Vector vec1, Vector vec2){	//vec1+vec2
	Vector result = vec1;
	result.x += vec2.x;
	result.y += vec2.y;
	result.z += vec2.z;
	return result;
}

Vector vec_subtr(Vector vec1, Vector vec2){	//vec1-vec2
	Vector result = vec1;
	result.x -= vec2.x;
	result.y -= vec2.y;
	result.z -= vec2.z;
	return result;
}

double vec_mag(Vector vec){	//|vec|
	return sqrt( pow(vec.x,2) + pow(vec.y,2) + pow(vec.z,2) );
}

Vector unit_vec(Vector vec){	//unit vector of vec
	return scalar_div(vec,vec_mag(vec));
}

Vector rel_pos(Vector pos1,Vector pos2){	//position of pos2 relative to pos1
	return vec_subtr(pos2,pos1);
}

Vector acc_gravity(Celestial * c1, Celestial * c2){	//c1's acceleration by gravity of c2
	Vector r = rel_pos(c1->pos,c2->pos);
	return scalar_multi( (G*c2->mass)/pow(vec_mag(r),2), unit_vec(r) );
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
	Vector L1;
	Vector L2;
	Vector L3;
	Vector L4;
	Vector L5;
	double angular_speed;
} cr_Binary_system;

typedef struct cr_system{
	cr_Binary_system binary_sys;
	Celestial_list inf_body;
} cr_System;

const Celestial_list EXIT_CL = {NULL,0};
const Simulation_settings EXIT_SS = {"",0,0};

int is_exit_cl(Celestial_list cl){
	return (cl.celest_pointer==EXIT_CL.celest_pointer) && (cl.number==EXIT_CL.number);
}

int is_exit_ss(Simulation_settings ss){
	return (!strcmp(ss.filename,EXIT_SS.filename)) && (ss.max_t==EXIT_SS.max_t) && (ss.delta_t==EXIT_SS.delta_t);
}
	
Celestial_list manual_n_body_set(){
	int i,j,key;
	int num_celest=0;
	mvprintw(2,10,"[Number of Celestial bodies]");

	InputBox num_celest_input = make_input('i',3,10,6,"");
	InputBox * cur_input = &num_celest_input;
		
	bool repeat = 1;
	while(repeat){
		draw_cur_input(cur_input);
				
		noecho();
		keypad(stdscr, TRUE);
		raw();
		key = getch();
		switch(key)
		{
			case 10:
			case 13:
			case KEY_ENTER:
				draw_edit_input_bg(cur_input);
				edit_input(cur_input);
				repeat=0;
				break;
			
			case 'q':
			case 'Q':
			case 3:
				return EXIT_CL;
		}
	}
	num_celest = input_to_int(cur_input);
	erase();
	color_set(0,NULL);
	//mvprintw(2,30,"num_celest: %d",num_celest);
	//napms(2000);

	//declare default unit inputs
	InputBox mass_default_unit_inputs[5];
	InputBox pos_default_unit_inputs[3];
	InputBox vel_default_unit_inputs[2];

	//define default unit inputs
	mvprintw(2,10,"[Mass default unit options]");

	for(i=0;i<5;i++)
		mass_default_unit_inputs[i]=make_radio_input(0,i,3,10+20*i,18,mass_unit_options[i]);
	
	mvprintw(5,10,"[Position default unit options]");
	
	for(i=0;i<3;i++)
		pos_default_unit_inputs[i]=make_radio_input(1,i,6,10+6*i,4,pos_unit_options[i]);
	
	mvprintw(8,10,"[Velocity default unit options]");
	
	for(i=0;i<2;i++)
		vel_default_unit_inputs[i]=make_radio_input(2,i,9,10+6*i,4,vel_unit_options[i]);
	
	InputBox default_unit_confirm_btn = make_input('q',11,10,10,"Confirm");
	
	//connect all the inputs
	for(i=0;i<4;i++)
		h_connect_input(&mass_default_unit_inputs[i],&mass_default_unit_inputs[i+1]);
	for(i=0;i<2;i++)
		h_connect_input(&pos_default_unit_inputs[i],&pos_default_unit_inputs[i+1]);
	h_connect_input(&vel_default_unit_inputs[0],&vel_default_unit_inputs[1]);

	for(i=0;i<3;i++)
		v_connect_input(&mass_default_unit_inputs[i],&pos_default_unit_inputs[i]);
	for(i=3;i<5;i++)
		mass_default_unit_inputs[i].below_input = &pos_default_unit_inputs[2];
	for(i=0;i<2;i++)
		v_connect_input(&pos_default_unit_inputs[i],&vel_default_unit_inputs[i]);
	pos_default_unit_inputs[2].below_input = &vel_default_unit_inputs[1];
	for(i=1;i>=0;i--)
		v_connect_input(&vel_default_unit_inputs[i],&default_unit_confirm_btn);

	//start default unit selection page
	for(i=0;i<3;i++)
		selected_default_units[i] = -1; 
	cur_input = &mass_default_unit_inputs[0];
		
	repeat=1;
	while(repeat)
	{
		for(i=0;i<5;i++)
			draw_input(&mass_default_unit_inputs[i]);

		for(i=0;i<3;i++)
			draw_input(&pos_default_unit_inputs[i]);

		for(i=0;i<2;i++)
			draw_input(&vel_default_unit_inputs[i]);

		draw_input(&default_unit_confirm_btn);

		draw_cur_input(cur_input);

		if(selected_default_units[0]>=0)
			draw_edit_input(&mass_default_unit_inputs[selected_default_units[0]]);
		if(selected_default_units[1]>=0)
			draw_edit_input(&pos_default_unit_inputs[selected_default_units[1]]);
		if(selected_default_units[2]>=0)
			draw_edit_input(&vel_default_unit_inputs[selected_default_units[2]]);

		noecho();
		keypad(stdscr, TRUE);
		raw();
		key = getch();
		switch(key)
		{
			case KEY_UP:
				if(cur_input->above_input!=NULL)
				cur_input = cur_input->above_input;
				break;
			case KEY_DOWN:
				if(cur_input->below_input!=NULL)
				cur_input = cur_input->below_input;
				break;
			case KEY_LEFT:
				if(cur_input->left_input!=NULL)
				cur_input = cur_input->left_input;
				break;
			case KEY_RIGHT:
				if(cur_input->right_input!=NULL)
				cur_input = cur_input->right_input;
				break;
			
			case 10:
			case 13:
			case KEY_ENTER:
				switch(cur_input->type)
				{
					case 'r':
						selected_default_units[cur_input->rgroup] = (signed char)(cur_input->ridx);
						break;
					case 'q':
						repeat=0;
						break;
				}
				break;
			
			case 'q':
			case 'Q':
			case 3:
				return EXIT_CL;
				
			default:
				break;
		}
	}

	erase();
	color_set(0,NULL);
	//mvprintw(3,30,"%d // %d / %d / %d",num_celest,selected_default_units[0],selected_default_units[1],selected_default_units[2]);
	//refresh();
	//napms(5000);


	//declare inputs
	InputBox * name_inputs = (InputBox *)malloc(sizeof(InputBox)*num_celest);    //InputBox name_inputs[num_celest];
	InputBox * mass_inputs = (InputBox *)malloc(sizeof(InputBox)*num_celest);    //InputBox mass_inputs[num_celest];

	//InputBox init_pos_inputs[num_celest][3];
	//InputBox init_vel_inputs[num_celest][3];
	InputBox ** init_pos_inputs = (InputBox **)malloc(sizeof(InputBox *) * num_celest);
	InputBox ** init_vel_inputs = (InputBox **)malloc(sizeof(InputBox *) * num_celest);
	for(i=0;i<num_celest;i++){
		init_pos_inputs[i] = (InputBox *)malloc(sizeof(InputBox)*3);
		init_vel_inputs[i] = (InputBox *)malloc(sizeof(InputBox)*3);
	}
	InputBox * next_buttons = (InputBox *)malloc(sizeof(InputBox)*num_celest);	//InputBox next_buttons[num_celest];
	InputBox * prev_buttons = (InputBox *)malloc(sizeof(InputBox)*num_celest);	//InputBox prev_buttons[num_celest];

	//define inputs
	#define START_LINE 4
	#define LINE_PAD 3
	for(i=0;i<num_celest;i++){
		int input_y = START_LINE;
		if(!i)
			mvaddstr(input_y-1,10,"[Name]");
		name_inputs[i] = make_input('s',input_y,10,30,"");
		input_y += LINE_PAD;
		if(!i)
			mvaddstr(input_y-1,10,"[Mass]");
		mass_inputs[i] = make_input('d',input_y,10,33,"");
		input_y += LINE_PAD;
		if(!i){
			mvaddstr(input_y-1,10,"[Initial Position] (x,y,z)");
			mvaddstr(input_y-1+LINE_PAD,10,"[Initial Velocity] (x,y,z)");
		}
		for(j=0;j<3;j++){
			init_pos_inputs[i][j] = make_input('d',input_y,10+22*j,20,"");
			init_vel_inputs[i][j] = make_input('d',input_y+LINE_PAD,10+22*j,20,"");
		}
		next_buttons[i]=make_input('n',START_LINE+2*LINE_PAD,76,2,"->");
		prev_buttons[i]=make_input('p',START_LINE+2*LINE_PAD,6,2,"<-");
	}

	//connect inputs
	for(i=0;i<num_celest;i++){
		h_connect_input(prev_buttons+i,&init_pos_inputs[i][0]);
		h_connect_input(&init_pos_inputs[i][2],next_buttons+i);

		name_inputs[i].left_input = prev_buttons+i;
		mass_inputs[i].left_input = prev_buttons+i;
		name_inputs[i].right_input = next_buttons+i;
		mass_inputs[i].right_input = next_buttons+i;
		init_vel_inputs[i][0].left_input = prev_buttons+i;
		init_vel_inputs[i][2].right_input = next_buttons+i;

		for(j=0;j<2;j++){
			h_connect_input(&init_pos_inputs[i][j],&init_pos_inputs[i][j+1]);
			h_connect_input(&init_vel_inputs[i][j],&init_vel_inputs[i][j+1]);
		}

		v_connect_input(name_inputs+i,mass_inputs+i);
		v_connect_input(mass_inputs+i,&init_pos_inputs[i][0]);
		for(j=1;j<3;j++){
			init_pos_inputs[i][j].above_input=mass_inputs+i;
		}
		for(j=0;j<3;j++){
			v_connect_input(&init_pos_inputs[i][j],&init_vel_inputs[i][j]);
		}
	}

	//start the page
	i=0;
	cur_input = &name_inputs[0];
	while(i<num_celest)
	{
		color_set(0,NULL);
		mvaddstr(1,10,"                     ");
		mvprintw(1,10,"<body_%d>",i+1);
		
		draw_input(name_inputs+i);
		draw_input(mass_inputs+i);
		for(j=0;j<3;j++){
			draw_input(&init_pos_inputs[i][j]);
			draw_input(&init_vel_inputs[i][j]);
		}
		draw_input(prev_buttons+i);
		draw_input(next_buttons+i);
		draw_cur_input(cur_input);

		noecho();
		keypad(stdscr, TRUE);
		raw();
		key = getch();
		switch(key)
		{
			case KEY_UP:
				if(cur_input->above_input!=NULL)
					cur_input = cur_input->above_input;
				break;
			case KEY_DOWN:
				if(cur_input->below_input!=NULL)
					cur_input = cur_input->below_input;
				break;
			case KEY_LEFT:
				if(cur_input->left_input!=NULL)
					cur_input = cur_input->left_input;
				break;
			case KEY_RIGHT:
				if(cur_input->right_input!=NULL)
					cur_input = cur_input->right_input;
				break;
			
			case 10:
			case 13:
			case KEY_ENTER:
				switch(cur_input->type)
				{
					case 's':
					case 'd':
						draw_edit_input_bg(cur_input);
						edit_input(cur_input);
						break;
					case 'p':
						if(i){
							i--;
							cur_input = name_inputs + i;
						}
						break;
					case 'n':
						i++;
						if(i<num_celest)
							cur_input = name_inputs + i;
						break;
				}
				break;
			
			case 'q':
			case 'Q':
			case 3:
				return EXIT_CL;
				
			default:
				break;
		}
	}

	Celestial * celest = (Celestial *)malloc(sizeof(Celestial)*num_celest);

	for(i=0;i<num_celest;i++){
		input_to_name(name_inputs+i,i);
		strcpy(celest[i].name,name_inputs[i].input_str);
		
		celest[i].pos = makeVec(input_to_double(&init_pos_inputs[i][0],'p'),
								input_to_double(&init_pos_inputs[i][1],'p'),
								input_to_double(&init_pos_inputs[i][2],'p'));

		celest[i].vel = makeVec(input_to_double(&init_vel_inputs[i][0],'v'),
								input_to_double(&init_vel_inputs[i][1],'v'),
								input_to_double(&init_vel_inputs[i][2],'v'));

		celest[i].acc = makeVec(0,0,0);
		celest[i].mass = input_to_double(mass_inputs+i,'M');
	}

	free(name_inputs);  free(mass_inputs);
	free(next_buttons); free(prev_buttons);
	for(i=0;i<num_celest;i++){
		free(init_pos_inputs[i]);
		free(init_vel_inputs[i]);
	}
	free(init_pos_inputs);
	free(init_vel_inputs);

	erase();

	Celestial_list celest_list = {celest,num_celest};
	return celest_list;
}

Simulation_settings simulation_set(int num_celest){
	int i,key;
	color_set(0,NULL);
	mvaddstr(2,10,"[max_t]");
	InputBox max_t_inputs[5];
	max_t_inputs[0] = make_input('d',3,10,7,"");
	max_t_inputs[1] = make_input('d',3,24,4,"");
	max_t_inputs[2] = make_input('d',5,10,3,"");
	max_t_inputs[3] = make_input('d',5,20,3,"");
	max_t_inputs[4] = make_input('d',5,32,3,"");
	
	
	
	mvaddstr(7,10,"[delta_t]");
	InputBox delta_t_input=make_input('d',8,10,15,"");
	
	mvaddstr(10,10,"[file_name]");
	InputBox file_name_input=make_input('s',11,10,50,"");
	InputBox last_confirm_btn=make_input('q',13,10,10,"Confirm");
	
	for(i=0;i<4;i++){
		if(i==1)
			continue;
		h_connect_input(max_t_inputs+i,max_t_inputs+i+1);
	}
	for(i=0;i<2;i++)
		v_connect_input(max_t_inputs+i,max_t_inputs+i+2);
	max_t_inputs[4].above_input = max_t_inputs+1;

	for(i=2;i<5;i++)
		max_t_inputs[i].below_input = &delta_t_input;
	delta_t_input.above_input = max_t_inputs+2;
	
	
	v_connect_input(&delta_t_input,&file_name_input);
	v_connect_input(&file_name_input,&last_confirm_btn);
	
	bool repeat=1;
	InputBox * cur_input = max_t_inputs;
	double temp_max_t,temp_delta_t;
	while(repeat)
	{
		temp_max_t = 0;
		for(i=0;i<5;i++){
			draw_input(max_t_inputs+i);
			temp_max_t += input_to_double(max_t_inputs+i,time_unit_types[i]);
		}
		draw_input(&delta_t_input);
		draw_input(&file_name_input);
		draw_input(&last_confirm_btn);
		draw_cur_input(cur_input);
		
		temp_delta_t = input_to_double(&delta_t_input,'s');
		
		color_set(0,NULL);
		mvaddstr(3,18,"years");
		mvaddstr(3,29,"days");
		mvaddstr(5,14,"hours");
		mvaddstr(5,24,"minutes");
		mvaddstr(5,36,"seconds");
		mvaddstr(8,26,"seconds");
		
		if(temp_delta_t && temp_max_t){
			double efs = sizeof(double)*num_celest*(1 + 3*( (int)(temp_max_t/temp_delta_t) + 2 ) );
			int size_unit;
			if( ((int)efs)/MEGA ){
				if( ((int)efs)/GIGA ){
					size_unit = 3;
					efs /= GIGA;
				}
				else{
					size_unit = 2;
					efs /= MEGA;
				}
			}
			else if( ((int)efs)/KILO){
				size_unit = 1;
				efs /= KILO;
			}
			else{
				size_unit = 0;
			}
			mvaddstr(12,10,"                                                ");
			mvprintw(12,10,"Estimated file size: %0.2f%s",efs,file_size_units[size_unit]);
		}

		noecho();
		keypad(stdscr, TRUE);
		raw();
		key = getch();
		switch(key)
		{
			case KEY_UP:
				if(cur_input->above_input!=NULL)
					cur_input = cur_input->above_input;
				break;
			case KEY_DOWN:
				if(cur_input->below_input!=NULL)
					cur_input = cur_input->below_input;
				break;
			case KEY_LEFT:
				if(cur_input->left_input!=NULL)
					cur_input = cur_input->left_input;
				break;
			case KEY_RIGHT:
				if(cur_input->right_input!=NULL)
					cur_input = cur_input->right_input;
				break;

			case 10:
			case 13:
			case KEY_ENTER:
				switch(cur_input->type)
				{
					case 's':
					case 'd':
						draw_edit_input_bg(cur_input);
						edit_input(cur_input);
						break;
					case 'q':
						repeat=0;
					break;
				}
				break;
				
			case 'q':
			case 'Q':
			case 3:
				return EXIT_SS;
				
			default:
				break;
		}
	}
	erase();

	double max_t=0;
	double delta_t=1;

	for(i=0;i<5;i++)
		max_t += input_to_double(max_t_inputs+i,time_unit_types[i]);

	delta_t = input_to_double(&delta_t_input,'s');
	
	Simulation_settings sim_settings={"",max_t,delta_t}; 
	strcpy(sim_settings.filename,file_name_input.input_str);
	strcat(sim_settings.filename,".ncrs");
	return sim_settings;
}

void n_body_sim(WINDOW *win){
	int i,j,k,key;
		
	Celestial_list celest_list =  manual_n_body_set();
	if(is_exit_cl(celest_list))
		return;
		
	Celestial * celest = celest_list.celest_pointer;
	int num_celest = celest_list.number;
	
	Simulation_settings sim_settings = simulation_set(num_celest);
	if(is_exit_ss(sim_settings))
		return;
	
	double t = 0;
	double max_t = sim_settings.max_t;
	double delta_t = sim_settings.delta_t;
	
	FILE * fp = fopen(sim_settings.filename,"wb");
	
	char simulation_type = 0;
	fwrite((void*)(&simulation_type),sizeof(char),1,fp);
	fwrite((void*)(&num_celest),sizeof(int),1,fp);
	fwrite((void*)(&max_t),sizeof(double),1,fp);
	fwrite((void*)(&delta_t),sizeof(double),1,fp);
	fwrite((void*)(selected_default_units),sizeof(char),3,fp);

	for(i=0;i<num_celest;i++)
		fwrite((void*)(celest[i].name),sizeof(char),strlen(celest[i].name)+1,fp);

	for(i=0;i<num_celest;i++)
		fwrite((void*)(&(celest[i].mass)),sizeof(double),1,fp);

	/*
	if(sizeof(Vector) == (sizeof(double)*3))
		mvaddstr(10,10,"good");
	else
		mvaddstr(10,10,"not good");
	refresh();
	napms(3000);
	erase();
	*/

	for(i=0;i<num_celest;i++)
		fwrite((void*)(&(celest[i].vel)),sizeof(Vector),1,fp);
	
	double half_delta_t = delta_t/2;
	
	for(t=0; t<=max_t; t+=delta_t){
		
		update_progress_bar(t,max_t);
			 
		//---"Kick"---//
		for(i=0; i<num_celest; i++){
			
			celest[i].acc.x=0;
			celest[i].acc.y=0;
			celest[i].acc.z=0;

			for(j=0;j<num_celest;j++){
				if(j==i)
					continue;
				celest[i].acc = vec_add( celest[i].acc, acc_gravity(celest+i,celest+j) );	//calculate net accelerations
			}

			fwrite((void*)(&(celest[i].pos)),sizeof(Vector),1,fp);	//write positions

			celest[i].vel = vec_add( celest[i].vel, scalar_multi( half_delta_t, celest[i].acc ) );	//v=v0+a(dt/2)

		}
		
		//---"Drift"---//
		for(k=0; k<num_celest; k++)
			celest[k].pos = vec_add( celest[k].pos, scalar_multi( delta_t, celest[k].vel ) );	//x=x0+vdt		

		//---"Kick"---//
		for(i=0; i<num_celest; i++){

			celest[i].acc.x=0;
			celest[i].acc.y=0;
			celest[i].acc.z=0;

			for(j=0;j<num_celest;j++){
				if(j==i)
					continue;
				celest[i].acc = vec_add( celest[i].acc, acc_gravity(celest+i,celest+j) );	//calculate net accelerations
			}

			celest[i].vel = vec_add( celest[i].vel, scalar_multi( half_delta_t, celest[i].acc ) );	//v=v0+a(dt/2)

		}
		
		//---"repeat"---//

	}
	
	for(i=0;i<num_celest;i++){
		fwrite((void*)(&(celest[i].vel)),sizeof(Vector),1,fp);
	}
	fclose(fp);
	free(celest);
	mvaddstr(12,10,"Complete!");
	mvaddstr(14,10,"Press ENTER to go back to menu...");
	refresh();
	bool repeat=1;
	while(repeat)
	{
		noecho();
		keypad(stdscr, TRUE);
		raw();
		key = getch();
		switch(key)
		{
			case 10:
			case 13:
			case KEY_ENTER:
				repeat=0;
				break;
			default:
				break;
		}
	}
	
	return;
}

/*
cr_System manual_cr3bp_set(){
	
}


cr_System JPL_cr3bp_set(){

}
*/

void cr3bp_sim(WINDOW *win){
	int i,j,k,key;
	int num_celest=0;
	
	char set_mode = 0;
	mvaddstr(2,10,"[Set Mode]");
	InputBox set_mode_inputs[2];
	set_mode_inputs[0]=make_radio_input(0,0,3,10,7,"Manual");
	set_mode_inputs[1]=make_radio_input(0,1,3,19,38,"JPL Three-Body Periodic Orbit Catalog");
	h_connect_input(set_mode_inputs,set_mode_inputs+1);

	InputBox * cur_input = set_mode_inputs;
	bool repeat=1;
	while(repeat)
	{
		for(i=0;i<2;i++)
			draw_input(set_mode_inputs+i);
		draw_cur_input(cur_input);
		
		noecho();
		keypad(stdscr, TRUE);
		raw();
		key = getch();
		switch(key)
		{
			case KEY_LEFT:
				if(cur_input->left_input!=NULL)
					cur_input = cur_input->left_input;
				break;
			case KEY_RIGHT:
				if(cur_input->right_input!=NULL)
					cur_input = cur_input->right_input;
				break;

			case 10:
			case 13:
			case KEY_ENTER:
				set_mode = cur_input->ridx;
				repeat=0;
				break;
			
			case 'q':
			case 'Q':
			case 3:
				return;
				
			default:
				break;
		}
	}

	cr_System sys;
	switch(set_mode)
	{
		case 0:
			//sys = manual_cr3bp_set();
		case 1:
			//sys = JPL_cr3bp_set();
	}
	
	return;
}

#define MAX_OPTIONS (2)

COMMAND command[MAX_OPTIONS] =
{
	{"N-body", n_body_sim},
	{"CR3BP", cr3bp_sim},
};

void loading_screen(FILE *f)
{
	if(f==NULL)
		return;
		
	int y0 = 1;
	int x0 = 30;
	
	short c;
	for(c=1;c<=3;c++){
		int x,y;
		fread((void *)(&y),sizeof(int),1,f);
		fread((void *)(&x),sizeof(int),1,f);
		
		int y_flip = 51-y+1;
		int y_line = ((y_flip-1)/2)+1;
		bool ub = y_flip%2;  // 0 when BBLOCK, 1 when UBLOCK
		
		cchar_t cch, prev_cch;
		mvin_wch(y0+y_line,x0+x,&prev_cch);
	
		wchar_t prev_wch[CCHARW_MAX];
		attr_t prev_attr;
		short prev_color;
	
		getcchar(&prev_cch, prev_wch, &prev_attr, &prev_color,NULL);
	
		if(ub){
			if((wch_cmpn(prev_wch, BBLOCK, BLOCK_LEN) || wch_cmpn(prev_wch, FBLOCK, BLOCK_LEN)) && (prev_color==c))
				setcchar(&cch, FBLOCK, A_NORMAL, c, NULL);
			
			else
				setcchar(&cch, UBLOCK, A_NORMAL, c, NULL);
		}
		else{
			if((wch_cmpn(prev_wch, UBLOCK, BLOCK_LEN) || wch_cmpn(prev_wch, FBLOCK, BLOCK_LEN)) && (prev_color==c))
				setcchar(&cch, FBLOCK, A_NORMAL, c, NULL);
			
			else
				setcchar(&cch, BBLOCK, A_NORMAL, c, NULL);
		}
	
		mvadd_wch(y0+y_line,x0+x,&cch);
	}
	
	refresh();
	
}

void display_menu(int old_option, int new_option)
{
	int lmarg = (COLS - 14) / 2,
	tmarg = (LINES - (MAX_OPTIONS + 2)) / 2;

	if(old_option==-1){
		int i;
		attrset(A_BOLD | A_REVERSE);
		mvprintw(tmarg-2, lmarg-9,"N-body & CR3BP Simulation");
		attrset(A_NORMAL);

		for(i=0;i<MAX_OPTIONS;i++)
			mvaddstr(tmarg + i, lmarg, command[i].text);
	}
	else
		mvaddstr(tmarg + old_option, lmarg, command[old_option].text);
	
	attrset(A_REVERSE);
	mvaddstr(tmarg + new_option, lmarg, command[new_option].text);
	attrset(A_NORMAL);
	
	mvaddstr(tmarg + MAX_OPTIONS + 2, lmarg - 23,
			"Use Up and Down Arrows to select - Enter to run - Q to quit");
	refresh();
}

int main(int argc, char *argv[])
{
	int i, key;
	int old_option = -1;
	int new_option = 0;
	bool quit = FALSE;
	
	initscr();
	setlocale(LC_ALL,"");
	cbreak();
	noecho();
	attrset(A_BLINK);
	mvaddstr(24, 52, "Press ENTER to start");

	FILE * file = fopen("planar_3body_tui.dat", "rb");
	if(file!=NULL){
		int num_of_bodies = 0;
		int time_len = 0;
		fread((void *)(&num_of_bodies), sizeof(int), 1, file);
		fread((void *)(&time_len),sizeof(int),1,file);
	}
	attrset(A_BOLD | A_REVERSE);
	mvprintw(1, 50,"N-body & CR3BP Simulation");
	attrset(A_NORMAL);
	//mvprintw(31, 30,"num_of_boies: %d, time_len: %d",num_of_bodies,time_len);

	start_color();
	init_pair(0,COLOR_WHITE,COLOR_BLACK);
	init_pair(1,COLOR_BLUE,COLOR_BLACK);
	init_pair(2,COLOR_GREEN,COLOR_BLACK);
	init_pair(3,COLOR_YELLOW,COLOR_BLACK);
	init_pair(4,COLOR_BLACK,COLOR_WHITE);
	init_pair(5,COLOR_WHITE,COLOR_YELLOW);
	init_pair(6,COLOR_WHITE,COLOR_BLUE);
	
	i=0;
	nodelay(stdscr,TRUE);
	while(1)
	{
		loading_screen(file);
		i++;
		if(i>=352){
			napms(3000);
			break;
		}
		raw();
		key=getch();
		if(key==10 || key==13 || key==KEY_ENTER)
			break;
		napms(30);
	}
	erase();
	nodelay(stdscr,FALSE);
	cbreak();
	display_menu(old_option, new_option);
	
	while(1)
	{
		noecho();
		keypad(stdscr, TRUE);
		raw();
		
		key = getch();

		switch(key)
		{
			case 10:
			case 13:
			case KEY_ENTER:
				old_option = -1;
				erase();
				refresh();
				(*command[new_option].function)(stdscr);
				erase();
				display_menu(old_option, new_option);
				break;

			case KEY_PPAGE:
			case KEY_HOME:
				old_option = new_option;
				new_option = 0;
				display_menu(old_option, new_option);
				break;

			case KEY_NPAGE:
			case KEY_END:
				old_option = new_option;
				new_option = MAX_OPTIONS - 1;
				display_menu(old_option, new_option);
				break;
			
			case KEY_UP:
				old_option = new_option;
				new_option = (new_option == 0) ? new_option : new_option - 1;
				display_menu(old_option, new_option);
				break;

			case KEY_DOWN:
				old_option = new_option;
				new_option = (new_option == MAX_OPTIONS - 1) ? new_option : new_option + 1;
				display_menu(old_option, new_option);
				break;
			#ifdef KEY_RESIZE
			case KEY_RESIZE:
				old_option = -1;
				erase();
				display_menu(old_option, new_option);
				break;
			#endif
			
			case 'q':
			case 'Q':
			case 3:
				quit = TRUE;
		}

		if(quit == TRUE)
			break;
	}
	
	endwin();
	return 0;
}
