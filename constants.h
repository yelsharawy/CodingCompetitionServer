#include <sys/types.h>

#define MAX_PLAYERS 255

typedef u_int8_t PlayerCount;
// typedef long long Score;
typedef long double Score;
typedef unsigned long long UserID;

typedef struct UserRankingData {
    UserID userID;
    Score score;
} UserRankingData;

typedef struct UserRanking {
    UserRankingData data;
    struct UserRanking *prev; // `prev` has higher score
    struct UserRanking *next; // `next` has lower score
} UserRanking;


// A xor C, B xor D  -(xor)-> A xor B xor C xor D
// (xor B xor C to both)
// A xor B, C xor D
// (xor A xor B xor C xor D to both)
// ->
// C xor D, A xor B