# Análisis Comparativo: Buddy System vs new/delete

A continuación se responden las preguntas planteadas sobre el comportamiento de ambos métodos de asignación de memoria en el proyecto de procesamiento de imágenes.

---

## 1. ¿Qué diferencia observaste en el tiempo de procesamiento entre los dos modos de asignación de memoria?

* **new/delete (convencional)** suele ser más rápido para asignaciones grandes y únicas, aprovechando las optimizaciones del allocador del sistema.
* **Buddy System** introduce una pequeña sobrecarga adicional en cada `alloc`/`free` debido a las operaciones de división y fusión de bloques (split/coalesce).
* En imágenes grandes o con muchas operaciones, el Buddy System puede llegar a ser **un 10–20% más lento**, especialmente si el algoritmo realiza numerosas subdivisiones.
* Sin embargo, el Buddy System tiende a tener tiempos **más predecibles** cuando se realizan múltiples asignaciones de tamaños variables.

## 2. ¿Cuál fue el impacto del tamaño de la imagen en el consumo de memoria y el rendimiento?

* Al aumentar el **tamaño de la imagen** (e.g., de 512×512 a 2048×2048):

  * Crece linealmente el buffer requerido (`alto × ancho × canales`).
  * El método convencional reserva un gran bloque seguido, mientras que el Buddy System puede:

    * Dividir bloques grandes en múltiples sub-bloques
    * Mantener fragmentación interna si el tamaño solicitado no es potencia de dos
* **Consumo de memoria**:

  * new/delete: la memoria real asignada suele ser cercana al tamaño solicitado más overhead del runtime.
  * Buddy: siempre reserva bloques de tamaño potencia de dos, provocando **fragmentación interna** que puede elevar el consumo efectivo hasta \~1.5× el tamaño.
* **Rendimiento**:

  * Imágenes muy grandes aumentan tanto el coste de copia de datos como la presión sobre el pool del Buddy, amplificando la latencia de `alloc`/`free`.

## 3. ¿Por qué el Buddy System es más eficiente o menos eficiente que el uso de new/delete en este caso?

* **Más eficiente cuando**:

  * Hay muchas asignaciones y liberaciones de tamaños variados, pues reduce la fragmentación externa.
  * Se desea control fino de un pool de memoria limitado (e.g., no hay swap).
* **Menos eficiente cuando**:

  * Las solicitudes son pocas y de gran tamaño (new/delete usa mmap/heap optimizado).
  * Se incurre en constante *split/coalesce*, añadiendo overhead.

## 4. ¿Cómo podrías optimizar el uso de memoria y tiempo de procesamiento en este programa?

1. **Pool estático para píxeles**: reasignar un único gran bloque y reusar sub-buffers para las distintas operaciones.
2. **Evitar copias innecesarias**: operar *in-place* o con punteros dobles cachés.
3. **Pre-calcular niveles** en Buddy para tamaños típicos (ej. 1024×1024×3).
4. **Paralelizar** aún más con OpenMP, evitando cuellos en asignaciones bloqueadas.
5. **Compresión temporal** de buffers (p. ej. tiling o streaming de regiones).

## 5. ¿Qué implicaciones podría tener esta solución en sistemas con limitaciones de memoria o en dispositivos embebidos?

* **Requisitos de pool**: el Buddy System necesita reservar su `totalSize` al inicio; en dispositivos con <32 MB, habría que reducir el pool.
* **Sin swap**: evita fallos por OOM si el pool es ajustado al máximo disponible.
* **Determinismo**: mejora la predictibilidad de latencias frente a malloc/free del SO.
* **Código más complejo**: aumenta el tamaño binario y la responsabilidad de gestionar correctamente `split/coalesce`.

## 6. ¿Cómo afectaría el aumento de canales (por ejemplo, de RGB a RGBA) en el rendimiento y consumo de memoria?

* Cada píxel pasa de 3 a 4 bytes → **33% más** de buffer.
* Buddy System puede elevar la fragmentación interna: la solicitud de `width×height×4` bytes puede caer en un bloque de potencia de dos mayor.
* El tiempo de procesamiento de cada operación (rotación, interpolación) crecerá proporcionalmente al número de canales.

## 7. ¿Qué ventajas y desventajas tiene el Buddy System frente a otras técnicas de gestión de memoria en proyectos de procesamiento de imágenes?

| Técnica              | Ventajas                                       | Desventajas                                   |
| -------------------- | ---------------------------------------------- | --------------------------------------------- |
| **Buddy System**     | - Fácil fusión/división de bloques.            | - Fragmentación interna (potencias de dos).   |
|                      | - Latencias predecibles en pools limitados.    | - Overhead en operaciones split/coalesce.     |
| **Malloc/Free (SO)** | - Optimizado globalmente para el sistema.      | - Potencial fragmentación externa.            |
|                      | - Manejo transparente al programador.          | - No siempre determinista en tiempo real.     |
| **Pool fijo**        | - Cero overhead de búsqueda (stack, arena).    | - Tamaño rígido; riesgo de overflow.          |
| **Slab Allocator**   | - Muy eficiente para objetos del mismo tamaño. | - No óptimo para tamaños variables (píxeles). |

---