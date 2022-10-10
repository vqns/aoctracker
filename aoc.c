#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>


#ifdef _MSC_VER
#define strcasecmp _stricmp
#else
#include <strings.h>
#endif

#define FILENAME         ".aoc"
#define YEAR(n)          (2015 + (n))
#define print_all()      (print(UINT64_MAX, UINT64_MAX, UINT32_MAX))

#define MASK(n)          (((uint64_t) 1) << ((n) % 64))
#define SET(b, n)        ((b) |= MASK(n))
#define CLEAR(b, n)      ((b) &= ~MASK(n))
#define IS_SET(b, n)     ((b) & MASK(n))
#define WORD(l, y)       ((l)->bits[y])

#define str_eq1(a, b)    (!strcasecmp((a), (b)))
#define str_eq2(a, b, c) (str_eq1(a, b) || (str_eq1(a, c)))
#define random(max)      ((int) ((double) rand() / ((double) RAND_MAX + 1) * (max)))


typedef enum {
    NOT_YET   = '.',
    STARTED   = 'S',
    COMPLETED = 'C'
} state;


typedef struct {
    char *name;
    uint64_t *bits;
} lang;


lang *langs = NULL;
size_t langs_sz = 0;
size_t alloc_sz = 0;
uint8_t max_year = 0;
int edit_mode = 0;


/* utils */

int index_of(const char *s) {
    int i = 0, n = -1;
    for (; i < langs_sz && (n = strcasecmp(langs[i].name, s)) < 0; ++i);
    return n ? -1 : i;
}


int estimate_index_of(const char *s) {
    int i = 0, n = -1;
    for (; i < langs_sz && (n = strcasecmp(langs[i].name, s)) < 0; ++i);
    return n ? i : -1;
}


lang *for_name(const char *s) {
    int i = index_of(s);
    return i < 0 ? NULL : &langs[i];
}


void free_lang(lang *l) {
    if (l->name) {
        free(l->name);
        l->name = NULL;
    }
    if (l->bits) {
        free(l->bits);
        l->bits = NULL;
    }
}


void clean_all() {
    max_year = 0;
    langs_sz = 0;
    alloc_sz = 0;
    if (!langs)
        return;
    for (size_t i = 0; i < langs_sz; ++i)
        free_lang(&langs[i]);
    free_lang(langs);
    langs = NULL;
}


int resize_langs(size_t new_sz) {
    if (!new_sz)
        return 0;

    if (new_sz == langs_sz)
        return 1;

    if (new_sz < langs_sz) {
        for (size_t i = langs_sz; i < new_sz; ++i)
            free_lang(&langs[i]);
        langs_sz = new_sz;
        return 1;
    }

    if (new_sz > alloc_sz) {
        lang *ptr = realloc(langs, new_sz * sizeof(lang));
        if (!ptr) {
            puts("Allocation error: OOM.");
            return 0;
        }
        alloc_sz = new_sz;
        langs = ptr;
    }

    for (size_t i = langs_sz; i < new_sz; ++i) {
        langs[i] = (lang) { NULL, NULL };
        if (max_year) {
            langs[i].bits = calloc(max_year, sizeof(uint64_t));
            if (langs[i].bits)
                continue;
            puts("Allocation error: OOM.");
            for (size_t j = langs_sz; j < i; ++j)
                free(langs[j].bits);
            return 0;
        }
    }

    return 1;
}


/* data */

state get_state(const lang *lang, int year, int day) {
    uint64_t word = WORD(lang, year);
    day <<= 1;
    if (IS_SET(word, day))
        return COMPLETED;
    else if (IS_SET(word, day + 1))
        return STARTED;
    else
        return NOT_YET;
}


void set_state(const lang *lang, int year, int day, state state) {
    uint64_t *word = &WORD(lang, year);
    int i = day << 1;
    if (state == COMPLETED) {
        SET(*word, i);
        CLEAR(*word, i + 1);
        printf("Completed %d %02d %s.\n", YEAR(year), day, lang->name);
    } else if (state == STARTED) {
        SET(*word, i + 1);
        CLEAR(*word, i);
        printf("Started %d %02d %s.\n", YEAR(year), day, lang->name);
    } else {
        CLEAR(*word, i);
        CLEAR(*word, i + 1);
        printf("Cleared %d %02d %s.\n", YEAR(year), day, lang->name);
    }
}


