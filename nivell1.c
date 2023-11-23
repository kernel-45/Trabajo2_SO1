#include <stdio.h>
#include <stdlib.h>
#include <string.h>



#define PROMPT '$'
#define MAX_COMAND_SIZE 1024  //llargada MAX de cada comanda
#define ARGS_SIZE 64

//declaració funcions
char *read_line(char *line);
void imprimir_prompt();
int execute_line(char *line);
int parse_args(char **args, char *line);
int check_internal(char **args);
int internal_cd(char **args);
int internal_export(char **args);
int internal_source(char **args);
int internal_jobs(char **args);
int internal_fg(char **args);
int internal_bg(char **args);


//Bucle infinit principal
int main(){
    char line[MAX_COMAND_SIZE]; //Array on guardem la comanda

    while(1){
        if(read_line(line)){ //Llegim i verifiquem que estigui correcte
            execute_line(line); //executem 
        }
    }

}
// Funció que ens imprimeix el directori on estem actualment del prompt
void imprimir_prompt(){
    char *user = getenv("USER"); //Obtenim el USER
    char *home = getenv("HOME"); //Obtenim el HOME
    char *pwd = getcwd(NULL, 0); //Obtenim el directori de treball actual

    printf("%s:%s%s%s$ ", user, home, strstr(pwd, home) + strlen(home), strstr(pwd, home) + strlen(home) == pwd ? "" : "/");
    
    free(pwd);  // Alliberar memoria assignada per getcwd
    fflush(stdout); //Ntetejem el buffer de sortida
    usleep(500000);  // Espera de 0.5 segons per poder imprimir altres missatges 
}

char *read_line(char *line) {
    imprimir_prompt();
    if (fgets(line, MAX_COMAND_SIZE, stdin) != NULL) { //Llegir linea i comprovar que la lectura estigui correcta
        size_t length = strlen(line); // Calcular llargada del String
        //Comprova si l'ultim caracter es un salt de linea, si ho és el canvia per un \0
        if (line[length - 1] == '\n') {
            line[length - 1] = '\0';
        }
        return line;
        //Comprovar si l'usuari ha apretat Ctrl+D
    } else if (feof(stdin)) {
        printf("\rAdiós!\n");
        exit(0);
        //Comprovar si hi ha hagut error al llegir la linea
    } else {
        perror("Error al leer la línea");
        exit(EXIT_FAILURE);
    }
}

int execute_line(char *line) {
    char *args[ARGS_SIZE];//Array on guardarem els tokens

    if (parse_args(args, line) > 0) {//Comprovem que haguem trobat almenys 1 token
        //comprovar si es una comanda interna
        if (check_internal(args)) {
            return 1;
        }
    }

    return 0;
}

int parse_args(char **args, char *line) {
    int token_count = 0;

    // Extreure el primer token
    args[token_count] = strtok(line, "\t\n\r");
    //Bucle que extreu tots els tokens fins que arriba a null o a #
    while (args[token_count] && args[token_count][0] != '#') {
        token_count++;
        args[token_count] = strtok(NULL, " \t\n\r");
    }

    if (args[token_count]){
        //Si es troba un # deixa de trosejar pero no deixa l'ultim token com a null
        //Aqui el posem a null si l'ultim no ho es, si ja ho es no entra al if
        args[token_count] = NULL;
    }
    return token_count;
}

int check_internal(char **args) {
    if (!strcmp(args[0], "cd")) {
        return internal_cd(args);
    }
    if (!strcmp(args[0], "export")) {
        return internal_export(args);
    }
    if (!strcmp(args[0], "source")) {
        return internal_source(args);
    }
    if (!strcmp(args[0], "jobs")) {
        return internal_jobs(args);
    }
    if (!strcmp(args[0], "fg")) {
        return internal_fg(args);
    }
    if (!strcmp(args[0], "cd")) {
        return internal_bg(args);
    }
    if (!strcmp(args[0], "exit")) {
        exit(0);
    }
    return 0;
}

//FUNCIONES PROVISIONALES
int internal_cd(char **args) {
    #if nivel1 
        fprintf(stderr, GRIS_T "[internal_cd()-> Esta función cambiará el directorio]\n");
    #endif
    return 1;
}

int internal_export(char **args) {
    #if nivel1 
        fprintf(stderr, GRIS_T "[internal_export()-> Esta función asignará valores a variables de entorno]\n");
    #endif
    return 1;
}

int internal_source(char **args) {
    #if nivel1 
        fprintf(stderr, GRIS_T "[internal_source()-> Esta función ejecutará un fichero de líneas de comandos]\n");
    #endif
    return 1;
}

int internal_jobs(char **args) {
    #if nivel1 
        fprintf(stderr, GRIS_T "[internal_jobs()-> Esta función mostrará el PID de los procesos que no estén en foreground]\n");
    #endif
    return 1;
}

int internal_fg(char **args) {
    #if nivel1 
        fprintf(stderr, GRIS_T "[internal_fg()-> Esta función enviará un trabajo detenido al foreground reactivando su ejecución, o uno del background al foreground]\n");
    #endif
    return 1;
}

int internal_bg(char **args) {
    #if nivel1 
        fprintf(stderr, GRIS_T "[internal_bg()-> Esta función reactivará un proceso detenido para que siga ejecutándose pero en segundo plano]\n");
    #endif
    return 1;
}



