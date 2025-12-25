#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

int max(int a, int b){
	if (a>b) return a;
	else return b;
}
struct number diff(struct number a, struct number b);
static const int max_denominator = 1000;

typedef double ddmap(double);

const char *F_NAME[] = {"exp", "log", "log10", "log2", "sin", "cos", "tan", "sinh", "cosh", "tanh", "diff", NULL};
enum func {f_exp, f_log, f_log10, f_log2, f_sin, f_cos, f_tan, f_sinh, f_cosh, f_tanh, f_diff, numFunctions};
ddmap *math_h[]={exp, log, log10, log2, sin, cos, tan, sinh, cosh, tanh};
// a number shall consist of several parts:
// (A + N/D + f) * 10^E,
// where A is the whole part of the number
// - N is a numerator
// - D is a denominator
// - f is a small floating point adjustment,
//     in case the rational representation is not precise enough.
// - u is the uncertainty of the number, defaults to 0.5/D
struct number {
	int a; // whole part
	int n; // numerator
	int d; // denominator
	int e; // base-10 exponent scale
	double f; // double precision correction term
};

const struct number zero = {0, 0, 1, 0, 0.0};
const struct number one = {1, 0, 1, 0, 0.0};
struct number reduce(struct number z);

struct stack {
	int max;
	int size;
	struct number *element;
};

/* Allocates memory for a pointer inside of a struct.      */
/* The struct is returned by value, including the pointer. */
/* So, it is OK to not return a pointer here.              */
struct stack stack_alloc(size_t initialAllocSize){
	struct stack s;
	s.max=initialAllocSize;
	s.size=0;
	s.element=malloc(sizeof(struct number)*s.max);
	s.element[0]=zero;
	return s;
}

void stack_push(struct stack *s, struct number a){
	if (s->size == s->max) {
		s->max += 8;
		s->element = realloc(s->element,sizeof(struct number)*s->max);
	}
	s->element[s->size] = a;
	s->size++;
}

/* The first element will be popped endlessly, implemented like
 * this.
 */
struct number stack_pop(struct stack *s){
	if (s->size>0){
		s->size--;
	}
	return s->element[s->size];
}

/* The entire purpose of this function is that I dislike type-casts,
 * and macros. Typecasts, `(double) n/(double) d` look much worse than
 * function calls `frac(n,d)`.
 */
double frac(double a, double b){
	return a/b;
}

double as_double(struct number z){
	return (z.a+frac(z.n,z.d)+z.f)*pow(10.0,z.e);
}

/* The number format is a;n;d; */
struct number read_number(const char *str){
	struct number z;
	char *eptr;
	const char *p = str;
	/* defaults */
	z.n = 1;
	z.d = 1;
	z.e = 0;
	z.f = 0.0;
	/* mandatory */
	z.a = strtol(p,&eptr,10);
	//printf("   p='%s' (%p)\n",p,p);
	//printf("eptr='%s' (%p)\n",eptr,eptr);
	if (p == eptr) return z;
	else p=eptr;
	/* optional */
	if (*p==';') {
		z.n = (z.a<0?-1:1)*strtol(++p,&eptr,10);
	}
	if (p == eptr) return z;
	else p=eptr;
	if (*p==';') {
		z.d = strtol(++p,&eptr,10);
	}
	if (z.d < 0){
		z.d*=-1;
		z.n*=-1;
	}
	if (fabs(z.n)>z.d && z.n>0) {
		z.a += z.n/z.d;
		z.n %= z.d;
	} else if (fabs(z.n)>z.d && z.n<0) {
		z.a -= z.n/z.d;
		z.n %= z.d;
	}
	if (p == eptr) return z;
	else p=eptr;
	if (*p==';') {
		z.e = strtol(++p,&eptr,10);
	}
	return z;
}

/* gcdr and gcdw: these two functions are equally fast with -O2 */
/* without optimization gcdw is faster. */
int gcdr(int a, int b){
	int r=a%b;
	if (r==0) return b;
	else return gcdr(b,r);
}

int gcdw(int a, int b){
	while (a && b && ((a%=b) && (b%=a)));
	return a|b;
}

void display_raw(struct number z){
	printf("%i;%i;%i;%i\t# %g\n",z.a,z.n,z.d,z.e,as_double(z));
}

