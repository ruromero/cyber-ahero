#ifndef APP_H
#define APP_H

struct LaptopConfig;

/* app_run's result when the Quit button ended it (ticket 05). */
#define APP_QUIT 2

/* kiosk: Kiosk mode (keyboard blocked while the menu shows, Esc and F5 do
 * not quit or rescan). Returns 0, 1 on start-up failure, or APP_QUIT. */
int app_run(const char *library_root, const struct LaptopConfig *cfg, int kiosk);

#endif
