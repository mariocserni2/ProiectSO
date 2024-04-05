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
