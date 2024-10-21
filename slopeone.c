#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

typedef struct {
    unsigned int itemId;
    double rating;
} Item;

typedef struct {
    Item* item;
    unsigned int itemCount;
    int itemCapacity;
} User;

typedef struct {
    double desviacion;
    int cardinalidad;
} ResultadoDesviacion;

typedef struct {
    unsigned int movieId;
    char name[200];
} Movie;

User* users;
Movie* movies;

int usersNums = 0;
int maxItems = 0;
int movieCount = 0;

int readItems(const char* filename, const char* delimiter, int hasHeader, int itemIndex) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error al abrir el archivo.\n");
        return -1;
    }

    //printf("HOAAAAAAAAAAAAAAAAAAAAA:\n");
    int existItemId = 0;
    char line[200];  // Línea temporal para leer datos
    char* token;

    // Si el archivo tiene encabezado, lo saltamos
    if (hasHeader) {
        fgets(line, sizeof(line), file);
    }

    // Contar cuántas líneas (películas) hay para asignar memoria
    while (fgets(line, sizeof(line), file)) {
        movieCount++;
        //printf("%d\n", movieCount);
    }

    // Regresar al inicio del archivo para leer los datos
    rewind(file);
    if (hasHeader) {
        fgets(line, sizeof(line), file);  // Saltar encabezado de nuevo
    }

    // Asignar memoria para las películas
    movies = (Movie*)malloc(movieCount * sizeof(Movie));

    int i = 0;
    while (fgets(line, sizeof(line), file) && i < movieCount) {
        // Obtener el primer token (idMovie)
        //printf("Línea leída: %s", line);
        token = strtok(line, delimiter);
        if (token != NULL) {
            movies[i].movieId = atoi(token);  // Convertir a entero

            if (movies[i].movieId == itemIndex) {
                existItemId = 1;  // Verificar si el id existe
            }

            //printf("movie: %d\n", movies[i].movieId);

            // Obtener el segundo token (Name)
            token = strtok(NULL, delimiter);
            if (token != NULL) {
                strcpy(movies[i].name, token);  // Copiar el nombre

                // Remover el salto de línea al final de la cadena
                movies[i].name[strcspn(movies[i].name, "\n")] = '\0';
            }
        }
        i++;
    }

    fclose(file);
    return existItemId;
}

int compareItems(const void *a, const void *b) {
    return ((const Item *)a)->itemId - ((const Item *)b)->itemId;
}

int getFirstValueFromLastLine(const char *filename, const char *delimiter) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error al abrir el archivo");
        return -1; // o un valor de error apropiado
    }

    // Mover al final del archivo
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    if (fileSize == 0) {
        fclose(file);
        return -1; // Archivo vacío
    }

    // Leer hacia atrás hasta encontrar la última línea
    long position = fileSize - 1;
    while (position > 0 && fgetc(file) != '\n') {
        fseek(file, --position, SEEK_SET);
    }

    // Ahora estamos al inicio de la última línea
    // Leer el primer valor
    int firstValue = 0;
    char c;
    int isFirstValue = 1;

    while ((c = fgetc(file)) != EOF && c != '\n') {
        if (c == delimiter[0] && isFirstValue) {
            break; // Si es el delimitador y ya leí el primer valor, salimos
        }
        if (isFirstValue && c != delimiter[0]) {
            firstValue = firstValue * 10 + (c - '0'); // Construir el número
        }
        if (c == delimiter[0]) {
            isFirstValue = 0; // Cambiamos a no primer valor
        }
    }

    fclose(file);
    return firstValue; // Devolver el primer valor encontrado
}

