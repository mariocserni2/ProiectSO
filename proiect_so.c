#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

void list_directory(const char *directory_path, int i) {
    DIR *dir = opendir(directory_path);

    if (!dir) {
        perror("Error opening directory");
        return;
    }

    struct dirent *entry;
    struct stat entry_stat;
    char path[1024];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        strcpy(path, directory_path); 
        strcat(path, "/");            
        strcat(path, entry->d_name); 

        if (stat(path, &entry_stat) == -1) {
            perror("stat");
            continue;
        }

        if (S_ISDIR(entry_stat.st_mode)) {
            for(int j = 0; j < i; j++)
            {
                printf("|   ");
            }
            printf("|_ %s\n", entry->d_name);
            list_directory(path,i+1);
        } else {
            for(int j = 0; j < i; j++)
              {
                  printf("|   ");
              }
            printf("|_ %s\n", entry->d_name);
        }
    }
    i = 0;
    closedir(dir);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    printf("%s\n",argv[1]);
    list_directory(argv[1],0);
    return 0;
}




#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

void write_to_snapshot(FILE *snapshot_file, const char *name, int depth, off_t size) {
    for (int i = 0; i < depth; i++) {
        fprintf(snapshot_file, "|   ");
    }
    fprintf(snapshot_file, "|_ %s - %ld\n", name, size);
}

void list_directory(const char *directory_path, FILE *snapshot_file, int depth) {
    DIR *dir = opendir(directory_path);
    if (!dir) {
        perror("Error opening directory");
        return;
    }

    struct dirent *entry;
    struct stat entry_stat;
    char path[1024];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(path, sizeof(path), "%s/%s", directory_path, entry->d_name);

        if (stat(path, &entry_stat) == -1) {
            perror("stat");
            continue;
        }

        if (S_ISDIR(entry_stat.st_mode)) {
            write_to_snapshot(snapshot_file, entry->d_name, depth, entry_stat.st_size);
            list_directory(path, snapshot_file, depth + 1);
        } else {
            write_to_snapshot(snapshot_file, entry->d_name, depth, entry_stat.st_size);
        }
    }

    closedir(dir);
}

void create_snapshot(const char *directory_path, const char *snapshot_filename) {
    FILE *snapshot_file = fopen(snapshot_filename, "w");
    if (!snapshot_file) {
        perror("Error opening snapshot file for writing");
        return;
    }

    list_directory(directory_path, snapshot_file, 0);
    fclose(snapshot_file);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *snapshot_filename = "snapshot.txt";
    create_snapshot(argv[1], snapshot_filename);
    // Aici ar trebui să urmeze logica de comparare a snapshot-urilor, dacă este necesară.
    
    return 0;
}
