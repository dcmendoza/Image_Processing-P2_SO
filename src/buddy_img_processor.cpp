// buddy_img_processor.cpp
#include "buddy_img_processor.h"
#include "../buddy_system/stb_image.h"
#include "../buddy_system/stb_image_write.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <cstring>

// Crear una instancia global optimizada del allocator
static BuddyAllocator globalAllocator(1024 * 1024 * 256); // 256MB (incrementado para soportar imágenes grandes)

ImagenOptimizada::ImagenOptimizada(const std::string& ruta, BuddyAllocator* allocator)
    : allocator(allocator ? allocator : &globalAllocator) {
    
    // Cargar imagen directamente a un buffer lineal
    buffer = stbi_load(ruta.c_str(), &ancho, &alto, &canales, 0);
    if (!buffer) {
        std::cerr << "Error: No se pudo cargar la imagen '" << ruta << "'.\n";
        exit(1);
    }
    
    // Mostrar información de la imagen cargada
    std::cout << "Imagen cargada: " << ancho << "x" << alto << " con " << canales << " canales.\n";
    
    // Calcular tamaño del buffer
    size_t tamBuffer = ancho * alto * canales;
    std::cout << "Tamaño del buffer: " << tamBuffer << " bytes\n";
    
    // Copiar el buffer a memoria administrada por el buddy allocator
    unsigned char* buddyBuffer = static_cast<unsigned char*>(this->allocator->alloc(tamBuffer));
    if (!buddyBuffer) {
        std::cerr << "Error: No se pudo asignar memoria para el buffer de imagen.\n";
        stbi_image_free(buffer);
        exit(1);
    }
    
    std::memcpy(buddyBuffer, buffer, tamBuffer);
    
    // Liberar el buffer original y usar el buffer del buddy
    stbi_image_free(buffer);
    buffer = buddyBuffer;
}

ImagenOptimizada::~ImagenOptimizada() {
    if (buffer && allocator) {
        allocator->free(buffer);
    }
}

void ImagenOptimizada::guardarImagen(const std::string& ruta) const {
    if (!buffer) {
        std::cerr << "Error: No hay datos de imagen para guardar.\n";
        return;
    }
    
    if (!stbi_write_jpg(ruta.c_str(), ancho, alto, canales, buffer, 95)) {
        std::cerr << "Error: No se pudo guardar la imagen en '" << ruta << "'.\n";
        exit(1);
    }
    std::cout << "[INFO] Imagen guardada correctamente en '" << ruta << "'.\n";
}

void ImagenOptimizada::mostrarInfo() const {
    std::cout << "Dimensiones: " << ancho << " x " << alto << std::endl;
    std::cout << "Canales: " << canales << std::endl;
    std::cout << "Tamaño total: " << (ancho * alto * canales) << " bytes" << std::endl;
}

unsigned char ImagenOptimizada::interpolacion_bilineal(float x, float y, int c) const {
    // Si el punto está fuera de la imagen, devolver negro
    if (x < 0 || x >= ancho - 1 || y < 0 || y >= alto - 1)
        return 0;

    // Coordenadas de los cuatro puntos que rodean la posición
    int x0 = static_cast<int>(x);
    int y0 = static_cast<int>(y);
    int x1 = std::min(x0 + 1, ancho - 1);
    int y1 = std::min(y0 + 1, alto - 1);
    
    // Pesos para la interpolación
    float dx = x - x0;
    float dy = y - y0;

    // Leer los valores de los cuatro puntos
    float p00 = pixel(y0, x0, c);
    float p10 = pixel(y0, x1, c);
    float p01 = pixel(y1, x0, c);
    float p11 = pixel(y1, x1, c);

    // Realizar la interpolación bilineal
    float interp = p00 * (1 - dx) * (1 - dy) + 
                  p10 * dx * (1 - dy) + 
                  p01 * (1 - dx) * dy + 
                  p11 * dx * dy;

    return static_cast<unsigned char>(interp);
}

