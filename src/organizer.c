#include "organizer.h"
#include "util.h"

#include <stdlib.h>
#include <string.h>

static const char *hidden_text(HiddenReason reason)
{
    switch (reason) {
    case HIDDEN_NO_LAUNCH_INFO:
        return "Oculto: sin información para lanzarlo";
    case HIDDEN_NOT_FOUND:
    default:
        return "Oculto: instalación no encontrada";
    }
}

static int missing_of(const Game *g)
{
    int m = 0;
    if (!g->cover)
        m |= ORGANIZER_NO_COVER;
    if (!g->video)
        m |= ORGANIZER_NO_PREVIEW;
    if (g->meta_count == 0)   /* no details.txt, or one with no rows */
        m |= ORGANIZER_NO_DETAILS;
    return m;
}

static char *missing_text(int m)
{
    static const struct { int bit; const char *word; } parts[] = {
        { ORGANIZER_NO_COVER, "portada" },
        { ORGANIZER_NO_PREVIEW, "vista previa" },
        { ORGANIZER_NO_DETAILS, "ficha" },
    };
    char buf[96] = "Falta: ";
    int first = 1;
    for (size_t i = 0; i < sizeof(parts) / sizeof(parts[0]); i++) {
        if (!(m & parts[i].bit))
            continue;
        if (!first)
            strcat(buf, ", ");
        strcat(buf, parts[i].word);
        first = 0;
    }
    return xstrdup(buf);
}

OrganizerReport *organizer_report(const Library *lib)
{
    OrganizerReport *r = xmalloc(sizeof(*r));
    memset(r, 0, sizeof(*r));
    int cap = lib->hidden_count + lib->count;
    if (cap > 0)
        r->rows = xmalloc((size_t)cap * sizeof(OrganizerRow));
    r->available_count = lib->count;

    for (int i = 0; i < lib->hidden_count; i++) {
        const HiddenEntry *h = &lib->hidden[i];
        OrganizerRow *row = &r->rows[r->count++];
        row->name = xstrdup(h->name);
        row->dir = xstrdup(h->dir);
        row->kind = ORGANIZER_HIDDEN;
        row->missing = 0;
        row->text = xstrdup(hidden_text(h->reason));
        r->hidden_count++;
    }
    for (int i = 0; i < lib->count; i++) {
        const Game *g = &lib->games[i];
        int m = missing_of(g);
        if (!m)
            continue;
        OrganizerRow *row = &r->rows[r->count++];
        row->name = xstrdup(g->name);
        row->dir = xstrdup(g->dir);
        row->kind = ORGANIZER_INCOMPLETE;
        row->missing = m;
        row->text = missing_text(m);
        r->incomplete_count++;
    }
    return r;
}

void organizer_report_free(OrganizerReport *r)
{
    if (!r)
        return;
    for (int i = 0; i < r->count; i++) {
        free(r->rows[i].name);
        free(r->rows[i].dir);
        free(r->rows[i].text);
    }
    free(r->rows);
    free(r);
}
