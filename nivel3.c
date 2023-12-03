#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

#define PROMPT '$'
#define MAX_COMAND_SIZE 1024  //llargada MAX de cada comanda
#define ARGS_SIZE 64
#define N_JOBS 10   //Numero de trabajos que almacena la tabla
static char mi_shell[MAX_COMAND_SIZE]; //variable global para guardar el nombre del minishell

//Declaración struct jobs
struct info_job {
   pid_t pid;
   char estado; // ‘N’, ’E’, ‘D’, ‘F’ (‘N’: Ninguno, ‘E’: Ejecutándose y ‘D’: Detenido, ‘F’: Finalizado) 
   char cmd[MAX_COMAND_SIZE]; // línea de comando asociada
};

//Declaración tabla jobs
static struct info_job jobs_list [N_JOBS]; 

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
void inicializar_job(); 
void actualizar_job(int numTabla, pid_t pid, char estado, char cmd[]); 

//Definició colors
#define RESET "\033[0m"     //Posar els colors per defecte
#define NEGRO_T "\x1b[30m"  //Text Negre
#define NEGRO_F "\x1b[40m"  //Fons negre
#define GRIS_T "\x1b[37m"   //La resta canvia el color del text per l'especificat per el nom
#define ROJO_T "\x1b[31m"
#define VERDE_T "\x1b[32m"
#define AMARILLO_T "\x1b[33m"
#define AZUL_T "\x1b[34m"
#define MAGENTA_T "\x1b[35m"
#define CYAN_T "\x1b[36m"
#define BLANCO_T "\x1b[97m"
#define NEGRITA "\x1b[1m"


//Bucle infinit principal
int main(int argc, char *argv[]){
    char line[MAX_COMAND_SIZE]; //Array on guardem la comanda
    //Inicialización del primer job
    inicializar_job(); 
    //Guardamos el nombre del programa
        if (argc > 0) {
        strcpy(mi_shell, argv[0]); // Copia el contenido de argv[0] a mi_shell
    } else {
        printf(NEGRITA ROJO_T "Ha habido un error almacenando el nombre del programa.\n" RESET);
    }

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

    printf(NEGRITA"%s:%s%s%s$ ", user, home, strstr(pwd, home) + strlen(home), strstr(pwd, home) + strlen(home) == pwd ? "" : "/");
    
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
        //Si llega aquí es que no es un comando interno, creamos un hijo para ejecutar el comando
        pid_t pid = fork();
        int status;
        if (pid == 0){ // Proceso hijo
            if(execvp(args[0], args) == -1){
                perror("Error ejecutando execvp"); // Imprime el mensaje de error según errno
                printf("Código de error (errno): %d\n", errno); // Imprime el valor numérico de errno
            }
        } else if (pid > 0){ // Proceso padre
            fprintf(stderr, GRIS_T "[execute_line()→ PID padre: %d (%s)]\n" RESET, getpid(), mi_shell);
            jobs_list_update(0, pid, 'E', line_inalterada);
        if (wait(&status) == -1)
        {
            perror("Error con wait()");
        }
        jobs_list_reset(0);
    }
    }

    return 0;
}

int parse_args(char **args, char *line) {
    int token_count = 0;

    // Extreure el primer token
    args[token_count] = strtok(line, " \t\n\r");
    //fprintf(stderr, GRIS_T "[parse_args()-> token %i: %s]\n" RESET, token_count, args[token_count]);
    //Bucle que extreu tots els tokens fins que arriba a null o a #
    while (args[token_count] && (args[token_count][0] != '#')) {
        token_count++;
        args[token_count] = strtok(NULL, " \t\n\r");
        //fprintf(stderr, GRIS_T "[parse_args()-> token %i: %s]\n" RESET, token_count, args[token_count]);
    }

    if (args[token_count]){
        //Si es troba un # deixa de trosejar pero no deixa l'ultim token com a null
        //Aqui el posem a null si l'ultim no ho es, si ja ho es no entra al if
        args[token_count] = NULL;
        //fprintf(stderr, GRIS_T "[parse_args()-> token %i corretgit: %s]\n" RESET, token_count, args[token_count]);
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
    char path[PATH_MAX];

    //Si posen cd sense cap més argument va al home
    if (args[1]==NULL){
    //Posem args[1], ja que a args[0] tenim el cd
        chdir(getenv("HOME"));
    }else{
        chdir(args[1]);
    }
    

    fprintf(stderr, GRIS_T "[internal_cd()-> Canvi de directori]\n");
    
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
        perror("Error al canviar la variable d'entorn");
        return 1;
    }

    //Mostrar nou valor
    char *nou = getenv(nom);
    fprintf(stderr, GRIS_T "Nou valor de %s: %s\n", nom, nou ? nou : "(no definido)" RESET);




    return 1;
}

int internal_source(char **args) {
    
        fprintf(stderr, GRIS_T"[internal_source()-> Executara caomandes fitxer]\n");
    
    return 1;
}

int internal_jobs(char **args) {
        fprintf(stderr, GRIS_T"[internal_jobs()-> PID processos no foreground]\n");

    return 1;
}

int internal_fg(char **args) {
    
        fprintf(stderr, GRIS_T "[internal_fg()-> Envia de background a foreground o viceversa]\n");
    
    return 1;
}

int internal_bg(char **args) {
    
        fprintf(stderr, GRIS_T"[internal_bg()-> Reactivara proces detingut en segón pla]\n");
    return 1;
}

void inicializar_job(){
    jobs_list[0].pid = 0; 
    jobs_list[0].estado = 'N'; 
    memset(jobs_list[0].cmd, '\0', MAX_COMAND_SIZE); 
}

void actualizar_job(int numTabla, pid_t pid, char estado, char cmd[]){
    
}

