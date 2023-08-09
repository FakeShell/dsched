// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2023 Bardia Moshiri <fakeshell@bardia.tech>

#include "dsched.h"

Process processes[MAX_PROCESSES];
SetProcess *set_pids = NULL;
PIDHash *checked_pids_hash = NULL;
PatternHash *pattern_hash = NULL;
int process_count = 0;
int set_pid_count = 0;
int gtk4_pids[MAX_PROCESSES];
int qt_pids[MAX_PROCESSES];
int checked_pids[MAX_PROCESSES];

char programs_to_schedule[MAX_PROGRAMS][MAX_NAME_LEN];
int gtk4_index = 0;
int qt_index = 0;
int program_count = 0;
int checked_pids_count = 0;

bool is_pid_checked(int pid) {
    PIDHash *s;

    HASH_FIND_INT(checked_pids_hash, &pid, s);
    return s != NULL;
}

bool isInPatternList(const char* pattern, const char* patterns[], int patternCount) {
    PatternHash *s;

    for (int i = 0; i < patternCount; i++) {
        HASH_FIND_STR(pattern_hash, patterns[i], s);
        if (s) {
            return true;
        }
    }

    return false;
}

void populate_process_and_gather_pids() {
    DIR *dir = opendir("/proc");
    struct dirent *entry;

    struct sched_param param;
    param.sched_priority = 0;
    if (sched_setscheduler(0, SCHED_OTHER, &param) == -1) {
        perror("Failed to set scheduling policy");
        exit(1);
    }

    int ret = setpriority(PRIO_PROCESS, 0, 10);
    if (ret == -1) {
        perror("Failed to set process priority");
        exit(1);
    }

    if (dir == NULL) {
        perror("Could not open /proc");
        exit(1);
    }

    char path[MAX_NAME_LEN];
    char library_check_buffer[BUFFER_SIZE];

    const char* skipPatterns[] = { "/android", "/run", "/usr/share", "/dev", "/var", "/usr/lib/locale", "rw-s", "r-xp", "---p", "r--p" };
    const char* libraryPatterns[] = { "libgtk-4.so", "libQt5Core.so.5", "libQt6Core.so.6" };
    const char* patterns[] = { "android", "vendor" };

    const char qt_prefix[] = "libQt";
    const char qt_suffix[] = "Core.so";

    while ((entry = readdir(dir)) != NULL) {
        usleep(5000);

        if (isdigit(entry->d_name[0])) {
            int pid = atoi(entry->d_name);
            if (pid < 1000 || is_pid_checked(pid)) continue;

            PIDHash *s = (PIDHash*)malloc(sizeof(PIDHash));
            s->pid = pid;
            HASH_ADD_INT(checked_pids_hash, pid, s);

            snprintf(path, sizeof(path), "/proc/%s/cmdline", entry->d_name);
            FILE *f = fopen(path, "r");
            if (f == NULL) continue;

            fgets(processes[process_count].name, sizeof(processes[process_count].name), f);

            if (isInPatternList(processes[process_count].name, patterns, sizeof(patterns) / sizeof(patterns[0]))) {
                fclose(f);
                continue;
            }

            processes[process_count].pid = pid;
            fclose(f);

            snprintf(path, sizeof(path), "/proc/%s/maps", entry->d_name);
            f = fopen(path, "r");
            if (f == NULL) continue;

            while (fgets(library_check_buffer, sizeof(library_check_buffer), f) != NULL) {
                if (isInPatternList(library_check_buffer, skipPatterns, sizeof(skipPatterns) / sizeof(skipPatterns[0]))) continue;

                if (strstr(library_check_buffer, "libgtk-4.so")) {
                    gtk4_pids[gtk4_index++] = pid;
                    break;
                } else {
                    char *start = strstr(library_check_buffer, qt_prefix);
                    if (start) {
                        char *end = strstr(start, qt_suffix);
                        if (end && end > start + strlen(qt_prefix) && *(end - 1) >= '0' && *(end - 1) <= '9') {
                           qt_pids[qt_index++] = pid;
                           break;
                        }
                    }
                }
            }

            fclose(f);
            process_count++;
        }
    }

    closedir(dir);
}

