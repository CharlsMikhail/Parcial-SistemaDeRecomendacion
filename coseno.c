#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>

#define MAX_STRING_MOVIE_NAME 200
#define MAX_STRING_LINE 300

typedef struct {
    unsigned int itemId;
    double rating;
} Item;

typedef struct {
    Item* item;
    unsigned int itemCount;
    int itemCapacity;
    double average;
} User;

typedef struct {
    unsigned int movieId;
    char name[MAX_STRING_MOVIE_NAME];
} Movie;

User* users;
Movie* movies;

int usersNums = 0;
int maxItems = 0;
int movieCount = 0;

// Lee los ítems (películas) de un archivo y verifica si el ítem dado existe.
int readItems(const char* filename, const char* delimiter, int hasHeader, int itemIndex) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error al abrir el archivo.\n");
        return -1;
    }

    int existItemId = 0;
    char line[MAX_STRING_LINE];
    char* token;

    if (hasHeader) {
        fgets(line, sizeof(line), file);
    }

    while (fgets(line, sizeof(line), file)) {
        movieCount++;
    }

    rewind(file);
    if (hasHeader) {
        fgets(line, sizeof(line), file);
    }

    movies = (Movie*)malloc(movieCount * sizeof(Movie));

    int i = 0;
    while (fgets(line, sizeof(line), file) && i < movieCount) {
        token = strtok(line, delimiter);
        if (token != NULL) {
            movies[i].movieId = atoi(token);  // Convertir a entero

            if (movies[i].movieId == itemIndex) {
                existItemId = 1;  // Verificar si el ítem existe
            }

            token = strtok(NULL, delimiter);
            if (token != NULL) {
                strcpy(movies[i].name, token);  // Guardar el nombre
                movies[i].name[strcspn(movies[i].name, "\n")] = '\0';  // Quitar salto de línea
            }
        }
        i++;
    }

    fclose(file);
    return existItemId;
}

// Lee los datos de ratings de un archivo, asigna usuarios y sus ítems.
void readData(const char* filename, const char* delimiter, int hasHeader, int sort) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error al abrir el archivo");
        return;
    }

    int capacity = 100;
    double counterItemsRating = 0.0;
    users = (User*)malloc(capacity * sizeof(User));
    char line[50];
    char format[20];

    sprintf(format, "%%d%s%%d%s%%lf", delimiter, delimiter);

    if (hasHeader) {
        fgets(line, sizeof(line), file);
    }

    while (fgets(line, sizeof(line), file)) {
        unsigned int uid, iid;
        double rating;
        sscanf(line, format, &uid, &iid, &rating);

        if (uid > usersNums) {
            if (uid >= capacity) {
                capacity *= 2;
                users = (User*)realloc(users, capacity * sizeof(User));
            }
            users[uid].itemCount = 0;
            users[uid].itemCapacity = 2;
            users[uid].item = (Item*)malloc(users[uid].itemCapacity * sizeof(Item));

            if (usersNums > 0 && users[usersNums].itemCount > maxItems) {
                maxItems = users[usersNums].itemCount;
            }

            users[usersNums].average = counterItemsRating / users[usersNums].itemCount;
            counterItemsRating = 0.0;
            usersNums++;
        }

        if (users[uid].itemCount == users[uid].itemCapacity) {
            users[uid].itemCapacity *= 2;
            users[uid].item = (Item*)realloc(users[uid].item, users[uid].itemCapacity * sizeof(Item));
        }

        users[uid].item[users[uid].itemCount].itemId = iid;
        users[uid].item[users[uid].itemCount].rating = rating;
        users[uid].itemCount++;
        counterItemsRating += rating;
    }

    if (users[usersNums].itemCount > maxItems) {
        maxItems = users[usersNums].itemCount;
    }

    printf("\nElte\n");

    fclose(file);
}

// Calcula la similitud de coseno ajustado entre dos ítems.
double hallarSimilitudDeCosenoAjustado(int item1, int item2) {
    double sumaNumerador = 0, sumaDenominador1 = 0, sumaDenominador2 = 0;

    for (int i = 1; i <= usersNums; i++) {
        double ratingItem1 = 0, ratingItem2 = 0;
        int haCalificadoItem1 = 0, haCalificadoItem2 = 0;

        for (int j = 0; j < users[i].itemCount; j++) {
            if (users[i].item[j].itemId == item1) {
                ratingItem1 = users[i].item[j].rating;
                haCalificadoItem1 = 1;
            }
            if (users[i].item[j].itemId == item2) {
                ratingItem2 = users[i].item[j].rating;
                haCalificadoItem2 = 1;
            }
            if (haCalificadoItem1 && haCalificadoItem2) {
                break;
            }
        }

        if (haCalificadoItem1 && haCalificadoItem2) {
            double ratingAjustado1 = ratingItem1 - users[i].average;
            double ratingAjustado2 = ratingItem2 - users[i].average;
            sumaNumerador += ratingAjustado1 * ratingAjustado2;
            sumaDenominador1 += ratingAjustado1 * ratingAjustado1;
            sumaDenominador2 += ratingAjustado2 * ratingAjustado2;
        }
    }

    if (sumaDenominador1 == 0 || sumaDenominador2 == 0) {
        return NAN;
    }

    return sumaNumerador / (sqrt(sumaDenominador1) * sqrt(sumaDenominador2));
}

