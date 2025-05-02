# Image\_Processing-P2\_SO

Proyecto de procesamiento de imágenes en C++ que compara dos métodos de gestión de memoria:

* **Buddy System Optimizado**: nuestro propio `BuddyAllocator` para asignación dinámica.
* **Método Convencional**: uso de `new/delete` y estructuras clásicas.

---

## Características

* Carga, rotación y escalado de imágenes JPEG.
* Comparativa de tiempo y memoria entre Buddy System y método convencional.
* Implementaciones:

  * `buddy_system/`: pool de 32 MB con algoritmo Buddy System.
  * `src/`: interfaz de línea de comandos que elige método y operaciones.

## Prerrequisitos

* GNU Make
* g++ (soporta C++17)
* POSIX (para `posix_memalign` y `getrusage`)

## Estructura de directorios

```bash\ nImage_Processing-P2_SO/
├── buddy_system/       # Código del BuddyAllocator y stubs STB
│   ├── buddy_allocator.h
│   ├── buddy_allocator.cpp
│   ├── stb_wrapper.cpp
│   └── Makefile
├── src/                # Programa principal y procesadores de imagen
│   ├── conv_img_processor.cpp
│   ├── buddy_img_processor.cpp
│   ├── main.cpp
│   └── Makefile
├── img/                # Imágenes de prueba (testImg01.jpg, testImg02.jpg)
└── README.md           # Este documento
```

## Compilación

1. **Compilar el módulo Buddy System**

   ```bash
   cd buddy_system
   make
   ```

2. **Compilar el programa principal**

   ```bash
   cd ../src
   make
   ```

> El `Makefile` de `src/` invoca automáticamente `make -C ../buddy_system`.

## Uso

Desde `src/`, tras compilar, ejecuta:

```bash
./Parcial2_Danna <entrada.jpg> <salida.jpg> [opciones]
```

### Opciones

* `-angulo N` : rota la imagen N grados (entero).
* `-escalar F`: escala la imagen por factor F (0.1–4.0).
* `-buddy`    : usa Buddy System en lugar de new/delete.

### Ejemplos

1. **Modo convencional, sólo rotación**

   ```bash
   ./Parcial2_Danna ../img/testImg01.jpg salida_rotada.jpg -angulo 90
   ```

2. **Buddy System, rotar 45° y escalar 1.5×**

   ```bash
   ./Parcial2_Danna ../img/testImg01.jpg salida.jpg -angulo 45 -escalar 1.5 -buddy
   ```

## Estadísticas y limpieza

* Tras la ejecución, el programa imprime tiempo (ms) y memoria (MB) usados.
* Para limpiar objetos y binarios:

  ```bash
  cd buddy_system && make clean
  cd ../src      && make clean
  ```

## Licencia

Este proyecto se distribuye bajo licencia MIT. Consulta los archivos `stb_image.h` y `stb_image_write.h` para sus respectivas condiciones.

---

¡Listo! Ahora tienes un README completo en Markdown para tu proyecto de sistemas operativos y procesamiento de imágenes.
