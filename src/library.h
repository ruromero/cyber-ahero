#ifndef LIBRARY_H
#define LIBRARY_H

typedef struct Game {
    char *name;
    char *dir;
    char *shortcut;
    char *exe;
    char *icon;
    char *cover;
    char *video;
    char *desc_path;
    char *desc_text;
    char *meta_path;
    char *cfg_path;
    char **meta_keys;
    char **meta_vals;
    int meta_count;
    int icon_score;
    int exe_score;
} Game;

typedef struct Library {
    char *root;
    Game *games;
    int count;
} Library;

Library *library_scan(const char *root);
void library_free(Library *lib);

#endif
