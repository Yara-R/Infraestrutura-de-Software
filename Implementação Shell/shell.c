#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>


// Função para separar comandos por ; (Alocação dinâmica de memória para n comandos por linha)
int separar_comandos(char *input, char ***commands);

// Função para executar em paralelo
void style_parallel(char **commands, int command_count);
void *execute_command(void *ptr);

// Função para executar sequencial
void style_sequential(char **commands, int command_count);

// Função para executar pipe (comando | comando)
int separar_comandos_pipe(char *input, char ***commands);
void suporte_pipe(char **commands, int command_count);

// Função history
void add_to_history(char *command);

void trazer_foreground(pid_t pid);

typedef struct {
    int count;
    char **commands;
} CommandData;

int process_order = 1;
int process_number;

pid_t pid_array[BUFSIZ / 2];

char *history[BUFSIZ];
int history_index = 0;

int main(int argc, char *argv[]) {

    char input[BUFSIZ]; 
    char *args[BUFSIZ / 2 + 1];


    while (1) {

        if (argc > 2){
          
            perror("Invalid number of arguments.");
        }
        else if (argc == 2) { // Modo Batch

            FILE *batch_file = fopen(argv[1], "r");

            if (batch_file == NULL) {
                perror("Error when opening the file.");
                exit(1);
            }

            while (fgets(input, sizeof(input), batch_file) != NULL) {

                input[strcspn(input, "\n")] = '\0';
                printf("Batch> %s\n", input);

                if (strcmp(input, "exit") == 0) {

                    exit(0);
                }

                if (strcmp(input, "!!") == 0) {

                    if (history_index == 0) {

                        printf("No commands\n");
                        continue;
                    } 

                    char *last_command = history[(history_index - 1 + BUFSIZ) % BUFSIZ];
                    printf("yri seq> %s\n", last_command);
                    style_sequential(&last_command, 1);

                } 
                else if (strncmp(input, "fg", 2) == 0) {

                    int pos;
                    sscanf(input + 3, "%d", &pos);

                    if (pos > 0 && pos < process_order) {
                        trazer_foreground(pid_array[pos]);
                    } 
                    else {
                        printf("Invalid position in the pid_array.\n");
                    }

                } 
                else if (strcmp(input, "style parallel") == 0) { // Modo paralelo

                    while (fgets(input, sizeof(input), batch_file) != NULL) {

                        input[strcspn(input, "\n")] = '\0';
                        printf("Batch> %s\n", input);

                        if (strcmp(input, "exit") == 0) {
                            exit(0);
                        }  // Exit

                        input[strcspn(input, "\n")] = '\0';

                        if (strcmp(input, "style sequential") == 0) {
                            break;
                        }

                        if (strcmp(input, "!!") == 0) {
                            if (history_index == 0) {
                                printf("No commands\n");
                                continue;
                            } 

                            char *last_command = history[(history_index - 1 + BUFSIZ) % BUFSIZ];
                            printf("yri par> %s\n", last_command);
                            style_parallel(&last_command, 1);
                        } 
                        else if (strncmp(input, "fg", 2) == 0) {
                            int pos;
                            sscanf(input + 3, "%d", &pos);

                            if (pos > 0 && pos < process_order) {
                                trazer_foreground(pid_array[pos]);
                            } 
                            else {
                                printf("Invalid position in the pid_array.\n");
                            }
                        } 
                        else {

                            int eh_pipe = 0;
                            int i = 0;

                            while (input[i] != '\0') {
                                if (input[i] == '|') {
                                    eh_pipe = 1;
                                    break;
                                }
                                i++;
                            }

                            if (eh_pipe) {

                                char **pipe_commands;
                                int pipe_command_count = separar_comandos_pipe(input, &pipe_commands);
                                suporte_pipe(pipe_commands, pipe_command_count);

                                for (int j = 0; j < pipe_command_count; j++) {
                                    free(pipe_commands[j]);
                                }
                                free(pipe_commands);

                            } 
                            else {

                                char **commands;
                                int command_count = separar_comandos(input, &commands);
                                style_parallel(commands, command_count);

                                for (int i = 0; i < command_count; i++) {
                                    free(commands[i]);
                                }
                                free(commands);
                            }
                        }
                    }
                } 
                else {
                    int eh_pipe = 0;
                    int i = 0;

                    while (input[i] != '\0') {
                        if (input[i] == '|') {
                            eh_pipe = 1;
                            break;
                        }
                        i++;
                    }

                    if (eh_pipe) {

                        char **pipe_commands;
                        int pipe_command_count = separar_comandos_pipe(input, &pipe_commands);
                        suporte_pipe(pipe_commands, pipe_command_count);

                        for (int j = 0; j < pipe_command_count; j++) {
                            free(pipe_commands[j]);
                        }
                        free(pipe_commands);
                    } 
                    else {

                        char **commands;
                        int command_count = separar_comandos(input, &commands);
                        style_sequential(commands, command_count);

                        for (int i = 0; i < command_count; i++) {
                            free(commands[i]);
                        }
                        free(commands);
                    }
                }
            }

            fclose(batch_file);
            break;
        }
   
        else { // Modo interativo

            while (1) {

                printf("yri seq> ");
                fflush(stdout);

                fgets(input, sizeof(input), stdin);
                input[strcspn(input, "\n")] = '\0';

                if (strcmp(input, "exit") == 0) {
                    exit(0);
                }

                if (feof(stdin)) {  // ctrl+d
                    exit(0);
                }

                if (strcmp(input, "!!") == 0) {
                    if (history_index == 0) {
                        printf("No commands\n");
                        continue;
                    }

                    char *last_command = history[(history_index - 1 + BUFSIZ) % BUFSIZ];
                    printf("yri seq> %s\n", last_command);
                    style_sequential(&last_command, 1);

                } 
                else if (strncmp(input, "fg", 2) == 0) {

                    int pos;
                    sscanf(input + 3, "%d", &pos);

                    if (pos > 0 && pos < process_order) {
                        trazer_foreground(pid_array[pos]);
                    } 
                    else {
                        printf("Invalid position in the pid_array.\n");
                    }

                } 
                else if (strcmp(input, "style parallel") == 0) { // Modo paralelo

                    while (1) {

                        printf("yri par> ");
                        fflush(stdout);

                        fgets(input, sizeof(input), stdin);
                        input[strcspn(input, "\n")] = '\0';

                        if (strcmp(input, "exit") == 0) {
                            exit(0);
                        }  // Exit

                        if (feof(stdin)) {
                            exit(0);
                        }  // ctrl+d

                        if (strcmp(input, "style sequential") == 0) {
                            break;
                        }

                        if (strcmp(input, "!!") == 0) {
                            if (history_index == 0) {
                                printf("No commands\n");
                                continue;
                            }

                            char *last_command = history[(history_index - 1 + BUFSIZ) % BUFSIZ];
                            printf("yri seq> %s\n", last_command);
                            style_parallel(&last_command, 1);

                        } 
                        else {

                            int eh_pipe = 0;
                            int i = 0;

                            while (input[i] != '\0') {
                                if (input[i] == '|' || input[i] == ';') {
                                    eh_pipe = 1;
                                    break;
                                }
                                i++;
                            }

                            if (eh_pipe) {

                                char **commands;
                                int command_count = separar_comandos(input, &commands);
                                style_parallel(commands, command_count);

                                for (int i = 0; i < command_count; i++) {
                                    free(commands[i]);
                                }
                                free(commands);
                            } 
                            else {

                                char **commands;
                                int command_count = separar_comandos(input, &commands);
                                style_sequential(commands, command_count);

                                for (int i = 0; i < command_count; i++) {
                                    free(commands[i]);
                                }
                                free(commands);
                            }
                        }
                    }
                } 
                else {
                    // Modo sequencial para um único comando
                    char **commands;
                    int command_count = separar_comandos(input, &commands);
                    style_sequential(commands, command_count);

                    for (int i = 0; i < command_count; i++) {
                        free(commands[i]);
                    }
                    free(commands);
                }
            }
        }


    }
    
    return 0;
}


