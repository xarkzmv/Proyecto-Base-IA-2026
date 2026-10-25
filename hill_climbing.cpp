// Segmentacion multiumbral con el criterio de Otsu.
// Tecnica: Hill Climbing con Mejor Mejora.
//
// Uso: ./hill_climbing <instancia.txt> <carpeta_salida> [semilla]
//
// Convencion de clases (la misma que mri::segmentar):
//   [0,t1], [t1+1,t2], ..., [tk+1,255]  (un pixel igual a un umbral queda en la clase inferior)

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;
namespace fs = filesystem;

const int L = 256;        // niveles de gris
const int T_MIN = 1;      // menor valor permitido para un umbral
const int T_MAX = L - 2;  // mayor valor permitido para un umbral (254)

// ---------------------------------------------------------------- Instancia
struct Instancia {
    string imagen;
    int k = 0;
    long long N = 0; // la suma del histograma -> N = ancho x alto
    array<long long, L> h{};
    // Sumas acumuladas: P[i] = sum_{g<i} h[g],  S[i] = sum_{g<i} g*h[g]
    array<long long, L + 1> P{}; 
    array<long long, L + 1> S{};
    double muT = 0.0; // Media global
};

Instancia leerInstancia(const fs::path& ruta) {
    ifstream in(ruta);
    if (!in) throw runtime_error("No se pudo abrir la instancia: " + ruta.string());

    Instancia ins;
    string cabecera;
    in >> cabecera;
    if (cabecera != "SEGMENTACION_MULTIUMBRAL_V1")
        throw runtime_error("Cabecera invalida en " + ruta.string());

    int ancho = 0, alto = 0, niveles = 0;
    bool hayHistograma = false;
    string clave;
    while (in >> clave) {
        if (clave == "imagen") in >> ins.imagen;
        else if (clave == "ancho") in >> ancho;
        else if (clave == "alto") in >> alto;
        else if (clave == "L") in >> niveles;
        else if (clave == "k") in >> ins.k;
        else if (clave == "N") in >> ins.N;
        else if (clave == "histograma") {
            for (int g = 0; g < L; ++g) {
                if (!(in >> ins.h[g])) throw runtime_error("Histograma incompleto");
            }
            hayHistograma = true;
            break;
        } else throw runtime_error("Clave desconocida: " + clave);
    }
    if (!hayHistograma) throw runtime_error("Falta el histograma");
    if (niveles != L) throw runtime_error("Se esperaban 256 niveles");
    if (ins.k < 1 || ins.k > T_MAX - T_MIN + 1) throw runtime_error("k invalido");
    if (ins.N != 1LL * ancho * alto) throw runtime_error("N no coincide con ancho*alto");

    // Validamos 
    long long suma = accumulate(ins.h.begin(), ins.h.end(), 0LL);
    if (suma != ins.N) throw runtime_error("La suma del histograma no es N");

    // Sumas acumuladas, 
    for (int g = 0; g < L; ++g) {
        // Cuántos píxeles hay con intensidad menor que i. 
        ins.P[g + 1] = ins.P[g] + ins.h[g]; 
        // La suma de g*h(g) para esos mismos niveles, es decir, la suma de todas las intensidades de esos pixeles. 
        ins.S[g + 1] = ins.S[g] + 1LL * g * ins.h[g]; 
    }
    // Media Global 
    // static_cast<double>: Convierte a decimal antes de dividir, porque dividir entre dos enteros descartaría los decimales. 
    ins.muT = static_cast<double>(ins.S[L]) / ins.N; // Suma de las intensidades de todos los px dividido en N. 
    return ins;
}

// ------------------------------------------------------- Funcion de evaluacion
// Recibe un vector de umbrales t, y devuelve f(t). 

