# Coding Competition Server
## Group Members
- Yusuf Elsharawy, Period 5
## Overview
Online coding competitions such as [Halite](https://halite.io) and similarly (but not exactly) [CodinGame](https://www.codingame.com) take user-submitted code and run them to compete in a game against each other repeatedly, usually with a ranking system that pits similarly-ranked players together.  
My project idea is to recreate this system to allow anyone to host their own coding competition server for anyone else to submit code to. It should be simple for any user of the server to create their own games to support any (feasible) number of players.
## User Interface
The entire interface, for both the server and client, will be through the command line. The client will naturally require more user interaction, while the server will require little to none. The client can also connect to a server on the same machine, in which case it's an "admin" client and can run priviledged commands (this feature may be left out, mostly because I can't think of a very practical purpose for an admin client, except maybe for throttling the number of matchmaking children for each game).  
I have not yet decided whether either program will only take command line arguments, or display a sort of "shell" of their own for live interaction.
## Technical Design
### Topics From Class
The included topics are:
- Allocating memory
  - This is rather straightforward -- the server will definitely need to allocate memory for all sorts of things (strings, data structures, etc.)
- Working with files
  - Source code files will be sent to/from the server/client, so both sides will have to work with files.
  - Information on both users and games will be stored in files.
  - Redirection of stdin and stdout will be necessary for communication between players and the game
- Processes
  - The server will operate with multiple subservers & other children, for the purposes of:
    - Matching users & competing their bots against each other (1-many children per game)
      - Children of these children will `exec` the user-submitted programs
      - One child of these children will run the actual game to communicate with the user-submitted programs
    - Re-ranking users after each match (1 child per game)
    - Interacting with each client (many children, fluctuating with respect to # of active users) 
    - Interacting with an "admin" client (1 child)
  - Other command line utilities such as `gcc`, `ftp` and/or `gzip` may be used via `fork` & `exec`.
- Semaphores
  - Accessing the leaderboard of a game should be locked behind a semaphore to prevent interference with the re-ranking process
- Pipes
  - The "admin" client will communicate with the server via a pipe.
  - The matchmaking children will send information on wins/losses to the re-ranking child through a single unnamed pipe.
  - The matchmaking children will also need to communicate through the user-submitted programs' `stdin` and `stdout`, which will be `dup2`'d to unnamed pipes.
- Sockets
  - All other clients will communicate with the server through sockets.
### Work Distribution
There is only me. I will do all of the work.  
### Data Structures
- A linked list (likely in shared memory) will be used to store all of the users, in order of their ranking, for each game
    ```c
    struct player_ranking {
        int user_id;
        int score;
        struct player_ranking *next;
    }
    ```
- A game will be stored as (roughly):
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
                game_info       // game's acceptde # of players, etc.
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
   - I can also double check to ensure there are no memory leaks or possible file table mishaps (accidentally leaving a pipe open before `exec`-ing, so a user-submitted program to write to it)
