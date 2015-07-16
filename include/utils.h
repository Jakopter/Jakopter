#ifndef JAKOPTER_UTILS_H
#define JAKOPTER_UTILS_H

/* C-style public-like keyword */
#if defined(__GNUC__) && __GNUC__ >= 4
#define JAKO_EXPORT __attribute__ ((visibility("default")))
#else
#define JAKO_EXPORT
#endif

#define JAKO_LUA_API_VERSION 1.1
#define JAKO_C_API_VERSION 1.1

/* Max number of digit into an integer plus the sign and \0. */
#define INT_LEN     13
/* 2 integers plus the dot and \0*/
#define TSTAMP_LEN  2*INT_LEN+2
/* Max number of digit we use in a float (nb digits of an int + 6 decimal digits) and \0*/
#define FLOAT_LEN INT_LEN+6

#endif