// f(t) = sum_j w_j (mu_j - muT)^2   (se maximiza). Costo O(k) gracias a P y S.
// Una clase vacia (w_j = 0) aporta 0.
double f(const vector<int>& t, const Instancia& ins) {
    // k sale del tamano de t y no de ins.k: el greedy evalua soluciones parciales
    // (con 1, 2, ... umbrales) mientras construye la solucion inicial.
    const int k = static_cast<int>(t.size());

    double valor = 0.0;

    // Para cada clase j, calcula su rango [lo, hi], cuántos píxeles tiene P[hi+1] - P[lo]
    for (int j = 0; j <= k; ++j) {

        int lo;
        if (j == 0) lo = 0; // la primera clase parte en el nivel 0
        else lo = t[j-1] + 1; // las demás parten justo después del umbral anterior

        // Límite superior de la clase j
        int hi;
        if (j == k) hi = L - 1; // la última clase termina en el nivel 255
        else hi = t[j]; // las demás terminan en su propio umbral
        
        // Cuántos píxeles tiene la clase (resta de sumas acumuladas)
        long long n = ins.P[hi + 1] - ins.P[lo];
        if (n == 0) continue;

        double w = static_cast<double>(n) / ins.N; // peso
        double mu = static_cast<double>(ins.S[hi + 1] - ins.S[lo]) / n; // media de la clase
        valor += w * (mu - ins.muT) * (mu - ins.muT);
    }
    return valor;
}

/// SOLUCIÓN INICIAL
// Se probaron con dos opciones, una es aleatoria y la otra es Greedy con aleatoriedad para combrobar cual es más eficiente. 

// ------------------------------------------------------------ Solucion inicial ALEATORIO

// k umbrales distintos elegidos al azar en [1,254] y ordenados.
vector<int> solucionInicialAleatoria(int k, mt19937& rng) {
    vector<int> valores(T_MAX - T_MIN + 1);
    iota(valores.begin(), valores.end(), T_MIN);
    shuffle(valores.begin(), valores.end(), rng);
    vector<int> t(valores.begin(), valores.begin() + k);
    sort(t.begin(), t.end());
    return t;
}

// ------------------------------------------------------------ Solucion inicial GREEDY con Aleatoriedad
// Constructivo greedy: parte sin umbrales y agrega uno por paso. En cada paso prueba
// los candidatos [1,254] que aun no se usan y se queda con el que deja la funcion
// objetivo mas alta (considerando los umbrales ya elegidos). Con m = 1 es determinista;
// con m > 1 se sortea entre los m mejores candidatos de cada paso (greedy aleatorizado).
const int M_GREEDY = 3;

vector<int> solucionInicialGreedy(const Instancia& ins, mt19937& rng, int m) {
    vector<int> t;
    for (int paso = 0; paso < ins.k; ++paso) {
        vector<pair<double, int>> candidatos;
        for (int c = T_MIN; c <= T_MAX; ++c) {
            if (find(t.begin(), t.end(), c) != t.end()) continue;
            vector<int> prueba = t;
            prueba.push_back(c);
            sort(prueba.begin(), prueba.end());
            candidatos.push_back({f(prueba, ins), c});
        }
        int tope = min<int>(m, candidatos.size());
        partial_sort(candidatos.begin(), candidatos.begin() + tope, candidatos.end(),
                     [](const pair<double, int>& a, const pair<double, int>& b) {
                         return a.first > b.first;
                     });
        int elegido = candidatos[uniform_int_distribution<int>(0, tope - 1)(rng)].second;
        t.push_back(elegido);
        sort(t.begin(), t.end());
    }
    return t;
}

// ----------------------------------------------- Hill Climbing, mejor mejora
// Vecindario: cada umbral puede quedarse igual o moverse hasta RADIO unidades hacia
// abajo o hacia arriba, y todos los umbrales se mueven a la vez. Con RADIO = 1 hay
// 3^k - 1 vecinos (728 para k = 6). Un vecino solo es valido si cumple
// 1 <= t1 < ... < tk <= 254, asi que las restricciones nunca se violan.
// En cada iteracion se evaluan todos los vecinos validos y se pasa al mejor,
// solo si mejora estrictamente a la solucion actual.

const int RADIO = 1;


struct Resultado {
    vector<int> umbrales;
    double valor = 0.0;
    long long iteraciones = 0;
    long long evaluaciones = 0;
};

