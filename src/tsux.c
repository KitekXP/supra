// Quite a bit of includes
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <security/pam_appl.h>
#include <grp.h>
#include <termios.h>
#include <errno.h>

// Include the config header
#include "config.h"

// Import the current user env
extern char **environ;

static char *tsux_getpass(const char *prompt)
{
    struct termios old, new;
    char *buf = malloc(MAX_PASS_LENGTH);
    if (!buf) return NULL;

    int i = 0;

    // Highlight the prompt in Bold Yellow
    printf("\x1b[1;33m%s\x1b[0m", prompt);
    fflush(stdout);

    tcgetattr(STDIN_FILENO, &old);

    new = old;
    new.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &new);

    while (i < MAX_PASS_LENGTH - 1) {
        char c;

        if (read(STDIN_FILENO, &c, 1) != 1)
            break;

        if (c == '\n' || c == '\r')
            break;

        if (c == 127 || c == '\b') {
            if (i > 0) {
                i--;
                printf("\b \b");
                fflush(stdout);
            }
            continue;
        }

        buf[i++] = c;
        printf("•");
        fflush(stdout);
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &old);

    buf[i] = '\0';
    putchar('\n');

    return buf;
}

static int tsux_conv(int num_msg, const struct pam_message **msg,
                     struct pam_response **resp, void *appdata_ptr) {
    (void)appdata_ptr; // Unused variable safeguard
    
    *resp = calloc(num_msg, sizeof(struct pam_response));
    if (*resp == NULL) {
        return PAM_BUF_ERR;
    }

    for (int i = 0; i < num_msg; ++i) {
        if (msg[i]->msg_style == PAM_PROMPT_ECHO_OFF) {
            char *pass = tsux_getpass(msg[i]->msg);
            if (pass) {
                (*resp)[i].resp = pass;
            }
        } else if (msg[i]->msg_style == PAM_PROMPT_ECHO_ON) {
            char input[256];
            printf("\x1b[1;33m%s\x1b[0m", msg[i]->msg);
            if (fgets(input, sizeof(input), stdin)) {
                input[strcspn(input, "\n")] = '\0';
                (*resp)[i].resp = strdup(input);
            }
        } else if (msg[i]->msg_style == PAM_TEXT_INFO) {
            printf("\x1b[0;35mINFO\x1b[0m: %s\n", msg[i]->msg);
        } else if (msg[i]->msg_style == PAM_ERROR_MSG) {
            fprintf(stderr, "\x1b[1;31mERROR\x1b[0m: %s\n", msg[i]->msg);
        }
    }
    return PAM_SUCCESS;
}

static struct pam_conv conv = {
    tsux_conv,
    NULL
};

int print_help()
{
    printf("\x1b[0;35mTsUX\x1b[0m\n");
    printf("\x1b[0;32mUsage:\x1b[0m\n");
    printf("   \x1b[0;35mtsux\x1b[0m <userid> <command> \x1b[0;34m[args]\x1b[0m\n");
    return 0;
}

int uid_allow_check(uid_t uid)
{
    FILE *f = fopen("/etc/tsux.allow", "r");
    if (!f) return 0;

    char line[MAX_NUM_UIDS];

    while (fgets(line, sizeof(line), f)) {
        char *end = NULL;
        unsigned long allowed = strtoul(line, &end, 10);

        if (end == line) continue;

        if (allowed == uid) {
            fclose(f);
            return 1;
        }
    }

    fclose(f);
    return 0;
}

int authenticate(const char *user)
{
    pam_handle_t *pamh = NULL;

    int ret = pam_start("tsux", user, &conv, &pamh);
    if (ret != PAM_SUCCESS)
        return 0;

    ret = pam_authenticate(pamh, 0);
    if (ret != PAM_SUCCESS) {
        pam_end(pamh, ret);
        return 0;
    }

    ret = pam_acct_mgmt(pamh, 0);
    pam_end(pamh, ret);

    return ret == PAM_SUCCESS;
}

