// Referencias
// https://www.asciitable.com/

#include <iostream>
#include <stdlib.h>
#include <string>
#include <windows.h>
#include <ctime>

using namespace std;

#define ENABLE_VIRTUAL_TERMINAL_INPUT 0x0200
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004

clock_t clock_ticks;

const int ACTION_DIE = 1;
const int ACTION_BORN = 2;
const int ACTION_NONE = 3;

const int MODE_SIMULATION = 1;
const int MODE_EDITOR = 2;

const int CELL_DISPLACEMENT[8][2] = {{-1, -1}, {-1, 0}, {-1, 1}, {0, -1},
                                     {0, 1},   {1, -1}, {1, 0},  {1, 1}};

bool grid[500][500];
int actionStack[500 * 500][3];
int actionStackSize;
int g_current_step = 1;

int x_start = 0;
int y_start = 0;
int x_end = 0;
int y_end = 0;

// Configuracion
int config_steps = -1;
int config_rows = 39;
int config_cols = 100;
int config_speed = 1;

// Editor / Simulacion (Valor MODE_*)
int config_mode = -1;

int editor_pos_x = 1;
int editor_pos_y = 1;

// Memoria utilizada por el editor
int editor_buffer_rows;
int editor_buffer_cols;
bool **editor_buffer = NULL;

int CANNON[36][2] = {{1, 5},  {1, 6},  {2, 5},  {2, 6},  {11, 5}, {11, 6},
                     {11, 7}, {12, 4}, {12, 8}, {13, 3}, {13, 9}, {14, 3},
                     {14, 9}, {15, 6}, {16, 4}, {16, 8}, {17, 5}, {17, 6},
                     {17, 7}, {18, 6}, {21, 3}, {21, 4}, {21, 5}, {22, 3},
                     {22, 4}, {22, 5}, {23, 2}, {23, 6}, {25, 1}, {25, 2},
                     {25, 6}, {25, 7}, {35, 3}, {35, 4}, {36, 3}, {36, 4}};

COORD coord;
HANDLE output_handler = GetStdHandle(STD_OUTPUT_HANDLE);
HANDLE input_handler = GetStdHandle(STD_INPUT_HANDLE);

DWORD inMode;
DWORD outMode;
BOOL _result1 = GetConsoleMode(input_handler, &inMode);
BOOL _result2 = GetConsoleMode(output_handler, &outMode);

DWORD original_console_mode;
BOOL _result = GetConsoleMode(input_handler, &original_console_mode);

// Imprime en pantalla las celdas activas y termina el programa con 0
void GameDebug();

// Mata todas las celdas de la rejilla
void GameGridClear();

// Retorna la cantidad de vecinos vivos de una celda
int GameGridGetAlivedNeighbors(const int x, const int y);

// Retorna una coordenada aplicando un desplazamiento
void GameGridGetNeighborCoord(const int dx, const int dy, int &x, int &y);

// Avanza al siguiente paso
void GameNextStep();

// Determina la accion que debe ejecutarse sobre una celda
int GameGridGetAction(const bool state, const int neighbors);

// Determina si un par de coordenadas son iguales
int GameCoordsEqual(int x1, int y1, int x2, int y2);

// Establece la region que debe renderizarse del juego
void GameSetViewPort(int view_rows, int view_cols);

// Simulacion continua
void GameRun();

// Traslada un par de coordenadas a las coordenadas del mundo
void GameViewToWorld(int &x, int &y);

// Toma un arreglo bidimencional y provoca que todas las
// celdas contenidas en el arreglo nazcan
void GameLoadData(int data[][2], const int size, bool truncate);

// Dibuja en pantalla el estado actual del juego
void GameRender();

// Dibuja la barra de estado
void GameRenderStatusBar();

// Establece el alto y ancho del juego de manera automatica
void GameAutoSize();

// Reinicia el reloj actual
void GameResetClock();

// Retorna true si el juego debe redibujarse de acuerdo a la
// velocidad actual
bool GameTick();

