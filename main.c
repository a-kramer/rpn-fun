#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <limits.h>
#include <stdint.h>
#include <ctype.h>
#include <unistd.h>
#include <assert.h>
#include "config.h"

#define CSTR(c) ((char[2]){(c)})

enum style {loose, compact, commented};

int max(int a, int b){
	if (a>b) return a;
	else return b;
}
struct number diff(struct number a, struct number b);

typedef double ddmap(double);
int is_numeric(const char*);

const char *F_NAME[] = {"acos", "acosh", "asin", "asinh", "atan", "atanh", "cbrt", "ceil", "cos", "cosh", "erf", "erfc", "exp", "exp2", "expm1", "fabs", "floor", "lgamma", "log", "log10", "log1p", "log2", "logb", "nearbyint", "rint", "round", "sin", "sinh", "sqrt", "tan", "tanh", "tgamma", "trunc", "j0", "j1", "y0", "y1", "significand", "exp10", "diff", NULL};
enum func {f_acos, f_acosh, f_asin, f_asinh, f_atan, f_atanh, f_cbrt, f_ceil, f_cos, f_cosh, f_erf, f_erfc, f_exp, f_exp2, f_expm1, f_fabs, f_floor, f_lgamma, f_log, f_log10, f_log1p, f_log2, f_logb, f_nearbyint, f_rint, f_round, f_sin, f_sinh, f_sqrt, f_tan, f_tanh, f_tgamma, f_trunc, f_j0, f_j1, f_y0, f_y1, f_significand, f_exp10, f_diff, numFunctions};
ddmap *math_h[]={acos, acosh, asin, asinh, atan, atanh, cbrt, ceil, cos, cosh, erf, erfc, exp, exp2, expm1, fabs, floor, lgamma, log, log10, log1p, log2, logb, nearbyint, rint, round, sin, sinh, sqrt, tan, tanh, tgamma, trunc, j0, j1, y0, y1, significand, exp10};
// a number shall consist of several parts:
// (A + N/D + f) * 10^E,
// where A is the whole part of the number
// - N is a numerator
// - D is a denominator
// - f is a small floating point adjustment,
//     in case the rational representation is not precise enough.
// - u is the uncertainty of the number, defaults to 0.5/D
struct number {
	long a; // whole part
	int n; // numerator
	int d; // denominator
	int e; // base-10 exponent scale
	double f; // double precision correction term
	double u; // (OPTIONAL) uncertainty
};

const struct number zero = {0, 0, 1, 0, 0.0, 0.0};
const struct number one = {1, 0, 1, 0, 0.0, 0.0};

struct number reduce(struct number z);
struct number as_rational(double x);

struct stack {
	int max;
	int size;
	struct number *element;
};

struct split {
	char **token;
	size_t size;
	size_t max;
};

struct split split_alloc(size_t max){
	struct split s;
	s.max=max;
	s.token=malloc(sizeof(char*)*s.max);
	s.size=0;
	return s;
}

void strsplit(char *str, struct split *s, char *delim){
	char *ptr=str;
	char *t=strtok_r(str,delim,&ptr);
	while (t){
		if (s->size==s->max) {
			s->max*=2;
			s->token=realloc(s->token,sizeof(char*)*(s->max));
		}
		s->token[s->size]=t;
		s->size++;
		t=strtok_r(NULL,delim,&ptr);
	}
}

void print_split(struct split *s, char sep, char final){
	int i;
	int n=s->size;
	for (i=0;i<n;i++){
		printf("%s%c",s->token[i],i<(n-1)?sep:final);
	}
}

void split_free(struct split **s){
	if (s && *s){
		if ((*s)->token) free((*s)->token);
		*s=NULL;
	}
}

/* The standard 64-bit FNV-1a hash function */
static uint64_t FNV1a(const char* str) {
	const uint64_t FNV_offset_basis =  0xcbf29ce484222325ULL;
	const uint64_t FNV_prime = 0x100000001b3ULL;
	uint64_t hash = FNV_offset_basis;
	for (const char* ptr = str; *ptr != '\0'; ++ptr) {
		hash ^= (uint64_t)(unsigned char)(*ptr);
		hash *= FNV_prime;
	}
	return hash;
}


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

