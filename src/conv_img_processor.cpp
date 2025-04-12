//conv_img_processor.cpp

#include "conv_img_processor.h"
#include "../buddy_system/stb_image.h"
#include "../buddy_system/stb_image_write.h"
#include <cmath>
#include <iostream>

constexpr unsigned char RELLENO = 0;

ConvImagen* cargar_imagen_conv(const std::string& ruta) {
    int ancho, alto, canales;
    unsigned char* buffer = stbi_load(ruta.c_str(), &ancho, &alto, &canales, 0);
    if (!buffer) {
        std::cerr << "Error al cargar imagen: " << ruta << std::endl;
        return nullptr;
    }

    auto* img = new ConvImagen{ancho, alto, canales, nullptr};
    img->pixeles = new unsigned char**[alto];

    int idx = 0;
    for (int y = 0; y < alto; ++y) {
        img->pixeles[y] = new unsigned char*[ancho];
        for (int x = 0; x < ancho; ++x) {
            img->pixeles[y][x] = new unsigned char[canales];
            for (int c = 0; c < canales; ++c) {
                img->pixeles[y][x][c] = buffer[idx++];
            }
        }
    }

    stbi_image_free(buffer);
    return img;
}

void procesar_imagen_conv(ConvImagen* img) {
    std::cout << "Imagen cargada (convencional):" << std::endl;
    std::cout << "Dimensiones: " << img->ancho << " x " << img->alto << std::endl;
    std::cout << "Canales: " << img->canales << std::endl;
}

void liberar_imagen_conv(ConvImagen* img) {
    for (int y = 0; y < img->alto; ++y) {
        for (int x = 0; x < img->ancho; ++x) {
            delete[] img->pixeles[y][x];
        }
        delete[] img->pixeles[y];
    }
    delete[] img->pixeles;
    delete img;
}

static float radianes(float grados) {
    return grados * M_PI / 180.0f;
}

static unsigned char bilinear(float x, float y, int c, ConvImagen* img) {
    int x0 = floor(x);
    int x1 = x0 + 1;
    int y0 = floor(y);
    int y1 = y0 + 1;

    if (x0 < 0 || x1 >= img->ancho || y0 < 0 || y1 >= img->alto)
        return RELLENO;

    float dx = x - x0;
    float dy = y - y0;

    float v00 = img->pixeles[y0][x0][c];
    float v10 = img->pixeles[y0][x1][c];
    float v01 = img->pixeles[y1][x0][c];
    float v11 = img->pixeles[y1][x1][c];

    float v0 = v00 * (1 - dx) + v10 * dx;
    float v1 = v01 * (1 - dx) + v11 * dx;

    return static_cast<unsigned char>(v0 * (1 - dy) + v1 * dy);
}

void rotar_imagen_conv(ConvImagen* img, int angulo, const std::string& salida) {
    float rad = radianes(static_cast<float>(angulo));
    float cosA = cos(rad);
    float sinA = sin(rad);

    int ancho = img->ancho;
    int alto = img->alto;
    int canales = img->canales;

    int cx = ancho / 2;
    int cy = alto / 2;

    unsigned char* buffer = new unsigned char[ancho * alto * canales];

    for (int y = 0; y < alto; ++y) {
        for (int x = 0; x < ancho; ++x) {
            float xp = x - cx;
            float yp = y - cy;

            float x_orig =  xp * cosA + yp * sinA + cx;
            float y_orig = -xp * sinA + yp * cosA + cy;

            for (int c = 0; c < canales; ++c) {
                buffer[(y * ancho + x) * canales + c] = bilinear(x_orig, y_orig, c, img);
            }
        }
    }

    if (!stbi_write_jpg(salida.c_str(), ancho, alto, canales, buffer, 100)) {
        std::cerr << "Error al guardar imagen rotada: " << salida << std::endl;
    } else {
        std::cout << "Imagen rotada guardada en: " << salida << std::endl;
    }

    delete[] buffer;
}

void escalar_imagen_conv(ConvImagen* img, float factor, const std::string& salida) {
    int nuevoAncho = static_cast<int>(img->ancho * factor);
    int nuevoAlto  = static_cast<int>(img->alto  * factor);
    int canales = img->canales;

    auto*** salidaPix = new unsigned char**[nuevoAlto];
    for (int y = 0; y < nuevoAlto; ++y) {
        salidaPix[y] = new unsigned char*[nuevoAncho];
        for (int x = 0; x < nuevoAncho; ++x) {
            salidaPix[y][x] = new unsigned char[canales];
        }
    }

    for (int y = 0; y < nuevoAlto; ++y) {
        for (int x = 0; x < nuevoAncho; ++x) {
            float srcY = y / factor;
            float srcX = x / factor;

            int y0 = static_cast<int>(srcY);
            int x0 = static_cast<int>(srcX);
            int y1 = std::min(y0 + 1, img->alto - 1);
            int x1 = std::min(x0 + 1, img->ancho - 1);
            float dy = srcY - y0;
            float dx = srcX - x0;

            for (int c = 0; c < canales; ++c) {
                float p00 = img->pixeles[y0][x0][c];
                float p10 = img->pixeles[y0][x1][c];
                float p01 = img->pixeles[y1][x0][c];
                float p11 = img->pixeles[y1][x1][c];

                float val0 = p00 * (1 - dx) + p10 * dx;
                float val1 = p01 * (1 - dx) + p11 * dx;
                salidaPix[y][x][c] = static_cast<unsigned char>(val0 * (1 - dy) + val1 * dy);
            }
        }
    }

    unsigned char* buffer = new unsigned char[nuevoAlto * nuevoAncho * canales];
    int idx = 0;
    for (int y = 0; y < nuevoAlto; ++y)
        for (int x = 0; x < nuevoAncho; ++x)
            for (int c = 0; c < canales; ++c)
                buffer[idx++] = salidaPix[y][x][c];

    stbi_write_jpg(salida.c_str(), nuevoAncho, nuevoAlto, canales, buffer, nuevoAncho * canales);

    std::cout << "Imagen escalada guardada en: " << salida << std::endl;
    std::cout << "Nuevo tamaño: " << nuevoAncho << " x " << nuevoAlto << std::endl;

    delete[] buffer;
    for (int y = 0; y < nuevoAlto; ++y) {
        for (int x = 0; x < nuevoAncho; ++x)
            delete[] salidaPix[y][x];
        delete[] salidaPix[y];
    }
    delete[] salidaPix;
}