// Determina la cantidad de filas de la consola
void ConsoleGetSize(int &rows, int &cols);

// Retorna la cantidad de filas de la consola
int ConsoleGetRows();

// Oculta el promp de la consola
void ConsoleEnterRenderMode();

// Restaura la consola a su modo inicial
void ConsoleRestoreMode();

// Limpia la pantalla
void ConsoleClearScreen();

// Maximiza la aplicacion
void ConsoleMaximize();

// Imprime un par de coordenadas en pantalla (para depuracion)
void ConsolePrintCoord(int x, int y);

// Escribe un caracter en una posicion del buffer de la consola
void ConsoleWriteAt(const short x, const short y, char character);

void ConsoleWriteFooter(const string str);

// Escribe un rectangulo perfecto en la consola
int ConsoleWriteSquare(const short x, const short y, char character);

// Escribe un rectangulo perfecto en la consola
int ConsoleWriteSquare(const short x, const short y, char character);

// Escribe una cadena en una posicion del buffer de la consola
void ConsoleWriteStringAt(const int x, const int y, string str);

// Limpia el contenido de una linea
void ConsoleClearLine(const int x, int size);

// Rellena un rectangulo perfecto en la consola
int ConsoleFillSquare(const short x, const short y);

// Vacía un rectangulo perfecto en la consola
void ConsoleEmptySquare(const short x, const short y);

// Leer un caracter de la consola manera asíncrona
bool ConsoleGetInputAsync(char &chr, WORD &vk);

// Espera la entrada de un entero
inline int NumberInput(int min, int max, string title, string errorMessage);

// Espera la entrada de una opcion
int Select(const string title, const string *options, const int size);

// Muestra el menu principal
bool MenuMain();

// Configurar rejilla
void MenuRejilla();

// Configurar cantidad de pasos
void MenuPasos();

// Imprime en pantalla las celdas activas y termina el programa con 0
void GameDebug() {
  int n = 0;

  cout << "Coordenadas iniciales: ";
  ConsolePrintCoord(x_start, y_start);

  cout << "Coordenadas finales: ";
  ConsolePrintCoord(x_end, y_end);

  cout << "Celdas activas: ";
  for (int y = y_start; y <= y_end; y++) {
    for (int x = x_start; x <= x_end; x++) {
      if (grid[x][y]) {
        ConsolePrintCoord(x, y);
        cout << endl;
      }
    }
  }

  // exit(0);
}

// Mata todas las celdas de la rejilla
inline void GameGridClear() {
  for (int x = 0; x < 500; x++) {
    for (int y = 0; y < 500; y++) {
      grid[x][y] = false;
    }
  }
}

// Retorna la cantidad de vecinos vivos de una celda
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

// Retorna una coordenada aplicando un desplazamiento
inline void GameGridGetNeighborCoord(const int dx, const int dy, int &x,
                                     int &y) {
  bool out_of_range = ((0 == y) && -1 == dy) || ((0 == x) && -1 == dx) ||
                      ((500 == (y + 1)) && (1 == dy)) ||
                      ((500 == (x + 1)) && (1 == dx));
  if (!out_of_range) {
    y += dy;
    x += dx;
  }
}

// Retorna true si las coordenadas estan dentro de la consola.
bool ConsoleIsCoordInsideConsole(const int x, const int y) {
  return ((x * 2 - 1) <= config_cols && y <= config_rows);
}

// Avanza al siguiente paso
void GameNextStep() {
  actionStackSize = 0;
  for (int x = 0; x < 500; x++) {
    for (int y = 0; y < 500; y++) {
      auto state = grid[x][y];
      int neighbors = GameGridGetAlivedNeighbors(x, y);
      int action = GameGridGetAction(state, neighbors);

      if (ACTION_NONE == action)
        continue;

      actionStack[actionStackSize][0] = x;
      actionStack[actionStackSize][1] = y;
      actionStack[actionStackSize][2] = action;
      ++actionStackSize;

      int console_x = x - x_start;
      int console_y = y - y_start;
      if (ACTION_BORN == action) {
        ConsoleFillSquare(console_x, console_y);
      } else {
        ConsoleEmptySquare(console_x, console_y);
      }
    }
  }

  int cpy_actionStackSize = actionStackSize;
  while (cpy_actionStackSize--) {
    auto action_item = actionStack[cpy_actionStackSize];
    int x = action_item[0];
    int y = action_item[1];
    int action = action_item[2];
    grid[x][y] = ACTION_BORN == action;
  }

  ++g_current_step;
  GameRenderStatusBar();
}

