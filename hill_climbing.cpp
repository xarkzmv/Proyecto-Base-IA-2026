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
    long long N = 0;
    array<long long, L> h{};
    // Sumas acumuladas: P[i] = sum_{g<i} h[g],  S[i] = sum_{g<i} g*h[g]
    array<long long, L + 1> P{}, S{};
    double muT = 0.0;
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

    long long suma = accumulate(ins.h.begin(), ins.h.end(), 0LL);
    if (suma != ins.N) throw runtime_error("La suma del histograma no es N");

    for (int g = 0; g < L; ++g) {
        ins.P[g + 1] = ins.P[g] + ins.h[g];
        ins.S[g + 1] = ins.S[g] + 1LL * g * ins.h[g];
    }
    ins.muT = static_cast<double>(ins.S[L]) / ins.N;
    return ins;
}

// ------------------------------------------------------- Funcion de evaluacion
// f(t) = sum_j w_j (mu_j - muT)^2   (se maximiza). Costo O(k) gracias a P y S.
// Una clase vacia (w_j = 0) aporta 0.
double evaluar(const vector<int>& t, const Instancia& ins) {
    const int k = static_cast<int>(t.size());
    double f = 0.0;
    for (int j = 0; j <= k; ++j) {
        int lo = (j == 0) ? 0 : t[j - 1] + 1;
        int hi = (j == k) ? L - 1 : t[j];
        long long n = ins.P[hi + 1] - ins.P[lo];
        if (n == 0) continue;
        double w = static_cast<double>(n) / ins.N;
        double mu = static_cast<double>(ins.S[hi + 1] - ins.S[lo]) / n;
        f += w * (mu - ins.muT) * (mu - ins.muT);
    }
    return f;
}

// ------------------------------------------------------------ Solucion inicial
// k umbrales distintos elegidos al azar en [1,254] y ordenados.
vector<int> solucionInicial(int k, mt19937& rng) {
    vector<int> valores(T_MAX - T_MIN + 1);
    iota(valores.begin(), valores.end(), T_MIN);
    shuffle(valores.begin(), valores.end(), rng);
    vector<int> t(valores.begin(), valores.begin() + k);
    sort(t.begin(), t.end());
    return t;
}

// ----------------------------------------------- Hill Climbing, mejor mejora
// Vecindario: mover un umbral una unidad hacia arriba o hacia abajo, sin salirse
// de [1,254] y sin alcanzar a sus vecinos (así se mantiene 1<=t1<...<tk<=254).
// En cada iteracion se evaluan todos los vecinos (a lo mas 2k) y se pasa al mejor,
// solo si mejora estrictamente a la solucion actual.
struct Resultado {
    vector<int> umbrales;
    double valor = 0.0;
    long long iteraciones = 0;
    long long evaluaciones = 0;
};

Resultado hillClimbing(const Instancia& ins, vector<int> t) {
    const int k = static_cast<int>(t.size());
    Resultado r;
    double actual = evaluar(t, ins);
    r.evaluaciones = 1;

    while (true) {
        double mejor = actual;
        int mejorI = -1, mejorValor = 0;

        for (int i = 0; i < k; ++i) {
            int min_i = (i == 0) ? T_MIN : t[i - 1] + 1;
            int max_i = (i == k - 1) ? T_MAX : t[i + 1] - 1;
            int original = t[i];
            for (int d : {-1, +1}) {
                int nuevo = original + d;
                if (nuevo < min_i || nuevo > max_i) continue;
                t[i] = nuevo;
                double v = evaluar(t, ins);
                ++r.evaluaciones;
                if (v > mejor + 1e-12) {
                    mejor = v;
                    mejorI = i;
                    mejorValor = nuevo;
                }
            }
            t[i] = original;
        }

        if (mejorI < 0) break;  // optimo local: ningun vecino mejora
        t[mejorI] = mejorValor;
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

        vector<int> inicial = solucionInicial(ins.k, rng);
        double fInicial = evaluar(inicial, ins);
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