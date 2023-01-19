El juego de la vida de Conway se desarrolla sobre un rejilla infinita. Cada celda de la rejilla puede adoptar uno de dos
estados: viva o muerta. 

Resulta imposible representar dicha rejilla infinita, sin embargo podemos representar 
en memoria un área suficientemente grande para tener una aproximación de la rejilla real
mientras que solo mostramos al usuario una pequeña sección.  De este modo se pueden reducir
el número errores de visualización que se presentan cuando celdas que deberían morir no 
lo hacen a razón de que sus vecinas han caído fuera de los límites representables y su estado 
es desconocido.

En nuestra implementación escogimos un tablero con dimensión 500x500: 

```c++
bool grid[500][500];
```

En lugar de hacer coincidir una coordenada en pantalla con la coordenada correspondiente en la 
representación en memoria nuestra estrategia solo tendría efecto si posicionamos el área visible 
en el centro de la representación. Por ejemplo si tenemos una rejilla de 100 x 100 y el área visible 
tiene dimensión 25x25 (25 columnas por 25 filas), la coordenada visual (0,0) correspondería a la coordenada (25,25)
en la representación en memoria. Para calcular este desplazamiento contamos con la función ``GameSetViewport``.

```c++
// Establece la region que debe renderizarse del juego
void GameSetViewPort(int view_rows, int view_cols) {
  const int world_rows = 500;
  const int world_cols = 500;

  const int world_x_center = world_cols / 2;
  const int view_x_center = view_cols / 2;

  const int world_y_center = world_rows / 2;
  const int view_y_center = view_rows / 2;

  y_start = world_y_center - view_y_center;
  y_end = y_start + (view_rows - 1);

  x_start = world_x_center - view_x_center;
  x_end = x_start + (view_cols - 1);
}
```

Siendo entonces las variables globales ``x_start`` y ``y_start`` las coordenadas a partir 
de las que partimos en la representación. 

Si nuestra coordenada en pantalla es (1,1), esto es ``x = x_start + 1`` y ``y = y_start + 1`` 
en la representacion en memoria.

En cada paso se decide el próximo estado de una celda teniendo en cuenta el número de vecinos. Toda celda
viva con más de 3 vecinos o menos de 2 muere por sobrepoblación o soledad respectivamente. Toda celda muerta 
nace si cuenta con 3 vecinos vivos. Por tanto, para poder determinar la acción sobre la celda 
tenemos que conocer el número de vecinos de la misma. Se consideran vecinas todas las celdas que le rodean en
cada una de las 8 direcciones.

La funcion ``GameGridGetAlivedNeighbors`` retorna el número de vecinos de una celda dadas sus coordenadas.

```c++
inline int GameGridGetAlivedNeighbors(const int x, const int y) {
  int result = 0;
  for (int i = 0; i < 8; i++) {
    const auto disp = CELL_DISPLACEMENT[i];
    const int dx = disp[0];
    const int dy = disp[1];
    int new_x = x;
    int new_y = y;
    GameGridGetNeighborCoord(dx, dy, new_x, new_y);
    if (!GameCoordsEqual(x, y, new_x, new_y) && grid[new_x][new_y]) {
      ++result;
    }
  }
  return result;
}
```

En esta función ``CELL_DISPLACEMENT`` corresponde a un arreglo bidimensional que nos ayuda a calcular
la posición de cada vecino desplazando nuestra coordenada por cada una de las 8 direcciones.

``GameGridGetNeighborCoord`` calcula dicha coordenada aplicando el desplazamiento contenido en ``CELL_DISPLACEMENT``.
Esta función se encarga además de ignorar las celdas que están fuera de la representación en memoria. Si se solicita 
una celda que no existe los argumentos ``x`` y ``y``, pasados por referencia, se mantienen intactos. Por tanto 
``GameGridGetAlivedNeighbors`` solo considera celdas diferentes a la celda actual. 

La siguiente función (``GameGridGetAction``) recibe un parámetro ``state`` con el estado de la celda y el número 
de vecinos encontrados. El valor de retorno corresponde con una constante que determina si 
la celda debe pasar a viva, muerta o no debe ejecutarse ninguna acción.