struct hash {
	uint64_t *val;
	size_t size;
	size_t max;
};

int is_header(const char *str){
	while (isblank(*str)) str++;
	if (isalpha(*str)) return 1;
	else return 0;
}

static int count(char *str, char c){
	int i=0;
	if (!str) return 0;
	while (*str){
		if ((*str)==c) i++;
		str++;
	}
	return i;
}

struct hash remember_header(struct split *s){
	int i;
	size_t n=s->size;
	struct hash H;
	H.max=n;
	H.val=malloc(sizeof(uint64_t)*H.max);
	for (i=0;i<n;i++){
		H.val[i] = FNV1a(s->token[i]);
		H.size++;
	}
	return H;
}

double derivative(ddmap f, double x0){
	double h=1e-10;
	if (-1.0 < x0 && x0<1.0){
		h+=1e-4*x0*x0;
	} else {
		h+=1e-7*sqrt(fabs(x0));
	}
	double d = 0.5*(f(x0+h)-f(x0-h))/h;
	return d;
}


/* The entire purpose of this function is that I dislike type-casts,
 * and macros. Typecasts, `(double) n/(double) d` look much worse than
 * function calls `frac(n,d)`.
 */
double frac(double a, double b){
	return a/b;
}

double as_double(struct number z){
	return (z.a+frac(z.n,z.d)+z.f)*exp10(z.e);
}

/* this will only read: 12.34(56)E-7, like this, with an E*/
struct number read_concise(char *line){
	struct number a=zero;
	//	char *ptr=line;
	char *ptr_u=line, *ptr_e=line;
	int vscale=0;
	double v=strtod(line,&ptr_u);
	int u=0;
	//int ulen=2;
	char *decimal=strchr(line,'.');
	int digits=0;
	if (decimal) digits=ptr_u-decimal-1;
	while (ptr_u && *ptr_u && !is_numeric(ptr_u)) ptr_u++;
	if (is_numeric(ptr_u)) {
		u=strtol(ptr_u,&ptr_e,10);
		//ulen=ptr_e-ptr_u;
	}
	while (ptr_e && *ptr_e && !is_numeric(ptr_e)) ptr_e++;
	if (is_numeric(ptr_e)) vscale=strtol(ptr_e,NULL,10);
	a.f=v*exp10(vscale);
	a.u=u*exp10(vscale-digits);
	return a;
}

void print_concise(struct number x, enum style s){
	double z=as_double(x);
	double w=x.u;
	int vscale=floor(log10(fabs(z+1e-8)));
	int uscale=floor(log10(fabs(w+1e-8)));
	int d=(vscale-uscale+1);
	double v=z*pow(10,-vscale);
	int u=w*pow(10,1-uscale);
	if (u%10==0 && d>0){
		d--;
		u/=10;
	}
	if (d<0) {
		v*=exp10(d);
		u*=exp10(d);
		vscale+=1-d;
		d=0;
	}
	if (vscale){
		printf("%.*f(%i)",d,v,u);
		printf("E");
		printf("%+i",vscale);
	} else {
		printf("%.*f(%i)",d,v,u);
	}
	if (s==commented) printf("\t# %g ± %g",z,w);
}
// if the number read so far is too big,
void fix_overflow(struct number *z, char *a){
	double g;
	long w,l;

	if (z->a == LONG_MAX || z->a == LONG_MIN) { // little fix for very large numbers
		g = strtod(a,NULL);
		l = round(log10(fabs(g)));
		w = l - 15;
		if (z->n == 0 && w<9) {
			z->a = floor(fabs(g)*exp10(-w))*(g>0?1:-1);
			z->n = strtol(a+16,NULL,10);
			z->d = exp10(w);
			z->e = w;
		} else { // we exceed double precision by just a bit;
			z->f = strtod(a+16+(*a=='-'),NULL)*exp10(-w)*(g>0?1:-1);
			z->a = floor(fabs(g)*exp10(-w))*(g>0?1:-1);
			z->e += (int) w;
		}
	}
}

