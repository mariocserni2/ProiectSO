#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define BUFFER_SIZE 4096

char *safe_dir = NULL;  

char* read_file_to_memory(int fd, size_t *size) {
    char *content = malloc(BUFFER_SIZE);
    if (!content) {
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    *size = 0;

    while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0) {
        content = realloc(content, *size + bytes_read + 1);
        if (!content) {
            perror("Failed to reallocate memory");
            exit(EXIT_FAILURE);
        }
        memcpy(content + *size, buffer, bytes_read);
        *size += bytes_read;
    }

    if (bytes_read < 0) {
        perror("Failed to read file");
        free(content);
        exit(EXIT_FAILURE);
    }

    content[*size] = '\0';
    return content;
}

void check_file_for_malicious_content(const char *file_path) {
    char command[1024];
    snprintf(command, sizeof(command), "./verify_for_malicious.sh %s", file_path);
    if (system(command) != 0) {
        printf("%s contains malicious words\n", file_path);
        if (safe_dir != NULL) {
            char new_path[1024];
            snprintf(new_path, sizeof(new_path), "%s/%s", safe_dir, strrchr(file_path, '/') + 1);
            if (rename(file_path, new_path) == -1) {
                perror("Error moving file to safe directory");
            } else {
                printf("Moved %s to %s\n", file_path, new_path);
            }
        }
    }
}

void list_directory(const char *directory_path, char **snapshot_content, size_t *content_size, int depth) {
    DIR *dir = opendir(directory_path);
    if (!dir) {
        perror("Error opening directory");
        return;
    }

    struct dirent *entry;
    struct stat entry_stat;
    char path[1024];
    char buffer[1024];
    char indent[256] = {0}; 

    for (int i = 0; i < depth; i++) {
        strcat(indent, "|   ");
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(path, sizeof(path), "%s/%s", directory_path, entry->d_name);

        if (stat(path, &entry_stat) == -1) {
            perror("stat");
            continue;
        }
        snprintf(buffer, sizeof(buffer), "%s|_ %s (%ld bytes)\n", indent, entry->d_name, entry_stat.st_size);
        
        size_t buf_len = strlen(buffer);
        if (*content_size + buf_len >= BUFFER_SIZE) {
            *snapshot_content = realloc(*snapshot_content, *content_size + buf_len + 1);
            if (!*snapshot_content) {
                perror("Failed to reallocate memory");
                exit(EXIT_FAILURE);
            }
        }
        strcat(*snapshot_content, buffer);
        *content_size += buf_len;

        if (S_ISDIR(entry_stat.st_mode)) {
            list_directory(path, snapshot_content, content_size, depth + 1);
        } else {
            check_file_for_malicious_content(path);
        }
    }

    closedir(dir);
}

void process_directory(const char *output_dir, const char *input_dir) {
    char snapshot_path[1024];
    snprintf(snapshot_path, sizeof(snapshot_path), "%s/%s_snapshot.txt", output_dir, input_dir);

    int fd_snapshot = open(snapshot_path, O_RDONLY);
    size_t old_content_size = 0;
    char *old_snapshot_content = fd_snapshot != -1 ? read_file_to_memory(fd_snapshot, &old_content_size) : NULL;
    if (fd_snapshot != -1) close(fd_snapshot);

    char *new_snapshot_content = calloc(1, BUFFER_SIZE * 10); 
    if (!new_snapshot_content) {
        perror("Failed to allocate memory for new snapshot");
        free(old_snapshot_content);
        return;
    }

    size_t new_content_size = 0;
    list_directory(input_dir, &new_snapshot_content, &new_content_size, 0);

    fd_snapshot = open(snapshot_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_snapshot == -1) {
        perror("Failed to open snapshot file for writing");
        free(old_snapshot_content);
        free(new_snapshot_content);
        return;
    }
    write(fd_snapshot, new_snapshot_content, strlen(new_snapshot_content));
    close(fd_snapshot);

    if (!old_snapshot_content || old_content_size != strlen(new_snapshot_content) ||
        memcmp(new_snapshot_content, old_snapshot_content, strlen(new_snapshot_content)) != 0) {
        printf("%s was modified\n", input_dir);
    }

    free(new_snapshot_content);
    free(old_snapshot_content);
}

int main(int argc, char **argv) {
    if (argc < 4 || strcmp(argv[1], "-o") != 0) {
        fprintf(stderr, "Usage: %s -o <output_dir> [-s <safe_dir>] <dir1> <dir2> ... <dirN>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *output_dir = argv[2];
    int dir_start = 3;

    if (argc > 3 && strcmp(argv[3], "-s") == 0) {
        if (argc < 6) {
            fprintf(stderr, "Usage: %s -o <output_dir> -s <safe_dir> <dir1> <dir2> ... <dirN>\n", argv[0]);
            exit(EXIT_FAILURE);
        }
        safe_dir = argv[4];
        dir_start = 5;
        mkdir(safe_dir, 0755); 
    }

    mkdir(output_dir, 0755);
    pid_t pid;

    for (int i = dir_start; i < argc; i++) {
        pid = fork();
        if (pid == 0) {
            process_directory(output_dir, argv[i]);
            exit(0);
        } else if (pid < 0) {
            perror("Failed to fork");
            exit(EXIT_FAILURE);
        }
    }

    int status;
    while ((pid = wait(&status)) > 0) {
        if (WIFEXITED(status)) {
            printf("Child with PID %ld exited with status %d.\n", (long)pid, WEXITSTATUS(status));
        }
    }

    return 0;
}

