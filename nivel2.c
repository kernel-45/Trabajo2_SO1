#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

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

//Definició colors
#define RESET "\033[0m"     //Posar els colors per defecte
#define NEGRO_T "\x1b[30m"  //Text Negre
#define NEGRO_F "\x1b[40m"  //Fons negre
#define GRIS_T "\x1b[90m"   //La resta canvia el color del text per l'especificat per el nom
#define ROJO_T "\x1b[31m"
#define VERDE_T "\x1b[32m"
#define AMARILLO_T "\x1b[33m"
#define AZUL_T "\x1b[34m"
#define MAGENTA_T "\x1b[35m"
#define CYAN_T "\x1b[36m"
#define BLANCO_T "\x1b[97m"
#define NEGRITA "\x1b[1m"

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

    printf(CYAN_T "%s", user);
    printf(RESET ":");
    printf(AMARILLO_T "%s%s%s", home, strstr(pwd, home) + strlen(home), strstr(pwd, home) + strlen(home) == pwd ? "" : "/");
    printf(RESET "$ ");

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
        //HACER ALGO PARA QUE DIGA ADIÓS AL FINAL
        printf("\r\nAdiós!\n");
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
    args[token_count] = strtok(line, " \t\n\r");
    fprintf(stderr, GRIS_T "[parse_args()-> token %i: %s]\n" RESET, token_count, args[token_count]);
    //Bucle que extreu tots els tokens fins que arriba a null o a #
    while (args[token_count] && (args[token_count][0] != '#')) {
        token_count++;
        args[token_count] = strtok(NULL, " \t\n\r");
        fprintf(stderr, GRIS_T "[parse_args()-> token %i: %s]\n" RESET, token_count, args[token_count]);
    }

    if (args[token_count]){
        //Si es troba un # deixa de trosejar pero no deixa l'ultim token com a null
        //Aqui el posem a null si l'ultim no ho es, si ja ho es no entra al if
        args[token_count] = NULL;
        fprintf(stderr, GRIS_T "[parse_args()-> token %i corretgit: %s]\n" RESET, token_count, args[token_count]);
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
    if (!strcmp(args[0], "bg")) {
        return internal_bg(args);
    }
    if (!strcmp(args[0], "exit")) {
        printf("\rAdiós!\n");
        exit(0);
    }
    return 0;
}

//FUNCIONES PROVISIONALES
int internal_cd(char **args) {
    
    char *path[PATH_MAX];

    //Posem args[1], ja que a args[0] tenim el cd
    if (args[1]==NULL){ //Si posen cd sense cap argument més, va al home
        chdir(getenv("HOME"));
    } else if ((args[1]!=NULL)&&(args[2]==NULL)){ //Si tiene un argumento en args[1] y no tiene argumentos en args[2]
        if (chdir(args[1]) == -1) {
            perror("chdir() error");
        } else{
            chdir(args[1]);
        }
    } else if (args[2]!=NULL) {//Si tiene más de un argumento (cd avanzado)
        char buffer[1024];

        buffer[0] = '\0';  // Inicializar buffer como una cadena vacía

        int i = 1;
        while (args[i] != NULL) {
            if ((strchr(args[i], '\'') == NULL) && (strchr(args[i], '\"') == NULL) && (strchr(args[i], '\\') == NULL)) {
                // Si no tiene caracteres especiales copiamos el token en buffer
                strcat(buffer, args[i]);
            } else {
                // Si tiene caracteres especiales copiamos el token en buffer sin el caracter especial
                for (size_t k = 0; k < strlen(args[i]); k++) {
                    if ((args[i][k] != '\'') && (args[i][k] != '\"') && (args[i][k] != '\\')) {
                        strncat(buffer, &args[i][k], 1);
                    }
                }
                printf("%s.", buffer);
            }

            //Añadimos un char espacio en buffer si hay otro token
            if (args[i + 1] != NULL) {
                strcat(buffer, " ");
            }
            i++;
        }

        // Eliminar el espacio extra al final del buffer
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == ' ') {
            buffer[len - 1] = '\0';
        }

        if (chdir(buffer) == -1) {
            perror("chdir() error");
        } else{
            chdir(buffer);
        }
    }
    
    return 1;
}

int internal_export(char **args) {
    
    char *nom = strtok(args[1], "=");

    char *valor = strtok(NULL, "=");

    //Comprovar sintaxis
    if(nom == NULL || valor == NULL){
        fprintf(stderr, ROJO_T  "Sintaxis incorrecta. Us: export NOM=VALOR\n" RESET);
        return 1;
    }

    fprintf(stderr, GRIS_T "[internal_export()-> nom: %s]\n" RESET, nom);
    fprintf(stderr, GRIS_T "[internal_export()-> valor: %s]\n" RESET, valor);

    //Mostrem el valor inicial de la variable d'entorn
    char *inicial = getenv(nom);
    fprintf(stderr, GRIS_T "Valor inicial de %s: %s\n", nom, inicial ? inicial : "(no definido)" RESET);

    //Assignar canvi
    if (setenv(nom, valor, 1) != 0){
        perror(ROJO_T "Error al canviar la variable d'entorn" RESET);
        return 1;
    }

    //Mostrar nou valor
    char *nou = getenv(nom);
    fprintf(stderr, GRIS_T "Nou valor de %s: %s\n", nom, nou ? nou : "(no definido)" RESET);

    return 1;
}

int internal_source(char **args) {
    
        fprintf(stderr, GRIS_T "[internal_source()-> Executará fitxer en linies de comandes]\n" RESET);
    
    return 1;
}

int internal_jobs(char **args) {

        fprintf(stderr, GRIS_T "[internal_jobs()-> Mostrará PID dels processos que no estiguin al foreground]\n" RESET);

    return 1;
}

int internal_fg(char **args) {
    
        fprintf(stderr, GRIS_T "[internal_fg()-> Envia de background a foreground o viceversa]\n" RESET);
    
    return 1;
}

int internal_bg(char **args) {
    
        fprintf(stderr, GRIS_T "[internal_bg()-> Reactivara proces detingut en segón pla]\n" RESET);
    
    return 1;
}