int new_year() {
    if (max_year == UINT8_MAX) {
        printf(
            "Error adding new year: maximum year is %u (%u).\n",
            UINT8_MAX, YEAR(UINT8_MAX)
        );
        return 0;
    }

    for (int i = 0; i < langs_sz; ++i) {
        uint64_t *ptr = realloc(langs[i].bits, (max_year + 1) * sizeof(uint64_t));
        if (!ptr) {
            puts("Error adding new year: OOM.");
            return 0;
        }
        langs[i].bits = ptr;
        langs[i].bits[max_year] = 0;
    }

    printf("Added year %u (%u).\n", max_year, YEAR(max_year));
    max_year++;

    return 1;
}


int del_year() {
    if (!max_year) {
        puts("Error removing year: cannot remove year 0 (2015).");
        return 0;
    }
    max_year--;
    for (int i = 0; i < langs_sz; ++i)
        langs[i].bits[max_year] = 0;
    printf("Removed year %u (%u).\n", max_year, YEAR(max_year));

    return 1;
}


lang *add_lang(const char *name) {
    if (langs_sz == INT32_MAX) {
        printf("Error adding lang: maximum %d languages allowed\n.", INT32_MAX);
        return 0;
    }

    if (strlen(name) > UINT8_MAX) {
        printf(
            "Error adding lang: '%s' length is too long (maximum %d).\n",
            name, UINT8_MAX
        );
        return NULL;
    }

    int n = index_of(name);
    if (n >= 0) {
        printf("Error adding lang: %s already exists.\n", langs[n].name);
        return NULL;
    }

    if (!resize_langs(langs_sz + 1))
        return 0;

    n = estimate_index_of(name);

    /* name is stack allocated */
    char *z = malloc(strlen(name) + 1);
    if (!z) {
        puts("Allocation error: OOM.");
        return 0;
    }
    strcpy(z, name);

    lang l = { z, langs[langs_sz].bits };
    if (n != langs_sz)
        memmove(&langs[n + 1], &langs[n], (langs_sz - n) * sizeof(lang));
    langs[n] = l;
    langs_sz++;

    return &langs[n];
}


int rename_lang(const char *oldn, const char *newn) {
    if (!strcmp(oldn, newn)) {
        printf(
            "Error renaming lang: newn name is identical to the oldn one ('%s')\n",
            oldn
        );
        return 0;
    }

    int oldi = index_of(oldn);
    if (oldi < 0) {
        printf("Error renaming lang: '%s' does not exist.\n", oldn);
        return 0;
    }

    if (!strcasecmp(oldn, newn)) {
        char *z = malloc(strlen(newn) + 1);
        if (!z) {
            puts("Allocation error: OOM.");
            return 0;
        }
        strcpy(z, newn);
        printf("Renamed lang '%s' to '%s'.\n", langs[oldi].name, newn);
        free(langs[oldi].name);
        langs[oldi].name = z;
        return 1;
    }

    int newi = estimate_index_of(newn);
    if (newi < 0) {
        newi = index_of(newn);
        printf("Error renaming lang: '%s' already exists.\n", langs[newi].name);
        return 0;
    }

    if (newi > oldi)
        newi--;

    if (oldi != newi) {
        lang l = langs[oldi];
        memmove(&langs[oldi], &langs[newi], abs(oldi - newi) * sizeof(lang));
        langs[newi] = l;
    }

    char *z = malloc(strlen(newn) + 1);
    if (!z) {
        puts("Allocation error: OOM.");
        return 0;
    }
    strcpy(z, newn);
    printf("Renamed lang '%s' to '%s'.\n", langs[newi].name, newn);
    free(langs[newi].name);
    langs[newi].name = z;

    return 1;
}


int remove_lang(const char *name) {
    int i = index_of(name);
    if (i < 0) {
        printf("Error removing lang: '%s' does not exist.\n", name);
        return 0;
    }

    free_lang(&langs[i]);
    langs_sz--;
    if (i < langs_sz)
        memmove(&langs[i], &langs[i + 1], (langs_sz - i) * sizeof(lang));
    langs[langs_sz] = (lang) { NULL, NULL };
    printf("Removed lang '%s'.\n", name);

    return 1;
}


/* (de)serialization */

