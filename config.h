/* edit this file to configure some of the details that need not change frequently */

/* the format of the base-10 exponent, on output */
const char *e10 = "*pow(10,%i)";

/* Alternatively, using the exp10 function:  */
/* const char *e10 = "*exp10(%i)"; */

/* for use in higher-level languages:  */
/* const char *e10 = "*10^(%i)"; */

/* For LaTeX: */
/* const char *e10 = "\\times 10^{%i}"; */

/* with a unicode character */
/* const char *e10 = "×10^(%i)";*/

/* when approximating floating point numbers, this is the maximum
 * fraction length
 */
const int max_denominator = 1000;
