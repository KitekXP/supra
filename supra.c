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

#define MAX_ARGS 64

extern char ** environ;

static struct pam_conv conv = {
    misc_conv,
    NULL
};

int uid_allow_check(uid_t uid) {
    FILE *f = fopen("/etc/supra.allow", "r");
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

    int ret = pam_start("supra", user, &conv, &pamh);
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

int get_priviliges(int uid) {
	int errcode = setuid(uid);

	if (errcode != 0) {
		printf("ERROR: check file previliges\n");
		printf("ERROR: the owner should be root and group root\n");
		printf("ERROR: previliges should be ---x--s--s\n");
		printf("ERROR: you can correct by executing:\n");
		printf("ERROR:   chown root:root /location/of/supra\n");
		printf("ERROR:   chmod 4111 /location/of/supra\n");
		printf("ERROR: order matters!\n");
		return 1;
	} else {
		printf("Authenticated successfully!\n");
		return 0;
	}
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

        perror("ERROR: command execution failed");
        _exit(1);
    }

    if (pid < 0) {
        perror("ERROR: can't fork process");
        return -1;
    }

    wait(NULL);
    return 0;
}

int full_exec(uid_t uid, char *shell, char **argv) {
    pid_t pid = fork();

    if (pid == 0) {
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

        perror("ERROR: command execution failed");
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
		printf("ERROR: not enough arguments, minimum 2\n");
		return 2;
	}

	if (!uid_allow_check(getuid())) { printf("ERROR: user id not in /etc/supra.allow\n"); return 1; }

	if (!authenticate(uid2nam(getuid()))) {
	    printf("ERROR: Auth failed\n");
	    return 1;
	}
	
	uid_t loginto = (uid_t)atoi(argv[1]);
	
	get_priviliges(loginto);
	user_info();

	full_exec(loginto, uid2shell(loginto), argv);
	
	return 0;
}