void readData(const char* filename, const char* delimiter, int hasHeader, int sort) {

    int estimatedUsers = getFirstValueFromLastLine(filename, delimiter);

    printf("estimatedUsers: %d\n", estimatedUsers);

    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        perror("Error al abrir el archivo");
        return;
    }

    int capacity = estimatedUsers+1;
    users = (User*)malloc((capacity) * sizeof(User));

    char line[50];
    char format[20];

    sprintf(format, "%%d%s%%d%s%%lf", delimiter, delimiter);

    if (hasHeader) {
        fgets(line, sizeof(line), file);
    }

    while (fgets(line, sizeof(line), file)) {
        unsigned int uid;
        unsigned int iid;
        double rating;

        sscanf(line, format, &uid, &iid, &rating);

        if (uid > usersNums) {

            users[uid].itemCount = 0;
            users[uid].itemCapacity = 2;
            users[uid].item = (Item*)malloc(users[uid].itemCapacity * sizeof(Item));

            if (sort && usersNums > 0) {
                qsort(users[usersNums].item, users[usersNums].itemCount, sizeof(Item), compareItems);
            }

            if (users[usersNums].itemCount > maxItems) {
                maxItems = users[usersNums].itemCount;
            }

            usersNums++;
        }

        if (users[uid].itemCount == users[uid].itemCapacity) {
            users[uid].itemCapacity *= 2;
            users[uid].item = (Item*)realloc(users[uid].item, users[uid].itemCapacity * sizeof(Item));
        }

        users[uid].item[users[uid].itemCount].itemId = iid;
        users[uid].item[users[uid].itemCount].rating = rating;
        users[uid].itemCount++;

    }

    if (sort) {
        qsort(users[usersNums].item, users[usersNums].itemCount, sizeof(Item), compareItems);
    }


    if (users[usersNums].itemCount > maxItems) {
        maxItems = users[usersNums].itemCount;
    }

    fclose(file);
}

ResultadoDesviacion hallarDesviacionYcardinalidad(int item1,int item2) {
    ResultadoDesviacion resultado = {0.0, 0};
    double sumatoria = 0;
    double card = 0;

    for (int i = 1; i <= usersNums; i++) {
        double ratingItem1 = 0;
        double ratingItem2 = 0;
        int haCalificadoItem1 = 0;
        int haCalificadoItem2 = 0;

        for (int j = 0; j < users[i].itemCount; j++) {
            if (users[i].item[j].itemId == item1) {
                //printf("item1: %d\n", item1);
                ratingItem1 = users[i].item[j].rating;
                //printf("ranking1: %lf\n", ratingItem1);
                haCalificadoItem1 = 1;
            }
            if (users[i].item[j].itemId == item2) {
                //printf("item2: %d\n", item2);
                ratingItem2 = users[i].item[j].rating;
                //printf("ranking2: %lf\n", ratingItem2);
                haCalificadoItem2 = 1;
            }
        }

        if (haCalificadoItem1 && haCalificadoItem2) {
            card++;
            sumatoria += ratingItem1 - ratingItem2;
        }
    }

    if (card == 0) {
        return resultado; // Retornar 0 si no hay datos suficientes
    }

    double desviacion = sumatoria / card;

    resultado.desviacion = desviacion;
    resultado.cardinalidad= card;
    //printf("\tITEMS: %d, %d\n", item1, item2);
    return resultado;
}


double encontrarRatingSegunId(int userIndex, int id_buscar) {
    for (int i = 0; i < users[userIndex].itemCount + 1; i++) {
        if ((users[userIndex].item[i].itemId) == id_buscar) {
            return users[userIndex].item[i].rating;
        }
    }

    return -1.0;
}


double slopeOne(int userPredict, int itemPredict) {

    double numerador = 0.0;
    double denominador = 0.0;

    double maxcount = users[userPredict].itemCount;
    maxcount = movieCount;
    for (int i = 0; i < maxcount; i++) {
        if (itemPredict == movies[i].movieId) {

            maxcount++;
            continue;
        }

        ResultadoDesviacion resultado = hallarDesviacionYcardinalidad(itemPredict, movies[i].movieId);
        double rating = encontrarRatingSegunId(userPredict, movies[i].movieId);

        if (rating == -1.0 || resultado.cardinalidad == 0) {
            continue;
        }
        //printf("\nitem2: %d\n", movies[i].movieId);
        //printf("cardinalidad: %d\n", resultado.cardinalidad);
        //printf("rating: %lf\n", rating);
        //printf("resultado.desviacion: %lf\n\n", resultado.desviacion);

        numerador += (rating + resultado.desviacion) * resultado.cardinalidad;
        denominador += resultado.cardinalidad;
    }

    //printf("numerador: %lf\n", numerador);
    //printf("denominador: %lf\n", denominador);
    //printf("\nresultado: %lf\n", (numerador / denominador));

    return numerador / denominador;
}

