#if 0  // the shell will auto run any file as a script so this actually dose get interpreted as it should
    cc                        \
        -Waggregate-return    \
        -Wall                 \
        -Wcast-align          \
        -Wcast-qual           \
        -Wconversion          \
        -Wdouble-promotion    \
        -Wextra               \
        -Wfloat-equal         \
        -Wno-sign-conversion  \
        -Wpointer-arith       \
        -Wshadow              \
        -Wswitch-default      \
        -Wswitch-enum         \
        -Wundef               \
        -Wunreachable-code    \
        -Wwrite-strings       \
        -Wformat=0            \
        -pipe                 \
        -O3                   \
        "$0"                  \
    && ./a.out "$@"
    rm -f ./a.out
    exit
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>


// TODO add loglevel and printably named enums
void errorf(const char * format, ...) {
    va_list args;
    va_start (args, format);
    vfprintf (stderr, format, args);
    va_end (args);
}


int setter_string(const char* arg, const char** element, const char** error_msg) {
    *error_msg = "ok";
    *element = arg;
    return 0;
}
void printer_string(const char** element) {
    printf("%s", *element);
}

typedef struct { unsigned int year, month, day; } Date;
int parse_date(Date* date, const char* str) {
    //                      0123456789
    // check for format     YYYY-MM-DD
    for (unsigned int i = 0; i < 10; i++) {
        if (str[i] == '\0')
            return 1;
        if (i == 4 || i == 7) {
            if (str[i] != '-')
                return 2;
        } else {
            if (str[i] > '9' || str[i] < '0') {
                return 4;
            }
        }
    }
    // atoi might be overkill here this works and dose not need me to get a mutable copy of str
    date->year  = 1000*str[0] + 100*str[1] + 10*str[2] + str[3] - 1111*'0';
    date->month =                            10*str[5] + str[6] -   11*'0';
    date->day   =                            10*str[8] + str[9] -   11*'0';
    if (date->month > 12)
        return 8;
    if (date->day > 31)
        return 16;
    return 0;
}
int setter_date(const char* arg, Date* element, const char** error_msg) {
    int code = parse_date(element, arg);
    if (code & 0x18) {
        *error_msg = "Parsed date is out of range";
        return code;
    }
    if (code & 0x7) { // Failed to parse have date command try to convert it
        char cmd[256];
        char result[16];
        snprintf(cmd, sizeof(cmd), "date -d \"%s\" +%%F", arg);
        FILE *fp = popen(cmd, "r");
        if (fp == NULL) {
            *error_msg = "failed to parse date and fall back failed to open pipe";
            return 32;
        }
        code = fgets(result, sizeof(result), fp) != NULL;
        pclose(fp);
        if (!code) {
            *error_msg = "failed to parse date and fall back failed to read pipe";
            return 64;
        }
        code = parse_date(element, result);
        if(!code) {
            *error_msg = "failed to parse date and fall back failed too";
            return code << 6;
        }
    }
    *error_msg = "ok";
    return 0;
}
void printer_date(const Date* element) {
    printf("%04d-%02d-%02d", element->year, element->month, element->day);
}

int file_exists(const char* path) {
    FILE* f = fopen(path, "r");
    if (f) {
        fclose(f);
        return 1;
    }
    return 0;
}
int setter_input_path(const char* arg, const char** element, const char** error_msg) {
    if (strcmp(arg, "-") == 0) {
        *element = "/dev/stdin";
    } else if (strcmp(arg, "_infer_") == 0) {
        const char* utt = "/mnt/d/val/time/utt.utt";
        if (!file_exists(utt)) 
            *element = utt;
        else
            *element = "_no-infer-possible_";
    } else {
        *element = arg;
        if (!file_exists(*element)) {
            *error_msg = "file not found";
            return 1;
        }
    }
    *error_msg = "ok";
    return 0;
}

int setter_bool(const char* arg, int* element, const char** error_msg) {
    *error_msg = "ok";
    if (strcmp(arg, "0") == 0)
        *element = 0;
    else
        *element = 1;
    return 0;
}
void printer_bool(int* element) {
    printf(*element ? "true" : "false");
}

