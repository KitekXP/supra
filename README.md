# tSUx
This is a small alternative for sudo, that probably has some questionable code but hey! It works!

## Installing
`make build`

## Building
`make build`

But you probably would like to make it work, if so use `make perms`
In that case it will also make so the owner is the group root and the user root, next it will do `chmod 4111` to make the `setuid()` and `setgid()` have sufficient permissions