// Determina la accion que debe ejecutarse sobre una celda
inline int GameGridGetAction(const bool state, const int neighbors) {
  if (state && (neighbors > 3 || neighbors < 2)) {
    return ACTION_DIE;
  } else if (!state && (3 == neighbors)) {
    return ACTION_BORN;
  }
  return ACTION_NONE;
}

// Determina si un par de coordenadas son iguales
inline int GameCoordsEqual(int x1, int y1, int x2, int y2) {
  return (x1 == x2) && (y1 == y2);
}

// Dibuja en pantalla el estado actual del juego
void GameRender() {
  g_current_step = 1;
  // Limpiamos antes de dibujar, así solo
  // debemos dibujar las vivas. El resto está muerto :)
  ConsoleClearScreen();
  // Coordenada en la consola basada en cero
  int screen_x = 0;
  // Solo se dibuja lo que es visible
  for (int y = y_start; y <= y_end; y++) {
    for (int x = x_start; x <= x_end; x++) {
      const auto state = grid[x][y];
      if (state) {
        // Dibujamos coordenadas basadas en 0
        ConsoleFillSquare(x - x_start, y - y_start);
      }
    }
  }
  // Dibujamos barra de estado
  GameRenderStatusBar();
}

// Dibuja en pantalla el estado actual del juego
void GameRenderStatusBar() {
  string current_step_str = to_string(g_current_step);
  string status_bar = "Salir: Q | Velocidad: (+/-) | Paso: " + current_step_str;
  ConsoleWriteFooter(status_bar);
}

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

// Renderiza los estados consecutivos del juego.
void GameRun(int steps = -1) {
  ConsoleEnterRenderMode();

  DWORD event_count = 0;

  GameResetClock();

  bool wait_exit = steps > -1;
  bool first_time = true;
  while ((steps == -1) || (steps > 0)) {

    // Tiempo de dibujar?
    if (!GameTick())
      continue;

    // Es la primera vez que dibujamos?
    if (first_time) {
      first_time = false;
      // Dibujar todos los elementos visibles
      // GameDebug();
      GameRender();
    } else {
      // Dibujar solo los elementos alterados
      GameNextStep();
    }

    // Reiniciamos la marca de timepo
    GameResetClock();

    // Decrementamos los pasos restantes si es necesario
    if (steps != -1) {
      --steps;
    }

    // Verificamos si el usuario ha introducido algun caracter
    // desde el teclado
    if (GameProcessSimulationInput())
      break;
  }

  // Solo nos detenemos si el usuario presiona 'q'
  if (wait_exit) {
    bool stop;
    while (true) {
      stop = GameProcessSimulationInput();
      if (stop)
        break;
    }
  }

  // Restablecemos el modo de la consola y limpiamos la pantalla
  ConsoleRestoreMode();
  ConsoleClearScreen();
}

// Traslada un par de coordenadas a las coordenadas del mundo
void GameViewToWorld(int &x, int &y) {
  y += y_start;
  x += x_start;
}

void GameLoadData(int data[][2], const int size, bool truncate = true) {
  if (truncate) {
    GameGridClear();
  }
  for (int i = 0; i < size; i++) {
    int *coord = data[i];
    int x = coord[0];
    int y = coord[1];
    GameViewToWorld(x, y);
    grid[x][y] = true;
  }
}

// Establece el alto y ancho del juego de manera automatica
void GameAutoSize() {
  ConsoleGetSize(config_rows, config_cols);
  config_rows -= 5;
}

