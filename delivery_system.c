#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define CANT_PRODUCTORES 3
#define CANT_COCINEROS 2
#define CANT_REPARTIDORES 2
#define TAM_COLAS 5
#define PEDIDOS_POR_PRODUCTOR 4
#define PERIODO_GENERACION 1

#define FIN -1

typedef struct {
    int id;
    int tiempo_preparacion;
    const char *tipo;
} Pedido;

typedef struct {
    Pedido pedidos[TAM_COLAS];
    int inicio;
    int fin;
} Cola;

Cola pendientes;
Cola listos;

pthread_mutex_t mutex_pendientes = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_listos = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_id = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_periodo = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_periodo = PTHREAD_COND_INITIALIZER;

sem_t hay_pendientes;
sem_t espacio_pendientes;
sem_t hay_listos;
sem_t espacio_listos;

int siguiente_id = 1;
int periodo_actual = 0;

const char *tipos[] = {
    "Pizza",
    "Hamburguesa",
    "Empanadas",
    "Milanesa",
    "Ensalada"
};

void agregar(Cola *cola, Pedido pedido) {
    cola->pedidos[cola->fin] = pedido;
    cola->fin = (cola->fin + 1) % TAM_COLAS;
}

Pedido quitar(Cola *cola) {
    Pedido pedido = cola->pedidos[cola->inicio];
    cola->inicio = (cola->inicio + 1) % TAM_COLAS;
    return pedido;
}

Pedido nuevo_pedido(void) {
    Pedido pedido;

    pthread_mutex_lock(&mutex_id);
    pedido.id = siguiente_id;
    siguiente_id++;
    pthread_mutex_unlock(&mutex_id);

    pedido.tiempo_preparacion = 1 + pedido.id % 3;
    pedido.tipo = tipos[pedido.id % 5];

    return pedido;
}

void poner_pendiente(Pedido pedido) {
    sem_wait(&espacio_pendientes);
    pthread_mutex_lock(&mutex_pendientes);
    agregar(&pendientes, pedido);
    pthread_mutex_unlock(&mutex_pendientes);
    sem_post(&hay_pendientes);
}

Pedido sacar_pendiente(void) {
    Pedido pedido;

    sem_wait(&hay_pendientes);
    pthread_mutex_lock(&mutex_pendientes);
    pedido = quitar(&pendientes);
    pthread_mutex_unlock(&mutex_pendientes);
    sem_post(&espacio_pendientes);

    return pedido;
}

void poner_listo(Pedido pedido) {
    sem_wait(&espacio_listos);
    pthread_mutex_lock(&mutex_listos);
    agregar(&listos, pedido);
    pthread_mutex_unlock(&mutex_listos);
    sem_post(&hay_listos);
}

Pedido sacar_listo(void) {
    Pedido pedido;

    sem_wait(&hay_listos);
    pthread_mutex_lock(&mutex_listos);
    pedido = quitar(&listos);
    pthread_mutex_unlock(&mutex_listos);
    sem_post(&espacio_listos);

    return pedido;
}

void *productor(void *dato) {
    int productor_id = *(int *)dato;
    int periodo;

    for (periodo = 1; periodo <= PEDIDOS_POR_PRODUCTOR; periodo++) {
        Pedido pedido;

        pthread_mutex_lock(&mutex_periodo);
        while (periodo_actual < periodo) {
            pthread_cond_wait(&cond_periodo, &mutex_periodo);
        }
        pthread_mutex_unlock(&mutex_periodo);

        pedido = nuevo_pedido();

        printf("Pedido generado: productor %d, pedido %d, comida %s\n",
               productor_id, pedido.id, pedido.tipo);
        poner_pendiente(pedido);
    }

    return NULL;
}

void *cocinero(void *dato) {
    int cocinero_id = *(int *)dato;

    while (1) {
        Pedido pedido = sacar_pendiente();

        if (pedido.id == FIN) {
            break;
        }

        printf("Pedido tomado por cocinero: cocinero %d, pedido %d\n",
               cocinero_id, pedido.id);

        sleep(pedido.tiempo_preparacion);
        poner_listo(pedido);
    }

    return NULL;
}

void *repartidor(void *dato) {
    int repartidor_id = *(int *)dato;

    while (1) {
        Pedido pedido = sacar_listo();

        if (pedido.id == FIN) {
            break;
        }

        sleep(1);

        printf("Pedido entregado: repartidor %d, pedido %d\n",
               repartidor_id, pedido.id);
    }

    return NULL;
}

void *reloj_generacion(void *dato) {
    int i;

    (void)dato;

    for (i = 0; i < PEDIDOS_POR_PRODUCTOR; i++) {
        sleep(PERIODO_GENERACION);

        pthread_mutex_lock(&mutex_periodo);
        periodo_actual++;
        printf("Periodo de generacion %d\n", periodo_actual);
        pthread_cond_broadcast(&cond_periodo);
        pthread_mutex_unlock(&mutex_periodo);
    }

    return NULL;
}

int main(void) {
    pthread_t productores[CANT_PRODUCTORES];
    pthread_t cocineros[CANT_COCINEROS];
    pthread_t repartidores[CANT_REPARTIDORES];
    pthread_t reloj;
    int id_productores[CANT_PRODUCTORES];
    int id_cocineros[CANT_COCINEROS];
    int id_repartidores[CANT_REPARTIDORES];
    Pedido fin;
    int i;

    sem_init(&hay_pendientes, 0, 0);
    sem_init(&espacio_pendientes, 0, TAM_COLAS);
    sem_init(&hay_listos, 0, 0);
    sem_init(&espacio_listos, 0, TAM_COLAS);

    for (i = 0; i < CANT_COCINEROS; i++) {
        id_cocineros[i] = i + 1;
        pthread_create(&cocineros[i], NULL, cocinero, &id_cocineros[i]);
    }

    for (i = 0; i < CANT_REPARTIDORES; i++) {
        id_repartidores[i] = i + 1;
        pthread_create(&repartidores[i], NULL, repartidor, &id_repartidores[i]);
    }

    for (i = 0; i < CANT_PRODUCTORES; i++) {
        id_productores[i] = i + 1;
        pthread_create(&productores[i], NULL, productor, &id_productores[i]);
    }

    pthread_create(&reloj, NULL, reloj_generacion, NULL);
    pthread_join(reloj, NULL);

    for (i = 0; i < CANT_PRODUCTORES; i++) {
        pthread_join(productores[i], NULL);
    }

    fin.id = FIN;
    fin.tiempo_preparacion = 0;
    fin.tipo = "FIN";

    for (i = 0; i < CANT_COCINEROS; i++) {
        poner_pendiente(fin);
    }

    for (i = 0; i < CANT_COCINEROS; i++) {
        pthread_join(cocineros[i], NULL);
    }

    for (i = 0; i < CANT_REPARTIDORES; i++) {
        poner_listo(fin);
    }

    for (i = 0; i < CANT_REPARTIDORES; i++) {
        pthread_join(repartidores[i], NULL);
    }

    sem_destroy(&hay_pendientes);
    sem_destroy(&espacio_pendientes);
    sem_destroy(&hay_listos);
    sem_destroy(&espacio_listos);
    pthread_cond_destroy(&cond_periodo);
    pthread_mutex_destroy(&mutex_periodo);

    return 0;
}