```c++
// Determina la accion que debe ejecutarse sobre una celda
inline int GameGridGetAction(const bool state, const int neighbors) {
    if (state && (neighbors > 3 || neighbors < 2)) {
        return ACTION_DIE;
    } else if (!state && (3 == neighbors)) {
        return ACTION_BORN;
    }
    return ACTION_NONE;
}
```

Cada paso del juego implica evaluar todas las celdas simultáneamente. Es decir, no existe un orden para evaluarlas,
no importa si comenzamos desde la primera y terminamos en la última o viceversa. En realidad decimos
que todas las celdas se evaluan simultáneamente, porque, en lugar de aplicar la acción sobre la celda 
inmediatamente de su evaluación, lo que hacemos es acumular a modo de comandos cada una de las acciones con sus 
respectivas coordenadas. Estos comandos son ejecutados, si y solo si, todas las celdas han sido evaluadas. 

Contamos con un arreglo bidimensional con espacio suficiente para almacenar un comando para cada celda de la rejilla.

``int actionStack[500 * 500][3];``

Para saber el número de comandos que deben aplicarse contamos con una variable a la que se le asigna ``0``
al comienzo de cada paso y al final de todas las evaluaciones contiene el número total de comandos que fueron
escritos en ``actionStack``. De este modo usamos ``actionStack`` como un ``vector`` sin hacer uso de estructuras o 
clases.

El siguiente fragmento de código pertenece a la función encargada de modificar el estado de nuestro juego para llevarlo 
al siguiente paso, ``GameNextStep``:

Siempre que debe realizarse una acción esta es depositada en ``actionStack``
ocupando la posición actual en ``actionStack`` con un arreglo de tres elementos
donde cada posición corresponde a las coordenadas en la representación en memoria 
y la acción que debe ejecutarse. 

```c++
  actionStack[actionStackSize][0] = x;
  actionStack[actionStackSize][1] = y;
  actionStack[actionStackSize][2] = action;
  ++actionStackSize;
```

### Funciones para el manejo de la consola
Ofrecemos una experiencia superior a la que se lograría usando solamente ``std::cout`` 
y ``std::cin``. Contamos con varias llamadas primitivas que nos permiten optimizar la entrada
por el teclado y la salida en pantalla en nuestra aplicación: 

```c++
void ConsoleGetSize(int &rows, int &cols);
```
Es capaz de determinar la cantidad exacta de filas y columnas de la consola abierta. ``ConsoleGetRows``
es provista por conveniencia y solamente retorna las filas. 

```c++
void ConsoleClearScreen();
```
Limpia todo el contenido del buffer de la consola. Nos permite simular un cambio de pantalla en los diferentes 
menus.

```c++
void ConsoleMaximize()
```
Es responsable de que nuestra aplicación inicie maximizada y emplee todo el espacio disponible en pantalla.

```c++
void ConsoleWriteAt(const short x, const short y, char character);
```
Es una función primitiva para escribir un caracter en una posición específica. Resulta útil para reducir el 
número de caracteres que deben escribirse para dibujar un estado de la rejilla. En lugar de dibujar todas las 
celdas visibles contar con esta función en ``ConsoleFillSquare`` y ``ConsoleEmptySquare`` nos permite 
actualizar solo las celdas que sufren cambios. 

```c++
void ConsoleClearLine(const int x, int size);
```
Es capaz de limpiar solo una línea de la consola. Es útil al dibujar el ``statusbar`` o ``footer`` del juego, debido
a que en este caso solo queremos limpiar la última línea de la pantalla y mantener el resto intacto con el estado 
del buffer del juego o del editor de escenario. 

```c++
// Leer un caracter de la consola manera asíncrona
bool ConsoleGetInputAsync(char &chr, WORD &vk);
```

Esta función es la responsable de que podamos interpretar la entrada del usuario sin necesidad de utilizar el modo 
clásico de introducir un valor y presionar ``enter``, como haríamos con ``std::cin``. En lugar de eso podemos procesar
la simple presión de una tecla, por ejemplo cuando nos desplazamos por la pantalla en el editor de escenario empleando
las teclas de dirección o cuando salimos de una pantalla presionando ``Q``. Esta función devuelve ``true`` si el usuario 
ha presiona una tecla. 

