/* DinoSaw (Tom Cooper, 1993; freeware re-release by Acorn Arcade, 2006):
 * what libroger needs to know about the game, and the BASIC parts of its
 * start-up translated by hand.
 *
 * !Run sets DinoSaw$Dir, *FX 200 3, WimpSlot 640k, then runs in turn:
 * Code.BertyCorp (BASIC: two title pictures, BertyCorp and FiveStar, each
 * faded in, held for 2.55s or a key, and faded out), Code.AdCode (an
 * absolute: one of the information screens at random, then the loading
 * screen; it returns), SCache (BASIC: SYS XOS_ScreenMode 4 - the Risc PC's
 * screen cache, an error ignored on an Archimedes; not run here),
 * Code.MCode (the game) and, when it returns, Code.EndCredits (BASIC: the
 * credits picture for 45s or a key). The BASIC programs are written out in
 * ../basic (make basic); each *Loads the same RLE decoder (Code.Crunchy,
 * Gyrinus II's, assembled there), unpacks a sprite file with it and plots
 * a sprite into both screen banks. Here they are C (berty, endcredits
 * below). MCode *RMLoads Music.PlayerMod (TrackerModule) itself. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <roger.h>

extern const struct roger_image recomp_image;          /* gen/mcode */
extern const struct roger_image recomp_adcode_image;   /* gen/adcode */
extern const struct roger_image recomp_tracker_image;  /* gen/tracker */

/* The BASIC programs' DIM'd buffers (size=&400*100: DIM sp size,decrunch
 * size): the packed sprite file and the unpacked sprite area. In BASIC
 * they're just above the program; anywhere clear of AdCode (&551C0) and
 * MCode (&89950) will do. */
#define AREA     0x10000u
#define DECRUNCH 0x2A000u

/* ---------------------------------------------------------------- BASIC
 * SYS and VDU as the BASIC programs use them */
static void sys(uint32_t n, uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3, uint32_t r4, uint32_t r5)
{
    R[0] = r0; R[1] = r1; R[2] = r2; R[3] = r3; R[4] = r4; R[5] = r5;
    roger_swi(n);
}
static void vdu(const uint8_t *s, int n) { for (int k = 0; k < n; k++) sys(0, s[k], 0, 0, 0, 0, 0); }
static void fx(uint32_t a, uint32_t x) { sys(6, a, x, 0, 0, 0, 0); }

/* MODE n: OFF */
static void mode(int m)
{
    const uint8_t s[] = {22, (uint8_t)m, 23, 1, 0, 0, 0, 0, 0, 0, 0, 0};
    vdu(s, sizeof s);
}

/* Code.Crunchy, the decoder: a byte-wise RLE. The source starts with its
 * length; a byte equal to the previous one is followed by a count of
 * further copies, and the run doesn't continue into the next byte. */
static void decode(uint32_t src, uint32_t dst)
{
    int32_t len = (int32_t)RD32(src);
    uint32_t last = 0xFFFFFFFF;
    src += 4;
    do {
        uint32_t byte = MEM[src++];
        if (byte == last) {
            int rep = MEM[src++] + 1;
            len--;
            while (--rep >= 0) MEM[dst++] = byte;
            last = 0xFFFFFFFF;
        } else {
            MEM[dst++] = byte;
            last = byte;
        }
    } while (--len > 0);
}

/* PROCpalette(brt%): the border and colours 0-7 (cols colours counted down
 * from cols-1) as greys at brightness brt% of 15. BertyCorp's loop runs
 * from 15, giving logical colours -8..-1 too (VDU 19 takes them modulo the
 * mode's colours, so 8-15) at brightnesses up to 546, sent modulo 256 as
 * VDU does. Then two vsyncs (*FX 19 twice). */