int separar_comandos(char *input, char ***commands) {

    int command_count = 0;
    char *token = strtok(input, ";");
    int buffer_size = BUFSIZ;
    *commands = malloc(sizeof(char *) * buffer_size);

    while (token != NULL) {

        (*commands)[command_count] = strdup(token);
        command_count++;
        token = strtok(NULL, ";");

        if (command_count >= buffer_size) {

            buffer_size *= 2;
            *commands = realloc(*commands, sizeof(char *) * buffer_size);
        }
    }
    

    return command_count;
}


void add_to_history(char *command) {

    free(history[history_index]);
  
    history[history_index] = strdup(command);
    history_index = (history_index + 1) % BUFSIZ;
}


void style_sequential(char **commands, int command_count) {
  
    for (int i = 0; i < command_count; i++) {
      
        char *command = commands[i];
        char *args[BUFSIZ / 2 + 1];
        char *token = strtok(command, " ");
        int arg_count = 0;
      
        int background = 0; 
      
        int input_redirection = 0;
        int output_redirection = 0;
      
        char *input_file = NULL;
        char *output_file = NULL;

        while (token != NULL) {
          
            if (strcmp(token, "&") == 0) {
              
                background = 1;
                args[arg_count - 1] = NULL;
            }
            else if (strcmp(token, "<") == 0) {
              
                input_redirection = 1;
                token = strtok(NULL, " ");

                if (token != NULL) {
                  
                    input_file = strdup(token);
                }
            } 
            else if (strcmp(token, ">") == 0) {
              
                output_redirection = 1;
                token = strtok(NULL, " ");

                if (token != NULL) {
                  
                    output_file = strdup(token);
                }
            } 
            else if (strcmp(token, ">>") == 0) {
              
                output_redirection = 2;
                token = strtok(NULL, " ");

                if (token != NULL) {
                  
                    output_file = strdup(token);
                }
            } 
            else {
              
                args[arg_count] = token;
                arg_count++;
            }

            token = strtok(NULL, " ");
        }

        if (arg_count == 0) {
            continue;
        }

        args[arg_count] = NULL;

        pid_t pid = fork();

        if (pid < 0) {
          
            fprintf(stderr, "Fork failed");
            exit(1);
        } 
        else if (pid == 0) { // processo filho
          
            if (input_redirection) {
              
                int input_fd = open(input_file, O_RDONLY);

                if (input_fd == -1) {
                  
                    perror("Error when opening the input file.");
                    exit(1);
                }

                dup2(input_fd, STDIN_FILENO);
                close(input_fd);
            }

            if (output_redirection) {
              
                if (output_redirection == 1) {
                  
                    int output_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);

                    if (output_fd == -1) {
                        perror("Erro ao criar ou abrir o arquivo de saída");
                        exit(1);
                    }

                    dup2(output_fd, STDOUT_FILENO);
                    close(output_fd);
                } 
                else if (output_redirection == 2) {
                  
                    int output_fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0666);

                    if (output_fd == -1) {
                      
                        perror("Erro ao criar ou abrir o arquivo de saída");
                        exit(1);
                    }

                    dup2(output_fd, STDOUT_FILENO);
                    close(output_fd);
                }
            }

            execvp(args[0], args);
            perror("Erro ao executar o comando");
            exit(1);
        } 
        else { // processo pai
          
            if (background == 0) {
              
                int status;
                waitpid(pid, &status, 0);
            } 
            else if (background == 1) {
              
                printf("[%d] %d\n", process_order, pid);
                pid_array[process_order] = pid;
                process_order++;
            }
        }
      
        free(input_file);
        free(output_file);
        add_to_history(args[0]);
    }
}


