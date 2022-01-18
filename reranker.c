#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <math.h>
#include "constants.h"

#define D 400
#define K 50
// ^ K should probably be individual to each player, as their score becomes more certain
// but im lazy (for now)

// "re-sorts" a single element in a doubly-linked list,
// assuming the rest of the list is already sorted
// also preserves order of equally scored users
void resort(UserRanking *ranking) {
    // move up in rankings until user above has greater or equal score
    while (ranking->prev && ranking->prev->data.score < ranking->data.score) {
        // swap adjacent nodes
        UserRanking *swapping = ranking->prev;
        swapping->next = ranking->next;
        ranking->prev = swapping->prev;
        // `swapping` and `ranking` now have same prev & next
        ranking->next = swapping;
        swapping->prev = ranking;
    }
    // move down in rankings until user below has lesser or equal score
    while (ranking->next && ranking->next->data.score > ranking->data.score) {
        // swap adjacent nodes
        UserRanking *swapping = ranking->next;
        swapping->prev = ranking->prev;
        ranking->next = swapping->next;
        // `swapping` and `ranking` now have same prev & next
        ranking->prev = swapping;
        swapping->next = ranking;
    }
}

void expected_outcomes(PlayerCount numplayers, UserRanking **scores, long double *dest) {
    int i, j;
    
    memset(dest, 0, sizeof(*dest)*numplayers);
    // ^ sets all to 0
    
    for (i = 0; i < numplayers; i++) {
        for (j = i + 1; j < numplayers; j++) {
            long double vi = powl(10.0, (scores[j]->data.score - scores[i]->data.score) / (long double)D);
            long double vj = powl(10.0, (scores[i]->data.score - scores[j]->data.score) / (long double)D);
            dest[i] += 1 / (1 + vi);
            dest[j] += 1 / (1 + vj);
        }
    }
    
    uint totalMatches = numplayers * (numplayers - 1) / 2;
    
    for (i = 0; i < numplayers; i++) dest[i] /= totalMatches;
}

// source: https://sradack.blogspot.com/2008/06/elo-rating-system-multiple-players.html
/* Rescores `numplayers` players, with a pointer to an array of pointers to scores given in `scores`.
 * `game_outcome` should point to an array of long doubles summing to 1 representing the result of a game
 */
void rescore(PlayerCount numplayers, UserRanking **rankings, long double *game_outcome) {
    long double *expected;
    expected = alloca(numplayers * sizeof(*expected));
    expected_outcomes(numplayers, rankings, expected);
    
    // for debugging purposes:
    long double sum = 0;
    for (int i = 0; i < numplayers; i++) {
        printf("player %d expected to get %Lf\n", i, expected[i]);
        sum += expected[i];
    }
    printf("sum of expected scores (should be 1): %Lf\n", sum);
    
    for (int i = 0; i < numplayers; i++) {
        Score scoreChange = K * (game_outcome[i] - expected[i]);
        rankings[i]->data.score += scoreChange;
        printf("player %lld score changed by %Lf\n", rankings[i]->data.userID, scoreChange);
        resort(rankings[i]);
    }
    
}

UserRanking *getbyID(UserID userID) {
    // test for now
    UserRanking *r;
    r = malloc(sizeof(*r));
    r->data.userID = userID;
    r->data.score = userID; // cuz
    r->prev = 0;
    r->next = 0;
}

void mainloop() {
    while (1) {
        PlayerCount numplayers;
        scanf("%hhu", &numplayers);
        
        UserRanking **rankings;
        rankings = malloc(numplayers * sizeof(*rankings));
        long double *outcome;
        outcome = malloc(numplayers * sizeof(*outcome));
        long long total = 0;
        for (int i = 0; i < numplayers; i++) {
            UserID id;
            unsigned long long out;
            scanf("%llu %llu", &id, &out); // could be sent as binary (since sent from 1 part of server to another)
            outcome[i] = out;
            total += out;
            rankings[i] = getbyID(id);
        }
        printf("got all input!\n");
        printf("weighted outcomes:\n");
        for (int i = 0; i < numplayers; i++) {
            outcome[i] /= total;
            printf("player %d: %Lf\n", i, outcome[i]);
        }
        rescore(numplayers, rankings, outcome);
        
        free(rankings);
        free(outcome);
    }
}

int main() {
    mainloop();
}