// Encuentra el rating dado por un usuario para un ítem.
double encontrarRatingSegunId(int userIndex, int id_buscar) {
    for (int i = 0; i < users[userIndex].itemCount; i++) {
        if (users[userIndex].item[i].itemId == id_buscar) {
            return users[userIndex].item[i].rating;
        }
    }
    return -1.0;
}

// Calcula la predicción de rating para un usuario y un ítem.
double hallarPrediccion(int userPredict, int itemPredict) {
    double minRating = 1, maxRating = 5;
    double sumaMatriz = 0.0, numeradorPredict = 0.0;

    for (int i = 0; i < users[userPredict].itemCount; i++) {

        double currentRating = encontrarRatingSegunId(userPredict, users[userPredict].item[i].itemId);
        double currentCoseno = hallarSimilitudDeCosenoAjustado(itemPredict, users[userPredict].item[i].itemId);

        if (isnan(currentCoseno) || currentRating == -1.0) {
            continue;
        }

        double userRatingsModified = ((2 * (currentRating - minRating)) - (maxRating - minRating)) / (maxRating - minRating);
        numeradorPredict += userRatingsModified * currentCoseno;
        sumaMatriz += fabs(currentCoseno);
    }

    double predictNomalized = numeradorPredict / sumaMatriz;
    return 0.5 * ((predictNomalized + 1) * (maxRating - minRating)) + minRating;
}

const char* buscarNombrePelicula(unsigned int movieId) {
    for (int i = 0; i < movieCount; i++) {

        if (movies[i].movieId == movieId) {
            return movies[i].name;
        }
    }
    return NULL; // Retorna NULL si no encuentra la película
}

// Inicializa la predicción para un usuario y un ítem, asegurándose de que existan en el dataset.
double inicializarPrediccion(int usuarioIndex, int itemIndex) {
    if (usuarioIndex <= 0 || usuarioIndex > usersNums || users[usuarioIndex].itemCount == 0) {
        printf("\nEl usuario no existe\n");
        exit(1);
    }

    if (encontrarRatingSegunId(usuarioIndex, itemIndex) != -1.0) {
        printf("\nEl usuario ya calificó al item: %d\n", itemIndex);
        exit(1);
    }


    double prediccion = hallarPrediccion(usuarioIndex, itemIndex);
    printf("\nSe predice que el usuario %d calificará el ítem: %s con: %lf\n", usuarioIndex, buscarNombrePelicula(itemIndex), prediccion);
    return prediccion;
}

// Programa principal que carga datos y realiza predicción.
int main() {
    struct timespec start_time, end_time;
    long long read_ratings_time_ns = 0;
    long long read_movies_time_ns = 0;
    long long prediction_time_ns = 0;

    // Medir el tiempo de lectura del dataset de ratings
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    readData("../dataset/ratings33m.csv", ",", 1, 0);
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    read_ratings_time_ns = (end_time.tv_sec - start_time.tv_sec) * 1000000000L + (end_time.tv_nsec - start_time.tv_nsec);
    printf("Tiempo de lectura del dataset de ratings: %.9f segundos\n", read_ratings_time_ns / 1e9);

    printf("Número de usuarios: %d\n", usersNums);
    printf("Máximo de ítems calificados: %d\n", maxItems);
    printf("\n\nPredicción...\n");

    // Pedir por consola el ID del usuario y del ítem
    int idUsuario, idItem;
    printf("\nIngrese ID del usuario para predecir: ");
    scanf("%d", &idUsuario);
    printf("\nIngrese ID del item para predecir: ");
    scanf("%d", &idItem);

    // Medir el tiempo de lectura del dataset de movies
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    if (!readItems("../dataset/movies33.csv", ",", 1, idItem)) {
        printf("\nNo existe este item: %d\n", idItem);
        exit(1);
    }
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    read_movies_time_ns = (end_time.tv_sec - start_time.tv_sec) * 1000000000L + (end_time.tv_nsec - start_time.tv_nsec);
    printf("Tiempo de lectura del dataset de movies: %.9f segundos\n", read_movies_time_ns / 1e9);

    // Sumar el tiempo total de lectura
    long long total_read_time_ns = read_ratings_time_ns + read_movies_time_ns;
    printf("Tiempo total de lectura de los datasets: %.9f segundos\n", total_read_time_ns / 1e9);

    // Medir el tiempo de ejecución de la predicción
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    // Inicializar la predicción
    inicializarPrediccion(idUsuario, idItem);

    // Calcular el tiempo de ejecución de la predicción
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    prediction_time_ns = (end_time.tv_sec - start_time.tv_sec) * 1000000000L + (end_time.tv_nsec - start_time.tv_nsec);
    printf("Tiempo de ejecución de la predicción: %.9f segundos\n", prediction_time_ns / 1e9);

    return 0;
}