bool is_process_running(const char *program_name, pid_t *out_pid) {
    for (int i = 0; i < process_count; i++) {
        if (strstr(processes[i].name, program_name) != NULL) {
            *out_pid = processes[i].pid;
            return true;
        }
    }

    return false;
}

void add_set_pid(int pid, const char *program_name) {
    SetProcess *s;

    HASH_FIND_INT(set_pids, &pid, s);
    if (s == NULL) {
      s = (SetProcess *)malloc(sizeof(SetProcess));
      s->pid = pid;
      HASH_ADD_INT(set_pids, pid, s);
    }

    strncpy(s->name, program_name, MAX_NAME_LEN - 1);
    s->name[MAX_NAME_LEN - 1] = '\0';
}

bool was_pid_set(int pid) {
    SetProcess *s;

    HASH_FIND_INT(set_pids, &pid, s);
    return s != NULL;
}

void set_sched_fifo(pid_t pid, const char *program_name) {
    if (pid == 1 || was_pid_set(pid)) return;

    struct sched_param param;
    param.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1;

    char *cmdline = NULL;
    for (int i = 0; i < process_count; i++) {
        if (processes[i].pid == pid) {
            cmdline = processes[i].name;
            break;
        }
    }

    if (cmdline == NULL) {
        strncpy(cmdline, "[ERROR]", MAX_NAME_LEN - 1);
    }

    if (sched_setscheduler(pid, SCHED_FIFO, &param) == -1) {
        perror("Failed to set scheduler");
    } else {
        printf("%s (PID: %d, CMDLINE: %s)\n", program_name, pid, cmdline);
    }

    add_set_pid(pid, program_name);
}

void load_dsched_programs() {
    DIR *dsched_dir = opendir("/etc/dsched");
    struct dirent *entry;

    if (dsched_dir == NULL) {
        perror("Could not open /etc/dsched");
        exit(1);
    }

    while ((entry = readdir(dsched_dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char file_path[MAX_NAME_LEN];
            snprintf(file_path, sizeof(file_path), "/etc/dsched/%s", entry->d_name);

            FILE *file = fopen(file_path, "r");
            if (file == NULL) continue;

            char program_name[MAX_NAME_LEN];
            while (fgets(program_name, sizeof(program_name), file) != NULL) {
                size_t len = strlen(program_name);
                if (len > 0 && program_name[len - 1] == '\n') {
                    program_name[len - 1] = '\0';
                }

                if (program_count < MAX_PROGRAMS) {
                    strncpy(programs_to_schedule[program_count], program_name, MAX_NAME_LEN);
                    program_count++;
                }
            }

            fclose(file);
        }
    }

    closedir(dsched_dir);
}

int check_batman_helper() {
    FILE *pipe;
    char buffer[16];

    setenv("XDG_RUNTIME_DIR", "/run/user/32011", 1);

    pipe = popen("batman-helper wlrdisplay", "r");
    if (pipe == NULL) {
        perror("popen");
        return -1;
    }

    fgets(buffer, sizeof(buffer), pipe);
    pclose(pipe);

    return strncmp(buffer, "yes", 3) != 0;
}

int main() {
    if (getuid() != 0) {
        fprintf(stderr, "dsched must run as root.\n");
        exit(1);
    }

    load_dsched_programs();

    void sched_check() {
        populate_process_and_gather_pids();
        for (int i = 0; i < program_count; i++) {
            pid_t pid;
            if (is_process_running(programs_to_schedule[i], &pid)) {
                set_sched_fifo(pid, programs_to_schedule[i]);
            }
        }

        for (int i = 0; i < gtk4_index; i++) {
            set_sched_fifo(gtk4_pids[i], "GTK4");
        }

        for (int i = 0; i < qt_index; i++) {
            set_sched_fifo(qt_pids[i], "QT");
        }
    }

    sched_check();

    while (check_batman_helper() != 0) {
        sleep(2);
    }

    while (true) {
        sleep(2);

        if (check_batman_helper()) {
            sched_check();
        }
    }

    return 0;
}

