57362u
// Just to be sure it uses gnu and posix functions
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

// Define the maximum amount of variables you can pass in one command
#define MAX_ARGS 64
// Define the maximum length for a password
#define MAX_PASS_LENGTH 256
// Define the max number of allowed uids
#define MAX_NUM_UIDS 64
// Define the minimum number of arguments required (YOU SHOULDN'T CHANGE THIS)
#define MIN_ARGS 2

// Import the current user env
extern char ** environ;

static char *tsux_getpass(const char *prompt)
{
	// Define variables needed later
	
    struct termios old, new;
    static char buf[MAX_PASS_LENGTH];
    int i = 0;

    printf("%s", prompt);
    fflush(stdout);

    tcgetattr(STDIN_FILENO, &old);

    new = old;
    new.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &new);

	// I won't... if it works don't touch it 
    while (i < (int)sizeof(buf) - 1) {
        char c;

        if (read(STDIN_FILENO, &c, 1) != 1)
            break;

        if (c == '\n' || c == '\r')
            break;

		// Removes dot when user uses backspace
        if (c == 127 || c == '\b') { /* backspace */
            if (i > 0) {
                i--;
                printf("\b \b");
                fflush(stdout);
            }
            continue;
        }
		// Print dots when user types
        buf[i++] = c;
        printf("•");
        fflush(stdout);
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &old);
	// Prints a newline when you're done typing your password
    buf[i] = '\0';
    putchar('\n');

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
            char buf[MAX_PASS_LENGTH];

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

// Tell pam to use my own conversation
static struct pam_conv conv = {
    tsux_conv,
    NULL
};

// Look at the name bruh
int print_help() {
    printf("\x1b[0;35mTsUX\x1b[0m\n");
    printf("\x1b[0;32mUsage:\x1b[0m\n");
    printf("   \x1b[0;35mtsux\x1b[0m <userid> <command> \x1b[0;34m[args]\x1b[0m\n");
    return 0;
}

int uid_allow_check(uid_t uid) {
	// Opens the file to check if the user is allowed to use tsux
    FILE *f = fopen("/etc/tsux.allow", "r");
    if (!f) return 0;

    char line[MAX_NUM_UIDS];

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

// After all this, finally authenticate the user
int authenticate(const char *user) {
    pam_handle_t *pamh = NULL;

	// Call my conversation to pam and some err handling
    int ret = pam_start("tsux", user, &conv, &pamh);
    if (ret != PAM_SUCCESS) return 0;

    ret = pam_authenticate(pamh, 0);
    if (ret != PAM_SUCCESS) {
        pam_end(pamh, ret);
        return 0;
    }

    ret = pam_acct_mgmt(pamh, 0);
    // End the authentication process
    pam_end(pamh, ret);

    return ret == PAM_SUCCESS;
}

// Use setuid(), setgid() and initgroups() to elevate permissions (or downgrade them)
int get_privileges(uid_t uid)
{
	// Also use this to get the pw database or whatever is that
    struct passwd *pw = getpwuid(uid);
    if (!pw) {
        printf("\x1b[0;31mERROR\x1b[0m: getpwuid");
        return -1;
    }

    if (initgroups(pw->pw_name, pw->pw_gid) != 0) {
        printf("\x1b[0;31mERROR\x1b[0m: initgroups");
        return -1;
    }

    if (setgid(pw->pw_gid) != 0) {
        printf("\x1b[0;31mERROR\x1b[0m: setgid");
        return -1;
    }

    if (setuid(uid) != 0) {
        printf("\x1b[0;31mERROR\x1b[0m: setuid");
        return -1;
    }

    printf("\x1b[0;35mINFO\x1b[0m: Privileges switched successfully\n\n");
    return 0;
}

int minimal_exec(char *command) {
	// Setup a minimal environment for non user commands
    char * minimal_env[] = {
        "PATH=/usr/bin:/bin",
        "HOME=/",
        NULL
    };

	// Fork the command process
    pid_t pid = fork();

	// Error handling in case of execve() or fork() failing
    if (pid == 0) {
        execve(command,
               (char * []) { command, NULL },
               minimal_env);

        printf("\x1b[0;31mERROR\x1b[0m: command execution failed");
        _exit(1);
    }

    if (pid < 0) {
        printf("\x1b[0;31mERROR\x1b[0m: can't fork process");
        return -1;
    }

    wait(NULL);
    return 0;
}

// The exec function that uses the forwarded env and is for user specified commands
int full_exec(uid_t uid, char *shell, char **argv) {
    pid_t pid = fork();

    if (pid == 0) {
    	// Get data about the user
        struct passwd *pw = getpwuid(uid);
        if (!pw) _exit(1);

        // Setup the uid, gid and groups
        initgroups(pw->pw_name, pw->pw_gid);
        setgid(pw->pw_gid);
        setuid(uid);

        // Join argv[2...] into one string
        size_t len = 0;
        for (int i = 2; argv[i]; i++)
            len += strlen(argv[i]) + 1;

		// Allocate memory
        char *cmd = malloc(len + 1);
        if (!cmd) _exit(1);

        cmd[0] = '\0';

        for (int i = 2; argv[i]; i++) {
            strcat(cmd, argv[i]);
            if (argv[i + 1]) strcat(cmd, " ");
        }

		// Make it use a shell for parsing
        char *sh_argv[] = { shell, "-c", cmd, NULL };

		// Execute using execve()
        execve(shell, sh_argv, environ);

        printf("\x1b[0;31mERROR\x1b[0m: command execution failed");
        _exit(1);
    }

    if (pid < 0) return -1;

    wait(NULL);
    return 0;
}

// Debug function for printing info
int user_info() {
	printf("Current user:\n");
	minimal_exec("/bin/whoami");
	printf("\n");
	printf("User id info:\n");
	minimal_exec("/bin/id");
	printf("\n");
	return 0;
}

// Converts a uid to a user default shell
char * uid2shell(uid_t uid) {
    struct passwd * pw = getpwuid(uid);
    if (!pw || !pw->pw_shell) return "/bin/sh";
	
    return pw->pw_shell;
}

// Coneverts a username to a uid
int nam2uid(char * name) {
    struct passwd * pw = getpwnam(name);
    if (!pw) return -1;
    return pw->pw_uid;
}

// Converts a uid to a username
char * uid2nam(uid_t uid) {
    struct passwd * pw = getpwuid(uid);
    if (!pw || !pw->pw_name) return NULL;

    return strdup(pw->pw_name);
}

// Start using all these functions
int main(int argc, char ** argv) {
	// Check if the user provided enough args
	if (argc <= MIN_ARGS) {
		printf("\x1b[0;31mERROR\x1b[0m: not enough arguments, minimum 2\n\n");
		print_help();
		return 2;
	}

	// Check if the user is allowed to elevate
	if (!uid_allow_check(getuid())) { 
		printf("\x1b[0;31mERROR\x1b[0m: user id not in /etc/tsux.allow\n");
		return 1;
	}

	// Get the username
	char *user = uid2nam(getuid());
	if (!user) return 1;

	// Authenticate the user
	if (!authenticate(user)) {
		// Free the variable to prevent mem leaks
	    free(user);
	    
	    printf("\x1b[0;31mERROR\x1b[0m: Auth failed\n");
	    return 1;
	}

	// Get the target user's uid
	uid_t loginto = (uid_t)atoi(argv[1]);

	// Elevate the privileges
	if (get_privileges(loginto) != 0) return 1;

	// Exec the command specified by the user
	full_exec(loginto, uid2shell(loginto), argv);
	
	return 0;
}