const char* buscarNombrePelicula(unsigned int movieId) {
    for (int i = 0; i < movieCount; i++) {

        if (movies[i].movieId == movieId) {
            return movies[i].name;
        }
    }
    return NULL; // Retorna NULL si no encuentra la película
}

double hallarPrediccion(int usuarioIndex, int itemIndex) {
    if (users[usuarioIndex].itemCount == 0 || usuarioIndex > usersNums || usuarioIndex < 0) {
        printf("\nEl usuario no existe\n");
        exit(1);
    } else if (encontrarRatingSegunId(usuarioIndex, itemIndex) != -1.0){
        printf("\nEl usuario ya califico al item: %d\n", itemIndex);
        exit(1);
    }


    printf("Movie count: %d\n", movieCount);
    double prediccion = 0.0;

    prediccion = slopeOne(usuarioIndex, itemIndex);

    printf("\nSe predice que: El usuario %d, calificara a el item: %s, con: %lf\n", usuarioIndex, buscarNombrePelicula(itemIndex), prediccion);

}

void imprimirUsuarios() {
    printf("\nDatos leídos:\n");
    for (int i = 1; i <= usersNums; i++) {  // Iterar desde 1 (usuarios comienzan desde 1)
        printf("Usuario %d: - NumItems: %d\n", i, users[i].itemCount);
        for (int j = 0; j < users[i].itemCount; j++) {
            printf("  Película %d: id = %d, rating = %.2f\n",
                   j + 1,
                   users[i].item[j].itemId,
                   users[i].item[j].rating);
        }
    }
}

int main() {
    struct timespec start_time, end_time;
    long long read_ratings_time_ns = 0;
    long long read_movies_time_ns = 0;

    // Pedir por consola el usuario y el ítem
    int userPredict;
    int itemPredict;
    printf("Ingresa el ID del usuario para predecir: ");
    scanf("%d", &userPredict);
    printf("Ingresa el ID del item para predecir: ");
    scanf("%d", &itemPredict);

    // Medir el tiempo de lectura del dataset de ratings
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    readData("../dataset/ratings33m.dat", ",", 1, 0);
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    read_ratings_time_ns = (end_time.tv_sec - start_time.tv_sec) * 1000000000L + (end_time.tv_nsec - start_time.tv_nsec);
    printf("Tiempo de lectura del dataset de ratings: %.9f segundos\n", read_ratings_time_ns / 1e9);

    // Medir el tiempo de lectura del dataset de movies
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    if (readItems("../dataset/movies33.dat", ",", 1, itemPredict) == 0) {
        printf("\nNo existe este item: %d\n", itemPredict);
        exit(1);
    }
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    read_movies_time_ns = (end_time.tv_sec - start_time.tv_sec) * 1000000000L + (end_time.tv_nsec - start_time.tv_nsec);
    printf("Tiempo de lectura del dataset de movies: %.9f segundos\n", read_movies_time_ns / 1e9);

    // Sumar el tiempo total de lectura
    long long total_read_time_ns = read_ratings_time_ns + read_movies_time_ns;
    printf("Tiempo total de lectura de los datasets: %.9f segundos\n", total_read_time_ns / 1e9);

    // Mostrar información sobre los usuarios y items
    printf("Número de usuarios: %d\n", usersNums);
    printf("Número máximo de items: %d\n", maxItems);

    // Medir el tiempo de ejecución de la predicción
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    // Realizar la predicción
    hallarPrediccion(userPredict, itemPredict);

    // Calcular el tiempo de ejecución de la predicción
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    long long predict_time_ns = (end_time.tv_sec - start_time.tv_sec) * 1000000000L + (end_time.tv_nsec - start_time.tv_nsec);
    printf("Tiempo de ejecución de la predicción: %.9f segundos\n", predict_time_ns / 1e9);

    // Imprimir nuevamente el número de usuarios y el número máximo de ítems
    printf("\nNúmero de usuarios: %d\n", usersNums);
    printf("Número máximo de items: %d\n", maxItems);

    // Liberar memoria
    for (int i = 1; i <= usersNums; i++) {
        free(users[i].item);
    }

    free(users);

    return EXIT_SUCCESS;
}