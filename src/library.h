#ifndef LIBRARY_H
#define LIBRARY_H

#include "fs.h"
#include "launch.h"

struct LaptopConfig;

/* Which Launch target resolution rule found an Entry's Launch target, in
 * resolution order (see resolve_launch_target in library.c). */
typedef enum TargetRule {
    RULE_OVERRIDE,      /* this laptop's exe.<Entry folder name>= */
    RULE_EXE_ABSOLUTE,  /* exe= as an absolute path */
    RULE_EXE_INSTALL,   /* relative exe= in Install folder number rule_install */
    RULE_EXE_ENTRY,     /* relative exe= in the Entry folder (or its juego.cfg's) */
    RULE_NAME_SEARCH,   /* exe='s file name searched in the Install folders */
    RULE_SHORTCUT,      /* a shortcut in the Entry folder */
    RULE_AUTOMATIC      /* automatic choice of an executable in the Entry folder */
} TargetRule;

/* An Available Entry (the type keeps its historical name). Exactly one of
 * shortcut / exe is set: the resolved Launch target. */
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
    TargetRule rule;   /* how the Launch target was found */
    int rule_install;  /* RULE_EXE_INSTALL: 1-based Install folder number, else 0 */
    /* Launching, from the Entry's options */
    char *args;      /* args=: launch arguments, or NULL */
    /* Working folder for exe: dir= (absolute, or relative to the
     * executable's folder), else the executable's folder. NULL for a
     * shortcut, which keeps its own. */
    char *work_dir;
    char *wait;      /* wait=: executable name of the real game process, or NULL */
    /* Gamepad mapper helper, from the Entry's options */
    char *con;       /* con=: the helper's executable, resolved like exe=, or NULL */
    char *con_args;  /* con_args=: its arguments (typically a profile), or NULL */
} Game;

typedef enum HiddenReason {
    HIDDEN_NO_LAUNCH_INFO,  /* no shortcut, no exe= and no executable */
    HIDDEN_NOT_FOUND        /* launch information given, but nothing exists */
} HiddenReason;

/* An Entry that is not shown on this laptop, and why. */
typedef struct HiddenEntry {
    char *name;
    char *dir;
    HiddenReason reason;
    char *exe;           /* exe= as written in the Entry's options, or NULL */
    char *exe_override;  /* this laptop's exe.<Entry folder name>=, or NULL */
} HiddenEntry;

typedef struct Library {
    char *root;
    Game *games;           /* Available Entries, sorted by name */
    int count;
    HiddenEntry *hidden;   /* hidden Entries, sorted by name */
    int hidden_count;
} Library;

/* Scans the Entry folders of root. cfg is this laptop's configuration (may be
 * NULL). */
Library *library_scan(const Fs *fs, const char *root, const struct LaptopConfig *cfg);
void library_free(Library *lib);

/* Everything launch_and_wait needs to start an Available Entry. The strings
 * belong to g. */
LaunchTarget game_launch_target(const Game *g);

#endif