/* The number format is a;n;d; */
struct number read_number(const char *cstr) {
	struct number z = zero;
	if (!cstr || !*cstr) return z;
	char *str=strdupa(cstr);
	while (*str==';') str++; // skip leading semicolons
	char *p=str+strlen(str)-1;
	while (*p==';') {
		*p='\0';
		p--;
	}

	int semicolons = count(str,';');
	if (semicolons>3) {
		fprintf(stderr,"too many delimiters (;) in «%s»\n",str);
		return zero;
	}
	char *saveptr;
	char *p0 = strtok_r(str, ";", &saveptr);
	char *p1 = strtok_r(NULL, ";", &saveptr);
	char *p2 = strtok_r(NULL, ";", &saveptr);
	char *p3 = strtok_r(NULL, ";", &saveptr);
	switch (semicolons) {
	case 0: /* Single whole number "123" */
		if (p0) z.a = strtol(p0, NULL, 0);
		fix_overflow(&z,p0);
		break;
	case 1: /* Fraction without whole part "1;2" -> 1/2 */
		if (p0) z.n = strtol(p0, NULL, 0);
		if (p1) z.d = strtol(p1, NULL, 0);
		break;
	case 2: /* Whole + Fraction "1;2;3" -> 1 + 2/3 */
		if (p0) z.a = strtol(p0, NULL, 0);
		if (p1) z.n = strtol(p1, NULL, 0);
		if (p2) z.d = strtol(p2, NULL, 0);
		fix_overflow(&z,p0);
		break;
	case 3: /* Whole + Fraction + Exponent "1;2;3;10" */
		if (p0) z.a = strtol(p0, NULL, 0);
		if (p1) z.n = strtol(p1, NULL, 0);
		if (p2) z.d = strtol(p2, NULL, 0);
		if (p3) z.e = strtol(p3, NULL, 0);
		fix_overflow(&z,p0);
		break;
	}
	if (z.d < 0) {
		z.d = -z.d;
		z.n = -z.n;
	}
	return reduce(z);
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



void display_raw(struct number z, char final, enum style s){
	printf("%li;%i;%i;%i",z.a,z.n,z.d,z.e);
	if (z.f!=0.0 && s==commented) printf("# correction (f): %+.15g",z.f);
	putchar(final);
}

void display_double(struct number z, char final, enum style s){
	int l=round(log10(fabs(z.f)))-6;
	printf("%.*g%c",l<0?-l:2,as_double(z),final);
}

void display_number(struct number z, char final, enum style s){
	if (z.u > 0.0) {
		print_concise(z,s);
		putchar(final);
		return;
	}
	if (z.n == 0 && z.e == 0) {
		if (z.a) printf("%li",z.a);
		if (s==loose || s==commented) putchar(' ');
		if (fabs(z.f) != 0.0) {
			printf("%+.4g",z.f);
		}
	} else {
		putchar('(');
		if (z.a) printf("%li",z.a);
		if (s==loose || s==commented) putchar(' ');
		if (abs(z.n) != 0) printf("%+i/%i",z.n,z.d);
		if (fabs(z.f) != 0.0) printf("%+.4g",z.f);
		putchar(')');
		if (z.e != 0) {
			printf(e10,z.e);
		}
	}
	if (s==commented) printf("\t# %g",as_double(z));
	putchar(final);
}

struct number negate(struct number z){
	z.a*=-1;
	z.n*=-1;
	z.f*=-1.0;
	return z;
}

struct number simple_rational(long a, int n, int d){
	struct number z={a,n,d?d:1,0,0.0,0.0};
	return z;
}

struct number as_rational(double x){
	struct number z=zero;
	if (fabs(x)==0.0) return z;
	int sign = x<0?-1:+1;
	int l = floor(log10(fabs(x)+1e-15)/3.0)*3;
	z.e = l;
	double y = fabs(x)/exp10(l);
	z.a = floor(y);
	y -= z.a;             /* 0 <= y < 1 */
	int a=0,b=1,c=1,d=1;
	double pq;
	while (b+d < max_denominator){
		pq=frac(a+c,b+d);
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
	x/=exp10(l);
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
double pow0(double b, long n){
	short sign=n<0?-1:1;
	n*=sign;
	double a=1.0;
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
	int p10n = exp10(n);
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
	double Z;
	z.e = x.e + y.e;
	z.a = x.a*y.a;
	z.n = x.n*y.n + x.a*y.n*x.d + y.a*x.n*y.d ;
	z.d = x.d*y.d;
	z.f = x.a*y.f + y.a*x.f + y.f*frac(x.n,x.d) + x.f*frac(y.n,y.d) + x.f*y.f;
	Z = as_double(z);
	z.u = hypot(x.u*Z/as_double(x),y.u*Z/as_double(y)); // sqrt(sumsq(a,b))
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
	z.u = hypot(x.u,y.u); //sqrt(x.u*x.u + y.u*y.u);
	return reduce(z);
}

struct number inverse(struct number z){
	struct number x=z;
	if (memcmp(&z,&one,sizeof(struct number))==0) return one;
	if (memcmp(&z,&zero,sizeof(struct number))==0) {
		fprintf(stderr,"[%s] cannot invert zero multiplicatively (%g).\n",__func__,as_double(z));
		abort();
	}
	if (fabs(as_double(z))==0.0) {
		fprintf(stderr,"[%s] cannot invert %g multiplicatively.\n",__func__,as_double(z));
		abort();
	}
	if (fabs(z.f)>0.0) return as_rational(1.0/as_double(z));
	x.a=0;
	x.n=z.d;
	x.d=z.n+z.a*z.d;
	x.e*=-1;
	x.u=pow(as_double(x),2)*x.u;
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


double constant(const char *Q){
	const char *name[]={"M_E", "M_LOG2E", "M_LOG10E", "M_LN2", "M_LN10", "M_PI", "M_PI_2", "M_PI_4", "M_1_PI", "M_2_PI", "M_2_SQRTPI", "M_SQRT2", "M_SQRT1_2", NULL};
	const double value[]= {M_E, M_LOG2E, M_LOG10E, M_LN2, M_LN10, M_PI, M_PI_2, M_PI_4, M_1_PI, M_2_PI, M_2_SQRTPI, M_SQRT2, M_SQRT1_2};
	int i=0;
	while (name[i] && strcmp(Q,name[i])) i++;
	if (name[i]) return value[i];
	else return NAN;
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

int is_numeric(const char *str){
	if (*str=='-' || *str=='+') str++;
	if ('0' <= *str && *str <= '9') return 1;
	return 0;
}

int is_double(const char *str){
	char *ptr=NULL;
	if (strchr(str,'.')) return 1;
	ptr = strchr(str,'e');
	if (!ptr) ptr=strchr(str,'E');
	if (ptr && ptr>str && is_numeric(ptr-1) && is_numeric(ptr+1)) return 1;
	return 0;
}

double PHI(double z){
	return 0.5*(1+erf(M_SQRT1_2*z));
}

/* P(a<b)*/
double CDF_LESS(struct number a, struct number b){
	double mu_a=as_double(a);
	double mu_b=as_double(b);
	return PHI((mu_b-mu_a)/hypot(a.u,b.u)); // sqrt(var_a+var_b));
}

//double JSD(struct number a, struct number b){
//}

double Bhattacharyya_distance(struct number a, struct number b){
	double mu_a=as_double(a);
	double mu_b=as_double(b);
	double var_a = a.u*a.u; // sigma_a^2
	double var_b = b.u*b.u; // sigma_b^2
	return 0.25*pow(mu_a-mu_b,2)/(var_a+var_b) + 0.5*log((var_a+var_b)/(2*a.u*b.u));
}

double OVL(struct number a, struct number b){
	double mu_a=as_double(a);
	double mu_b=as_double(b);
	double sigma=0.5*(a.u+b.u);
	return 2*(1-PHI(fabs(mu_a-mu_b)/(2*sigma)));
}

void stack_push_d(struct stack *s, double d){
	stack_push(s,as_rational(d));
}

/* Table of operators:
 * + adds the top two numbers on te stack
 * - negates the top of the stack
 * @ inverts the top number: 1.0/num
 * * multiplies the top two numbers
 * / divides two numbers
 * \ divides two numbers in reverse order compared to /
 */

/* Evaluates an RPN program (with a stack), optionally, with table                                   */
/* cells as context.                                                                                 */
/* The cells can be referenced by name through the header of the                                     */
/*                                                                                                   */
/* table:                         RPN program                   &optional      table                 */
/*                    ╭─────────────────────────────────╮  ╭──────────────────────────────────────╮  */
void evaluate(int NR, struct stack *s, struct split *prog, struct split *cells, struct hash *header){
	char *item;
	struct number z,a,b;
	enum func fn;
	uint64_t h;
	int i,j;
	//assert(cells->size == header->size);
	for (j=0;j<prog->size;j++){
		item=prog->token[j];
		if (cells){
			if (*item == '$') {
				// this is for refs such as $0
				i=strtol(item+1,NULL,0);
				if (i==0){
					item=NULL;
				} else if (0<i && i<=cells->size) {
					item=cells->token[i-1];
				} else {
					fprintf(stderr,"[%s] $%i out of bounds ($0-$%li).\n",__func__,i,cells->size);
					abort();
				}
			} else if (strlen(item)==1 && isupper(*item)) {
				// this is for refs such as A
				i=item[0]-'A';
				if (0<=i && i<cells->size) {
					item=cells->token[i];
				} else {
					fprintf(stderr,"[%s] %c out of bounds (A-%c).\n",__func__,*item,'A'+(char) (cells->size-1));
					abort();
				}
			} else if (header){
				// this is for named references, TSV must have header line
				h=FNV1a(item);
				for (i=0; i<header->size; i++){
					if (header->val[i] == h){
						//printf("«%s» is actually «%s» here.\n",item,cells->token[i]);
						item=cells->token[i];
						break;
					}
				}
			}
		}
		if (!item){
			stack_push(s,simple_rational(NR,0,0));
		} else if (strchr(item,'(')){          /* uncertain number */
			z=read_concise(item);
			stack_push(s,z);
		} else if (strchr(item,';')){          /* rational number*/
			z=reduce(read_number(item));
			stack_push(s,z);
		} else if (*item=='M' && *(item+1)=='_'){/* mathematical constant */
			z=as_rational(constant(item));
			stack_push(s,z);
		} else if (is_double(item)){           /* floating point number */
			z=as_rational(strtod(item,NULL));
			stack_push(s,z);
		} else if (is_numeric(item)){
			stack_push(s,read_number(item));
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
				z.u=fabs(derivative(math_h[fn],as_double(a)))*fabs(a.u);
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
				if (a.u || b.u) stack_push(s,as_rational(CDF_LESS(a,b)));
				else stack_push(s,as_rational(as_double(a)<=as_double(b)));
			} else if (strcmp(">=",item)==0){
				b=stack_pop(s);
				a=stack_pop(s);
				if (a.u || b.u) stack_push(s,as_rational(CDF_LESS(b,a)));
				else stack_push(s,as_rational(as_double(a)>=as_double(b)));
			} else if (strcmp("==",item)==0){ // this is the strict 'equal'
				b=stack_pop(s);
				a=stack_pop(s);
				stack_push(s,as_rational(0==memcmp(&a,&b,sizeof(struct number))));
			} else if (strcmp("!=",item)==0) {
				b=stack_pop(s);
				a=stack_pop(s);
				stack_push(s,as_rational(0!=memcmp(&a,&b,sizeof(struct number))));
			} else if (strcmp("<>",item)==0){ // not equal in the mathematical sense
				b=stack_pop(s);
				a=stack_pop(s);
				if (a.u && b.u) {
					stack_push(s,as_rational(sqrt(Bhattacharyya_distance(a,b))));
				} else {
					stack_push(s,as_rational(as_double(a) != as_double(b)));
				}
			} else if (strcmp("+-",item)==0 || strcmp("±",item)==0){
				b=stack_pop(s);
				a=stack_pop(s);
				z=zero;
				z.f=as_double(a);
				z.u=as_double(b);
				stack_push(s,z);
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
				if (a.u || b.u) stack_push(s,as_rational(CDF_LESS(a,b)));
				else stack_push(s,as_rational(as_double(a)<as_double(b)));
				break;
			case '>':
				b=stack_pop(s);
				a=stack_pop(s);
				if (a.u || b.u) stack_push(s,as_rational(CDF_LESS(b,a)));
				else stack_push(s,as_rational(as_double(a)>as_double(b)));
				break;
			case '=': // in the mathematical sense
				b=stack_pop(s);
				a=stack_pop(s);
				if (a.u || b.u) stack_push(s,as_rational(OVL(a,b))); // overlap
				else stack_push(s,as_rational(fabs(as_double(a)-as_double(b))<1e-15));
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
			case '%':
				b=stack_pop(s);
				a=stack_pop(s);
				z=simple_rational(((long) as_double(a)) % ((long) as_double(b)),0,1);
				stack_push(s,z);
				break;
			case ',':
				b=stack_pop(s);
				a=stack_pop(s);
				z=zero;
				z.f=as_double(a);
				z.u=as_double(b);
				stack_push(s,z);
				break;
			}
		}
	}
}

enum output {human, raw, flt};


void print_stack(struct stack *s, enum output o, char sep, char final, enum style l){
	int i;
	int n=s->size;
	for (i=0;i<n;i++){
		switch(o){
		case human:
			display_number(s->element[i],i<n-1?sep:final,l);
			break;
		case raw:
			display_raw(s->element[i],i<n-1?sep:final,l);
			break;
		case flt:
			display_double(s->element[i],i<n-1?sep:final,l);
			break;
		}
	}
}

void reset_stack(struct stack *s){
	s->size=0;
}

int main(int argc, char *argv[]){
	if (argc==1) return EXIT_FAILURE;
	char *prog_buffer=NULL;
	struct split prog=split_alloc(64);
	struct split cells=split_alloc(64);
	char *buffer;
	struct split res_names=split_alloc(64);
	int j;
	struct split names=split_alloc(64);
	struct hash header={NULL,0,0};
	enum stack_state {forget, remember} in_between=forget;
	ssize_t m;
	size_t n=128;
	char *line=malloc(n);
	int piped=!isatty(STDIN_FILENO);
	struct stack s = stack_alloc(32);
	enum output o=human;
	int NR=0;
	for (j=1;j<argc;j++){
		if (strcmp("-r",argv[j])==0){
			o=raw;
		} else if (strcmp("-d",argv[j])==0){
			o=flt;
		} else if (strcmp("-H",argv[j])==0){
			buffer=strdup(argv[++j]);
			strsplit(buffer,&res_names,",;");
		} else if (strcmp("-&",argv[j])==0) {
			in_between=remember;
		} else {
			prog_buffer=strdupa(argv[j]);
			strsplit(prog_buffer,&prog," ");
		}
	}
	if (!prog_buffer) abort(); // no program was supplied
	while (piped && (m=getline(&line,&n,stdin))>0 && !feof(stdin)) {
		line[m-1]='\0';
		if (NR==0 && line && is_header(line)){
			strsplit(line,&names,"\t");
			header = remember_header(&names);
			if (res_names.size>0){
				print_split(&names,'\t','\t');
				print_split(&res_names,'\t','\n');
			}
		} else {
			cells.size=0;
			strsplit(line,&cells,"\t");
			evaluate(NR,&s,&prog,&cells,&header);
			print_split(&cells,'\t','\t');
			print_stack(&s,o,'\t','\n',compact);
			if (in_between==forget) reset_stack(&s);
		}
		NR++;
	}
	if (!piped){
		evaluate(0,&s,&prog,NULL,NULL);
		print_stack(&s,o,'\n','\n',commented);
	}
	return EXIT_SUCCESS;
}
