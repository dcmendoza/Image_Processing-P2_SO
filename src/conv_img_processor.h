//conv_img_processor.h

#ifndef CONV_IMG_PROCESSOR_H
#define CONV_IMG_PROCESSOR_H

#include <string>

struct ConvImagen {
    int ancho;
    int alto;
    int canales;
    unsigned char*** pixeles;
};

ConvImagen* cargar_imagen_conv(const std::string& ruta);
void procesar_imagen_conv(ConvImagen* img);
void liberar_imagen_conv(ConvImagen* img);
void rotar_imagen_conv(ConvImagen* img, int angulo, const std::string& salida);
void escalar_imagen_conv(ConvImagen* img, float factor, const std::string& salida);

#endif