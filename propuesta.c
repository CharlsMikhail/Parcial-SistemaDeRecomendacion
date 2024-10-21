#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <omp.h>
#include "uthash.h"

typedef struct {
    unsigned int movieId;
    char name[200];
} Movie;

Movie *movies = NULL;  // Puntero para el array dinámico de películas
unsigned int movieCount;


unsigned int contarLineas(FILE *file) {
    unsigned int lines = 0;
    char ch;

    while (!feof(file)) {
        ch = fgetc(file);
        if (ch == '\n') {
            lines++;
        }
    }

    // Volver al inicio del archivo
    fseek(file, 0, SEEK_SET);
    return lines;
}


void leerItems(const char *filename, const char *delimiter, int tieneHeader) {
    printf("Leyendo datos del archivo: %s\n", filename);
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("No se pudo abrir el archivo.\n");
        return;
    }

    // Contar cuántas líneas tiene el archivo
    movieCount = contarLineas(file);

    // Reservar memoria para todas las películas
    movies = (Movie *)malloc((--movieCount) * sizeof(Movie));

    // Leer cada línea
    char buffer[1024];  // Buffer para almacenar cada línea del archivo
    char *token;
    unsigned int movieId;
    unsigned int i = 0;  // Contador para el índice de películas

    // Si tiene encabezado, saltarlo
    if (tieneHeader) {
        fgets(buffer, sizeof(buffer), file);
    }

    while (fgets(buffer, sizeof(buffer), file)) {
        // Leer el movieId
        token = strtok(buffer, delimiter);
        if (token != NULL) {
            movieId = atoi(token);
            movies[i].movieId = movieId;
        }

        // Leer el nombre, considerando que puede estar entre comillas
        token = strtok(NULL, "\"");
        if (token != NULL) {
            // Copiar el nombre, ignorando lo que venga después
            char *nombre = strtok(token, delimiter);  // Solo tomar hasta el siguiente delimitador
            if (nombre != NULL) {
                strcpy(movies[i].name, nombre);
            }
        } else {
            // Si no tiene comillas, leer el nombre normalmente
            token = strtok(NULL, delimiter);
            if (token != NULL) {
                strcpy(movies[i].name, token);
            }
        }

        // Incrementar el índice de películas
        i++;
    }

    fclose(file);
}


const char* buscarNombrePelicula(unsigned int movieId) {
    for (unsigned int i = 0; i < movieCount; i++) {
        if (movies[i].movieId == movieId) {
            return movies[i].name;
        }
    }
    return NULL; // Si no se encuentra, retorna NULL
}


// Definir las estructuras necesarias
typedef struct {
    unsigned int itemId;    // ID del ítem
    double rating;          // Calificación dada por el usuario
    UT_hash_handle hh;      // Manejador para uthash
} Item;

typedef struct {
    unsigned int userId;    // ID del usuario
    Item* items;            // Hash map de ítems calificados por el usuario
    UT_hash_handle hh;      // Manejador para uthash
} User;

// Hash map global de usuarios
User* users = NULL;

// Función para agregar un usuario
User* agregarUsuario(unsigned int userId) {
    User* user;
    HASH_FIND_INT(users, &userId, user);
    if (user == NULL) {
        user = (User*)malloc(sizeof(User));
        user->userId = userId;
        user->items = NULL;
        HASH_ADD_INT(users, userId, user);
    }
    return user;
}

// Función para agregar una calificación de ítem a un usuario
void agregarItem(User* user, unsigned int itemId, double rating) {
    Item* item;
    HASH_FIND_INT(user->items, &itemId, item);
    if (item == NULL) {
        item = (Item*)malloc(sizeof(Item));
        item->itemId = itemId;
        item->rating = rating;
        HASH_ADD_INT(user->items, itemId, item);
    } else {
        item->rating = rating;  // Actualizar calificación si ya existe
    }
}

// Función para buscar un usuario por su ID
User* buscarUsuario(unsigned int userId) {
    User* user;
    HASH_FIND_INT(users, &userId, user);
    return user;
}

// Función para predecir la calificación de un ítem no calificado por un usuario
double predecirSlopeOne(unsigned int userId, unsigned int itemPredict) {
    User* user = buscarUsuario(userId);
    if (user == NULL) {
        printf("Usuario no encontrado.\n");
        return NAN;
    }

    Item* item = NULL;
    HASH_FIND_INT(user->items, &itemPredict, item);
    if (item != NULL) {
        printf("El usuario %u ya calificó el ítem %u con una calificación de %.2lf. No se predice.\n", user->userId, itemPredict, item->rating);
        return NAN;  // Salir de la función sin predecir
    }


    double numerador = 0.0;
    double denominador = 0.0;

    // Obtener ítems calificados por el usuario
    Item *itemsArray[HASH_COUNT(user->items)];
    Item *tmpItem;
    int i = 0;
    for (Item *item = user->items; item != NULL; item = (Item*)(item->hh.next)) {
        itemsArray[i++] = item;
    }

    #pragma omp parallel for reduction(+:numerador,denominador) private(tmpItem)
    for (i = 0; i < HASH_COUNT(user->items); i++) {
        Item* item = itemsArray[i];
        if (item->itemId == itemPredict) continue; // Saltar si es el mismo ítem

        double deviation = 0.0;
        unsigned int count = 0;

        User* otherUser;
        User* tmpUser;

        // Recorrer los usuarios para comparar ítems
        for (otherUser = users; otherUser != NULL; otherUser = (User*)(otherUser->hh.next)) {
            Item* itemPred;
            Item* itemRated;

            // Ver si este usuario ha calificado ambos ítems
            HASH_FIND_INT(otherUser->items, &itemPredict, itemPred);  // Calificación del ítem a predecir
            HASH_FIND_INT(otherUser->items, &item->itemId, itemRated);  // Calificación del ítem calificado por el usuario

            if (itemPred != NULL && itemRated != NULL) {
                deviation += (itemPred->rating - itemRated->rating);  // Diferencia entre el ítem a predecir y el ítem calificado
                count++;
            }
        }

        if (count > 0) {
            deviation /= count;  // Promediar la desviación
            numerador += (fmax(0.0, fmin(5.0, deviation + item->rating))) * count;  // Ajustar la calificación basada en la desviación
            denominador += count;
        }
    }

    if (denominador == 0.0) {
        return NAN;  // No hay suficientes datos para hacer la predicción
    }

    return numerador / denominador;  // Calificación predicha
}

