#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>



int lerCustomer(int ***matriz, int *numClientes, int *numRecursos, int ***matrizClientesOriginal);

void processarComandos(FILE *commands, FILE *outputFile, int **matrizClientes, int *instancias, int numClientes, int numRecursos, int **matrizClientesOriginal, int **allocation, int **need, int *available);

void alocarRecursos(FILE *outputFile, int **matrizClientes, int numClientes, int numRecursos, int *request, int **matrizClientesOriginal, int **allocation, int **need, int *available);
void liberarRecursos(FILE *outputFile, int **matrizClientes, int numClientes, int numRecursos, int *release, int **allocation);
void mostrarEstado(FILE *outputFile, int numClientes, int numRecursos, int **matrizClientesOriginal, int **allocation, int **need, int *available);

int verificarAlocacaoSegura(int **allocation, int **need, int *available, int numClientes, int numRecursos);

int main(int argc, char *argv[]) {

//------ Linha de Comando ----------------------------------------------------------- 

    if (argc < 2) {
        printf("Not enough arguments\n");
        return 1;
    }

    int num_recursos = argc - 1;

    int *instancias = (int *)malloc(num_recursos * sizeof(int));

    for (int i = 1; i < argc; i++) {
        instancias[i - 1] = atoi(argv[i]);
    }

    // printf("Atribuindo instâncias:\n");
    // for (int i = 0; i < num_recursos; i++) {
    //     printf("Recurso %d: %d instâncias\n", i + 1, instancias[i]);
    // }

//------ Costumer -----------------------------------------------------------

    int **matrizClientes, **matrizClientesOriginal;
    int numClientes, numRecursos;
    
    FILE *customer = fopen("customer.txt", "r");

    if (customer == NULL) {
        printf("Fail to read customer.txt\n");
        return 1;
    }

    fseek(customer, 0, SEEK_END);
    long tamanhoArquivoCustomer = ftell(customer);

    if (tamanhoArquivoCustomer == 0) {
        printf("Fail to read customer.txt\n");
        fclose(customer);
        return 1;
    }

    if (!lerCustomer(&matrizClientes, &numClientes, &numRecursos, &matrizClientesOriginal)) {
        printf("Fail to read customer.txt\n");
        return 1; 
    }
    
    if (num_recursos != numRecursos) {
        printf("Incompatibility between customer.txt and command line\n");
        return 1;
    }

    // printf("%d\n%d\n", num_recursos, numRecursos);

    // printf("Matriz costumer:\n");
    // for (int i = 0; i < numClientes; i++) {
    //     for (int j = 0; j < numRecursos; j++) {
    //         printf("%d ", matrizClientes[i][j]);
    //     }
    //     printf("\n");
    // }

    // Até aki td certo

//------ Commands ------------------------------------------------------------------

    int **allocation = (int **)malloc(numClientes * sizeof(int *));
    for (int i = 0; i < numClientes; i++) {
        allocation[i] = (int *)malloc(numRecursos * sizeof(int));
        for (int j = 0; j < numRecursos; j++) {
            allocation[i][j] = 0;
        }
    }
 
    int **need = (int **)malloc(numClientes * sizeof(int *));
    for (int i = 0; i < numClientes; i++) {
        need[i] = (int *)malloc(numRecursos * sizeof(int));
        for (int j = 0; j < numRecursos; j++) {
            need[i][j] = 0;
        }
    }

    int *available = (int *)malloc(numRecursos * sizeof(int));
    int numRecursosCommands = 0;

    FILE *commands = fopen("commands.txt", "r");

    if (commands == NULL) {
        printf("Fail to read commands.txt\n");
        return 1;
    }

    fseek(commands, 0, SEEK_END);
    long tamanhoArquivoCommands = ftell(commands);

    if (tamanhoArquivoCommands == 0) {
        printf("Fail to read commands.txt\n");
        fclose(customer);
        return 1;
    }

    rewind(commands);

    int primeiraLinha = 1;
    char count;
    while ((count = fgetc(commands)) != '\n') {
        if (count == ' ') {
            if (primeiraLinha) {
                (numRecursosCommands)++;
            }
        }
    }

    //printf("%d", numRecursosCommands);

    rewind(commands);

    if((numRecursosCommands - 1) != num_recursos){
        printf("Incompatibility between customer.txt and command line\n");
    }

    FILE *result = fopen("result.txt", "w");
    if (result == NULL) {
        printf("Fail to create result.txt\n");
        return 1;
    }
    

    FILE *outputFile = fopen("result.txt", "w");
	if (outputFile == NULL) {
		printf("Fail to create result.txt\n");
		for (int i = 0; i < numClientes; i++) {
			free(matrizClientes[i]);
		}
		free(matrizClientes);
		return 1;
	}

    processarComandos(commands, outputFile, matrizClientes, instancias, numClientes, numRecursos, matrizClientesOriginal, allocation, need, available);
    

    fclose(commands);


    // printf("Matriz costumer:\n");
    // for (int i = 0; i < numClientes; i++) {
    //     for (int j = 0; j < numRecursos; j++) {
    //         printf("%d ", matrizClientes[i][j]);
    //     }
    //     printf("\n");
    // }

	fclose(outputFile);


//---- Retirar linha extra do results.txt -------------------------------------------------------------------------------------------------------

	FILE *resultFile = fopen("result.txt", "r+");

	if (resultFile != NULL) {

		fseek(resultFile, -1, SEEK_END);
		char lastChar;
		fread(&lastChar, 1, 1, resultFile);

		if (lastChar == '\n') {
			fseek(resultFile, -1, SEEK_END);

			ftruncate(fileno(resultFile), ftell(resultFile));
		}

		fclose(resultFile);
	}



//------ Liberar Memória -----------------------------------------------------------

    for (int i = 0; i < numClientes; i++) {
        free(matrizClientes[i]);
    }
    free(matrizClientes);
    free(instancias);
    for (int i = 0; i < numClientes; i++) {
        free(allocation[i]);
    }
    free(allocation);



    return 0;
}



