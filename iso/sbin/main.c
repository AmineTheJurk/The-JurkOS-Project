/* Copyright (c) 2026 AmineTheJurk */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <errno.h>

#define SETUP_MARKER "/encrypted/.setup_done"
#define USER_DB_PATH "/encrypted/userlogin/"

static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char *base64_encode(const char *data) {
    size_t input_len = strlen(data);
    size_t output_len = 4 * ((input_len + 2) / 3);
    char *encoded_data = malloc(output_len + 1);
    if (encoded_data == NULL) return NULL;
    for (size_t i = 0, j = 0; i < input_len;) {
        uint32_t octet_a = i < input_len ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_len ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_len ? (unsigned char)data[i++] : 0;
        uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;
        encoded_data[j++] = base64_table[(triple >> 18) & 0x3F];
        encoded_data[j++] = base64_table[(triple >> 12) & 0x3F];
        encoded_data[j++] = (i > input_len + 1) ? '=' : base64_table[(triple >> 6) & 0x3F];
        encoded_data[j++] = (i > input_len) ? '=' : base64_table[triple & 0x3F];
    }
    encoded_data[output_len] = '\0';
    return encoded_data;
}

void setup_system() {
    mount(NULL, "/", NULL, MS_REMOUNT, NULL);
    mkdir("/proc", 0755); mount("proc", "/proc", "proc", 0, NULL);
    mkdir("/sys", 0755); mount("sysfs", "/sys", "sysfs", 0, NULL);
    mkdir("/dev", 0755); mount("devtmpfs", "/dev", "devtmpfs", 0, NULL);
    mkdir("/etc", 0755); symlink("/proc/mounts", "/etc/mtab");
    setenv("PATH", "/bin:/sbin", 1);
    setenv("TERM", "dumb", 1);
    setenv("LS_COLORS", "none", 1);
    system("busybox mdev -s");
}

void expand_filesystem() {
    printf("[JurkOS] Expanding storage...\n");
    system("resize2fs /dev/disk/by-partuuid/12345678-01");
}

void strip_newline(char *str) {
    size_t len = strlen(str);
    while (len > 0 && (str[len-1] == '\n' || str[len-1] == '\r')) {
        str[len-1] = '\0';
        len--;
    }
}

void run_shell(const char *username) {
    char home_dir[256];
    snprintf(home_dir, sizeof(home_dir), "/home/%s", username);
    mkdir("/home", 0755); mkdir(home_dir, 0755);
    chdir(home_dir);
    setenv("HOME", home_dir, 1);

    char input[1024];
    printf("\n--- JurkOS Shell ---\n");

    while (1) {
        char cwd[256]; getcwd(cwd, sizeof(cwd));
        printf("jurkos@%s:%s$ ", username, cwd);
        if (fgets(input, sizeof(input), stdin) == NULL) break;
        strip_newline(input);
        if (strlen(input) == 0 || input[0] == '\033') continue;

        // C-Shell Built-ins
        if (strncmp(input, "cd ", 3) == 0 || strcmp(input, "cd") == 0) {
            char *path = (strlen(input) > 3) ? input + 3 : home_dir;
            if (chdir(path) != 0) printf("cd: no such directory: %s\n", path);
            continue;
        }
        if (strcmp(input, "help") == 0) {
            printf("Built-ins: help, sysinfo, whoami, clear, logout, reboot, cd, exit\n");
            printf("All BusyBox applets (ls, cat, vi, grep, etc.) and shell scripts are supported.\n");
            continue;
        }
        if (strcmp(input, "clear") == 0) { printf("\033[H\033[J"); continue; }
        if (strcmp(input, "logout") == 0 || strcmp(input, "exit") == 0) break;
        if (strcmp(input, "reboot") == 0) { sync(); system("reboot"); continue; }
        if (strcmp(input, "sysinfo") == 0) { printf("OS: JurkOS 1.0 Alpha\nKernel: Linux\n"); continue; }
        if (strcmp(input, "whoami") == 0) { printf("%s\n", username); continue; }

        // Logic: Try running the command via system()
        // system() uses /bin/sh -c "[input]", which handles pipes, redirects, and all BusyBox applets.

        // Patch 'ls' to never use color
        char final_cmd[2048];
        if (strncmp(input, "ls", 2) == 0 && (input[2] == ' ' || input[2] == '\0')) {
             if (strstr(input, "--color") == NULL) snprintf(final_cmd, sizeof(final_cmd), "%s --color=never", input);
             else strcpy(final_cmd, input);
        } else {
            strcpy(final_cmd, input);
        }

        int status = system(final_cmd);

        // If the shell returns 127, it means 'Command not found'
        if (WIFEXITED(status) && WEXITSTATUS(status) == 127) {
            printf("No Such Command, Did you misstype?\n");
        }
    }
}

int main() {
    setup_system();
    FILE *rw_check = fopen("/.rw_test", "w");
    if (!rw_check) {
        printf("\nUSB is read-only! Check flash mode.\n");
        while(1) sleep(100);
    }
    fclose(rw_check); remove("/.rw_test");

    struct stat st = {0};
    if (stat(SETUP_MARKER, &st) == -1) {
        printf("What would you like to call you?\n");
        char username[64], password[64];
        printf("Username : ");
        if (!fgets(username, sizeof(username), stdin)) return 0;
        strip_newline(username);
        printf("Password : ");
        if (!fgets(password, sizeof(password), stdin)) return 0;
        strip_newline(password);

        expand_filesystem();
        mkdir("/encrypted", 0755); mkdir(USER_DB_PATH, 0755);
        char user_path[256]; snprintf(user_path, sizeof(user_path), "%s%s", USER_DB_PATH, username);
        mkdir(user_path, 0755);
        char creds[128], auth_file[512];
        snprintf(creds, sizeof(creds), "%s:%s", username, password);
        char *encoded = base64_encode(creds);
        snprintf(auth_file, sizeof(auth_file), "%s/auth", user_path);
        FILE *f = fopen(auth_file, "w");
        if (f) { fprintf(f, "%s", encoded); fclose(f); }
        free(encoded);
        FILE *marker = fopen(SETUP_MARKER, "w");
        if (marker) { fprintf(marker, "%s", username); fclose(marker); }
        run_shell(username);
    } else {
        char username[64];
        FILE *marker = fopen(SETUP_MARKER, "r");
        if (!fgets(username, sizeof(username), marker)) { fclose(marker); return 1; }
        fclose(marker); strip_newline(username);
        while (1) {
            printf("\nPassword? : ");
            char password[64];
            if (!fgets(password, sizeof(password), stdin)) break;
            strip_newline(password);
            if (strlen(password) == 0) { run_shell(username); continue; }

            char user_path[256], auth_file[512];
            snprintf(user_path, sizeof(user_path), "%s%s", USER_DB_PATH, username);
            snprintf(auth_file, sizeof(auth_file), "%s/auth", user_path);
            FILE *f = fopen(auth_file, "r");
            if (!f) { printf("User data lost.\n"); break; }
            char stored_encoded[256]; fgets(stored_encoded, sizeof(stored_encoded), f); fclose(f);
            char input_creds[128];
            snprintf(input_creds, sizeof(input_creds), "%s:%s", username, password);
            char *input_encoded = base64_encode(input_creds);
            if (strcmp(stored_encoded, input_encoded) == 0) {
                free(input_encoded); run_shell(username);
            } else {
                printf("Incorrect password.\n"); free(input_encoded);
            }
        }
    }
    return 0;
}
