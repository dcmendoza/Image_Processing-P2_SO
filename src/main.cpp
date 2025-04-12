#include <iostream>
#include <string>
#include <cstdlib>
#include "conv_img_processor.h"
#include "buddy_img_processor.h"
#include <chrono>
#include <sys/resource.h>
#include <cstdio>
#include "../buddy_system/stb_image.h"

long memoria_actual_kb() {
    struct rusage uso;
    getrusage(RUSAGE_SELF, &uso);
    return uso.ru_maxrss;
}

long tiempo_ms(std::chrono::steady_clock::time_point ini, std::chrono::steady_clock::time_point fin) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(fin - ini).count();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0] << " <entrada.jpg> [salida.jpg] [-angulo N] [-escalar F] [-buddy]" << std::endl;
        return 1;
    }

    std::string entrada = argv[1];
    std::string salida = "";
    int angulo = 0;
    float factorEscala = 1.0f;
    bool tieneSalida = false;
    bool tieneAngulo = false;
    bool tieneEscala = false;
    bool usarBuddy = false;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-buddy") {
            usarBuddy = true;
        } else if (arg == "-angulo" && i + 1 < argc) {
            angulo = std::stoi(argv[++i]);
            tieneAngulo = true;
        } else if (arg == "-escalar") {
            tieneEscala = true;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                factorEscala = std::stof(argv[++i]);
            } else {
                std::cout << "Ingrese el factor de escalado (0.1 - 4.0): ";
                std::cin >> factorEscala;
            }
        } else if (salida.empty()) {
            salida = arg;
            tieneSalida = true;
        }
    }

    if (!tieneSalida) {
        std::cout << "Ingrese el nombre del archivo de salida: ";
        std::cin >> salida;
    }

    if (!tieneAngulo && !tieneEscala) {
        std::cout << "Ingrese el ángulo de rotación (0 para omitir): ";
        std::cin >> angulo;
        tieneAngulo = angulo != 0;

        std::cout << "¿Desea escalar la imagen? (s/n): ";
        char resp;
        std::cin >> resp;
        if (resp == 's' || resp == 'S') {
            std::cout << "Ingrese el factor de escalado (0.1 - 4.0): ";
            std::cin >> factorEscala;
            tieneEscala = true;
        }
    }

    if (factorEscala <= 0.0f || factorEscala > 4.0f) {
        std::cerr << "Error: el factor de escalado debe estar entre 0.1 y 4.0" << std::endl;
        return 1;
    }

    bool fueRotada = false;
    bool fueEscalada = false;
    int ancho = 0, alto = 0, canales = 0;
    long tiempo = 0;
    long memoria = 0;

    std::cout << "\n=== PROCESAMIENTO DE IMAGEN ===\n";
    std::cout << "Archivo de entrada: " << entrada << "\n";
    std::cout << "Archivo de salida: " << salida << "\n";
    std::cout << "Modo de asignación de memoria: " << (usarBuddy ? "Buddy System Optimizado" : "Convencional") << "\n";
    std::cout << "------------------------\n";

    auto t0 = std::chrono::steady_clock::now();
    long mem0 = memoria_actual_kb();

    if (usarBuddy) {
        ImagenOptimizada* img = cargar_imagen_buddy_opt(entrada);
        if (!img) return 1;

        if (tieneAngulo) {
            rotar_imagen_buddy_opt(img, angulo, tieneEscala ? "__tmp_rotada.jpg" : salida);
            fueRotada = true;
        }
        
        if (tieneEscala) {
            if (tieneAngulo) {
                // Cargar la imagen rotada
                delete img;
                img = cargar_imagen_buddy_opt("__tmp_rotada.jpg");
            }
            escalar_imagen_buddy_opt(img, factorEscala, salida);
            fueEscalada = true;
        }

        ancho = img->getAncho();
        alto = img->getAlto();
        canales = img->getCanales();
        
        delete img;
    } else {
        ConvImagen* img = cargar_imagen_conv(entrada);
        if (!img) return 1;

        if (tieneAngulo) {
            rotar_imagen_conv(img, angulo, tieneEscala ? "__tmp_rotada.jpg" : salida);
            fueRotada = true;
        }
        
        if (tieneEscala) {
            if (tieneAngulo) {
                liberar_imagen_conv(img);
                img = cargar_imagen_conv("__tmp_rotada.jpg");
            }
            escalar_imagen_conv(img, factorEscala, salida);
            fueEscalada = true;
        }

        ancho = img->ancho;
        alto = img->alto;
        canales = img->canales;
        
        liberar_imagen_conv(img);
    }

    auto t1 = std::chrono::steady_clock::now();
    long mem1 = memoria_actual_kb();
    tiempo = tiempo_ms(t0, t1);
    memoria = mem1 - mem0;

    std::cout << "Dimensiones originales: " << ancho << " x " << alto << "\n";
    if (fueEscalada) {
        int nuevoAncho = static_cast<int>(ancho * factorEscala);
        int nuevoAlto = static_cast<int>(alto * factorEscala);
        std::cout << "Dimensiones finales: " << nuevoAncho << " x " << nuevoAlto << "\n";
    }
    std::cout << "Canales: " << canales << " (RGB)\n";
    if (fueRotada) std::cout << "Ángulo de rotación: " << angulo << " grados\n";
    if (fueEscalada) std::cout << "Factor de escalado: " << factorEscala << "\n";

    std::cout << "------------------------\n";
    if (fueRotada) std::cout << "[INFO] Imagen rotada correctamente.\n";
    if (fueEscalada) std::cout << "[INFO] Imagen escalada correctamente.\n";

    std::cout << "------------------------\n";
    std::cout << "TIEMPO DE PROCESAMIENTO:\n";
    std::cout << " - " << (usarBuddy ? "Con" : "Sin") << " Buddy System: " << tiempo << " ms\n\n";

    std::cout << "MEMORIA UTILIZADA:\n";
    std::cout << " - " << (usarBuddy ? "Con" : "Sin") << " Buddy System: " << (memoria / 1024.0f) << " MB\n";
    std::cout << "------------------------\n";
    std::cout << "[INFO] Imagen guardada correctamente en " << salida << "\n";

    std::remove("__tmp_rotada.jpg");
    std::remove("__tmp_escalada.jpg");

    return 0;
}