void display_double(struct number z){
	int l=round(log10(fabs(z.f)))-6;
	printf("%.*g\n",l<0?-l:2,as_double(z));
}

void display_number(struct number z){
	printf("(%i",z.a);
	if (abs(z.n) != 0) printf(" %+i/%i",z.n,z.d);
	if (fabs(z.f) > 1e-15*fabs(z.a) + 1e-15) printf(" %+.4g",z.f);
	putchar(')');
	if (z.e != 0) printf("*pow(10,%i)",z.e);
	printf("\t# %g",as_double(z));
	//printf("\t# gcd(n,d) = %i",gcdr(z.n,z.d));
	putchar('\n');
}

struct number negate(struct number z){
	z.a*=-1;
	z.n*=-1;
	z.f*=-1.0;
	return z;
}

struct number as_rational(double x){
	struct number z=zero;
	if (fabs(x)==0.0) return z;
	int sign = x<0?-1:+1;
	int l = floor(log10(fabs(x)+1e-15)/3.0)*3;
	z.e = l;
	double y = fabs(x)/pow(10,l);
	z.a = floor(y);
	y -= z.a;             /* 0 <= y < 1 */
	int a=0,b=1,c=1,d=1;
	double pq;
	while (b+d < max_denominator){
		pq=frac(a+c,b+d);
		//fprintf(stderr,"[%s] b+d=%i",__func__,b+d);
		if (pq <= y && y <= frac(c,d)){
			a+=c;
			b+=d;
		} else {
			c+=a;
			d+=b;
		}
	}
	if (fabs(y-frac(c,d))<fabs(y-frac(a,b))){
		z.n = c;
		z.d = d;
	} else {
		z.n = a;
		z.d = b;
	}
	z.f = y-frac(z.n,z.d);
	if (sign<0) return reduce(negate(z));
	else return reduce(z);
}

// with tolerances
struct number as_rational_tol(double x, double abs_tol, double rel_tol){
	struct number z;
	int l = floor(log10(x+1e-15)/3.0)*3;
	x/=pow(10,l);
	z.a = floor(x);
	x -= z.a;
	int a=0,b=1,c=1,d=1;
	double pq;
	z.e = l;
	if (fabs(x) < abs_tol+fabs(x)*rel_tol) {
		z.n = 0;
		z.d = 1;
		z.f = 0.0;
		return z;
	}
	while (fabs((pq=frac(a+c,b+d))-x) > abs_tol+fabs(x)*rel_tol){
		if (pq < x && x < frac(c,d)){
			a+=c;
			b+=d;
		} else {
			c+=a;
			d+=b;
		}
	}
	z.n=a+c;
	z.d=b+d;
	z.f=frac(z.n,z.d)-x;
	return z;
}

// Example
// x ** 0b1101 = x ** (2**3 + 2**2 + 2**0) = x ** (2**3) + x ** (2**2) * x
//             = x**8 + x**4 + x
//             = ((x**2)**2)**2 + (x**2)**2 + x
//
// calculates a = b**n; integer exponentiation
// n>=0
double pow0(double b, unsigned long n){
	int sign=b<0?-1:1;
	b*=sign;
	double a=1;
	if (n==0) return 1.0;
	while (n){
		if (n&1) a*=b;
		b*=b;
		n>>=1;
	}
	return sign<0 ? 1.0/a : a;
}

struct number reduce(struct number z){
	int sign = z.a<0?-1:+1;
	if (sign<0) {
		z=negate(z);
	}
	//int n=z.n;
	z.a+=z.n/z.d;
	z.n%=z.d;
	int g=gcdr(z.n,z.d);
	z.n/=g;
	z.d/=g;
	if (z.n==1 && z.d==1) {
		z.a++;
		z.n--;
	}
	if (sign<0) return negate(z);
	return z;
}

/* scales a number by pow(10,n)
 * returns z*pow(10,n)
 */
struct number scale10(struct number z, int n){
	int p10n = pow(10,n);
	z.n += (z.a % p10n)*z.d;
	z.a /= p10n;
	z.d *= p10n;
	z.f /= p10n;
	return reduce(z);
}

