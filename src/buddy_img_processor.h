#ifndef BUDDY_IMG_PROCESSOR_H
#define BUDDY_IMG_PROCESSOR_H

#include "../buddy_system/imagen.h"
#include "buddy_allocator.h"
#include <string>

class ImagenOptimizada {
public:
    ImagenOptimizada(const std::string& ruta, BuddyAllocator* allocator = nullptr);
    ~ImagenOptimizada();
    
    void guardarImagen(const std::string& ruta) const;
    void mostrarInfo() const;
    
    // Funciones optimizadas
    void rotar(int angulo);
    void escalar(float factor);
    
    int getAncho() const { return ancho; }
    int getAlto() const { return alto; }
    int getCanales() const { return canales; }
    
private:
    int ancho;
    int alto;
    int canales;
    unsigned char* buffer;  // Buffer lineal en lugar de matriz 3D
    BuddyAllocator* allocator;
    
    // Métodos de acceso optimizados
    inline unsigned char& pixel(int y, int x, int c) {
        return buffer[(y * ancho + x) * canales + c];
    }
    
    inline const unsigned char& pixel(int y, int x, int c) const {
        return buffer[(y * ancho + x) * canales + c];
    }
    
    // Método para interpolación bilineal optimizada
    unsigned char interpolacion_bilineal(float x, float y, int c) const;
};

// Nuevas funciones optimizadas
ImagenOptimizada* cargar_imagen_buddy_opt(const std::string& ruta);
void procesar_imagen_buddy_opt(ImagenOptimizada* img);
void rotar_imagen_buddy_opt(ImagenOptimizada* img, int angulo, const std::string& salida);
void escalar_imagen_buddy_opt(ImagenOptimizada* img, float factor, const std::string& salida);

#endif // BUDDY_IMG_PROCESSOR_OPTIMIZED_H