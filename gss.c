#include <math.h>

static const double r = (sqrt(5)-1)/2;

double golden_section_search_max(double (*f)(double), double a, double b, double DeltaX){
	double t = (log(DeltaX/(b-a))/log(r)) + 1;
	int n = ceil(t);
	
	int i;
	double x1 = a + r*(b-a);
	double x2 = b - r*(b-a);
	
	for(i=0;i<n;i++){
		if(f(x1)>f(x2)){
			a = x2;
			x2 = x1;
			x1 = a + r*(b-a);
		}
		else{
			b = x1;
			x1 = x2;
			x2 = b - r*(b-a);
		}
	}
	
	return (a+b)/2;
}

double golden_section_search_min(double (*f)(double), double a, double b, double DeltaX){
	double t = (log(DeltaX/(b-a))/log(r)) + 1;
	int n = ceil(t);
	
	int i;
	double x1 = a + r*(b-a);
	double x2 = b - r*(b-a);
	
	for(i=0;i<n;i++){
		if(f(x1)<f(x2)){
			a = x2;
			x2 = x1;
			x1 = a + r*(b-a);
		}
		else{
			b = x1;
			x1 = x2;
			x2 = b - r*(b-a);
		}
	}
	
	return (a+b)/2;
}