int get_privileges(uid_t uid)
{
    struct passwd *pw = getpwuid(uid);
    if (!pw) {
        fprintf(stderr, "\x1b[1;31mERROR\x1b[0m: getpwuid failed\n");
        return -1;
    }

    if (initgroups(pw->pw_name, pw->pw_gid) != 0) {
        fprintf(stderr, "\x1b[1;31mERROR\x1b[0m: initgroups failed\n");
        return -1;
    }

    if (setgid(pw->pw_gid) != 0) {
        fprintf(stderr, "\x1b[1;31mERROR\x1b[0m: setgid failed\n");
        return -1;
    }

    if (setuid(uid) != 0) {
        fprintf(stderr, "\x1b[1;31mERROR\x1b[0m: setuid failed\n");
        return -1;
    }

    printf("\x1b[1;32mSUCCESS\x1b[0m: Privileges switched to UID %d successfully\n\n", uid);
    return 0;
}

int minimal_exec(char *command)
{
    char *minimal_env[] = {
        "PATH=/usr/bin:/bin",
        "HOME=/",
        NULL
    };

    pid_t pid = fork();

    if (pid == 0) {
        execve(command,
               (char *[]) { command, NULL },
               minimal_env);

        fprintf(stderr, "\x1b[1;31mERROR\x1b[0m: command execution failed\n");
        _exit(1);
    }

    if (pid < 0) {
        fprintf(stderr, "\x1b[1;31mERROR\x1b[0m: can't fork process\n");
        return -1;
    }

    waitpid(pid, NULL, 0);
    return 0;
}

int full_exec(char *shell, char **argv)
{
    pid_t pid = fork();

    if (pid == 0) {
        size_t len = 0;
        for (int i = 2; argv[i]; i++)
            len += strlen(argv[i]) + 1;

        char *cmd = malloc(len + 1);
        if (!cmd) _exit(1);

        cmd[0] = '\0';

        for (int i = 2; argv[i]; i++) {
            strcat(cmd, argv[i]);
            if (argv[i + 1]) strcat(cmd, " ");
        }

        char *sh_argv[] = { shell, "-c", cmd, NULL };

        execve(shell, sh_argv, environ);

        fprintf(stderr, "\x1b[1;31mERROR\x1b[0m: command execution failed\n");
        _exit(1);
    }

    if (pid < 0)
        return -1;

    wait(NULL);
    return 0;
}

char *getshell(uid_t uid)
{
    struct passwd *pw = getpwuid(uid);

    if (!pw || !pw->pw_shell || pw->pw_shell[0] == '\0')
        return "/bin/sh";

    return pw->pw_shell;
}

int nam2uid(char *name)
{
    struct passwd *pw = getpwnam(name);
    if (!pw) return -1;

    return pw->pw_uid;
}

char *uid2nam(uid_t uid)
{
    struct passwd *pw = getpwuid(uid);
    if (!pw || !pw->pw_name) return NULL;

    return strdup(pw->pw_name);
}

int main(int argc, char **argv)
{
    if (argc <= MIN_ARGS) {
        fprintf(stderr, "\x1b[1;31mERROR\x1b[0m: not enough arguments, minimum 2\n\n");
        print_help();
        return 2;
    }

    if (!uid_allow_check(getuid())) {
        fprintf(stderr, "\x1b[1;31mERROR\x1b[0m: user id %d not in /etc/tsux.allow\n", getuid());
        return 3;
    }

    char *user = uid2nam(getuid());
    if (!user) return 3;

    if (!authenticate(user)) {
        free(user);
        fprintf(stderr, "\x1b[1;31mERROR\x1b[0m: Authentication failed\n");
        return 1;
    }

    free(user);

    char *end = NULL;
    unsigned long loginto = strtoul(argv[1], &end, 10);

    if (end == argv[1] || *end != '\0') {
        fprintf(stderr, "\x1b[1;31mERROR\x1b[0m: invalid uid selection\n");
        return 3;
    }

    if (get_privileges(loginto) != 0)
        return 1;

    char *usershell = getshell(loginto);

    if (!usershell) {
        fprintf(stderr, "\x1b[1;31mERROR\x1b[0m: invalid target shell\n");
        return 3;
    }

    return full_exec(usershell, argv);
}