int lerCustomer(int ***matriz, int *numClientes, int *numRecursos, int ***matrizClientesOriginal) {
    FILE *customer = fopen("customer.txt", "r");

    if (customer == NULL) {
        printf("Fail to read customer.txt\n");
        return 0;
    }

    fseek(customer, 0, SEEK_END);
    long tamanhoArquivoCustomer = ftell(customer);

    if (tamanhoArquivoCustomer == 0) {
        printf("Fail to read customer.txt\n");
        return 0;
    }

    rewind(customer);

    *numClientes = 0;
    *numRecursos = 1;
    char c;
    int primeiraLinha = 1; // Flag para indicar a primeira linha
    while ((c = fgetc(customer)) != EOF) {
        if (c == ',') {
            if (primeiraLinha) {
                (*numRecursos)++;
            }
        } else if (c == '\n') {
            (*numClientes)++;
            primeiraLinha = 0;
        }
    }

    if (c != '\n' && *numClientes > 0) {
        (*numClientes)++;
    }

    rewind(customer);

    *matriz = (int **)malloc(*numClientes * sizeof(int *));
    for (int i = 0; i < *numClientes; i++) {
        (*matriz)[i] = (int *)malloc(*numRecursos * sizeof(int));
    }

    for (int i = 0; i < *numClientes; i++) {
        for (int j = 0; j < *numRecursos; j++) {
            fscanf(customer, "%d,", &(*matriz)[i][j]);
        }
        fscanf(customer, "\n");
    }

    fclose(customer);

    *matrizClientesOriginal = (int **)malloc(*numClientes * sizeof(int *));
    for (int i = 0; i < *numClientes; i++) {
        (*matrizClientesOriginal)[i] = (int *)malloc(*numRecursos * sizeof(int));
        for (int j = 0; j < *numRecursos; j++) {
            (*matrizClientesOriginal)[i][j] = (*matriz)[i][j];
        }
    }

    return 1;
}