// ARG(        type,     name, n, "f",                                    "default",                            "description",            setter,        printer)
#define ARGS \
    ARG(        int,     help, 0, "h",                                          "0",                   "Print usage and help",       setter_bool,   printer_bool) \
    ARG(        int,  verbose, 0, "v",                                          "0",                   "Print usage and help",       setter_bool,   printer_bool) \
    ARG(const char*,    title, 1, "l",                               "cenpo report",                "The title of the report",     setter_string, printer_string) \
    ARG(const char*,     file, 1, "i",                                    "_infer_",        "The name of the file to be read", setter_input_path, printer_string) \
    ARG(       Date,    start, 1, "s",                                 "0001-01-01",                 "The first date to list",       setter_date,   printer_date) \
    ARG(       Date,      end, 1, "e",                                 "9999-12-31",                  "The last date to list",       setter_date,   printer_date) \
    ARG(const char*,   format, 1, "f", "%i, %y, %m, %d, %H, %M, %t, %hh%nm, %p, %D", "Output format string with placeholders",     setter_string, printer_string) \
    ARG(        int, noheader, 0, "v",                                          "0",                    "print a header line",       setter_bool,   printer_bool)

// TODO catually write something that accepts multiple args and than also make multi args work for fournt -1

int usage() {
    printf("cenpo (a tenpo and utt clone) [build " __DATE__ " " __TIME__ "]\n");
    #define ARG(type, name, count, flag, default, description, ...) \
        printf("  -" flag "  --" #name);                            \
        if (count != 0)                                             \
            printf("...    default:[%s]\n", default);               \
        else                                                        \
            printf("\n");                                           \
        printf("    " description "\n");
    ARGS
    #undef ARG
    return 0;
}

typedef struct {
    #define ARG(type, name, count, flag, default, description, setter, printer) type name;
    ARGS
    #undef ARG
} Setting;
Setting setting = {}; // keep the settings as global available to everything

int print_setting() {
    printf("Setting {\n");
    int maxNameLength = 0, nameLength = 0;
    #define ARG(type, name, ...)        \
        nameLength = strlen(#name);     \
        if (nameLength > maxNameLength) \
            maxNameLength = nameLength;
    ARGS
    #undef ARG
    #define ARG(type, name, count, flag, default, description, setter, printer) \
        printf("  % *s = \"", maxNameLength, #name);                            \
        printer(&setting.name);                                                  \
        printf("\"\n");
    ARGS
    #undef ARG
    printf("}\n");
    return 0;
}

int parse_args(int argc, char** argv) {
    // set the default values
    const char* error_msg = 0;
    int code = 0;
    int errors = 0;
    #define ARG(type, name, count, flag, default, description, setter, printer) \
        code = setter(default, &setting.name, &error_msg); \
        if(code) \
            errorf("Failed to set default <" default "> for argument <" #name "> error code <%d> error message <%s>\n", code, error_msg);
    ARGS
    #undef ARG
    // actually iterate though all the args
    for(int i = 1; i < argc; i++) {
        #define ARG(type, name, count, flag, default, description, setter, printer)                                               \
            if (strcmp("-" flag, argv[i]) == 0 || strcmp("--" #name, argv[i]) == 0) {                                             \
                if (count == 0) {                                                                                                 \
                    if (setter(argv[i], &setting.name, &error_msg)) {                                                            \
                        errorf("Failed to set argument <" #name "> error message <%s>\n", error_msg);                             \
                    }                                                                                                             \
                } else {                                                                                                          \
                    int start = i;                                                                                                \
                    while (i - start < count) {                                                                                   \
                        if (++i >= argc) {                                                                                        \
                            errorf("Failed to provide the expected " #count " value(s) for %s\n", argv[start]);                   \
                            errors++;                                                                                             \
                            break;                                                                                                \
                        }                                                                                                         \
                        if (setter(argv[i], &setting.name, &error_msg)) {                                                        \
                            errorf("Failed to set value <%s> for argument <" #name "> error message <%s>\n", argv[i], error_msg); \
                            errors++;                                                                                             \
                            break;                                                                                                \
                        }                                                                                                         \
                    }                                                                                                             \
                }                                                                                                                 \
                continue;                                                                                                         \
            }
        ARGS
        errorf("skipped unknown argument \"%s\"\n", argv[i]);
        #undef ARG
    }
    return errors;
}

int print_arg_table(int argc, char** argv) {
    int length = 0; // = argv |> lenth |> max_reduce
    for (int i = 0; i < argc; i++) {
        int len = (int)strlen(argv[i]);
        if (len > length)
            length = len;
    }
    printf("|  i | % *s |\n|----|-", length, "arg");
    for (int i = 0; i < length; i++)
        putchar('-'); // fill with minus signs
    puts("-|");
    for (int i = 0; i < argc; i++)
        printf("| % 2d | % *s |\n", i, length, argv[i]);
    return 0;
}


typedef struct { 
    int year, month, day, hour, minute; 
} DateTime;

typedef struct { 
    DateTime* end; 
    char* name;
    char* description; 
    int name_length;
    int description_length; 
    int minutes; 
    int record;
} Project;

int csv_project_printer(const Project* project, void** state) {
    #define MARKER_NAME_CHECKS                                             \
        MARKER_NAME_CHECK('i', "index",       "%d", project->record)       \
        MARKER_NAME_CHECK('y', "year",        "%d", project->end->year)    \
        MARKER_NAME_CHECK('m', "month",       "%d", project->end->month)   \
        MARKER_NAME_CHECK('d', "day",         "%d", project->end->day)     \
        MARKER_NAME_CHECK('H', "hour",        "%d", project->end->hour)    \
        MARKER_NAME_CHECK('M', "minute",      "%d", project->end->minute)  \
        MARKER_NAME_CHECK('t', "total",       "%d", project->minutes)      \
        MARKER_NAME_CHECK('h', "x",           "%d", project->minutes / 60) \
        MARKER_NAME_CHECK('n', "x",           "%d", project->minutes % 60) \
        MARKER_NAME_CHECK('p', "project",     "%s", project->name)         \
        MARKER_NAME_CHECK('D', "description", "%s", project->description)

    if (*state == NULL) {
        // this is the first call init the state
        *state = (void*)1; // remember that we are are done printing the header
        if (!setting.noheader) {
            for (int i = 0; setting.format[i] != '\0'; i++) {
                if (setting.format[i] == '%') {
                    #define MARKER_NAME_CHECK(marker, word, ...) \
                        if (setting.format[i+1] == marker) {     \
                            printf(word);                        \
                            i++;                                 \
                            continue;                            \
                        }
                    MARKER_NAME_CHECKS
                    #undef MARKER_NAME_CHECK
                }
                putchar(setting.format[i]);
            }
            putchar('\n');
        }
    }

    if (project == NULL) {
        // we are done do the clean up the state
        *state = NULL;
        return 0;
    }

    // do some actual work
    for (int i = 0; setting.format[i] != '\0'; i++) {
        if (setting.format[i] == '%') {
            #define MARKER_NAME_CHECK(marker, word, fmt, code) \
                if (setting.format[i+1] == marker) {           \
                    printf(fmt, code);                         \
                    i++;                                       \
                    continue;                                  \
                }
            MARKER_NAME_CHECKS
            #undef MARKER_NAME_CHECK
        }
        putchar(setting.format[i]);
    }
    putchar('\n');

    return 0;
}

int project_printer(const Project* project, void** state) {
    return csv_project_printer(project, state);
}

int print_csv_fmt(const char* path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        errorf("failed open file <%s>\n", path);
        return 1;
    }
    char * line = NULL;
    size_t capacity = 0;
    ssize_t length;
    int index = 0;
    //                     YYYY-mm-DD HH:MM 
    const char format[] = "dddd-dd-dd dd:dd "; // TODO also support "dd-dd-dd dd:dd "
    const int format_length = sizeof(format) / sizeof(format[0]) - 1;

    DateTime dta = {}, dtb = {};
    DateTime* current = &dta;
    DateTime* last = &dtb;
    DateTime* tmp_dt = NULL;

    Project project = {
        .record = 0
    };

    int minute_in_day = 0;
    int tmp_minute_in_day = 0;

    int colon_index;
    int isHello;

    void* printer_state = NULL; // NULL tells the printer that it needs to init ist state

    int error_code = 0;
    while ((length = getline(&line, &capacity, f)) != -1 && error_code == 0) {

        // ignore empty lines
        if (strncmp(line, "\n", length) == 0)
            continue;

        // check date time format
        line[length-1] = '\0';
        if (length < format_length) {
            errorf("line paring failed on line:%d <%s> because its length <%d> is too short\n", index, line, length);
            error_code = 2;
            break;
        }
        for (int i = 0; i < format_length; i++)
            if ((format[i] != 'd' && line[i] != format[i]) || (format[i] == 'd' && (line[i] < '0' || line[i] > '9'))) {
                errorf("line parsing failed on line:%d <%s> because index %d is %c instead of the expected ", index, line, i, line[i]);
                if (format[i] == 'd')
                    errorf("digit");
                else
                    errorf("'%c'", format[i]);
                errorf("\n");
                error_code = 4;
                break;
            }
        if (error_code != 0)
            break;

        // read date time
        current->year = 1000*line[0] + 100*line[1] + 10*line[2] + line[3] - 1111*'0';
        #define READ(i) 10*line[i] + line[i+1] - 11*'0';
        current->month  = READ( 5);
        current->day    = READ( 8);
        current->hour   = READ(11);
        current->minute = READ(14);

        // read project or hello
        isHello = strcmp(&line[17], "hello") == 0;
        if (isHello)
            minute_in_day = current->hour * 60 + current->minute;

        // validate DateTime
        if (project.record > 0 && !isHello && (current->year != last->year || current->month != last->month || current->day != last->day)) {
            errorf("line parsing failed on line:%d <%s> because date changed without hello\n", index, line);
            error_code = 8;
            break;
        }
        if ((project.record > 0 || !isHello) && ((current->hour * 60 + current->minute) < minute_in_day)) {
            errorf("line parsing failed on line:%d <%s> because time is non increasing\n", index, line);
            error_code = 16;
            break;
        }

        if (!isHello) {
            project.end = current;
            colon_index = 0;
            for (int i = 17; i < length; i++) 
                if (line[i] == ':') {
                    colon_index = i;
                    break;
                }
            if (colon_index == 0 || colon_index == length - 1) {
                errorf("line parsing failed on line:%d <%s> because no colon terminating the progect name was found\n", index, line);
                error_code = 32;
                break;
            }
            line[colon_index]          = '\0';
            project.name               = &line[17];
            project.name_length        = colon_index - 17;
            project.description        = &line[colon_index + (line[colon_index + 1] == ' ' ? 2 : 1)];
            project.description_length = (int)length - colon_index;
            tmp_minute_in_day          = current->hour * 60 + current->minute;
            project.minutes            = tmp_minute_in_day - minute_in_day;
            minute_in_day              = tmp_minute_in_day;
            project.record++;

            // print
            project_printer(&project, &printer_state);
        }
        if (error_code != 0)
            break;

        // swap DateTime buffers
        tmp_dt = current;
        current = last;
        last = tmp_dt;

        // increment index
        index++;
    }
    fclose(f);
    if (line)
        free(line);
    project_printer(NULL, &printer_state); // have the printer do it's own cleanup on printer_state
    return error_code;
}

int main(int argc, char** argv) {
    parse_args(argc, argv);
    if (setting.verbose) {
        printf("Compleated parsing settings\n");
        print_setting();
    }
    if (setting.help)
        return usage();
    print_csv_fmt(setting.file);
    return 0;
}