//          x.a               x.n/x.d               x.f
//        -------------   ----------------  -------------
//  y.a     x.a*y.a          y.a*x.n/x.d         y.a*x.f
// y.n/y.d  x.a*y.n/y.d    x.n*y.n/x.d*y.d    x.f*y.n/y.d
//  y.f    x.a*y.f            x.n*y.f/x.d        x.f*y.f
struct number prod(struct number x, struct number y){
	struct number z = zero;
	z.e = x.e + y.e;
	z.a = x.a*y.a;
	z.n = x.n*y.n + x.a*y.n*x.d + y.a*x.n*y.d ;
	z.d = x.d*y.d;
	z.f = x.a*y.f + y.a*x.f + y.f*frac(x.n,x.d) + x.f*frac(y.n,y.d) + x.f*y.f;
	return reduce(z);
}

struct number add(struct number x, struct number y){
	struct number z=x;
	if (x.e > y.e) y=scale10(y,x.e-y.e);
	if (x.e < y.e) x=scale10(x,y.e-x.e);
	z.a = x.a + y.a;
	z.n = x.n*y.d + y.n*x.d;
	z.d = x.d*y.d;
	z.f = x.f + y.f;
	z.a+= (x.n/x.d);
	x.n%= x.d;
	z.e = max(x.e,y.e);
	return reduce(z);
}

struct number inverse(struct number z){
	struct number x=z;
	if (memcmp(&z,&one,sizeof(struct number))==0) return one;
	if (memcmp(&z,&zero,sizeof(struct number))==0) abort();
	if (fabs(as_double(z))==0.0) abort();
	if (fabs(z.f)>0.0) return as_rational(1.0/as_double(z));
	x.a=0;
	x.n=z.d;
	x.d=z.n+z.a*z.d;
	x.e*=-1;
	return reduce(x);
}

/* Absolute differece: |a-b|
 */
struct number diff(struct number a, struct number b){
	struct number d=add(a,negate(b));
	if (as_double(d)<0) return negate(d);
	else return d;
}

double seconds(double a, double b){
	return (a-b)/CLOCKS_PER_SEC;
}

void tests(){
	int a=5*7;
	int b=8*7;
	int i,g=0;
	int N=12;
	clock_t c[2];

	printf("[%s] gcdr(%i,%i) = %i\n",__func__,a,b,gcdr(a,b));
	printf("[%s] gcdw(%i,%i) = %i\n",__func__,a,b,gcdw(a,b));

	c[0] = clock();
	srand(1337);
	for (i=0;i<N;i++){
		a=rand();
		b=rand();
		g+=gcdr(a,b);
	}
	c[1]=clock();
	printf("[%s] time %i×gcdr(a,b): %f s (sum: %i)\n",__func__,N,seconds(c[1],c[0]),g);
	g=0;
	c[0] = clock();
	srand(1337);
	for (i=0;i<N;i++){
		a=rand();
		b=rand();
		g+=gcdw(a,b);
	}
	c[1]=clock();
	printf("[%s] time %i×gcdw(a,b): %f s (sum: %i)\n",__func__,N,seconds(c[1],c[0]),g);
	// powers
	printf("[%s] pow(0.9,13) = %f; pow0(0.9,13) = %f\n",__func__,pow(0.9,13),pow0(0.9,13));
}

int match_function(char *str, const char* functions[]){
	int i=0;
	while(functions && *functions){
		if (strcmp(*functions,str)==0){
			return i; // enums are constrained integers
		} else {
			functions++;
			i++;
		}
	}
	fprintf(stderr,"[%s] The string «%s» does not correspond to a known function.\n",__func__, str); abort();
	return numFunctions; // no function found
}

int is_numeric(char *str){
	if (*str=='-' || *str=='+') str++;
	if ('0' <= *str && *str <= '9') return 1;
	return 0;
}

int is_double(char *str){
	char *ptr=NULL;
	if (strchr(str,'.')) return 1;
	ptr = strchr(str,'e');
	if (!ptr) ptr=strchr(str,'E');
	if (ptr && ptr>str && is_numeric(ptr-1) && is_numeric(ptr+1)) return 1;
	return 0;
}



/* Table of operators:
 * + adds the top two numbers on te stack
 * - negates the top of the stack
 * / inverts the top number: 1.0/num
 * * multiplies the top two numbers
 */

