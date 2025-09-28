#ifndef PROC_READER_H
#define PROC_READER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <ctype.h>

// Function prototypes
int list_process_directories(void);
int read_process_info(const char* pid);
int show_system_info(void);
void compare_file_methods(void);

// Helper function prototypes
int is_number(const char* str);
int read_file_with_syscalls(const char* filename);
int read_file_with_library(const char* filename);

#endif