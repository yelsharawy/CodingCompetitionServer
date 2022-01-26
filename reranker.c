#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <math.h>
#include <sys/file.h>
#include <errno.h>
#include "common.h"
#include "users.h"
#include "reranker.h"

// sorted from HIGHEST score to LOWEST score
UserRanking *ranker_root = NULL;
// = { .data = { .score = -1ULL, .userID = 0 }, .prev = NULL, .next = NULL};

#define D 400
#define K 5
// ^ K should probably be individual to each match, as their score becomes more certain
// but im lazy (for now)

// "re-sorts" a single element in a doubly-linked list,
// assuming the rest of the list is already sorted
// also preserves order of equally scored users
void resort(UserRanking *ranking) {
    // printf("ranking: %p\n", ranking);
    // printf("ranking->prev: %p\n", ranking->prev);
    // printf("ranking->next: %p\n", ranking->next);
    // printf("about to resort!\n");
    // print_rankings();
    // move up in rankings until user above has greater or equal score
    while (ranking->prev && ranking->prev->data.score < ranking->data.score) {
        // printf("going up!\n");
        // swap adjacent nodes
        UserRanking *swapping = ranking->prev;
        swapping->next = ranking->next;
        ranking->prev = swapping->prev;
        // `swapping` and `ranking` now have same prev & next
        ranking->next = swapping;
        swapping->prev = ranking;
        if (ranking->prev) ranking->prev->next = ranking;
        if (swapping->next) swapping->next->prev = swapping;
        // print_rankings();
    }
    if (ranking->prev == NULL) ranker_root = ranking;
    // print_rankings();
    // move down in rankings until user below has lesser or equal score
    while (ranking->next && ranking->next->data.score > ranking->data.score) {
        // print_rankings();
        // printf("going down!\n");
        // swap adjacent nodes
        UserRanking *swapping = ranking->next;
        swapping->prev = ranking->prev;
        ranking->next = swapping->next;
        // `swapping` and `ranking` now have same prev & next
        ranking->prev = swapping;
        swapping->next = ranking;
        if (ranking->next) ranking->next->prev = ranking;
        if (swapping->prev) swapping->prev->next = swapping;
        
        if (swapping->prev == NULL) ranker_root = swapping;
        // print_rankings();
    }
    // print_rankings();
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
        // printf("player %d expected to get %Lf\n", i, expected[i]);
        sum += expected[i];
    }
    // printf("sum of expected scores (should be 1): %Lf\n", sum);
    
    for (int i = 0; i < numplayers; i++) {
        Score scoreChange = K * (game_outcome[i] - expected[i]);
        rankings[i]->data.score += scoreChange;
        // printf("player "USERID_SPEC" score changed by %Lf\n", rankings[i]->data.userID, scoreChange);
        resort(rankings[i]);
    }
    
}

UserRanking *getbyID(UserID userID) {
    UserRanking *cur = ranker_root;
    while (cur) {
        if (cur->data.userID == userID) return cur;
        // printf("id: "USERID_SPEC"\n", cur->data.userID);
        cur = cur->next;
    }
    // CHECKZEROMSG(1, "user with id "USERID_SPEC" not found in rankings", userID);
    return NULL;
    // static int s = 400;
    // // test for now
    // UserRanking *r;
    // r = malloc(sizeof(*r));
    // r->data.userID = userID;
    // r->data.score = (s += 100);
    // r->prev = 0;
    // r->next = 0;
}

int rankonce() {
    int ret;
    PlayerCount numplayers;
    ret = scanf("%hhu", &numplayers);
    if (ret != 1) return 0;
    
    UserRanking **rankings;
    rankings = malloc(numplayers * sizeof(*rankings));
    long double *outcome;
    outcome = malloc(numplayers * sizeof(*outcome));
    long long total = 0;
    for (int i = 0; i < numplayers; i++) {
        UserID id;
        unsigned long long out;
        scanf(USERID_SPEC" %llu", &id, &out); // could be sent as binary (since sent from 1 part of server to another)
        // printf("userid: "USERID_SPEC"\t outcome: %llu\n", id, out);
        outcome[i] = out;
        total += out;
        rankings[i] = getbyID(id);
        CHECKTRUE(rankings[i] != NULL);
    }
    // printf("got all input!\n");
    // printf("weighted outcomes:\n");
    for (int i = 0; i < numplayers; i++) {
        outcome[i] /= total;
        // printf("player %d: %Lf\n", i, outcome[i]);
    }
    rescore(numplayers, rankings, outcome);
    
    free(rankings);
    free(outcome);
    return 1;
}