static void palette(int brt, int cols)
{
    uint8_t s[6] = {19, 0, 24, (uint8_t)(16 * brt), (uint8_t)(16 * brt), (uint8_t)(16 * brt)};
    vdu(s, 6);
    for (int col = cols - 1; col >= 0; col--) {
        int val = (int)(255.0 * col / 7 * brt) / 15;     /* 255*col%/7*brt%DIV15 */
        uint8_t c[6] = {19, (uint8_t)(7 - col), 16, (uint8_t)val, (uint8_t)val, (uint8_t)val};
        vdu(c, 6);
    }
    fx(19, 0);
    fx(19, 0);
}

/* PROCloadingscreen: load and unpack a sprite file, MODE m+128 without a
 * cursor, plot the sprite at 0,0 into both banks (OS_SpriteOp 34+256: by
 * name, user area), show bank 1, fade in, wait (wait() below), fade out */
static void screen(const char *file, const char *sprite, int m, int cols, void (*wait)(void))
{
    char cmd[256];
    snprintf(cmd, sizeof cmd, "Load <DinoSaw$Dir>.Grafix.%s %X", file, AREA);
    roger_cli(cmd);
    decode(AREA, DECRUNCH);
    mode(m);
    palette(0, cols);
    for (int bank = 1; bank <= 2; bank++) {
        fx(112, bank);
        vdu((const uint8_t[]){12}, 1);                   /* CLS */
        sys(0x2E, 290, DECRUNCH, roger_scratch(sprite), 0, 0, 0);
    }
    fx(112, 1);
    fx(113, 1);
    for (int p = 0; p <= 15; p++) palette(p, cols);
    fx(15, 0);                                           /* *FX 15: flush the input buffers */
    wait();
    for (int p = 15; p >= 0; p--) palette(p, cols);
}

/* the programs' ends: VOICES 4, *FX 15 0, both banks cleared, MODE 13+128 */
static void tidy(void)
{
    sys(0x40140, 4, 0, 0, 0, 0, 0);                      /* Sound_Configure: 4 channels */
    fx(15, 0);
    fx(112, 1);
    vdu((const uint8_t[]){12}, 1);
    fx(112, 2);
    vdu((const uint8_t[]){12}, 1);
    mode(13 + 128);
}

static uint32_t now(void) { sys(0x42, 0, 0, 0, 0, 0, 0); return R[0]; }   /* OS_ReadMonotonicTime */

/* A$=INKEY$(255): a key, or 2.55s */
static void inkey255(void) { sys(6, 129, 255, 0, 0, 0, 0); }

/* REPEAT A$=INKEY$(0) UNTIL A$<>"" OR TIME>t%, t% 45s on. Polled a
 * centisecond at a time (INKEY$(1)) to keep the display going. */
static void credits_wait(void)
{
    uint32_t t = now();
    do sys(6, 129, 1, 0, 0, 0, 0);
    while (R[2] != 0 && now() - t <= 4500);
}

/* Code.BertyCorp, up to its last steps (tidy()). MODE 12 before each
 * picture's MODE 9+128 (line 211): kept, it clears the screen first. */
static void berty(void)
{
    mode(12);
    screen("BertyCorp", "DinoIntr", 9 + 128, 16, inkey255);
    mode(12);
    screen("FiveStar", "DinoIntr", 9 + 128, 16, inkey255);
}

/* Code.EndCredits, after MCode returns */
static void endcredits(void)
{
    if (!roger_interactive()) return;
    screen("Credits", "Credits", 12 + 128, 8, credits_wait);
    tidy();
}

/* Code.AdCode: loaded at &551C0, run from its execution address, &55EB8.
 * It sets up its own stack and returns to its caller. Dispatched through
 * its own translation while it runs. */
#define ADCODE      0x551C0u
#define ADCODE_EXEC 0x55EB8u
static void adcode(void)
{
    char host[2048];
    if (!roger_hostpath("<DinoSaw$Dir>.Code.AdCode", host) || roger_load_file(host, ADCODE) <= 0)
        roger_finish("no Code.AdCode");
    roger_set_image(&recomp_adcode_image);
    roger_call_program(ADCODE_EXEC, ROGER_HANDLER_RET);
    roger_set_image(&recomp_image);
}