// Reinicia el reloj actual
void GameResetClock() { clock_ticks = clock(); }

// Retorna true si el juego debe redibujarse de acuerdo a la
// velocidad actual
bool GameTick() {
  if ((clock() - clock_ticks) < CLOCKS_PER_SEC / (10 * config_speed))
    return false;
  GameResetClock();
  return true;
}

// Vacía un rectangulo perfecto en la consola
void ConsoleEmptySquare(const short x, const short y) {
  ConsoleWriteSquare(x, y, ' ');
}

// Leer un caracter de la consola manera asíncrona
bool ConsoleGetInputAsync(char &chr, WORD &vk) {
  DWORD event_count;
  if (GetNumberOfConsoleInputEvents(input_handler, &event_count)) {
    if (event_count) {
      INPUT_RECORD record;
      DWORD read;
      ReadConsoleInput(input_handler, &record, 1, &read);
      if (KEY_EVENT == record.EventType && !record.Event.KeyEvent.bKeyDown) {
        chr = record.Event.KeyEvent.uChar.AsciiChar;
        vk = record.Event.KeyEvent.wVirtualKeyCode;
        return true;
      }
    }
  }
  return false;
}

// Espera la entrada de un entero
inline int NumberInput(int min, int max, string title, string errorMessage) {
  while (true) {
    int input;
    cout << title << endl;
    cout << ">>> ";
    cin >> input;
    if (!cin) {
      cin.clear();
      string line;
      getline(cin, line);
    } else {
      if ((-1 == min || input >= min) && (-1 == max || input <= max)) {
        return input;
      }
    }
    if (errorMessage.size()) {
      cout << errorMessage << endl;
    } else {
      cout << "Introduce un valor dentro del rango (" << min << "," << max
           << ")" << endl;
    }
  }
}

// Espera la entrada de una opcion
int Select(const string title, const string *options, const int size) {
  int option;
  while (true) {
    cout << title << endl;
    for (int i = 0; i < size; i++) {
      cout << "[" << i + 1 << "]"
           << " - " << options[i] << endl;
    }
    cout << ">>> ";
    cin >> option;
    if (!cin) {
      cin.clear();
      string line;
      std::getline(cin, line);
    } else {
      if (option >= 1 && (option <= size)) {
        return option;
      }
    }
    cout << "[!] Debes introducir un elemento del menu" << endl;
  }
}

bool Confirm(string msg) {
  const string options[] = {"Si", "No"};
  int selection = Select(msg, options, 2);
  return 1 == selection;
}