void rankerloop(char *file) {
    load_rankings(file);
    while (rankonce());
    save_rankings(file);
}

void free_rankings() {
    UserRanking *cur = ranker_root;
    while (cur) {
        UserRanking *n = cur->next;
        free(cur);
        cur = n;
    }
    ranker_root = NULL;
}

void print_rankings() {
    printf("Leaderboard:\n");
    UserRanking *prev = NULL; 
    UserRanking *cur = ranker_root;
    for (int i = 1; cur; i++) {
        UserData data;
        get_userdata(cur->data.userID, &data);
        // printf("\t%p", cur);
        printf("  %d. "USERID_SPEC"\t%Lf\t(%s)\n", i, cur->data.userID, cur->data.score, data.username);
        CHECKTRUE(cur->prev == prev);
        prev = cur;
        cur = cur->next;
    }
}

int load_rankings(char *file) {
    int counted = 0;
    
    free_rankings(); // just in case one's already loaded
    int fd;
    CHECKNEG1(fd = open(file, O_RDONLY | O_CREAT, 0755));
    CHECKZERO(flock(fd, LOCK_EX) == -1 && errno == EBADFD);
    
    UserRanking *prev = NULL;
    UserRanking **cur = &ranker_root;
    UserRankingData buf;
    int ret;
    while ((ret = read(fd, &buf, sizeof(UserRankingData))) == sizeof(UserRankingData)) {
        // printf("loading: prev: %p\n", prev);
        *cur = malloc(sizeof(UserRanking));
        (*cur)->data = buf;
        // memcpy(&(*cur)->data, &buf, sizeof(UserRankingData));
        (*cur)->prev = prev;
        prev = *cur;
        cur = &((*cur)->next);
        counted++;
    }
    CHECKNEG1MSG(ret, "read failed");
    *cur = NULL;
    
    CHECKNEG1(close(fd));
    // printf("at the end of load_rankings:\n");
    // print_rankings();
    return counted;
}

void save_rankings(char *file) {
    // printf("at the beginning of save_rankings:\n");
    // print_rankings();
    int fd;
    CHECKNEG1(fd = open(file, O_WRONLY | O_CREAT, 0755));
    CHECKNEG1(flock(fd, LOCK_EX));
    
    UserRanking *cur = ranker_root;
    while (cur) {
        // printf("cur: %p\n", cur);
        CHECKTRUEMSG(write(fd, &(cur->data), sizeof(UserRankingData)) == sizeof(UserRankingData), "write failed: %m");
        
        cur = cur->next;
    }
    
    CHECKNEG1(close(fd));
}

void remove_ranking(UserRanking *ranking) {
    if (ranking) {
        if (ranking->prev) ranking->prev->next = ranking->next;
        if (ranking->next) ranking->next->prev = ranking->prev;
        free(ranking);
    }
    // print_rankings();
}

void add_ranking_sorted(UserRankingData data) {
    remove_ranking(getbyID(data.userID));
    UserRanking *pushedUp = NULL;
    UserRanking *cur = ranker_root;
    // move forward until `cur` is lower than `data`
    while (cur && cur->data.score >= data.score) {
        pushedUp = cur;
        cur = cur->next;
    }
    UserRanking *to_insert = malloc(sizeof(UserRanking));
    to_insert->data = data;
    to_insert->next = cur;
    to_insert->prev = pushedUp;
    if (cur) cur->prev = to_insert;
    if (pushedUp) pushedUp->next = to_insert;
    else ranker_root = to_insert;
    // print_rankings();
}

// int main() {
//     mainloop();
// }