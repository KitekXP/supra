#include <unistd.h>
#include <grp.h>
#include <sys/types.h>
#include <limits.h>
#include <string.h>

int initgroups(const char *name, gid_t basegid) {
    gid_t groups[NGROUPS_MAX + 1];
    int ngroups = 0;

    // 1. Always include the primary/base group first
    groups[ngroups++] = basegid;

    // 2. Scan the system's group database for any auxiliary memberships
    setgrent(); // Reset group file iterator
    struct group *gr;
    while ((gr = getgrent()) != NULL) {
        // Skip if this group ID is already the base group ID
        if (gr->gr_gid == basegid) {
            continue;
        }

        // Check if our user is explicitly listed in this group's members array
        for (int i = 0; gr->gr_mem[i] != NULL; i++) {
            if (strcmp(gr->gr_mem[i], name) == 0) {
                // Prevent array overflow if the user is in too many groups
                if (ngroups < NGROUPS_MAX) {
                    groups[ngroups++] = gr->gr_gid;
                }
                break;
            }
        }
    }
    endgrent(); // Close group file streams cleanly

    // 3. Commit the array to the OS kernel for the current process
    return setgroups(ngroups, groups);
}
