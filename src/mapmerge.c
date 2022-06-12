
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 40000

struct info {
    int roomcount;
    int vnum;
};

struct info get_room(char *mapfile, char *srcroom);
char *opposite(char *direction);
int direction_to_dirflag(char *direction);
void copy_map(FILE *out, char *orig, int src_vnum, int dest_vnum, char *direction);
void import_map(FILE *out, char *importfile, int offset, int src_vnum, int dest_vnum, char *direction, int both);

void usage() {
    printf("usage: mapmerge source.map src_room direction import.map dest_room [both]\n");
}

/*
 * mapmerge - a standalone tool for importing the rooms from one tintin++ map file into another
 *
 * `make mapmerge` produces the executable for me
 *
 * Use case:
 *   You have a main map file (main.map) you use in your mud.
 *   You get transported to some area you don't know so you create a new map with `#map create`
 *   You explore this new area and write it to disk with `#map write new.map`
 *   Eventually you find your way back to a place where the two maps link up.
 *   If there's a room in main.map named 'North of the Two Rings' has a north exit to the room 'Small house' in new.map
 *   We can link the two maps with `mapmerge main.map 'North of the Two Rings' n new.map 'Small house' both
 *   Note: this does not use brackets around the room names like tt++ map commands.
 * 
 *   If the 'both' argument is present, a south exit in 'Small house' will be added linking to 'North of the Two Rings'
 * 
 * How it works:
 *   It reads the first map file and finds the source room and the highest vnum
 *   The highest vnum will be used to offset the vnums of the rooms and exits in the second map file
 *   Reads in the second map file and finds the target room's vnum
 *   Creates a new map file named 'merged.map'
 *   Copies the contents of the first map file, and while copying inserts an exit to the target room (target vnum + offset)
 *   Appends a copy of the rooms and exits in the second map file, offsetting the imported vnums by the offset
 *   If the 'both' argument is provided, when the target room is written, a back-exit to the source room is inserted
 * 
 * The settings and terrain from the first map file are preserved.
 * Only the rooms and exits from the second file are imported, settings and terrain are dropped 
 */
int main(int argc, char *argv[]) {
    char *srcmap = NULL;
    char *import = NULL;
    char *srcroom = NULL;
    char *direction = NULL;
    char *destroom = NULL;
    int both = 0;
    // mapmerge nanvaent.map 'North of the Two Rings' n darklands.map 'Small house' both
    //     0     1             2                        3     4             5             6
    printf("%d\n", argc);
    if (argc != 6 && argc != 7) {
        usage();
        return 1;
    }
    if (argc == 7) {
        both = 1;
    }
    srcmap = argv[1];
    srcroom = argv[2];
    direction = argv[3];
    import = argv[4];
    destroom = argv[5];

    struct info info = get_room(srcmap, srcroom);
    printf("%d rooms, srcroom = %d\n", info.roomcount, info.vnum);

    struct info importInfo = get_room(import, destroom);

    FILE *out = fopen("merged.map", "w");
    if (out) {
        copy_map(out, srcmap, info.vnum, importInfo.vnum + info.roomcount, direction);
        fprintf(out, "\n");
        import_map(out, import, info.roomcount, importInfo.vnum + info.roomcount, info.vnum, opposite(direction), both);
    }
}

// R { 1395} {0} {} {North of the Two Rings} { } {} {} {} {road} {} {1.000} {}
void get_roomname(char *line, char *name) {
    int bracketcount = 0;
    int len = strlen(line);
    int j;
    for (int i = 0; i < len; i++) {
        if (line[i] == '{') {
            bracketcount++;
            if (bracketcount == 4) {
                for (j = 0; line[++i] != '}'; j++) {
                    name[j] = line[i];
                }
                name[j] = 0;
            }
        }
    }
}

struct info get_room(char *mapfile, char *srcroom) {
    struct info info = {0};
    int max = 0;

    char buffer[BUFFER_SIZE];
    FILE *f = fopen(mapfile, "r");
    if (!f) {
        return info;
    }

    int vnum;
    char name[128];

    while (fgets(buffer, BUFFER_SIZE - 1, f)) {
        if (sscanf(buffer, "R {    %d}", &vnum) == 1) {
            if (vnum > max) {
                max = vnum;
            }
            get_roomname(buffer, name);
            if (strcmp(srcroom, name) == 0 || atoi(srcroom) == vnum) {
                info.vnum = vnum;
            }
        }
    }
    fclose(f);
    info.roomcount = max;