int deserialize() {
    if (edit_mode)
        return 1;

    FILE *fp = fopen(FILENAME, "rb+");
    if (!fp) {
        printf("Error reading data: '%s' could not be opened.\n", FILENAME);
        return 0;
    }

    fseek(fp, 0, SEEK_END);
    size_t sz = ftell(fp);
    if (!sz)
        goto invalid_file;
    rewind(fp);

    max_year = fgetc(fp);
    if (max_year) {
        size_t est_sz = sz / (max_year * 8);
        if (est_sz && !resize_langs(est_sz)) {
            fclose(fp);
            return 0;
        }
    }

    char name[UINT8_MAX];
    memset(name, 0, sizeof(name));

    while (sz - ftell(fp) > 0) {
        uint8_t name_sz = fgetc(fp);
        if (!name_sz || sz - ftell(fp) < name_sz + max_year * sizeof(int64_t))
            goto invalid_file;

        fread(name, 1, name_sz, fp);
        name[name_sz] = '\0';

        lang *l = add_lang(name);
        if (!l)
            goto invalid_file;

        if (max_year)
            fread(l->bits, sizeof(int64_t), max_year, fp);
    }

    fclose(fp);
    return 1;

    invalid_file:
    printf("Error reading data: '%s' contains invalid data.\n", FILENAME);
    fclose(fp);
    return 0;
}


int serialize() {
    if (edit_mode)
        return 1;

    FILE *fp = fopen(FILENAME, "wb");
    if (!fp) {
        printf("Error saving data: '%s' could not be opened.\n", FILENAME);
        return 0;
    }

    fputc(max_year, fp);

    for (int i = 0; i < langs_sz; ++i) {
        uint8_t sz = strlen(langs[i].name);
        if (!fputc(sz, fp)
            || !fwrite(langs[i].name, 1, sz, fp)
            || !fwrite(langs[i].bits, 1, max_year * sizeof(uint64_t), fp))
            goto failure;
    }

    fclose(fp);
    printf("Saved data to '%s'.\n", FILENAME);
    return 1;

    failure:
    printf("Error saving data: could not write to '%s'.\n", FILENAME);
    fclose(fp);
    return 0;
}


/* print */

void print(uint64_t fl, uint64_t fy, uint32_t fd) {
    int max_length = 4;

    for (int i = 0; i < langs_sz; ++i) {
        if (!IS_SET(fl, i))
            continue;
        int n = strlen(langs[i].name);
        max_length = (n > max_length) ? n : max_length;
    }

    int line_length = max_length + 2;
    for (int i = 0; i < 25; ++i)
        if (IS_SET(fd, i))
            line_length += 3;

    for (int z = 0; z < langs_sz; ++z) {
        if (!IS_SET( fl, z))
            continue;

        printf("%*s |", max_length, langs[z].name);
        for (int day = 0; day < 25; ++day)
            if (IS_SET(fd, day))
                printf(" %2d", day + 1);
        putchar('\n');
        for (int i = 0; i < line_length; ++i)
            putchar('_');
        putchar('\n');

        for (int year = 0; year < max_year; ++year) {
            if (!IS_SET(fy, year))
                continue;
            printf("%*d |", max_length, YEAR(year));
            for (int day = 0; day < 25; ++day)
                if (IS_SET(fd, day))
                    printf("  %c", get_state(&langs[z], year, day));
            putchar('\n');
        }
        putchar('\n');
    }
}


/* commands */

void dispatch_cmd(int argc, char **argv);


int parse_filters(int argc, char **argv,
                  uint64_t *tl, uint64_t *ty, uint32_t *td, uint8_t *ts) {
    for (int i = 0; i < argc;) {
        if (str_eq2(argv[i], "-d", "--days")) {
            if (*td == UINT32_MAX)
                *td = 0;
            i++;
            for (; i < argc && strncmp(argv[i], "-", 1); i++) {
                int d = strtol(argv[i], NULL, 10) - 1;
                if (d > -1 && d < 25)
                    SET(*td, d);
            }
        } else if (str_eq2(argv[i], "-l", "--langs")) {
            if (*tl == UINT64_MAX)
                *tl = 0;
            i++;
            for (; i < argc && strncmp(argv[i], "-", 1); i++) {
                int n = index_of(argv[i]);
                if (n >= 0)
                    SET(*tl, n);
            }
        } else if (str_eq2(argv[i], "-y", "--years")) {
            if (*ty == UINT64_MAX)
                *ty = 0;
            i++;
            for (; i < argc && strncmp(argv[i], "-", 1); i++) {
                int y = strtol(argv[i], NULL, 10);
                if (y > 2000)
                    y -= 2000;
                y -= 15;
                if (y > -1 && y < max_year)
                    SET(*ty, y);
            }
        } else if (str_eq2(argv[i], "-s", "--states")) {
            if (*ts == UINT8_MAX)
                *ts = 0;
            i++;
            for (; i < argc && strncmp(argv[i], "-", 1); i++) {
                if (str_eq1(argv[i], "not_yet"))
                    SET(*ts, 0);
                else if (str_eq1(argv[i], "started"))
                    SET(*ts, 1);
                else if (str_eq1(argv[i], "completed"))
                    SET(*ts, 2);
            }
        } else {
            printf(
                "Unknown argument: '%s' (expected '-d', '-y' or '-l').\n",
                argv[i]
            );
            return 0;
        }
    }

    return 1;
}