void evaluate(struct stack *s, char *prog){
	char *octothorpe=NULL;
	if ((octothorpe=strchr(prog,'#'))!=NULL) *octothorpe='\0';
	char *saveptr;
	char *item = strtok_r(prog," ",&saveptr);
	struct number z,a,b;
	enum func fn;
	//tests();
	while (item){
		if (strchr(item,';')){          /* rational number*/
			z=reduce(read_number(item));
			stack_push(s,z);
		} else if (is_double(item)){   /* floating point number */
			z=as_rational(strtod(item,NULL));
			stack_push(s,z);
		} else if (is_numeric(item)){
			stack_push(s,as_rational(strtol(item,NULL,0)));
		} else if (strlen(item)>2) {    /* a function */
			fn=match_function(item,F_NAME);
			switch(fn){
			case f_diff:
				a=stack_pop(s);
				b=stack_pop(s);
				stack_push(s,diff(a,b));
				break;
			default:
				a=stack_pop(s);
				z=as_rational(math_h[fn](as_double(a)));
				stack_push(s,z);
			}
		} else if (strlen(item)==2){/* two-letter operators*/
			if (strcmp("**",item)==0){
				b=stack_pop(s);
				a=stack_pop(s);
				z=as_rational(pow0(as_double(a),b.a));
				stack_push(s,z);
			} else if (strcmp("<=",item)==0){
				b=stack_pop(s);
				a=stack_pop(s);
				stack_push(s,as_rational(as_double(a)<=as_double(b)));
			} else if (strcmp(">=",item)==0){
				b=stack_pop(s);
				a=stack_pop(s);
				stack_push(s,as_rational(as_double(a)>=as_double(b)));
			} else if (strcmp("==",item)==0){
				b=stack_pop(s);
				a=stack_pop(s);
				stack_push(s,as_rational(as_double(a)==as_double(b)));
			}
		} else {                    /* an operator: +-^*/
			switch(*item){
			case '+':               /* add two numbers */
				a=stack_pop(s);
				b=stack_pop(s);
				z=add(a,b);
				stack_push(s,z);
				break;
			case '-':               /* negate top number */
				z=negate(stack_pop(s));
				stack_push(s,z);
				break;
			case '*':
				a=stack_pop(s);
				b=stack_pop(s);
				z=prod(a,b);
				stack_push(s,z);
				break;
			case '^':
				a=stack_pop(s);
				b=stack_pop(s);
				stack_push(s,as_rational(pow(as_double(b),as_double(a))));
				break;
			case '<':
				b=stack_pop(s);
				a=stack_pop(s);
				stack_push(s,as_rational(as_double(a)<as_double(b)));
				break;
			case '>':
				b=stack_pop(s);
				a=stack_pop(s);
				stack_push(s,as_rational(as_double(a)>as_double(b)));
				break;
			case '=':
				b=stack_pop(s);
				a=stack_pop(s);
				stack_push(s,as_rational(as_double(a)==as_double(b)));
				break;
			case '/':
				b=stack_pop(s);
				a=stack_pop(s);
				stack_push(s,prod(a,inverse(b)));
				break;
			case '\\':
				b=stack_pop(s);
				a=stack_pop(s);
				stack_push(s,prod(inverse(a),b));
				break;
			case '@':
				z=stack_pop(s);
				stack_push(s,inverse(z));
				break;
			}
		}
		item=strtok_r(NULL," ",&saveptr);
	}
}

int main(int argc, char *argv[]){ //(setq c-basic-offset 4)
	if (argc==1) return EXIT_FAILURE;
	char *prog="";
	int j;
	struct stack s = stack_alloc(32);
	enum output {human, raw, flt} o=human;
	for (j=1;j<argc;j++){
		if (strcmp("-r",argv[j])==0){
			o=raw;
		} else if (strcmp("-d",argv[j])==0){
			o=flt;
		} else if (strcmp("-e",argv[j])==0){
			prog=argv[j+1];
		} else {
			prog=argv[j];
		}
	}
	evaluate(&s,prog);
	int i;
	for (i=0;i<s.size;i++){
		switch(o){
		case human:
			display_number(s.element[i]);
			break;
		case raw:
			display_raw(s.element[i]);
			break;
		case flt:
			display_double(s.element[i]);
			break;
		}
	}
	return EXIT_SUCCESS;
}
