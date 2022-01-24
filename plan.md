# How user programs will be submitted, compiled, and run:
## Submission
The user submits a compressed version of their source code files, in a `.zip`, `.tar.gz`, whatever floats their boat.
A `makefile` should be included at the top-level of their compressed source: `make` (the first "default" target) will be used for compilation, `make run` will be used to run your program, and `make clean` will be used after every compilation & run to remove any files you do not want.  
<!-- The user will also specify  -->

## Compilation
Upon receiving the file, the server will unzip the folder into the user's submission folder. A container is then created with their submission folder `flock`'d (as a psuedo-semaphore, to ensure no other process is using it) and mounted as the container's home directory, with reads & writes allowed. A `seccomp`'d process then runs `make` and `make clean` to compile the programn, the lock is released, and the container is discarded. The submission folder now contains the user's compiled program.

## Running
A container is created with the user's submission folder `flock`'d and mounted as the home directory. A `seccomp`'d process then runs `make run` (with silent mode on: we don't want to mix the program's output with `make run`'s output). After the game finishes, `make clean` is run to be ready for the next run, then the lock is released and the container is discarded.

## Re-submission
If a submission already exists, compilation goes as normal after removing all previous files. (This may be changed to allow some files to stay, maybe through a `make remove` which removes only the files unwanted from previous submissions).