void cmd_help() {
    static const char *help =
        "Usage:\n"
        "\nadd l1 l2 ... lN\n"
        "   Add languages 'l1' to 'ln'. Case is respected.\n"
        "   (Note: a language name may be at most 255 characters long.)\n"
        "\nclear YYYY DD L\n"
        "   Remove data for the given day (i.e mark it as 'non started').\n"
        "   * `YYYY` being the year (both `YYYY` and `YY` are accepted).\n"
        "   * `DD` being the day (ranges from '1' to '25').\n"
        "   * `L` being the language name (case insensitive).\n"
        "\ncomplete YYYY DD L\n"
        "   Mark the given day as completed.\n"
        "   * `YYYY` being the year (both `YYYY` and `YY` are accepted).\n"
        "   * `DD` being the day (ranges from '1' to '25').\n"
        "   * `L` being the language name (case insensitive).\n"
        "\nedit\n"
        "   Enter edit mode, which enables the following additional commands:\n"
        "   (Note: commands entered in edit may be at most 255 characters long.)\n"
        "   * exit\n"
        "     Exit edit mode without saving.\n"
        "   * reload\n"
        "     Reload data from the file without saving.\n"
        "   * save\n"
        "     Save data to the file.\n"
        "\nfile\n"
        "   Show the data file name.\n"
        "\nget\n"
        "   Get a random 'year, day, language' combination. Available filters:\n"
        "   * -d, --days d1 d2 ... dN\n"
        "     Restrict to the days 'd1', 'd2', etc. Ranges from '1' to '25'.\n"
        "   * -l, --langs l1 l2 ... lN\n"
        "     Restrict to the languages 'l1', 'l2', etc. Case is ignored.\n"
        "   * -y, --years y1 y2 ... yN\n"
        "     Restrict to the years 'y1', 'y2', etc. Ranges from '15' to max year.\n"
        "     Both `YY` and `YYYY` are accepted. (e.g '17' and '2017')\n"
        "   * -s, --states s1 s2 ... sN\n"
        "     Restrict to the states 's1', 's2', etc. Correct values: 'not_yet',\n"
        "     'started', 'completed' (case is ignored). If unspecified, defaults to\n"
        "     'not_yet'.\n"
        "\nh, help, -h, --help\n"
        "   Show this message.\n"
        "\ninit\n"
        "   Create and initialize the data file.\n"
        "\nrandom\n"
        "   Alias for get.\n"
        "\nrename old new\n"
        "   Rename language 'old' to 'new'. Case is ignored for 'old'.\n"
        "\nrm l1 l2 ... lN\n"
        "   Remove languages 'l1' to 'ln'. Case is ignored.\n"
        "\nshow\n"
        "   Show tracked progress. Available filters:\n"
        "   * -d, --days d1 d2 ... dN\n"
        "     Only show the days 'd1', 'd2', etc. Ranges from '1' to '25'.\n"
        "   * -l, --langs l1 l2 ... lN\n"
        "     Only show the languages 'l1', 'l2', etc. Case is ignored.\n"
        "   * -y, --years y1 y2 ... yN\n"
        "     Only show the years 'y1', 'y2', etc. Ranges from '15' to max year.\n"
        "     Both `YY` and `YYYY` are accepted. (e.g '17' and '2017')\n"
        "\nstart YYYY DD L\n"
        "   Mark the given day as started.\n"
        "   * `YYYY` being the year (both `YYYY` and `YY` are accepted).\n"
        "   * `DD` being the day (ranges from '1' to '25').\n"
        "   * `L` being the language name (case insensitive).\n"
        "\nyear\n"
        "   Year related operations. The following options are exclusive.\n"
        "   * add\n"
        "     Add a new year.\n"
        "   * rm\n"
        "     Remove the last year.\n"
    ;
    puts(help);
}