void processarComandos(FILE *commands, FILE *outputFile, int **matrizClientes, int *instancias, int numClientes, int numRecursos, int **matrizClientesOriginal, int **allocation, int **need, int *available) {


    for(int j=0; j<numRecursos; j++){
        available[j] = instancias[j];
    }


    char *comando = (char *)malloc(3 * sizeof(char));
    int *request = (int *)malloc(numRecursos * sizeof(int) + 1);
    int *release = (int *)malloc((numClientes + 1) * sizeof(int));

    while (fscanf(commands, "%s", comando) != EOF) {

        if (comando[0] == 'R' && comando[1] == 'Q') {

            fscanf(commands, "%d", &request[0]);

            for (int i = 0; i < numRecursos; i++) {
                fscanf(commands, "%d", &request[i + 1]);
            }

            alocarRecursos(outputFile, matrizClientes, numClientes, numRecursos, request, matrizClientesOriginal, allocation, need, available);        
        } 
        else if (comando[0] == 'R' && comando[1] == 'L') {

            fscanf(commands, "%d", &release[0]);

            for (int i = 0; i < numRecursos; i++) {
                fscanf(commands, "%d", &release[i + 1]);
            }

            liberarRecursos(outputFile, matrizClientes, numClientes, numRecursos, release, allocation);
        } 
        else if (comando[0] == '*') {

            mostrarEstado(outputFile, numClientes, numRecursos, matrizClientesOriginal, allocation, need, available);

        } 
        else {
            fprintf(outputFile, "Invalid command: %s\n", comando);
        }


        for (int i = 0; i < numClientes; i++) {
            for (int j = 0; j < numRecursos; j++) {
                need[i][j] = matrizClientesOriginal[i][j] - (allocation[i][j]);
            }
        }
 
        int *soma = (int *)malloc(numRecursos * sizeof(int));

        for (int i = 0; i < numRecursos; i++) {
            soma[i] = 0; 
        }

        for (int i = 0; i < numClientes; i++) {
            for (int j = 0; j < numRecursos; j++) {
                soma[j] += allocation[i][j];
            }
        }

        for (int j = 0; j < numRecursos; j++) {
            available[j] = instancias[j] - soma[j];
        }
    }

    free(comando);
    free(request);
    free(release);
}



void alocarRecursos(FILE *outputFile, int **matrizClientes, int numClientes, int numRecursos, int *request, int **matrizClientesOriginal, int **allocation, int **need, int *available) {

    int cliente = request[0];

    if (cliente < 0 || cliente >= numClientes) {
        fprintf(outputFile, "Invalid customer ID in RQ command\n");
        return;
    }

    for (int i = 0; i < numRecursos; i++) {

        if (request[i + 1] > matrizClientesOriginal[cliente][i]) {
            fprintf(outputFile, "The customer %d request", cliente);
            for (int j = 0; j < numRecursos; j++) {
                fprintf(outputFile, " %d", request[j + 1]);
            }
            fprintf(outputFile, " was denied because it exceed its maximum need\n");
            return;
        }
    }

    for (int i = 0; i < numRecursos; i++) {

        if (request[i + 1] > available[i] || request[i + 1] < 0) {

            fprintf(outputFile, "The resources");

            for (int j = 0; j < numRecursos; j++) {
                fprintf(outputFile, " %d", available[j]);
            }

            fprintf(outputFile, " are not enough to customer %d request", cliente);

            for (int j = 0; j < numRecursos; j++) {
                fprintf(outputFile, " %d", request[j + 1]);
            }
            fprintf(outputFile, "\n");
            return;
        }
    }

    if (verificarAlocacaoSegura(allocation, need, available, numClientes, numRecursos)) {

        for (int i = 0; i < numRecursos; i++) {
            matrizClientes[cliente][i] -= request[i + 1];
            allocation[cliente][i] += request[i + 1];
        }

        fprintf(outputFile, "Allocate to customer %d the resources", cliente);

        for (int i = 0; i < numRecursos; i++) {
            fprintf(outputFile, " %d", request[i + 1]);
        }
        fprintf(outputFile, "\n");
    } 
    else {

        fprintf(outputFile, "The customer %d request", cliente);

        for (int i = 0; i < numRecursos; i++) {
            fprintf(outputFile, " %d", request[i + 1]);
        }
        fprintf(outputFile, " was denied because it would result in an unsafe state\n");
        return;
    }

}




