#ifndef INCLUDE_OV01_TU056_H
#define INCLUDE_OV01_TU056_H

/* Functions of src/ov01/tu056.c that other files call, and the menu panel record that tu056 and src/ov01/tu062.c share. */

/* Request state of a menu panel (state at +0x90): menuWepSpPanelShow and Hide set a request, the step consumes it. */
enum MenuPanelState {
    MENU_PANEL_STATE_IDLE = 0,
    MENU_PANEL_STATE_SHOW_REQUEST = 1,
    MENU_PANEL_STATE_HIDE_REQUEST = 2
};

/* The menu panel record behind menuWepSpPanel, menuTargetListPanel, menuTgtStatusPanel (tu056) and menuPanelCurrent (tu062).
 * origin is the slide position, aux the (16, row) pair of a shown panel, state the request word (0 idle, 1 show, 2 hide
 * for tu056; 0 idle or counting down, 1 sliding in for tu062), flag nonzero while the panel is shown, wait, value and
 * countdown are tu062's stat and timed panel words. The 0x18 word has two views: the alias-0 store follows the origin stores. */
typedef struct MenuState {
    unsigned char unmodeled_00[4];
    void (*update)(struct MenuState *); /* +0x04: the state step, menuPanelStep, menuWepSpPanelStep or menuTargetListStep */
    void (*draw)(struct MenuState *);   /* +0x08: the draw, menuPanelDraw, menuWepSpPanelDraw or menuTargetListDraw */
    unsigned char unmodeled_0C[4];
    float origin[2];                    /* +0x10: x and y; the panel slides toward it from off-screen */
    union {
        int unmodeled_18;               /* +0x18: stored as 0 by the constructor, role not evidenced */
        unsigned int unmodeled_18Word;  /* the same word, read in its own alias set: the store follows the origin stores */
    };
    unsigned char unmodeled_1C[0x70 - 0x1C];
    float aux[2];                       /* +0x70: (16, row) on a panel shown in a slot; role not evidenced in tu056 */
    unsigned char unmodeled_78[0x18];
    union {
        int state;                      /* +0x90 */
        unsigned int stateWord;         /* the same word, read in its own alias set: the store follows the palette stores */
    };
    union {
        int flag;                       /* +0x94: nonzero while the panel is shown and drawn */
        unsigned int flagWord;          /* the same word, read in its own alias set: the palette copy stores follow it */
    };
    int wait;                           /* +0x98: ticks down while state is 0 (menuStatPanelStep) */
    int value;                          /* +0x9C: the number a stat panel prints at kind 7 (menuStatPanelDraw) */
    int countdown;                      /* +0xA0: ticks down while state is 0 (menuTimedPanelStep) */
} MenuState;

void menuWepSpPanelNew(void);
void menuTargetListNew(void);
void menuTgtStatusNew(void);
void menuWepSpPanelShow(void);
void menuWepSpPanelHide(void);
void menuTargetListShow(void);
void menuTargetListHide(void);
void menuTgtStatusHide(void);

#endif /* INCLUDE_OV01_TU056_H */
