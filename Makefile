CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -pedantic
OPENCV = $(shell pkg-config --cflags --libs opencv4)


# Parametros de "make run" (se pueden cambiar: make run SEMILLA=7 SALIDA=soluciones_v2)
INSTANCIAS ?= imagenes_preprocesadas/instancias
SALIDA ?= soluciones
SEMILLA ?= 42
SCRIPT ?= run_segmentar.sh


.PHONY: all run comparar clean

all: preprocesar segmentar hill_climbing

segmentar: segmentar.cpp imagenes.cpp imagenes.hpp
	$(CXX) $(CXXFLAGS) segmentar.cpp imagenes.cpp -o segmentar $(OPENCV)

preprocesar: preprocesar.cpp
	$(CXX) $(CXXFLAGS) preprocesar.cpp -o preprocesar $(OPENCV)


# No usa OpenCV: solo lee el histograma de la instancia
hill_climbing: hill_climbing.cpp
	$(CXX) $(CXXFLAGS) hill_climbing.cpp -o hill_climbing

# Recompila hill_climbing si cambio el codigo y corre las 30 instancias
run: hill_climbing
	bash $(SCRIPT) $(INSTANCIAS) $(SALIDA) $(SEMILLA)

# Compara el resumen con el optimo exacto de cada instancia
comparar:
	python3 scripts/comparar_optimo.py $(INSTANCIAS) $(SALIDA)/resumen.csv

clean:
	$(RM) preprocesar segmentar hill_climbing *.o *.d