void cmd_edit() {
    if (edit_mode)
        return;

    if (!deserialize())
        return;
    srand(time(NULL));
    edit_mode = 1;

    puts("Enabled edit mode.");
    char cmd[UINT8_MAX];
    char *argv[128];
    memset(argv, 0, sizeof(argv));

    while (edit_mode) {
        memset(cmd, 0, sizeof(cmd));
        scanf("%[^\n]", cmd);
        /* extra \n */
        getchar();

        int argc = 0;
        for (int i = 0; i < UINT8_MAX && cmd[i] != '\0'; ++i) {
            if (cmd[i] == ' ') {
                cmd[i] = '\0';
                continue;
            }

            argc++;
            if (cmd[i] == '"' || cmd[i] == '\'') {
                char c = cmd[i];
                cmd[i] = '\0';
                int ok = 1;
                while (ok) {
                    i++;
                    if (cmd[i] == '\\')
                        i += 2;

                    ok = i < UINT8_MAX && cmd[i] != '\0' && cmd[i] != c;
                }
                if (cmd[i] == c)
                    cmd[i] = '\0';
            } else
                while (i < 255 && cmd[i + 1] != ' ')
                    i++;
        }

        int c = 0;
        for (int i = 0; i < argc; ++i) {
            for (; c < UINT8_MAX && cmd[c] == '\0'; ++c);
            argv[i] = &cmd[c];
            for (; c < UINT8_MAX && cmd[c] != '\0'; ++c);
        }

        dispatch_cmd(argc, argv);
        for (int i = 0; i < argc; ++i)
            argv[i] = NULL;
    }
}


static inline void cmd_exit() {
    edit_mode = 0;
    puts("Exited edit mode.");
}


static inline void cmd_file() {
    puts(FILENAME);
}


void cmd_get(int argc, char **argv) {
    if (!deserialize())
        return;

    uint64_t tl = UINT64_MAX;
    uint64_t ty = UINT64_MAX;
    uint32_t td = UINT32_MAX;
    uint8_t  ts = UINT8_MAX;

    if (argc) {
        if (!parse_filters(argc, argv, &tl, &ty, &td, &ts))
            return;

        if (!tl || !ty || !td || !ts) {
            puts("No match found.");
            return;
        }
    } else {
        ts = 0;
        SET(ts, 0);
    }

    if (!edit_mode)
        srand(time((NULL)));

    int n = 0;
    int nl = -1, ny = -1, nd = -1;
    for (int l = 0; l < langs_sz; ++l) {
        if (!IS_SET(tl, l))
            continue;
        for (int y = 0; y < max_year; ++y) {
            if (!IS_SET(ty, y))
                continue;
            for (int d = 0; d < 25; ++d) {
                if (!IS_SET(td, d))
                    continue;
                state s = get_state(&langs[l], y, d);
                if ((s == NOT_YET   && IS_SET(ts, 0))
                    || (s == STARTED   && IS_SET(ts, 1))
                    || (s == COMPLETED && IS_SET(ts, 2))) {
                    n++;
                    if (!random(n)) {
                        nl = l; ny = y; nd = d;
                    }
                }
            }
        }
    }

    if (nl >= 0)
        printf("%d %02d %s\n", YEAR(ny), nd + 1, langs[nl].name);
    else
        puts("No match found.");
}


void cmd_init() {
    FILE *fp = fopen(FILENAME, "r");
    if (fp) {
        fclose(fp);
        printf("'%s' already exists.\nOverwrite? [y/N]\n", FILENAME);
        char c;
        scanf(" %c", &c);
        if (c != 'y' && c != 'Y')
            return;
    }

    fp = fopen(FILENAME, "wb");
    if (!fp) {
        printf("Error saving data: '%s' could not be opened.\n", FILENAME);
        return;
    }
    fputc(1, fp);
    fclose(fp);
    printf("Initialized '%s'.\n", FILENAME);
}


/* 1 = add, 2 = rm, 3 = rename */
void cmd_lang(int argc, char **argv, int cmd) {
    if (cmd == 1 || cmd == 2) {
        if (argc == 0) {
            printf(
                "Incorrect argument count: %d (expected at least 1).\n", argc
            );
            return;
        }
        if (!deserialize())
            return;
        cmd = cmd == 1;
        int mod = 0;
        for (int i = 0; i < argc; ++i)
            mod = cmd ? add_lang(argv[i]) != NULL : remove_lang(argv[i]) || mod;
        if (mod)
            serialize();
    } else if (cmd == 3) {
        if (argc != 2) {
            printf("Incorrect argument count: %d (expected 2).\n", argc - 1);
            return;
        }
        if (deserialize() && rename_lang(argv[0], argv[1]))
            serialize();
    }
}