    return info;
}

char *opposite(char *direction) {
    char *op = NULL;
    if (strcmp(direction, "nw") == 0) {
        op = "se";
    } else if (strcmp(direction, "n") == 0) {
        op = "s";
    } else if (strcmp(direction, "ne") == 0) {
        op = "sw";
    } else if (strcmp(direction, "w") == 0) {
        op = "e";
    } else if (strcmp(direction, "e") == 0) {
        op = "w";
    } else if (strcmp(direction, "sw") == 0) {
        op = "ne";
    } else if (strcmp(direction, "s") == 0) {
        op = "n";
    } else if (strcmp(direction, "se") == 0) {
        op = "nw";
    } else if (strcmp(direction, "u") == 0) {
        op = "d";
    } else if (strcmp(direction, "d") == 0) {
        op = "u";
    }
    return op;
}

int direction_to_dirflag(char *direction) {
    int dirflag = 0;
    if (strcmp(direction, "nw") == 0) {
        dirflag = 9;
    } else if (strcmp(direction, "n") == 0) {
        dirflag = 1;
    } else if (strcmp(direction, "ne") == 0) {
        dirflag = 3;
    } else if (strcmp(direction, "w") == 0) {
        dirflag = 8;
    } else if (strcmp(direction, "e") == 0) {
        dirflag = 2;
    } else if (strcmp(direction, "sw") == 0) {
        dirflag = 12;
    } else if (strcmp(direction, "s") == 0) {
        dirflag = 4;
    } else if (strcmp(direction, "se") == 0) {
        dirflag = 6;
    } else if (strcmp(direction, "u") == 0) {
        dirflag = 16;
    } else if (strcmp(direction, "d") == 0) {
        dirflag = 32;
    }
    return dirflag;
}

void copy_map(FILE *out, char *orig, int src_vnum, int dest_vnum, char *direction) {
    FILE *in = fopen(orig, "r");
    char buffer[BUFFER_SIZE];
    int in_room = 0;

    while (fgets(buffer, BUFFER_SIZE - 1, in)) {
        int vnum;
        if (sscanf(buffer, "R {    %d}", &vnum) == 1) {
            if (vnum == src_vnum) {
                in_room = 1;
            }
        }
        if (in_room && strcmp(buffer, "\n") == 0) {
            int dirflag = direction_to_dirflag(direction);
            fprintf(out, "E {%5d} {%s} {%s} {%d} {0} {} {1.000} {} {0.00}\n", dest_vnum, direction, direction, dirflag);
            in_room = 0;
        }
        fprintf(out, "%s", buffer);
        fflush(out);
    }
}

void import_map(FILE *out, char *importfile, int offset, int src_vnum, int dest_vnum, char *direction, int both) {
    FILE *in = fopen(importfile, "r");
    char buffer[BUFFER_SIZE];
    int in_room = 0;
    int printing = 0;
    char *remainder;
    while (fgets(buffer, BUFFER_SIZE - 1, in)) {
        int vnum;
        int exit_vnum;
        remainder = buffer;
        if (sscanf(buffer, "R { %d}", &vnum) == 1) {
            vnum += offset;
            printing = 1;
            if (vnum == src_vnum) {
                in_room = 1;
            }
            fprintf(out, "R {%5d} ", vnum);
            remainder = strstr(remainder, "{") + 1;
            remainder = strstr(remainder, "{");
            fprintf(out, "%s", remainder);
        } else if (sscanf(buffer, "E { %d}", &exit_vnum) == 1) {
            exit_vnum += offset;
            fprintf(out, "E {%5d} ", exit_vnum);
            remainder = strstr(remainder, "{") + 1;
            remainder = strstr(remainder, "{");
            fprintf(out, "%s", remainder);
        } else if (printing) {
            if (in_room && both && strcmp(buffer, "\n") == 0) {
                int dirflag = direction_to_dirflag(direction);
                fprintf(out, "E {%5d} {%s} {%s} {%d} {0} {} {1.000} {} {0.00}\n", dest_vnum, direction, direction, dirflag);
                in_room = 0;
            }
            fprintf(out, "%s", buffer);
        }
        if (printing) {
            fflush(out);
        }
    }
}
