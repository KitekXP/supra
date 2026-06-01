// Just to be sure it uses gnu and posix functions
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

// Define the maximum amount of variables you can pass in one command
#define MAX_ARGS 64
// Define the maximum length for a password
#define MAX_PASS_LENGTH 256
// Define the max number of allowed uids
#define MAX_NUM_UIDS 64
// Define the minimum number of arguments required (YOU SHOULDN'T CHANGE THIS)
#define MIN_ARGS 2
