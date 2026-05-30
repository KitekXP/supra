#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <security/pam_appl.h>
#include <security/pam_misc.h>
#include <grp.h>
#include <termios.h>

extern int initgroups(const char *, gid_t);

#define MAX_ARGS 64

extern char ** environ;

static char *tsux_getpass(const char *prompt)
{
    struct termios old, new;
    static char buf[256];
    int i = 0;

    printf("%s", prompt);
    fflush(stdout);

    tcgetattr(STDIN_FILENO, &old);
    new = old;
    new.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new);

    while (i < (int)sizeof(buf) - 1) {
        char c = getchar();
        if (c == '\n' || c == '\r')
            break;
        buf[i++] = c;
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &old);

    buf[i] = '\0';
    printf("\n");

    return buf;
}

static int tsux_conv(int num_msg,
                     const struct pam_message **msg,
                     struct pam_response **resp,
                     void *appdata_ptr)
{
    (void)appdata_ptr;

    *resp = calloc(num_msg, sizeof(struct pam_response));
    if (!*resp)
        return PAM_CONV_ERR;

    for (int i = 0; i < num_msg; i++) {

        switch (msg[i]->msg_style) {

        case PAM_PROMPT_ECHO_OFF: {
            char *pw = tsux_getpass("\x1b[0;32mpass\x1b[0m=> ");
            if (!pw)
                return PAM_CONV_ERR;

            (*resp)[i].resp = strdup(pw);
            break;
        }

        case PAM_PROMPT_ECHO_ON: {
            char buf[256];

            printf("%s", msg[i]->msg);
            fflush(stdout);

            if (!fgets(buf, sizeof(buf), stdin))
                return PAM_CONV_ERR;

            buf[strcspn(buf, "\n")] = 0;
            (*resp)[i].resp = strdup(buf);
            break;
        }

        case PAM_TEXT_INFO:
            fprintf(stderr, "%s\n", msg[i]->msg);
            break;

        case PAM_ERROR_MSG:
            fprintf(stderr, "ERROR: %s\n", msg[i]->msg);
            break;
        }
    }

    return PAM_SUCCESS;
}

static struct pam_conv conv = {
    tsux_conv,
    NULL
};

int print_help() {
    printf("\x1b[0;35mTsUX\x1b[0m\n");
    printf("\x1b[0;32mUsage:\x1b[0m\n");
    printf("   \x1b[0;35mtsux\x1b[0m <userid> <command> \x1b[0;34m[args]\x1b[0m\n");
    return 0;
}

int uid_allow_check(uid_t uid) {
    FILE *f = fopen("/etc/tsux.allow", "r");
    if (!f) return 0;

    char line[64];

    while (fgets(line, sizeof(line), f)) {
        uid_t allowed = (uid_t)atoi(line);
        if (allowed == uid) {
            fclose(f);
            return 1;
        }
    }

    fclose(f);
    return 0;
}

int authenticate(const char *user) {
    pam_handle_t *pamh = NULL;

    int ret = pam_start("tsux", user, &conv, &pamh);
    if (ret != PAM_SUCCESS) return 0;

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
        perror("getpwuid");
        return -1;
    }

    if (initgroups(pw->pw_name, pw->pw_gid) != 0) {
        perror("initgroups");
        return -1;
    }

    if (setgid(pw->pw_gid) != 0) {
        perror("setgid");
        return -1;
    }

    if (setuid(uid) != 0) {
        perror("setuid");
        return -1;
    }

    printf("\x1b[0;35mINFO\x1b[0m: Privileges switched successfully\n\n");
    return 0;
}

int minimal_exec(char *command) {
    char * minimal_env[] = {
        "PATH=/usr/bin:/bin",
        "HOME=/",
        NULL
    };

    pid_t pid = fork();

    if (pid == 0) {
        execve(command,
               (char * []) { command, NULL },
               minimal_env);

        perror("\x1b[0;31mERROR\x1b[0m: command execution failed");
        _exit(1);
    }

    if (pid < 0) {
        perror("\x1b[0;31mERROR\x1b[0m: can't fork process");
        return -1;
    }

    wait(NULL);
    return 0;
}

int full_exec(uid_t uid, char *shell, char **argv) {
    pid_t pid = fork();

    if (pid == 0) {
        struct passwd *pw = getpwuid(uid);
        if (!pw) _exit(1);
        
        initgroups(pw->pw_name, pw->pw_gid);
        setgid(pw->pw_gid);
        setuid(uid);

        // join argv[2...] into one string
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

        perror("\x1b[0;31mERROR\x1b[0m: command execution failed");
        _exit(1);
    }

    if (pid < 0) return -1;

    wait(NULL);
    return 0;
}

int user_info() {
	printf("Current user:\n");
	minimal_exec("/bin/whoami");
	printf("\n");
	printf("User id info:\n");
	minimal_exec("/bin/id");
	printf("\n");
	return 0;
}

char * uid2shell(uid_t uid) {
    struct passwd * pw = getpwuid(uid);
    if (!pw || !pw->pw_shell) return "/bin/sh";
	
    return pw->pw_shell;
}

int nam2uid(char * name) {
    struct passwd * pw = getpwnam(name);
    if (!pw) return -1;
    return pw->pw_uid;
}

char * uid2nam(uid_t uid) {
    struct passwd * pw = getpwuid(uid);
    if (!pw || !pw->pw_name) return NULL;

    return strdup(pw->pw_name);
}

int main(int argc, char ** argv) {
	int min_args = 2;
	
	if (argc <= min_args) {
		printf("\x1b[0;31mERROR\x1b[0m: not enough arguments, minimum 2\n\n");
		print_help();
		return 2;
	}

	if (!uid_allow_check(getuid())) { printf("\x1b[0;31mERROR\x1b[0m: user id not in /etc/tsux.allow\n"); return 1; }

	char *user = uid2nam(getuid());
	if (!user) return 1;

	if (!authenticate(user)) {
	    free(user);
	    printf("\x1b[0;31mERROR\x1b[0m: Auth failed\n");
	    return 1;
	}
	
	free(user);
	
	uid_t loginto = (uid_t)atoi(argv[1]);
	
	if (get_privileges(loginto) != 0)
    return 1;
    
	user_info();

	full_exec(loginto, uid2shell(loginto), argv);
	
	return 0;
}