void ConsoleEnableVirtualMode() {
  SetConsoleMode(input_handler, inMode |= ENABLE_VIRTUAL_TERMINAL_INPUT);
  SetConsoleMode(output_handler, outMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

void ConsoleRestoreMode() {
  SetConsoleMode(input_handler, inMode);
  SetConsoleMode(output_handler, outMode);
}

void ConsoleEnterRenderMode() {
  // ConsoleEnableVirtualMode();
  SetConsoleMode(input_handler, ENABLE_VIRTUAL_TERMINAL_INPUT);
  SetConsoleMode(output_handler,
                 ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

void ConsoleGetSize(int &rows, int &cols) {
  CONSOLE_SCREEN_BUFFER_INFO info;
  GetConsoleScreenBufferInfo(output_handler, &info);
  cols = info.srWindow.Right - info.srWindow.Left + 1;
  rows = info.srWindow.Bottom - info.srWindow.Top + 1;
}

// Retorna la cantidad de filas de la consola
int ConsoleGetRows() {
  int rows, cols;
  ConsoleGetSize(rows, cols);
  return rows - 1;
}

// Limpia la pantalla y coloca el cursor al comienzo de la pantalla
void ConsoleClearScreen() {
  system("cls");
  // cout << "\x1b[2J\x1b[1;1H";
}

void ConsoleMaximize() {
  HWND console_window = GetConsoleWindow();
  ShowWindow(console_window, SW_SHOWMAXIMIZED);
}

// Imprime un par de coordenadas en pantalla (para depuracion)
void ConsolePrintCoord(int x, int y) {
  cout << "x: " << x << " | y: " << y << endl;
}

// Escribe un caracter en la consola
void ConsoleWriteAt(const short x, const short y, char character) {
  DWORD written;
  FillConsoleOutputCharacter(output_handler, character, 1, {x, y}, &written);
}

void ConsoleWriteFooter(const string str) {
  const auto size = str.size();
  int rows = ConsoleGetRows();
  for (int i = 0; i < size; i++) {
    ConsoleWriteAt(i, rows, str[i]);
  }
}

// Escribe un rectangulo perfecto en la consola
int ConsoleWriteSquare(const short x, const short y, char character) {
  int x_start = (x * 2) - 1;
  ConsoleWriteAt(x_start, y, character);
  ConsoleWriteAt(x_start + 1, y, character);
  return x_start + 1;
}

// Escibre una cadena en una posicion del buffer de la consola
void ConsoleWriteStringAt(const int x, const int y, string str) {
  int cur_x = x;
  int size = str.size();
  for (int i = 0; i < size; i++) {
    ConsoleWriteAt(cur_x++, y, str[i]);
  }
}

void ConsoleClearLine(const int y, int size = -1) {
  if (size == -1) {
    int rows, cols;
    ConsoleGetSize(rows, cols);
    size = cols;
  }
  string str(size, ' ');
  ConsoleWriteStringAt(1, y, str);
}

// Rellena un rectangulo perfecto en la consola
int ConsoleFillSquare(const short x, const short y) {
  return ConsoleWriteSquare(x, y, (char)219);
}

bool MenuMain() {
  string opciones[] = {"Iniciar simulacion", "Configurar rejilla",
                       "Cantidad de pasos", "Editor de escenario", "Salir"};

  while (true) {
    ConsoleClearScreen();
    cout << "Juego de la vida v1.0.3" << endl;
    cout << "=======================" << endl;
    int opcion = Select("Menu principal", opciones, 5);
    switch (opcion) {
    case 1: // Iniciar simulación

      config_mode = MODE_SIMULATION;
      return false;
    case 2: // Configurar rejilla
      MenuRejilla();
      break;
    case 3: // Configurar cantidad de pasos
      MenuPasos();
      break;
    case 4: // Editor de escenario
      config_mode = MODE_EDITOR;
      return false;
    case 5:
      return true;
    }
  }
}

void MenuRejilla() {
  int input;
  while (true) {
    string filasConfig = "Filas [" + std::to_string(config_rows) + "]";
    string columnasConfig = "Columnas [" + std::to_string(config_cols) + "]";

    string opciones[] = {filasConfig, columnasConfig,
                         "Configuracion automatica", "Atras"};
    ConsoleClearScreen();
    int opcion = Select("Configuracion de rejilla", opciones, 4);

    switch (opcion) {
    case 1: // Configurar filas
      config_rows = NumberInput(1, 500, "Filas:", "");
      editor_pos_y = min(config_rows - 1, editor_pos_y);
      break;
    case 2: // Configurar columnas
      config_cols = NumberInput(1, 500, "Columnas:", "");
      editor_pos_x = min(config_cols, editor_pos_x);
      break;
    case 3: // Configuracion automatica
      GameAutoSize();
      break;
    case 4: // Atras
      return;
    }
  }
}

// Reserva espacio en memoria para un arreglo bidimensional
// (Clase 12)
bool **MemoryAlloc2DBoolArray(const int rows, const int cols) {
  bool **p = new bool *[rows];
  for (int i = 0; i < rows; i++) {
    p[i] = new bool[cols];
  }
  return p;
}

// Dibuja la barra de estado del editor
void EditorRenderStatusBar() {
  string x_str = to_string(editor_pos_x);
  string y_str = to_string(editor_pos_y);
  string status_bar = "Salir: Q | Guardar: G | Editar: Espacio | x: " + x_str +
                      " | y: " + y_str;
  ConsoleWriteFooter(status_bar);
}

// Mueve el puntero del editor
void EditorMovePointer(int dx, int dy) {

  // Borrar el cursor anterior a menos
  // que la celda este activa en el buffer
  if (!editor_buffer[editor_pos_y][editor_pos_x]) {
    ConsoleEmptySquare(editor_pos_x, editor_pos_y);
  }

  // Nos movemos en X si es posible
  if (dx > 0) {
    editor_pos_x = min(editor_pos_x + 1, config_cols);
  } else if (dx < 0) {
    // No queremos escribir en la columna 0 (se ve feo)
    editor_pos_x = max(1, editor_pos_x - 1);
  }

  // Nos movemos en Y si es posible
  if (dy > 0) {
    // La ultima fila esta reservada para la barra de estado
    // por es eso config_rows - 1
    editor_pos_y = min(editor_pos_y + 1, config_rows - 1);
  } else if (dy < 0) {
    editor_pos_y = max(0, editor_pos_y - 1);
  }

  // Escribir el nuevo cursor, pero solo si la celda no esta
  // activa
  if (!editor_buffer[editor_pos_y][editor_pos_x]) {
    ConsoleFillSquare(editor_pos_x, editor_pos_y);
  }
}

// Libera el buffer del editor
void EditorBufferFree() {
  for (int i = 0; i < editor_buffer_rows; i++) {
    // delete es lo inverso de new
    // new reserva y delete libera memoria
    // Liberamos cada columna
    delete editor_buffer[i];
    editor_buffer[i] = NULL; // No es necesario pero es buena practica
  }
  // Liberamos todas las filas
  delete editor_buffer;
  // Garantizamos error si se trata de acceder sin reservar
  editor_buffer = NULL;
}

// Mata todas las celdas del buffer del editor
void EditorBufferClear() {
  // un chekeo necesario
  if (!editor_buffer) {
    cout << "Error: Buffer del editor sin reservar";
    return;
  }
  // Recorremos todas las filas y columnas
  for (int row = 0; row < editor_buffer_rows; row++) {
    for (int col = 0; col < editor_buffer_cols; col++) {
      editor_buffer[row][col] = false;
    }
  }
}

// Reserva la memoria el buffer del editor de acuerdo con
// la cantidad de filas y columnas de la configuracion
bool EditorBufferAlloc() {
  // Ya existe un buffer, debemos revisarlo
  if (editor_buffer) {
    // Liberamos el buffer anterior si las dimensiones no
    // coinciden
    if ((editor_buffer_cols != config_cols) ||
        (editor_buffer_rows != config_rows)) {
      // Liberamos el buffer anterior
      EditorBufferFree();
    }
  }

  // Creamos un nuevo buffer solo si el anterior fue eliminado
  if (NULL == editor_buffer) {
    // Reservamos un nuevo buffer
    editor_buffer = MemoryAlloc2DBoolArray(config_rows, config_cols);
    editor_buffer_cols = config_cols;
    editor_buffer_rows = config_rows;
    // Matamos todas las celdas sucias
    EditorBufferClear();
    return true;
  }

  return false;
}

// Renderiza el estado inicial del buffer
void EditorBufferRender() {
  for (int y = 0; y < editor_buffer_rows; y++) {
    for (int x = 0; x < editor_buffer_cols; x++) {
      if (editor_buffer[y][x]) {
        ConsoleFillSquare(x, y);
      }
    }
  }
}

// Guarda el contenido del buffer en el mundo
// de la simulacion
void EditorSave() {
  GameGridClear();
  GameSetViewPort(config_rows, config_cols);
  for (int y = 0; y < editor_buffer_rows; y++) {
    for (int x = 0; x < editor_buffer_cols; x++) {
      bool status = editor_buffer[y][x];
      int g_x = x;
      int g_y = y;
      GameViewToWorld(g_x, g_y);
      if (status) {
        grid[g_x][g_y] = true;
      }
    }
  }
}

void EditorRun() {
  // True si se creo un nuevo buffer
  // False si se reutilizo el buffer anterior
  bool allocated;

  // Inicializar memoria
  allocated = EditorBufferAlloc();

  // Usados para capturar flechas
  WORD vk;
  // Usado para capturar caracteres
  char input;
  bool update_status;

  ConsoleEnterRenderMode();
  ConsoleClearScreen();

  // Solo renderizamos si se reutilizo
  // el buffer anterior. Un buffer reservado
  // no tiene contendo, no necesita ser renderizado
  if (!allocated) {
    EditorBufferRender();
  }

  int old_pos_x = editor_pos_x;
  int old_pos_y = editor_pos_y;

  bool salir_editor = false;
  while (true) {
    // Continuar o salir del editor
    if (salir_editor)
      break;
    // Capturamos la entrada del usuario
    if (ConsoleGetInputAsync(input, vk)) {
      // Capturar flechas
      switch (vk) {
      case VK_LEFT:
        EditorMovePointer(-1, 0);
        break;
      case VK_RIGHT:
        EditorMovePointer(1, 0);
        break;
      case VK_UP:
        EditorMovePointer(0, -1);
        break;
      case VK_DOWN:
        EditorMovePointer(0, 1);
        break;
      }

      if (vk) {
        // Redibujar la barra de estado.
        // Porque cambio la posicion del cursor
        EditorRenderStatusBar();
      }

      bool new_state;
      // Capturar caracteres
      switch (input) {
      case 'g':
      case 'G':
        // Guardar y salir
        EditorSave();
        salir_editor = true;
        break;
      case ' ':
        new_state = !editor_buffer[editor_pos_y][editor_pos_x];
        // Cambiar estado de celda por su contrario
        editor_buffer[editor_pos_y][editor_pos_x] = new_state;
        // Dibujar en pantalla de acuerdo a la celda
        if (new_state) {
          ConsoleFillSquare(editor_pos_x, editor_pos_y);
        } else {
          ConsoleEmptySquare(editor_pos_x, editor_pos_y);
        }
        break;
      case 'q':
      case 'Q':
        // Salir del modo de edicion
        salir_editor = true;
        break;
      }
    }
  }

  ConsoleRestoreMode();
}

// Configurar cantidad de pasos
void MenuPasos() {
  while (true) {
    const string infinito = (-1 == config_steps) ? "Activado" : "Desactivado";
    const string finito =
        (-1 != config_steps) ? std::to_string(config_steps) : "Desactivado";

    string opciones[] = {"Fija [" + finito + "]", "Infinita [" + infinito + "]",
                         "Atras"};

    ConsoleClearScreen();

    int opcion = Select("Cantidad de pasos", opciones, 3);
    switch (opcion) {
    case 1: // Configurar cantidad de pasos
      config_steps = NumberInput(1, -1, "Pasos:", "");
      break;
    case 2: // Pasos infinitos
      config_steps = -1;
      break;
    case 3: // Atras
      return;
    }
  }
}

int main() {
  ConsoleMaximize();

  ConsoleEnableVirtualMode();

  ConsoleClearScreen();

  // Calculamos el tamaño de la consola
  GameAutoSize();

  bool salir;
  while (true) {

    // Mostramos el menu principal
    salir = MenuMain();

    if (salir) {
      ConsoleClearScreen();
      if (Confirm("Esta seguro que desea salir de este fabuloso "
                  "juego?")) {
        break;
      } else {
        // Restablecemos el modo o de lo contrario
        // podriamos entrar al ultimo introducido.
        // Regresamos al menu.
        config_mode = -1;
      }
    }

    // Determinar el modo (simulacion/editor de escenario)
    switch (config_mode) {
    case MODE_SIMULATION:
      // Las columnas pudieron cambiar
      GameSetViewPort(config_rows, config_cols);
      // GameLoadData(CANNON, 36);
      GameRun(config_steps);
      break;
    case MODE_EDITOR:
      EditorRun();
      break;
    }
  }

  // Limpiamos la pantalla antes de continuar
  ConsoleClearScreen();

  return 0;
}