Resultado hillClimbing(const Instancia& ins, vector<int> t) {
    const int k = ins.k;
    const int base = 2 * RADIO + 1;       // opciones por umbral: -RADIO, ..., 0, ..., +RADIO
    long long total = 1;
    for (int i = 0; i < k; ++i) total *= base;   // combinaciones posibles de movimientos

    Resultado r;
    double actual = f(t, ins);
    r.evaluaciones = 1;

    while (true) {
        double mejor = actual;
        vector<int> mejorVecino;             // queda vacio si ningun vecino mejora
        vector<int> vecino(k);

        for (long long codigo = 0; codigo < total; ++codigo) {
            // Decodifica el codigo: un desplazamiento por umbral
            long long resto = codigo;
            bool sinCambios = true;
            for (int i = 0; i < k; ++i) {
                int desplazamiento = static_cast<int>(resto % base) - RADIO;
                resto /= base;
                if (desplazamiento != 0) sinCambios = false;
                vecino[i] = t[i] + desplazamiento;
            }
            if (sinCambios) continue;        // es la solucion actual, no un vecino

            // Restricciones: 1 <= t1 < ... < tk <= 254
            bool valido = (vecino[0] >= T_MIN && vecino[k - 1] <= T_MAX);
            for (int i = 1; i < k && valido; ++i) {
                if (vecino[i] <= vecino[i - 1]) valido = false;
            }
            if (!valido) continue;

            double v = f(vecino, ins);
            ++r.evaluaciones;
            if (v > mejor + 1e-12) {
                mejor = v;
                mejorVecino = vecino;
            }
        }
        if (mejorVecino.empty()) break;      // optimo local: ningun vecino mejora
        t = mejorVecino;
        actual = mejor;
        ++r.iteraciones;
    }
    r.umbrales = t;
    r.valor = actual;
    return r;
}

// ---------------------------------------------------------------------- Salida
void escribirSolucion(const fs::path& ruta, const string& nombre, const Instancia& ins,
                      unsigned semilla, const Resultado& r) {
    ofstream out(ruta);
    if (!out) throw runtime_error("No se pudo escribir: " + ruta.string());
    out << "instancia " << nombre << "\n";
    out << "imagen " << ins.imagen << "\n";
    out << "k " << ins.k << "\n";
    out << "semilla " << semilla << "\n";
    out << "umbrales";
    for (int t : r.umbrales) out << ' ' << t;
    out << "\n";
    out << setprecision(12) << "f " << r.valor << "\n";
    out << "iteraciones " << r.iteraciones << "\n";
    out << "evaluaciones " << r.evaluaciones << "\n";
}

int main(int argc, char* argv[]) {
    if (argc < 3 || argc > 4) {
        cerr << "Uso: " << argv[0] << " <instancia.txt> <carpeta_salida> [semilla]\n";
        return 1;
    }
    try {
        fs::path rutaInstancia = argv[1];
        fs::path carpetaSalida = argv[2];
        unsigned semilla = (argc == 4) ? static_cast<unsigned>(stoul(argv[3])) : 42u;

        Instancia ins = leerInstancia(rutaInstancia);
        mt19937 rng(semilla);

        //vector<int> inicial = solucionInicialAleatoria(ins.k, rng);
        vector<int> inicial = solucionInicialGreedy(ins, rng, M_GREEDY);
        double fInicial = f(inicial, ins);
        Resultado r = hillClimbing(ins, inicial);

        fs::create_directories(carpetaSalida);
        string nombre = rutaInstancia.stem().string();
        escribirSolucion(carpetaSalida / (nombre + "_sol.txt"), nombre, ins, semilla, r);

        cout << "Instancia: " << nombre << " (k=" << ins.k << ", semilla " << semilla << ")\n";
        cout << "f inicial: " << setprecision(10) << fInicial << "\n";
        cout << "f final:   " << setprecision(10) << r.valor << "\n";
        cout << "Umbrales:";
        for (int t : r.umbrales) cout << ' ' << t;
        cout << "\nIteraciones: " << r.iteraciones << ", evaluaciones: " << r.evaluaciones << "\n";
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}