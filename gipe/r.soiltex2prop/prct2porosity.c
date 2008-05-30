#include<stdio.h>

#define POLYGON_DIMENSION 50

struct vector{
	double sand;
	double clay;
	double silt;
};

double prct2porosity(double sand_input, double clay_input){
	int i,index;
	double temp,porosity;
	double silt_input=0.0; 	//Rawls et al (1990)
				//do not have silt input
	// set up mark index for inside/outside polygon check
	double mark[POLYGON_DIMENSION]={0.0};
	//printf("in prct2poros(), Volume Fraction\n");
	//setup the 3Dvectors and initialize them
	struct vector cls[POLYGON_DIMENSION] = {0.0};
	//In case silt is not == 0.0, fill up explicitly
	for(i=0;i<POLYGON_DIMENSION;i++){
		cls[i].sand=0.0;
		cls[i].clay=0.0;
		cls[i].silt=0.0;
	}
	//transform input from [0,1] to [0,100]
	sand_input *= 100.0;
	clay_input *= 100.0;
	//fill up initial polygon points
	cls[0].sand=0.0;
	cls[0].clay=100.0;
	cls[1].sand=10.0;
	cls[1].clay=90.0;
	cls[2].sand=25.0;
	cls[2].clay=75.0;
	//Get started
	mark[0]=point_in_triangle(sand_input,clay_input,silt_input,cls[0].sand,cls[0].clay,cls[0].silt,cls[1].sand,cls[1].clay,cls[1].silt,cls[2].sand,cls[2].clay,cls[2].silt);
	if(mark[0]==1){
		porosity=0.575;
		index=1;
		//printf("Poros=0.575\n");
	}
	if (index==0){// if index not found then continue
		cls[0].sand=10.0;
		cls[0].clay=0.0;
		cls[1].sand=20.0;
		cls[1].clay=20.0;
		cls[2].sand=50.0;
		cls[2].clay=0.0;
		mark[0]=point_in_triangle(sand_input,clay_input,silt_input,cls[0].sand,cls[0].clay,cls[0].silt,cls[1].sand,cls[1].clay,cls[1].silt,cls[2].sand,cls[2].clay,cls[2].silt);
		if(mark[0]==1){
			index=1;
			//printf("Poros=0.575\n");
			porosity=0.575;
		}
	}
	if (index==0){// if index not found then continue
		cls[0].sand=100.0;
		cls[0].clay=0.0;
		cls[1].sand=50.0;
		cls[1].clay=50.0;
		cls[2].sand=50.0;
		cls[2].clay=43.0;
		mark[0]=point_in_triangle(sand_input,clay_input,silt_input,cls[0].sand,cls[0].clay,cls[0].silt,cls[1].sand,cls[1].clay,cls[1].silt,cls[2].sand,cls[2].clay,cls[2].silt);
		if(mark[0]==1){
			index=1;
			//printf("Poros=0.425\n");
			porosity=0.425;
		}
	}
	if (index==0){// if index not found then continue
		cls[0].sand=100.0;
		cls[0].clay=0.0;
		cls[1].sand=50.0;
		cls[1].clay=43.0;
		cls[2].sand=52.0;
		cls[2].clay=33.0;
		mark[0]=point_in_triangle(sand_input,clay_input,silt_input,cls[0].sand,cls[0].clay,cls[0].silt,cls[1].sand,cls[1].clay,cls[1].silt,cls[2].sand,cls[2].clay,cls[2].silt);
		if(mark[0]==1){
			index=1;
			//printf("Poros=0.425\n");
			porosity=0.425;
		}
	}
	if (index==0){// if index not found then continue
		cls[0].sand=100.0;
		cls[0].clay=0.0;
		cls[1].sand=52.0;
		cls[1].clay=33.0;
		cls[2].sand=57.0;
		cls[2].clay=25.0;
		mark[0]=point_in_triangle(sand_input,clay_input,silt_input,cls[0].sand,cls[0].clay,cls[0].silt,cls[1].sand,cls[1].clay,cls[1].silt,cls[2].sand,cls[2].clay,cls[2].silt);
		if(mark[0]==1){
			index=1;
			//printf("Poros=0.425\n");
			porosity=0.425;
		}
	}
	if (index==0){// if index not found then continue
		cls[0].sand=100.0;
		cls[0].clay=0.0;
		cls[1].sand=57.0;
		cls[1].clay=25.0;
		cls[2].sand=87.0;
		cls[2].clay=0.0;
		mark[0]=point_in_triangle(sand_input,clay_input,silt_input,cls[0].sand,cls[0].clay,cls[0].silt,cls[1].sand,cls[1].clay,cls[1].silt,cls[2].sand,cls[2].clay,cls[2].silt);
		if(mark[0]==1){
			index=1;
			//printf("Poros=0.425\n");
			porosity=0.425;
		}
	}
	if (index==0){// if index not found then continue
		cls[0].sand=0.0;
		cls[0].clay=0.0;
		cls[1].sand=25.0;
		cls[1].clay=75.0;
		cls[2].sand=0.0;
		cls[2].clay=90.0;
		mark[0]=point_in_triangle(sand_input,clay_input,silt_input,cls[0].sand,cls[0].clay,cls[0].silt,cls[1].sand,cls[1].clay,cls[1].silt,cls[2].sand,cls[2].clay,cls[2].silt);
		if(mark[0]==1){
			index=1;
			//printf("Poros=0.525\n");
			porosity=0.525;
		}
	}
	if (index==0){// if index not found then continue
		cls[0].sand=0.0;
		cls[0].clay=0.0;
		cls[1].sand=25.0;
		cls[1].clay=75.0;
		cls[2].sand=10.0;
		cls[2].clay=0.0;
		mark[0]=point_in_triangle(sand_input,clay_input,silt_input,cls[0].sand,cls[0].clay,cls[0].silt,cls[1].sand,cls[1].clay,cls[1].silt,cls[2].sand,cls[2].clay,cls[2].silt);
		if(mark[0]==1){
			index=1;
			//printf("Poros=0.525\n");
			porosity=0.525;
		}
	}
	if (index==0){// if index not found then continue
		cls[0].sand=10.0;
		cls[0].clay=0.0;
		cls[1].sand=25.0;
		cls[1].clay=75.0;
		cls[2].sand=20.0;
		cls[2].clay=20.0;
		mark[0]=point_in_triangle(sand_input,clay_input,silt_input,cls[0].sand,cls[0].clay,cls[0].silt,cls[1].sand,cls[1].clay,cls[1].silt,cls[2].sand,cls[2].clay,cls[2].silt);
		if(mark[0]==1){
			index=1;
			//printf("Poros=0.525\n");
			porosity=0.525;
		}
	}
	if (index==0){// if index not found then continue
		cls[0].sand=20.0;
		cls[0].clay=20.0;
		cls[1].sand=25.0;
		cls[1].clay=75.0;
		cls[2].sand=25.0;
		cls[2].clay=55.0;
		mark[0]=point_in_triangle(sand_input,clay_input,silt_input,cls[0].sand,cls[0].clay,cls[0].silt,cls[1].sand,cls[1].clay,cls[1].silt,cls[2].sand,cls[2].clay,cls[2].silt);
		if(mark[0]==1){
			index=1;
			//printf("Poros=0.525\n");
			porosity=0.525;
		}
	}
	if (index==0){// if index not found then continue
		cls[0].sand=20.0;
		cls[0].clay=20.0;
		cls[1].sand=25.0;
		cls[1].clay=55.0;
		cls[2].sand=27.0;
		cls[2].clay=45.0;
		mark[0]=point_in_triangle(sand_input,clay_input,silt_input,cls[0].sand,cls[0].clay,cls[0].silt,cls[1].sand,cls[1].clay,cls[1].silt,cls[2].sand,cls[2].clay,cls[2].silt);
		if(mark[0]==1){
			index=1;
			//printf("Poros=0.525\n");
			porosity=0.525;
		}
	}
	if (index==0){// if index not found then continue
		cls[0].sand=20.0;
		cls[0].clay=20.0;
		cls[1].sand=27.0;
		cls[1].clay=45.0;
		cls[2].sand=50.0;
		cls[2].clay=0.0;
		mark[0]=point_in_triangle(sand_input,clay_input,silt_input,cls[0].sand,cls[0].clay,cls[0].silt,cls[1].sand,cls[1].clay,cls[1].silt,cls[2].sand,cls[2].clay,cls[2].silt);
		if(mark[0]==1){
			index=1;
			//printf("Poros=0.525\n");
			porosity=0.525;
		}
	}
	if (index==0){// if index not found then continue
		cls[0].sand=50.0;
		cls[0].clay=0.0;
		cls[1].sand=70.0;
		cls[1].clay=0.0;
		cls[2].sand=37.0;
		cls[2].clay=25.0;
		mark[0]=point_in_triangle(sand_input,clay_input,silt_input,cls[0].sand,cls[0].clay,cls[0].silt,cls[1].sand,cls[1].clay,cls[1].silt,cls[2].sand,cls[2].clay,cls[2].silt);
		if(mark[0]==1){
			index=1;
			//printf("Poros=0.525\n");
			porosity=0.525;
		}
	}
	if (index==0){// if index not found then continue
		cls[0].sand=25.0;
		cls[0].clay=75.0;
		cls[1].sand=25.0;
		cls[1].clay=55.0;
		cls[2].sand=28.0;
		cls[2].clay=61.0;
		mark[0]=point_in_triangle(sand_input,clay_input,silt_input,cls[0].sand,cls[0].clay,cls[0].silt,cls[1].sand,cls[1].clay,cls[1].silt,cls[2].sand,cls[2].clay,cls[2].silt);
		if(mark[0]==1){
			index=1;
			//printf("Poros=0.525\n");
			porosity=0.525;
		}
	}
	if (index==0){// if index not found then continue
		cls[0].sand=25.0;
		cls[0].clay=75.0;
		cls[1].sand=37.0;
		cls[1].clay=65.0;
		cls[2].sand=28.0;
		cls[2].clay=61.0;
		mark[0]=point_in_triangle(sand_input,clay_input,silt_input,cls[0].sand,cls[0].clay,cls[0].silt,cls[1].sand,cls[1].clay,cls[1].silt,cls[2].sand,cls[2].clay,cls[2].silt);
		if(mark[0]==1){
			index=1;
			//printf("Poros=0.525\n");
			porosity=0.525;
		}
	}
	if (index==0){// if index not found then continue
		index=1;
		//printf("Poros=0.475\n");
		porosity=0.475;
	}
	return porosity;
}