static inline void cmd_reload() {
    clean_all();
    edit_mode = 0;
    if (deserialize())
        puts("Reloaded data.");
    edit_mode = 1;
}


static inline void cmd_save() {
    edit_mode = 0;
    serialize();
    edit_mode = 1;
}


void cmd_set(int argc, char **argv, state s) {
    if (argc != 3) {
        printf("Incorrect argument count: %d (expected 3).\n", argc);
        return;
    }

    deserialize();

    int y = strtol(argv[0], NULL, 10);
    if (y > 2000)
        y -= 2000;
    y -= 15;
    if (y < 0 || y >= max_year) {
        printf(
            "Incorrect year value: '%d' (expected '%d' to '%d').\n",
            YEAR(y), YEAR(0), YEAR(max_year - 1)
        );
        return;
    }

    int d = strtol(argv[1], NULL, 10);
    if (d <= 0 || d >= 26) {
        printf("Incorrect day value: '%d' (expected '1' to '25').\n", d);
        return;
    }
    d--;

    lang *l = for_name(argv[2]);
    if (!l) {
        printf("Incorrect lang name: '%s' does not exist.\n", argv[2]);
        return;
    }

    set_state(l, y, d, s);
    serialize();
}


void cmd_show(int argc, char **argv) {
    if (!deserialize())
        return;

    if (!argc) {
        print_all();
        return;
    }

    uint64_t tl = UINT64_MAX;
    uint64_t ty = UINT64_MAX;
    uint32_t td = UINT32_MAX;
    uint8_t ignored = UINT8_MAX;

    if (!parse_filters(argc, argv, &tl, &tl, &td, &ignored))
        return;

    print(tl, ty, td);
}


void cmd_year(int argc, char **argv) {
    if (argc != 1) {
        printf("Incorrect argument count: %d (expected 1).\n", argc);
        return;
    }

    if (str_eq1(argv[0], "add")) {
        if (deserialize() && new_year())
            serialize();
    } else if (str_eq1(argv[0], "rm")) {
        if (deserialize() && del_year())
            serialize();
    } else
        printf("Unkown argument: '%s' (expected 'add' or 'rm').\n", argv[0]);
}


void dispatch_cmd(int argc, char **argv) {
    if (argc == 0
        || str_eq2(argv[0], "h", "help")
        || str_eq2(argv[0], "-h", "--help"))
        cmd_help();
    else if (str_eq1(argv[0], "add"))
        cmd_lang(argc - 1, &argv[1], 1);
    else if (str_eq1(argv[0], "clear"))
        cmd_set(argc - 1, &argv[1], NOT_YET);
    else if (str_eq1(argv[0], "complete"))
        cmd_set(argc - 1, &argv[1], COMPLETED);
    else if (!edit_mode && str_eq1(argv[0], "edit"))
        cmd_edit();
    else if (edit_mode && str_eq1(argv[0], "exit"))
        cmd_exit();
    else if (str_eq1(argv[0], "file"))
        cmd_file();
    else if (str_eq2(argv[0], "get", "random"))
        cmd_get(argc - 1, &argv[1]);
    else if (str_eq1(argv[0], "init"))
        cmd_init();
    else if (edit_mode && str_eq1(argv[0], "reload"))
        cmd_reload();
    else if (str_eq1(argv[0], "rename"))
        cmd_lang(argc - 1, &argv[1], 3);
    else if (str_eq1(argv[0], "rm"))
        cmd_lang(argc - 1, &argv[1], 2);
    else if (edit_mode && str_eq1(argv[0], "save"))
        cmd_save();
    else if (str_eq1(argv[0], "start"))
        cmd_set(argc - 1, &argv[1], STARTED);
    else if (str_eq1(argv[0], "show"))
        cmd_show(argc - 1, &argv[1]);
    else if (str_eq1(argv[0], "year"))
        cmd_year(argc - 1, &argv[1]);
    else
        printf("Unknown argument: %s. See 'help'.\n", argv[0]);
}


int main(int argc, char **argv) {
    dispatch_cmd(argc - 1, &argv[1]);
    clean_all();
    return EXIT_SUCCESS;
}