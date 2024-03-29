# Coding Competition Server
## Group Members
- Yusuf Elsharawy, Period 5
## Overview
Online coding competitions such as [Halite](https://halite.io) and similarly (but not exactly) [CodinGame](https://www.codingame.com) take user-submitted code and run them to compete in a game against each other repeatedly, usually with a ranking system that pits similarly-ranked players together.  
My project recreates this system in an extensible way, allowing anyone to host a coding competition server for others to submit code to. It should be simple for the host to create their own games to support any (feasible) number of players.
## Features
### Security
  - Each player's code is run in an isolated container (using Linux namespaces), only with access to the bare necessities and revoked access to security-breaking functions (i.e. `chroot`). Stack & memory usage is also limited, so there should be no way to crash the host.
    - Child processes that execute arbitrary code are not able to access the parent process's files.
    - The only things a bad actor could do, as far as I know, is fill up the filesystem with tons of files, and idle forever to stall the server. A fork bomb may also be an issue, but I'm too scared to test it.
      - Since the server cannot install submissions on its own, these cases can be caught by the operator with minimal damage.
  - If the container's "creator" process encounters an error, the container's root process and all of its children will be killed, to prevent orphan processes.
    - The same will happen if the container's creator's parent encounters an error, while also ensuring to clean up.
### Cleanliness/Robustness
  - All containers exist temporarily in the `/tmp` directory and are removed when no longer needed. They're generated with random names ensured to not collide via `mkdtemp`.
  - There should not be any cases of orphaned processes.
  - Probably valgrind clean (i.e. no memory leaks). I couldn't figure out how to test it because `valgrind` doesn't like the `clone3` system call or something.
### Flexibility
  - (Note: in this context, "AI" stands for "artificial intelligence", not "machine learning" or "neural network". A deterministic, hard-coded AI is still an AI.)
  - Language
    - Any game and any AI for any game can be written in any language, so long as you can create a makefile for it.
  - Game Creation
    - It's really easy to make a game! The game is not responsible for handling containers or reranking players, allowing the game to follow a very simple protocol to communicate with its parent. Through `stdin` and `stdout`, it only has to:
      - Get the paths to the AIs' input/output pipes from the parent.
      - Report back to the parent on the outcome of the game, all through `stdin` and `stdout`.
      
      You can also use `stderr` to print things to the terminal (for debugging/logging). In case something goes wrong, just `exit`, and the parent will handle it!
    - Although the game has to (to some extent) follow my made-up protocol, you can design whatever protocol you like for communication between your game and the players. Data is sent directly between the game and players with no process in between.
    - Games aren't limited to just 2 players! I made su0re _everywhere_ to provide support for games with more players, even using a generalized version of the Elo rating system for ranking players.
  - AI Creation
    - Similarly, it's really easy to make an AI! Again, `stdin` and `stdout` are redirected (and `stderr` isn't), so as long as your language has input and output, you can write an AI.
### Program Design
  - At the time of completing this project, I was very proud of the functions and macros I made behind the scenes, such as `clone_fork` and `create_container` in `containers.c`, as well as my error-checking macros and `execlp_block`.
### Miscellaneous
  - When creating this project, I let my workspace be a bit messy. I had cleaned up some of this mess by putting it into the `misc` folder, such as the `create_container.sh` script, and `readwrite`, which I used for testing containers and protocols respectively.
## Required Libraries
Nothing, aside from POSIX C on GCC, as my project makes use of many GNU extensions. In hindsight, it might've sped up development to find something to do the container work for me.
## Instructions
Clone this repo, and run `make`. You should get a program called `ccs` (which stands for Coding Competition Server, a misnomer now that it's not really a server).  
Run `./css` for a list of commands and their usages. An example consisting of a series of commands will also show up, which you should be able to directly run exactly as is. Remember that my project is only the server backend (the operator is expected to provide their own games & AIs), but you can use the example `tic-tac-toe.tar` game and `random_player.tar` submission to test it.  
As far as I know, no errors should come up during proper usage. Any errors that *do* come up should have descriptive error messages pointing to the specific line where it occured (segmentation faults eradicated).

The rest of the README is my old vision of the project, with edits made for significant changes.
## User Interface
The entire interface, for both the server and client, will be through the command line. The client will naturally require more user interaction, while the server will require little to none. The client can also connect to a server on the same machine, in which case it's an "admin" client and can run priviledged commands (this feature may be left out, mostly because I can't think of a very practical purpose for an admin client, except maybe for throttling the number of matchmaking children for each game).  
I have not yet decided whether either program will only take command line arguments, or display a sort of "shell" of their own for live interaction.
## Technical Design
### Topics From Class
(Crossed out sections are unimplemented due to time constraints)  
The included topics are:
- Allocating memory
  - This is rather straightforward -- the server will definitely need to allocate memory for all sorts of things (strings, data structures, etc.)
- Working with files
  - ~~Source code files will be sent to/from the server/client, so both sides will have to work with files.~~
  - User information and leaderboard stats are stored in files.
  - Redirection of stdin and stdout will be necessary for communication between players and the game.
  - The `flock` function is also used to ensure only one process creates a container around a folder at a time.
