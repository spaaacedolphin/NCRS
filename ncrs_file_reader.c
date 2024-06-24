#include <stdio.h>
#include <stdlib.h>

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

int main(void)
{
    FILE *fp = fopen("binary_planar_3body.ncrs", "rb");

    char simulation_type;
    int num_celest;
    double max_t;
    double delta_t;
    signed char selected_default_units[3];

    fread((void*)(&simulation_type),sizeof(char),1,fp);
	fread((void*)(&num_celest),sizeof(int),1,fp);
	fread((void*)(&max_t),sizeof(double),1,fp);
	fread((void*)(&delta_t),sizeof(double),1,fp);
	fread((void*)(selected_default_units),sizeof(char),3,fp);

	Celestial * celest = (Celestial *)malloc(sizeof(Celestial)*num_celest);

    int i,j;
    for(i=0;i<num_celest;i++){
        for(j=0;j<31;j++){
            celest[i].name[j]=0;
        }
    }

	for(i=0;i<num_celest;i++){
        j=0;
        while(1){
            char cur_ch;
            fread((void*)(&cur_ch),sizeof(char),1,fp);
            if(cur_ch=='\0')
                break;
            else
                celest[i].name[j]=cur_ch;
                j++;
        }
	}

    for(i=0;i<num_celest;i++){
        fread((void*)(&(celest[i].mass)),sizeof(double),1,fp);
    }

    for(i=0;i<num_celest;i++){
        fread((void*)(&(celest[i].vel)),sizeof(Vector),1,fp);
    }

    for(i=0;i<num_celest;i++){
        fread((void*)(&(celest[i].pos)),sizeof(Vector),1,fp);
    }

    char ** type_str[2] = {"N-body","CR3BP"};
    char ** mass_unit_str[5] = {"kg","Solar Mass","Jupiter Mass","Earth Mass","Lunar Mass"};
    char ** pos_unit_str[3] = {"m","km","au"};
    char ** vel_unit_str[2] = {"m/s","km/s"};

    printf("simulation_type: %s\n",type_str[simulation_type]);
    printf("num_celest: %d\n",num_celest);
    printf("max_t: %f\tdelta_t: %f\n",max_t,delta_t);
    printf("default unit of mass: %s\n",mass_unit_str[selected_default_units[0]]);
    printf("default unit of position: %s\n",pos_unit_str[selected_default_units[1]]);
    printf("default unit of velocity: %s\n",vel_unit_str[selected_default_units[2]]);

    for(i=0;i<num_celest;i++){
        printf("\n--------------------\n");
        printf("body_%d\n",i+1);
        printf("name: %s\n",celest[i].name);
        printf("mass: %f\n",celest[i].mass);
        printf("initial position: (%f, %f, %f)\n",celest[i].pos.x,celest[i].pos.y,celest[i].pos.z);
        printf("initial velocity: (%f, %f, %f)\n",celest[i].vel.x,celest[i].vel.y,celest[i].vel.z);
    }
    return 0;
}