// Función para liberar la memoria
void liberarMemoria() {
    User* user, *tmpUser;
    Item* item, *tmpItem;

    HASH_ITER(hh, users, user, tmpUser) {
        HASH_ITER(hh, user->items, item, tmpItem) {
            HASH_DEL(user->items, item);
            free(item);
        }
        HASH_DEL(users, user);
        free(user);
    }

     // Liberar memoria
    free(movies);
}

// Función para leer los datos de un archivo CSV
void leerUsuarios(const char* archivo, const char* delimitador, int header) {
    printf("Leyendo datos del archivo: %s\n", archivo);
    FILE* file = fopen(archivo, "r");
    if (file == NULL) {
        printf("No se puede abrir el archivo %s\n", archivo);
        return;
    }

    char linea[1024];
    unsigned int userId, itemId;
    double rating;

    // Si hay un encabezado, saltarlo
    if (header) {
        fgets(linea, sizeof(linea), file);
    }


    while (fgets(linea, sizeof(linea), file)) {
        char* token = strtok(linea, delimitador);

        userId = atoi(token);

        token = strtok(NULL, delimitador);
        itemId = atoi(token);

        token = strtok(NULL, delimitador);
        rating = atof(token);

        User* user = agregarUsuario(userId);
        agregarItem(user, itemId, rating);
    }

    fclose(file);
}

// Nueva función para buscar por nombre (substring o nombre exacto)
void buscarPorNombre(const char* nombreBuscado, int* resultados, int* totalResultados) {
    *totalResultados = 0;
    for (int i = 0; i < movieCount; i++) {
        if (strstr(movies[i].name, nombreBuscado)) {
            resultados[(*totalResultados)++] = movies[i].movieId;
        }
    }
}

// Función principal
int main() {
    // Leer datos de usuarios e ítems desde el archivo CSV
    double startTime, endTime;
    startTime = omp_get_wtime();
    leerUsuarios("../dataset/ratings33m.csv", ",", 1);
    leerItems("../dataset/movies33.csv", ",", 1);
    endTime = omp_get_wtime();
    printf("Tiempo de lectura de datos: %f segundos\n\n", endTime - startTime);

    // Solicitar IDs del usuario e ítem a predecir
    unsigned int userId, itemPredict;
    char nombrePeliculaBuscada[256];
    printf("Ingrese el ID del usuario: ");
    scanf("%u", &userId);

    // Opción para buscar película por nombre
    printf("¿Desea buscar la película por nombre? (s/n): ");
    char opcion;
    scanf(" %c", &opcion);

    if (opcion == 's' || opcion == 'S') {
        printf("Ingrese el nombre o parte del nombre de la película: ");
        scanf(" %s[^\n]", nombrePeliculaBuscada);  // Leer entrada incluyendo espacios

        int resultados[100];  // Asumiendo que no habrá más de 100 resultados, ajustar según sea necesario
        int totalResultados;
        buscarPorNombre(nombrePeliculaBuscada, resultados, &totalResultados);

        if (totalResultados > 0) {
            printf("Películas encontradas:\n");
            for (int i = 0; i < totalResultados; i++) {
                printf("%d. %s (ID: %u)\n", i + 1, buscarNombrePelicula(resultados[i]), resultados[i]);
            }
            printf("Seleccione el número de la película: ");
            int seleccion;
            scanf("%d", &seleccion);
            if (seleccion > 0 && seleccion <= totalResultados) {
                itemPredict = resultados[seleccion - 1];
            } else {
                printf("Selección no válida.\n");
                return 1;
            }
        } else {
            printf("No se encontraron películas con ese nombre.\n");
            return 1;
        }
    } else {
        printf("Ingrese el ID del ítem (película): ");
        scanf("%u", &itemPredict);
    }

    startTime = omp_get_wtime();
    // Realizar la predicción con Slope One
    double prediccion = predecirSlopeOne(userId, itemPredict);
    const char* nombrePelicula = buscarNombrePelicula(itemPredict);
    if (!isnan(prediccion)) {
        if (nombrePelicula != NULL) {
            printf("\nPredicción de calificación para el usuario %u sobre el ítem %s (%u): %.6lf\n",
                    userId, nombrePelicula, itemPredict, prediccion);
        } else {
            printf("\nPredicción de calificación para el usuario %u sobre el ítem %u: %.6lf\n",
                    userId, itemPredict, prediccion);
        }
    } else {
        printf("No se pudo realizar la predicción.\n");
    }

    endTime = omp_get_wtime();
    printf("\nTiempo de ejecución del algoritmo: %f segundos\n", endTime - startTime);

    // Liberar memoria
    liberarMemoria();

    return 0;
}
