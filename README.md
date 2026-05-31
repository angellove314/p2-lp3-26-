# Trabajo Práctico 2 - LP3

#Sistema Concurrente de Atención de Pedidos

delivery_system.c
README.md
.gitignore
------------------------------------------------------------------------------------------------------------------------
Compilación

Para compilar el programa, ejecutar:


gcc -Wall -Wextra -pthread delivery_system.c -o delivery_system

Ejecución


Para ejecutar el programa:
./delivery_system
---------------------------------------------------------------------------------------------------------
## Descripción de funcionamiento

El programa utiliza una cola de pedidos pendientes y una cola de pedidos listos para entrega.

Los productores generan pedidos periódicamente. Para controlar los períodos de generación se utiliza un hilo reloj junto con una variable de condición.

Los cocineros retiran pedidos de la cola de pendientes, simulan la preparación con `sleep()` y luego colocan los pedidos en la cola de listos.

Los repartidores retiran pedidos listos, simulan la entrega y marcan el pedido como entregado.

Los mutex protegen las colas compartidas y los datos globales. Los semáforos controlan la disponibilidad de pedidos y la capacidad máxima de las colas.

```
