#ifndef ORGANIZER_H
#define ORGANIZER_H

/* The Organizer report: the rows the Organizer view shows for a scanned
 * Library. Portable (no SDL, no Win32). Hidden Entries come first, with
 * their reason; then the Available Entries that lack a cover, a preview or
 * details, with what they lack. Complete Entries are not listed. Within each
 * group rows keep the Library's order (sorted by name). */

#include "library.h"

typedef enum OrganizerKind {
    ORGANIZER_HIDDEN,
    ORGANIZER_INCOMPLETE
} OrganizerKind;

enum {
    ORGANIZER_NO_COVER = 1,
    ORGANIZER_NO_PREVIEW = 2,
    ORGANIZER_NO_DETAILS = 4
};

typedef struct OrganizerRow {
    char *name;
    char *dir;
    OrganizerKind kind;
    int missing;   /* ORGANIZER_NO_* bits, for ORGANIZER_INCOMPLETE rows */
    char *text;    /* what the view shows next to the name (UTF-8, Spanish) */
} OrganizerRow;

typedef struct OrganizerReport {
    OrganizerRow *rows;
    int count;
    int hidden_count;
    int incomplete_count;
    int available_count;
} OrganizerReport;

OrganizerReport *organizer_report(const Library *lib);
void organizer_report_free(OrganizerReport *r);

#endif