- Finding information about files
  - To test for the existence of a directory and ensure that it _is_ a directory, `stat` is used.
  - Going through the subfolders of the `users` directory is also used to find a specific user by their username.
- Processes
  - The server will operate with multiple subservers & other children, for the purposes of:
    - ~~Matching users &~~ competing user submissions against each other ~~(1-many children per game)~~
      - Children of these children will `exec` the user-submitted programs
      - One child of these children will run the actual game to communicate with the user-submitted programs
    - Re-ranking users after each match (1 child per game)
    - ~~Interacting with each client (many children, fluctuating with respect to # of active users)~~
    - ~~Interacting with an "admin" client (1 child)~~
  - Other command line utilities such as `make`, `rm` and `tar` are used via `fork` & `exec`.
- Signals
  - When building & running games & user submissions, signals are used to terminate the process if the parent dies or requests for it.
  - A signal handler is also used to make sure all cleaning up is done properly upon receival of `SIGTERM`.
- ~~Semaphores~~
  - ~~Accessing the leaderboard of a game should be locked behind a semaphore to prevent interference with the re-ranking process~~ (design changed)
- Pipes
  - ~~The "admin" client will communicate with the server via a pipe.~~
  - The matchmaking children will send information on wins/losses to the re-ranking child through a single unnamed pipe.
  - The running game will also need to communicate through the user-submitted programs' `stdin` and `stdout`, which will have named pipes `dup2`'d into them.
- ~~Sockets~~
  - ~~All other clients will communicate with the server through sockets.~~
### Work Distribution
There is only me. I will do all of the work.  
### Data Structures
- A doubly-linked list ~~(likely in shared memory)~~ will be used to store all of the users, in order of their ranking, for each game
    ```c
    struct player_ranking {
        int user_id;
        int score;
        struct player_ranking *prev;
        struct player_ranking *next;
    }
    ```
- ~~A game will be stored as (roughly):~~ (game info is not stored in memory)
    ```c
    struct game {
      struct player_ranking *leaderboard; // `game->leadboard->next` is first player
      char[NAME_MAX+1] name; // `NAME_MAX` comes from `<limits.h>`
      unsigned char min_players;
      unsigned char max_players;
      // anything else i may add
    };
    ```
- The file system will be used by the server for storing a lot of data, including:
  - Registered users' info & submitted programs
    - Each user gets their own directory, in which subdirectories contain their submissions for each game  
        ```
        server_data/
            users/
                8005882300/                     // randomly generated ID
                    user_info                   // username, key, etc.
                    submissions/
                        tic-tac-toe/
                            submission.c        // user-submitted
                            prog                // executable
                        connect-four/
                            Submission.java     // user-submitted
                            Submission.class
                            prog                // executable shim
                        ...
        ```
  - All games on this server and their leaderboards (it's still undecided whether `game_info` is really necessary, as I could require each game to provide info when executed with certain command line arguments)
    ```
    server_data/
        games/
            tic-tac-toe/
                game_info       // game's accepted # of players, etc.
                prog            // executable
                leaderboard     // list of players & their scores
    ```
### Timeline
I will focus on one section at a time, in mostly, but not strictly, this order:
1. Running 2 player-programs and a game-program in communication with each other (by 01/14)
    - An example game I may create is tic-tac-toe, checkers, or connect four
    - I decide the "protocol" by which a game has to follow (e.g. game prints to stdout how many players it wants, receives in stdin the names of each player's stdin/stdout pipes)
    - The game creator decides by what format input & output is sent in between the game program and the player.
2. Sending outcomes of a match through a pipe to a "re-ranking" process that will, accordingly, re-rank the players (by 01/17)
    - The re-ranking process and matchmaking processes will be "siblings", using shared memory created by their parent
    - Ranking will use the [Elo rating system](https://en.wikipedia.org/wiki/Elo_rating_system) or something similar. Each match will change the player's scores and have their player be sorted
3. Loading games from the file system to start running & matchmaking on (by 01/19)
    - Support for multiple games per server may be cut out due to time constraints
4. Server-client interaction (by 01/24)
    - Registering a new user
      - Designate a directory in which the user's info will be stored
      - Some sort of public/private key system for future login attempts?
    - Sending & receiving files over a network, and replacing potentially existing ones
      - The leadboard score for a resubmission will be reset as well
    - Compiling the user-submitted program
    - Handling other user requests, such as viewing their position on the leaderboard
5. Putting all of the server components together as separate children in one program (by 01/26)

The remaining features are auxiliary, and may not be implemented due to time constraints:  
1. Additional user requests (by 01/31)
   - Users may request to view the whole leaderboard
   - Users may request to see games recently played by their bot
   - Users may request to receive logs on specific previous games (consisting of the info sent to/from their program, and the opponent's program)
   - Users may request to play a match against any other user, without getting scored for the result
2. Security features (by 01/31)
   - Running user-submitted code can be a huge security risk, so, if I have time, I will try to run the user-submitted programs in a sandbox.
   - I can also double check to ensure there are no memory leaks or possible file table mishaps (accidentally leaving a pipe open before `exec`-ing, so a user-submitted program could write to it)
