// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2023 Bardia Moshiri <fakeshell@bardia.tech>

#ifndef DSCHED_H
#define DSCHED_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sched.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <uthash.h>
#include <sys/resource.h>
#include <errno.h>
#include <glib.h>
#include <batman/wlrdisplay.h>

#define MAX_PROGRAMS 1000

#define MAX_PROCESSES 32768
#define MAX_NAME_LEN 1024
#define BUFFER_SIZE 8192

typedef struct {
    pid_t pid;
    char name[MAX_NAME_LEN];
} Process;

typedef struct {
    pid_t pid;
    char name[MAX_NAME_LEN];
    UT_hash_handle hh;
} SetProcess;

typedef struct {
    int pid;
    UT_hash_handle hh;
} PIDHash;

typedef struct {
    char pattern[MAX_NAME_LEN];
    UT_hash_handle hh;
} PatternHash;

extern Process processes[MAX_PROCESSES];
extern SetProcess *set_pids;
extern int process_count;
extern int gtk4_pids[MAX_PROCESSES];
extern int qt_pids[MAX_PROCESSES];
extern int qml_pids[MAX_PROCESSES];

extern char programs_to_schedule[MAX_PROGRAMS][MAX_NAME_LEN];
extern int gtk4_index;
extern int qt_index;
extern int qml_index;
extern int program_count;

void populate_process_and_gather_pids();
bool is_process_running(const char *program_name, pid_t *out_pid);
bool was_pid_set(pid_t pid);
void add_set_pid(pid_t pid, const char *program_name);
void set_sched_fifo(pid_t pid, const char *program_name);
void load_dsched_programs();

#endif // DSCHED_H