void liberarRecursos(FILE *outputFile, int **matrizClientes, int numClientes, int numRecursos, int *release, int **allocation) {
    int cliente = release[0];

    if (cliente < 0 || cliente >= numClientes) {
        fprintf(outputFile, "Invalid customer ID in RL command\n");
        return;
    }

    for (int i = 0; i < numRecursos; i++) {

        if (release[i + 1] > allocation[cliente][i]) {
            fprintf(outputFile, "The customer %d release", cliente);

            for (int j = 0; j < numRecursos; j++) {
                fprintf(outputFile, " %d", release[j + 1]);
            }
            fprintf(outputFile, " was denied because it exceed its maximum allocation\n");

            return;
        }
    }

    for (int i = 0; i < numRecursos; i++) {
        matrizClientes[cliente][i] += release[i + 1];
        allocation[cliente][i] -= release[i + 1];
    }

    fprintf(outputFile, "Release from customer %d the resources", cliente);

    for (int i = 0; i < numRecursos; i++) {
        fprintf(outputFile, " %d", release[i + 1]);
    }
    fprintf(outputFile, "\n");
}



void mostrarEstado(FILE *outputFile, int numClientes, int numRecursos, int **matrizClientesOriginal, int **allocation, int **need, int *available) {

    int colWidths[3] = {7, 13, 4}; // Largura inicial para "MAXIMUM", "ALLOCATION" e "NEED"

    // Calcula a largura máxima para cada seção
    for (int i = 0; i < numClientes; i++) {
        for (int j = 0; j < numRecursos; j++) {

            colWidths[0] = (numRecursos*2-1 > colWidths[0]) ? numRecursos*2-1 : colWidths[0];
            colWidths[1] = (numRecursos*2+2 > colWidths[1]) ? numRecursos*2+2 : colWidths[1];
        }
    }


    fprintf(outputFile, "MAXIMUM ");
    for (int i = 7; i < colWidths[0]; i++) {
        fprintf(outputFile, " ");
    }
    fprintf(outputFile, "| ALLOCATION ");
    for (int i = 13; i < colWidths[1]; i++) {
        fprintf(outputFile, " ");
    }
    fprintf(outputFile, "| NEED\n");


    for (int i = 0; i < numClientes; i++) {
        // MAXIMUM
        for (int j = 0; j < numRecursos; j++) {
            fprintf(outputFile, "%d ", matrizClientesOriginal[i][j]);
        }
        for (int j = numRecursos*2-1; j < colWidths[0]; j++) {
            fprintf(outputFile, " ");
        }
        fprintf(outputFile, "| ");

        // ALLOCATION
        for (int j = 0; j < numRecursos; j++) {
            fprintf(outputFile, "%d ", allocation[i][j]);
        }
        for (int j = numRecursos*2+2; j < colWidths[1]; j++) {
            fprintf(outputFile, " ");
        }
        fprintf(outputFile, "| ");

        // NEED
        for (int j = 0; j < numRecursos; j++) {
            fprintf(outputFile, "%d ", need[i][j]);
        }

        fprintf(outputFile, "\n");
    }

    // AVAILABLE
    fprintf(outputFile, "AVAILABLE");
    for (int j = 0; j < numRecursos; j++) {
        fprintf(outputFile, " %d", available[j]);
    }
    fprintf(outputFile, "\n");
}


int verificarAlocacaoSegura(int **allocation, int **need, int *available, int numClientes, int numRecursos) {
    int *work = (int *)calloc(numRecursos, sizeof(int)); 
    int *finish = (int *)calloc(numClientes, sizeof(int)); 

    for (int i = 0; i < numRecursos; i++) {
        work[i] = available[i];
    }

    for (int i = 0; i < numClientes; i++) {
        finish[i] = 0;
    }

    int count = 0;

    while (count < numClientes) {
        int found = 0;

        for (int i = 0; i < numClientes; i++) {
            if (finish[i] == 0) {
                int j;

                for (j = 0; j < numRecursos; j++) {
                    if (need[i][j] > work[j] || allocation[i][j] < 0) {
                        break;
                    }
                }

                if (j == numRecursos) {
                    for (int k = 0; k < numRecursos; k++) {
                        work[k] += allocation[i][k];
                    }

                    finish[i] = 1;
                    found = 1;
                    count++;
                }
            }
        }

        if (found == 0) {
            free(work);
            free(finish);
            return 0; 
        }
    }

    for (int i = 0; i < numClientes; i++) {
        for (int j = 0; j < numRecursos; j++) {
            if (allocation[i][j] > available[j]) {
                free(work);
                free(finish);
                return 0;
            }
        }
    }

    free(work);
    free(finish);
    return 1;
}