void ImagenOptimizada::rotar(int angulo) {
    if (angulo == 0) return;  // No hacer nada si el ángulo es 0
    
    std::cout << "Rotando imagen " << angulo << " grados...\n";
    
    float radianes = angulo * M_PI / 180.0f;
    float cosA = cos(radianes);
    float sinA = sin(radianes);
    
    // Punto central de la imagen
    float cx = ancho / 2.0f;
    float cy = alto / 2.0f;
    
    // Reservar buffer para la imagen rotada
    size_t tamBuffer = ancho * alto * canales;
    unsigned char* rotadaBuffer = static_cast<unsigned char*>(allocator->getCache(tamBuffer));
    if (!rotadaBuffer) {
        std::cerr << "Error: No se pudo asignar memoria para la imagen rotada.\n";
        return; // No modificar la imagen si no se puede asignar memoria
    }
    
    // Inicializar buffer con negro
    std::memset(rotadaBuffer, 0, tamBuffer);
    
    // Aplicar rotación a cada pixel
    #pragma omp parallel for collapse(2)
    for (int y = 0; y < alto; ++y) {
        for (int x = 0; x < ancho; ++x) {
            // Coordenadas relativas al centro
            float xr = x - cx;
            float yr = y - cy;
            
            // Aplicar la rotación
            float xp = xr * cosA - yr * sinA + cx;
            float yp = xr * sinA + yr * cosA + cy;
            
            // Copiar los valores de color usando interpolación bilineal
            for (int c = 0; c < canales; ++c) {
                unsigned char valor = interpolacion_bilineal(xp, yp, c);
                rotadaBuffer[(y * ancho + x) * canales + c] = valor;
            }
        }
    }
    
    // Intercambiar los buffers
    std::swap(buffer, rotadaBuffer);
    allocator->releaseCache();
    
    std::cout << "Rotación completada.\n";
}

void ImagenOptimizada::escalar(float factor) {
    if (factor == 1.0f) return;  // No hacer nada si el factor es 1
    
    std::cout << "Escalando imagen por factor " << factor << "...\n";
    
    int nuevoAncho = static_cast<int>(ancho * factor);
    int nuevoAlto = static_cast<int>(alto * factor);
    
    // Calcular y mostrar el nuevo tamaño
    size_t tamBuffer = nuevoAncho * nuevoAlto * canales;
    std::cout << "Nuevo tamaño: " << nuevoAncho << "x" << nuevoAlto 
              << " (" << tamBuffer << " bytes)\n";
    
    // Verificar límites razonables
    if (nuevoAncho <= 0 || nuevoAlto <= 0 || nuevoAncho > 20000 || nuevoAlto > 20000) {
        std::cerr << "Error: Factor de escala " << factor << " produce dimensiones no válidas.\n";
        return;
    }
    
    // Reservar buffer para la imagen escalada
    unsigned char* escaladaBuffer = static_cast<unsigned char*>(allocator->alloc(tamBuffer));
    if (!escaladaBuffer) {
        std::cerr << "Error: No se pudo asignar memoria para la imagen escalada.\n";
        return; // No modificar la imagen si no se puede asignar memoria
    }
    
    // Inicializar buffer con negro
    std::memset(escaladaBuffer, 0, tamBuffer);
    
    // Aplicar escalado
    #pragma omp parallel for collapse(2)
    for (int y = 0; y < nuevoAlto; ++y) {
        for (int x = 0; x < nuevoAncho; ++x) {
            // Mapear coordenadas
            float srcX = x / factor;
            float srcY = y / factor;
            
            // Copiar los valores de color usando interpolación bilineal
            for (int c = 0; c < canales; ++c) {
                escaladaBuffer[(y * nuevoAncho + x) * canales + c] = interpolacion_bilineal(srcX, srcY, c);
            }
        }
    }
    
    // Actualizar los atributos de la imagen
    void* oldBuffer = buffer;
    buffer = escaladaBuffer;
    ancho = nuevoAncho;
    alto = nuevoAlto;
    
    // Liberar buffer antiguo
    if (oldBuffer) {
        allocator->free(oldBuffer);
    }
    
    std::cout << "Escalado completado.\n";
}

// Implementaciones de las funciones wrapper
ImagenOptimizada* cargar_imagen_buddy_opt(const std::string& ruta) {
    return new ImagenOptimizada(ruta, &globalAllocator);
}

void procesar_imagen_buddy_opt(ImagenOptimizada* img) {
    std::cout << "Imagen cargada (Buddy System Optimizado):" << std::endl;
    img->mostrarInfo();
}

void rotar_imagen_buddy_opt(ImagenOptimizada* img, int angulo, const std::string& salida) {
    img->rotar(angulo);
    img->guardarImagen(salida);
    std::cout << "Imagen rotada (Buddy Optimizado) guardada en: " << salida << std::endl;
}

void escalar_imagen_buddy_opt(ImagenOptimizada* img, float factor, const std::string& salida) {
    img->escalar(factor);
    img->guardarImagen(salida);
    std::cout << "Imagen escalada (Buddy Optimizado) guardada en: " << salida << std::endl;
    std::cout << "Nuevo tamaño: " << img->getAncho() << " x " << img->getAlto() << std::endl;
}