Veamos un ejemplo en la función ``bool GameProcessSimulationInput()`` encargada de procesar la entrada específicamente 
para la pantalla donde visualizamos la simulación: 

```c++
bool GameProcessSimulationInput() {
  char chr;
  WORD vk;
  bool stop = false;
  bool has_input = ConsoleGetInputAsync(chr, vk);
  if (has_input) {
    switch (chr) {
    case 'q':
    case 'Q':
      return true;
    case '+':
      config_speed = min(config_speed + 1, 10);
      break;
    case '-':
      config_speed = max(config_speed - 1, 1);
    }
  }
  return false;
}
```

Al llamar ``GameProcessSimulationInput()`` en un ciclo infinito logramos capturar la entrada del usuario y 
responder a ella si es aplicable sin importar en qué momento se presione la tecla. Cuando la función es llamada 
y ninguna tecla ha sido presionada, simplemente no ocurre nada.

### Efecto de continuidad en la simulación y aumento de la velocidad 
En lugar de solicitar de mostrar al usuario solamente el paso que desea visualizar escogimos brindar el efecto de 
continuidad en la misma, es decir, podemos visualizar un número finito o infinito de pasos a lo largo del tiempo. 

Esto lo logramos gracias a una función llamada ``GameTick``. 

```c++
// Retorna true si el juego debe redibujarse de acuerdo a la
// velocidad actual
bool GameTick() {
  if ((clock() - clock_ticks) < CLOCKS_PER_SEC / (10 * config_speed))
    return false;
  GameResetClock();
  return true;
}
```

A simple vista parece complicada, así que vamos a analizarla
línea por línea.

La función clock() es una función de la librería estándar y su valor de retorno depende 
en gran manera de la implementación ofrecida por el compilador. Lo que es importante saber
es que el valor retornado corresponde a la cantida de tiempo transcurrido desde un punto 
de referencia en la línea de tiempo hasta el momento en que la función fue llamada. No es necesario saber cual es momento exacto
de refencia (podría ser un instante dado desde el año 1970), incluso la unidad de tiempo exacta para 
representar este valor es poco relevante para nuestros fines, así que simplemente los llamaremos ``clocks``. 
Pero podemos saber cuántos ``clocks`` corresponde a un 1 segundo dividiendo la cantidad de ``clocks`` obtenidos
por la constante ``CLOCKS_PER_SEC``. 

En nuestra implementación ``GameResetClock()`` guarda la cantidad de ``clocks`` actual 
en la variable global ``clock_ticks``.

Luego simplemente volvemos a llamar la funcion ``clock()`` y calculamos la diferencia entre este nuevo valor y 
el anterior. Esto no es mas que conocer el números de ``clocks`` transcurridos desde la última llamada a ``GameResetClock()``.
Si aun no hemos alcanzado un intervalo cuya duración corresponde al tiempo que deseamos esperar antes de recalcular 
y redibujar el juego simplemente retornamos false:

```c++
if ((clock() - clock_ticks) < CLOCKS_PER_SEC / (10 * config_speed))
    return false;
```

La expresión ``CLOCKS_PER_SEC / (10 * config_speed)`` con la que estamos comparando corresponde al tiempo que deseamos 
esperar y se interpreta como un fracción de segundo que puede ser mayor o menor si ``config_speed`` aumenta o disminuye.
Entre mayor sea ``config_speed`` menor será la duración (en una división a mayor divisor, menor será el cociente). 
``config_speed`` es precisamente la variable que aumentamos o disminuimos cuando presionamos las telcas ``+`` y ``-``
en plena simulación.

```c++
  GameResetClock();
  return true;
```

Luego, si el intervalo corresponde al tiempo que deseamos esperar o lo supera reasignamos un valor a ``clock_ticks`` y 
retornamos ``true``. 

Esto significa que la mayoría de la veces que llamemos ``GameTick()`` esta retornará false. Mientras que unas pocas veces,
retornará ``true`` indicando que podemos redibujar el juego. 

Observe la siguiente línea en ``GameRun()``: 

```c++
  // Tiempo de dibujar?
    if (!GameTick())
      continue;
```



