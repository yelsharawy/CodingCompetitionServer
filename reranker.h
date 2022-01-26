#pragma once


typedef struct UserRankingData {
    UserID userID;
    Score score;
} UserRankingData;

typedef struct UserRanking {
    UserRankingData data;
    struct UserRanking *prev; // `prev` has higher score
    struct UserRanking *next; // `next` has lower score
} UserRanking;

extern UserRanking *ranker_root;

int rankonce();
void rankerloop();

void print_rankings();
void free_rankings();
int load_rankings(char *file);
void save_rankings(char *file);
void add_ranking_sorted(UserRankingData data);
