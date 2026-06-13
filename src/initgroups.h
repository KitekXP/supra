#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>

int initgroups(const char *name, gid_t basegid) {
    long max_groups = sysconf(_SC_NGROUPS_MAX);
    if (max_groups < 0) max_groups = 32;

    gid_t *groups = malloc((max_groups + 1) * sizeof(gid_t));
    if (!groups) return -1;

    int ngroups = 0;
    groups[ngroups++] = basegid;

    // Open and parse /etc/group manually
    FILE *f = fopen("/etc/group", "r");
    if (f) {
        char line[1024];
        while (fgets(line, sizeof(line), f)) {
            line[strcspn(line, "\n")] = '\0';

            char *gr_name = strtok(line, ":");
            char *gr_passwd = strtok(NULL, ":");
            char *gr_gid_str = strtok(NULL, ":");
            char *gr_mem_list = strtok(NULL, ":");

            if (!gr_name || !gr_gid_str) continue;

            gid_t gr_gid = (gid_t)strtoul(gr_gid_str, NULL, 10);
            if (gr_gid == basegid) continue;

            if (gr_mem_list) {
                char *user = strtok(gr_mem_list, ",");
                while (user) {
                    if (strcmp(user, name) == 0) {
                        if (ngroups < max_groups) {
                            groups[ngroups++] = gr_gid;
                        }
                        break;
                    }
                    user = strtok(NULL, ",");
                }
            }
        }
        fclose(f);
    }
   
    int result = syscall(SYS_setgroups, ngroups, groups);
    
    free(groups);
    return result;
}