void style_parallel(char **commands, int command_count) {
  
    pthread_t *threads = malloc(sizeof(pthread_t) * command_count);

    if (threads == NULL) {
      
        perror("Erro ao alocar memória para threads");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < command_count; i++) {
      
        char *command = commands[i];

        add_to_history(command);

        int result = pthread_create(&threads[i], NULL, execute_command, (void *)command);

        if (result) {
          
            fprintf(stderr, "Erro - pthread_create() retornou código %d\n", result);
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < command_count; i++) {
      
        pthread_join(threads[i], NULL);
    }

    free(threads);
}


void *execute_command(void *ptr) {
    char *command = (char *)ptr;
    char *args[BUFSIZ / 2 + 1];
    char *token = strtok(command, " ");
    int arg_count = 0;

    int background = 0;

    int input_redirection = 0;
    int output_redirection = 0;

    char *input_file = NULL;
    char *output_file = NULL;

    while (token != NULL) {
      
        if (strcmp(token, "&") == 0) {
          
            background = 1;
            break;
        } 
        else if (strcmp(token, "<") == 0) {
          
            input_redirection = 1;
            token = strtok(NULL, " ");
          
            if (token != NULL) {
              
                input_file = strdup(token);
            }
        } 
        else if (strcmp(token, ">") == 0) {
          
            output_redirection = 1;
            token = strtok(NULL, " ");
          
            if (token != NULL) {
              
                output_file = strdup(token);
            }
        } 
        else if (strcmp(token, ">>") == 0) {
          
            output_redirection = 2;
            token = strtok(NULL, " ");
          
            if (token != NULL) {
                output_file = strdup(token);
            }
        } 
        else {
          
            args[arg_count] = token;
            arg_count++;
        }

        token = strtok(NULL, " ");
    }

    args[arg_count] = NULL;

    pid_t pid = fork();

    if (pid < 0) {
        fprintf(stderr, "Fork failed");
        exit(1);
    } 
    else if (pid == 0) { // processo filho
        execvp(args[0], args);
        perror("Erro ao executar o comando");
        exit(1);
    } 
    else { // processo pai
      
        if (background == 1) {
          
            printf("[%d] %d\n", process_order, getpid()); // Exibe o PID do processo em segundo plano
            pid_array[process_order] = getpid(); // Armazena o PID no array
            process_order++;
        } 
        else {
          
            int status;
            waitpid(pid, &status, 0);
        }
    }

    pthread_exit(NULL);
}


int separar_comandos_pipe(char *input, char ***commands) {
  
    int command_count = 0;
    char *token = strtok(input, "|");
    int buffer_size = BUFSIZ;
    *commands = malloc(sizeof(char *) * buffer_size);

    while (token != NULL) {

        (*commands)[command_count] = strdup(token);
        command_count++;
        token = strtok(NULL, "|");

        if (command_count >= buffer_size) {

            buffer_size *= 2;
            *commands = realloc(*commands, sizeof(char *) * buffer_size);
        }
    }

    return command_count;
}


void suporte_pipe(char **commands, int command_count) {
  
    int fd[2];
    pid_t pid;
    int prev_fd = -1;

    for (int i = 0; i < command_count; i++) {
      
        if (pipe(fd) == -1) {
          
            fprintf(stderr, "Pipe failed");
            exit(1);
        }

        pid = fork();

        if (pid < 0) {
          
            fprintf(stderr, "Fork Failed");
            exit(1);
        }

        if (pid == 0) { // Processo filho
            if (i != 0) {
              
                dup2(prev_fd, STDIN_FILENO);
                close(prev_fd);
            }

            if (i != command_count - 1) {
              
                dup2(fd[1], STDOUT_FILENO); 
                close(fd[0]);
            }

            char *command = commands[i];
            char *args[BUFSIZ / 2 + 1];
            char *token = strtok(command, " ");
            int arg_count = 0;

            while (token != NULL) {
              
                args[arg_count] = token;
                arg_count++;
                token = strtok(NULL, " ");
            }

            args[arg_count] = NULL;

            execvp(args[0], args);
          
            perror("Erro ao executar o comando");
            exit(1);
        } 
        else { // Processo pai
          
            close(fd[1]); 
          
            prev_fd = fd[0];
          
            int status;
            waitpid(pid, &status, 0);
        }
    }
}


void trazer_foreground(pid_t pid) {
  
    int status;
    waitpid(pid, &status, 0);
}