/* !Run's start-up, before MCode runs. Headless runs (--limit) skip the
 * pictures, but not what they leave set up. */
static void init(void)
{
    roger_cli("FX 200 3");
    if (roger_interactive()) berty();
    tidy();
    if (roger_interactive()) adcode();
}

/* --level=N: a new game starts at level N (1-11), as if the levels
 * before it had been completed. MCode's workspace is at &89450 (R12
 * throughout); +&180 is the level, which a new game sets to 1 and
 * completing one increments (after the 11th the game is won, &A1EDC).
 * &A1734 names the level's file from it, Level0.Level<A + level / 16><A +
 * level % 16>, and *Loads it: a load of level 1's (LevelAB) becomes level
 * N's, with the level word changed to match. */
#define WS    0x89450u
#define LEVEL (WS + 0x180)
static int start_level;

static int arg(const char *a)
{
    if (!strncmp(a, "--level=", 8) && atoi(a + 8) >= 1 && atoi(a + 8) <= 11) start_level = atoi(a + 8);
    else return 0;
    return 1;
}

static int game_swi(uint32_t n, int x)
{
    (void)x;
    if (start_level <= 1 || (n & ~0x20000u) != 5 || RD32(LEVEL) != 1) return 0;
    static const char load[] = "Load <DinoSaw$Dir>.Level0.LevelAB ";
    if (memcmp(MEM + R[0], load, sizeof load - 1)) return 0;
    WR32(LEVEL, start_level);
    MEM[R[0] + sizeof load - 4] = (uint8_t)('A' + start_level / 16);
    MEM[R[0] + sizeof load - 3] = (uint8_t)('A' + start_level % 16);
    return 0;                                       /* the *Load itself goes ahead */
}

static const struct roger_module modules[] = {
    /* MCode: RmEnsure TrackerModule 1.04 RmLoad <DinoSaw$Dir>.Music.PlayerMod */
    {"../game/!DinoSaw/Music/PlayerMod,fffffa48-5638f998", &recomp_tracker_image, 1},
    {NULL, NULL, 0}};

/* The headless scenario (--limit runs; instruction counts, 2000 a ms):
 * Space on the title page starts a game, then X walks right, C jumps */
static const struct roger_key keys[] = {
    {99, 100000000, 101000000},                    /* Space */
    {67, 200000000, 300000000},                    /* X */
    {83, 240000000, 241000000},                    /* C */
};

static const char *const variables[] = {
    "DinoSaw$Dir", "!DinoSaw",                     /* !Run: Set DinoSaw$Dir <Obey$Dir> */
    "CycWork$Dir", "!DinoSaw",                     /* !Boot */
    NULL};

static const struct roger_config config = {
    .title = "DinoSaw (recompiled)",
    /* a controller: Z X walk, up / A / B jump (C), start Space */
    .pad_map = "left=Z,right=X,up=C,a=C,b=C,start=Space",
    /* MCode: load address &89950, execution address &A1DFC */
    .image_path = "../game/!DinoSaw/Code/MCode,00089950-000a1dfc",
    .entry = 0xA1DFC,
    .image = &recomp_image,
    .modules = modules,
    .appdir = "../game",
    .variables = variables,
    .initial_mode = 12,                            /* the desktop's */
    .wimpslot = 640 * 1024,                        /* WimpSlot 640k */
    .keys = keys,
    .nkeys = sizeof keys / sizeof keys[0],
    .init = init,
    .exited = endcredits,
    .swi = game_swi,
    .arg = arg,
    .usage = "  --level=N         a new game starts at level N (1-11)\n",
};

int main(int argc, char **argv) { roger_main(argc, argv, &config); }
