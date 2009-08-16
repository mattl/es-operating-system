/* es  sqConfig.h -- platform identification and configuration */

#define printf      esReport
#undef  putchar
#define putchar(c)  (esReport("%c", (c)))

/* x86 systems */
#define DOUBLE_WORD_ALIGNMENT
#define DOUBLE_WORD_ORDER

#define SQ_CONFIG_DONE
