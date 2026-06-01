% TSUX(1)
% Tomasz Wojtczak
% June 1, 2026

# TSUX

tsux - a smallish alternative for sudo

# SYNOPSIS

tsux <uid> <command> [args]

# DESCRIPTION

Tsux is a small alsternative to sudo that passes most environment variables trough.
It also passes the commands to the specified users default shell.

Example:
	tsux 0 whoami

	tsux 0 mkdir /test

# NOTES

The source code of tsux can be accessed and downloaded at <https://github.com/KitekXP/tsux>.
