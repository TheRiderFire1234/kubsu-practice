#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <vector>
#include <array>
#include <io.h>
#include <fcntl.h>
#include <windows.h>

using namespace std;

static const int CHROMO_LENGTH = 3;
static const int DOMAIN_VALUES[] = {1, 2, 3, 4, 5};
static const int DOMAIN_SIZE = sizeof(DOMAIN_VALUES) / sizeof(DOMAIN_VALUES[0]);

struct Individual {
    string chromosome;
    int x;
    double fitness;
};

int decodeChromosome(const string &chromosome) {
    int value = 0;
    for (char bit : chromosome) {
        value = (value << 1) | (bit == '1');
    }
    value = value % DOMAIN_SIZE;
    if (value >= DOMAIN_SIZE) value = DOMAIN_SIZE - 1;
    return DOMAIN_VALUES[value];
}

string encodeX(int x) {
    int index = find(DOMAIN_VALUES, DOMAIN_VALUES + DOMAIN_SIZE, x) - DOMAIN_VALUES;
    if (index < 0 || index >= DOMAIN_SIZE) {
        index = 0;
    }
    string bits(CHROMO_LENGTH, '0');
    for (int i = CHROMO_LENGTH - 1; i >= 0; --i) {
        bits[i] = ((index & 1) != 0) ? '1' : '0';
        index >>= 1;
    }
    return bits;
}

double evaluateFitness(int x) {
    return 5.0 * x * x * x - 4.0;
}

Individual makeIndividual(const string &chromosome) {
    Individual ind;
    ind.chromosome = chromosome;
    ind.x = decodeChromosome(chromosome);
    ind.fitness = evaluateFitness(ind.x);
    return ind;
}

string randomChromosome(default_random_engine &rng) {
    string chromosome(CHROMO_LENGTH, '0');
    uniform_int_distribution<int> bitDist(0, 1);
    for (int i = 0; i < CHROMO_LENGTH; ++i) {
        chromosome[i] = bitDist(rng) ? '1' : '0';
    }
    return chromosome;
}

vector<Individual> createInitialPopulation(int size, int strategy, default_random_engine &rng) {
    vector<Individual> population;
    population.reserve(size);
    uniform_int_distribution<int> xDist(1, 5);
    uniform_real_distribution<double> prob(0.0, 1.0);
    for (int i = 0; i < size; ++i) {
        int x;
        if (strategy == 1) {
            x = xDist(rng);
        } else {
            if (prob(rng) < 0.7) {
                x = (prob(rng) < 0.75) ? 1 : 2;
            } else {
                x = xDist(rng);
            }
        }
        population.push_back(makeIndividual(encodeX(x)));
    }
    return population;
}

Individual selectParent(const vector<Individual> &population, int selectionType, default_random_engine &rng) {
    if (selectionType == 1) {
        uniform_int_distribution<int> indexDist(0, (int)population.size() - 1);
        return population[indexDist(rng)];
    }

    int half = max(1, (int)population.size() / 2);
    vector<Individual> topHalf(population.begin(), population.begin() + half);
    uniform_int_distribution<int> indexDist(0, (int)topHalf.size() - 1);
    return topHalf[indexDist(rng)];
}

string crossoverOnePoint(const string &a, const string &b, default_random_engine &rng) {
    uniform_int_distribution<int> cutDist(1, CHROMO_LENGTH - 1);
    int cut = cutDist(rng);
    string child = a.substr(0, cut) + b.substr(cut);
    return child;
}

string crossoverTwoPoint(const string &a, const string &b, default_random_engine &rng) {
    uniform_int_distribution<int> cutDist(1, CHROMO_LENGTH - 1);
    int cut1 = cutDist(rng);
    int cut2 = cutDist(rng);
    if (cut1 > cut2) {
        swap(cut1, cut2);
    }
    if (cut1 == cut2) {
        cut2 = min(cut1 + 1, CHROMO_LENGTH - 1);
        if (cut2 == cut1) {
            cut1 = 1;
            cut2 = CHROMO_LENGTH - 1;
        }
    }
    string child = a;
    for (int i = cut1; i < cut2; ++i) {
        child[i] = b[i];
    }
    return child;
}

string crossoverPartialMatchOnePoint(const string &a, const string &b, default_random_engine &rng) {
    uniform_int_distribution<int> cutDist(1, CHROMO_LENGTH - 1);
    int cut = cutDist(rng);
    string child = a.substr(0, cut) + b.substr(cut);
    uniform_int_distribution<int> posDist(0, CHROMO_LENGTH - 2);
    int pos = posDist(rng);
    if (child[pos] != child[pos + 1]) {
        swap(child[pos], child[pos + 1]);
    }
    return child;
}

string crossoverFibonacci(const string &a, const string &b, default_random_engine &rng) {
    vector<int> fib = {1, 2, 3};
    uniform_int_distribution<int> indexDist(0, (int)fib.size() - 1);
    int point = fib[indexDist(rng)];
    if (point >= CHROMO_LENGTH) {
        point = CHROMO_LENGTH - 1;
    }
    return a.substr(0, point) + b.substr(point);
}

string mutatePoint(const string &chromosome, default_random_engine &rng) {
    uniform_int_distribution<int> posDist(0, CHROMO_LENGTH - 1);
    string result = chromosome;
    int pos = posDist(rng);
    result[pos] = (result[pos] == '0') ? '1' : '0';
    return result;
}

string mutateTransposition(const string &chromosome, default_random_engine &rng) {
    uniform_int_distribution<int> posDist(0, CHROMO_LENGTH - 1);
    int i = posDist(rng);
    int j = posDist(rng);
    while (i == j) {
        j = posDist(rng);
    }
    string result = chromosome;
    swap(result[i], result[j]);
    return result;
}

vector<Individual> proportionalSelection(const vector<Individual> &candidates, int keepCount, default_random_engine &rng) {
    vector<double> weights;
    weights.reserve(candidates.size());
    for (auto &ind : candidates) {
        weights.push_back(1.0 / (1.0 + ind.fitness));
    }
    discrete_distribution<int> distribution(weights.begin(), weights.end());
    vector<Individual> nextGen;
    nextGen.reserve(keepCount);
    for (int i = 0; i < keepCount; ++i) {
        nextGen.push_back(candidates[distribution(rng)]);
    }
    sort(nextGen.begin(), nextGen.end(), [](const Individual &a, const Individual &b) {
        return a.fitness < b.fitness;
    });
    return nextGen;
}

string chooseCrossover(const string &a, const string &b, int type, default_random_engine &rng) {
    switch (type) {
        case 1: return crossoverOnePoint(a, b, rng);
        case 2: return crossoverTwoPoint(a, b, rng);
        case 3: return crossoverPartialMatchOnePoint(a, b, rng);
        case 4: return crossoverFibonacci(a, b, rng);
        default: return crossoverOnePoint(a, b, rng);
    }
}

string chooseMutation(const string &chromosome, int type, default_random_engine &rng) {
    switch (type) {
        case 1: return mutatePoint(chromosome, rng);
        case 2: return mutateTransposition(chromosome, rng);
        default: return mutatePoint(chromosome, rng);
    }
}

void writePlotFiles(const vector<pair<int, double>> &trace, const Individual &best) {
    {
        ofstream funcData("func_data.txt");
        for (int x : DOMAIN_VALUES) {
            funcData << x << " " << evaluateFitness(x) << "\n";
        }
    }
    {
        ofstream traceData("trace_data.txt");
        for (auto &item : trace) {
            traceData << item.first << " " << item.second << "\n";
        }
    }
    {
        ofstream bestData("best_point.txt");
        bestData << best.x << " " << best.fitness << "\n";
    }
    ofstream script("plot.gnuplot");
    script << "set terminal pngcairo size 900,650 enhanced font 'Arial,12'\n";
    script << "set output 'ga_plot.png'\n";
    script << "set title 'Генетический алгоритм: минимизация 5x^3 - 4 на [1,5]'\n";
    script << "set xlabel 'x'\n";
    script << "set ylabel 'f(x)'\n";
    script << "set grid\n";
    script << "set xtics 1,1,5\n";
    script << "plot 'func_data.txt' with linespoints lt rgb 'black' pt 7 ps 1.5 lw 2 title 'f(x)', \\" << "\n";
    script << "     'trace_data.txt' using 1:2 with points pt 7 ps 1.5 lc rgb 'blue' title 'лучшие за поколение', \\" << "\n";
    script << "     'best_point.txt' using 1:2 with points pt 7 ps 3 lc rgb 'red' title 'окончательный оптимум'\n";
}

int main() {
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
    setlocale(LC_ALL, ".UTF8");
    
    cout << "Генетический алгоритм для минимизации f(x)=5x^3 - 4 на {1,2,3,4,5}" << endl;
    int popSize = 10;
    int generations = 50;
    int initialStrategy = 1;
    int selectionType = 1;
    int crossoverType = 1;
    int mutationType = 1;
    double crossoverProb = 0.7;
    double mutationProb = 0.2;
    char useDefaults = 'y';

    cout << "Использовать параметры по умолчанию? (y/n) [size=10, gen=50, px=70%, pm=20%] ";
    cin >> useDefaults;
    if (useDefaults == 'n' || useDefaults == 'N') {
        cout << "Введите размер популяции (целое > 1): ";
        cin >> popSize;
        if (popSize < 2) popSize = 10;
        cout << "Введите число поколений (>= 50): ";
        cin >> generations;
        if (generations < 50) generations = 50;
        cout << "Введите вероятность кроссинговера (%) [0..100]: ";
        cin >> crossoverProb;
        crossoverProb = max(0.0, min(crossoverProb / 100.0, 1.0));
        cout << "Введите вероятность мутации (%) [0..100]: ";
        cin >> mutationProb;
        mutationProb = max(0.0, min(mutationProb / 100.0, 1.0));
        cout << "Выберите стратегию начальной популяции:\n";
        cout << " 1 - Одеяло (равномерно по области)\n";
        cout << " 2 - Фокусировка (более тесная начальная зона около 1-2)\n";
        cin >> initialStrategy;
        if (initialStrategy != 2) initialStrategy = 1;
        cout << "Выберите тип селекции:\n";
        cout << " 1 - Случайная\n";
        cout << " 2 - Инбридинг (из топ-50%)\n";
        cin >> selectionType;
        if (selectionType != 2) selectionType = 1;
        cout << "Выберите оператор кроссинговера:\n";
        cout << " 1 - Одноточечный\n";
        cout << " 2 - Двухточечный\n";
        cout << " 3 - Частично соответствующий одноточечный\n";
        cout << " 4 - На основе чисел Фибоначчи\n";
        cin >> crossoverType;
        if (crossoverType < 1 || crossoverType > 4) crossoverType = 1;
        cout << "Выберите оператор мутации/инверсии:\n";
        cout << " 1 - Точечная мутация\n";
        cout << " 2 - Транспозиция\n";
        cin >> mutationType;
        if (mutationType != 2) mutationType = 1;
    }

    default_random_engine rng((unsigned)chrono::high_resolution_clock::now().time_since_epoch().count());
    auto population = createInitialPopulation(popSize, initialStrategy, rng);
    sort(population.begin(), population.end(), [](const Individual &a, const Individual &b) {
        return a.fitness < b.fitness;
    });

    vector<pair<int, double>> trace;
    trace.reserve(generations);
    Individual bestOverall = population.front();

    for (int generation = 1; generation <= generations; ++generation) {
        Individual parent1 = selectParent(population, selectionType, rng);
        Individual parent2 = selectParent(population, selectionType, rng);
        string childChrom1 = parent1.chromosome;
        string childChrom2 = parent2.chromosome;
        uniform_real_distribution<double> prob(0.0, 1.0);

        if (prob(rng) < crossoverProb) {
            childChrom1 = chooseCrossover(parent1.chromosome, parent2.chromosome, crossoverType, rng);
            childChrom2 = chooseCrossover(parent2.chromosome, parent1.chromosome, crossoverType, rng);
        }
        if (prob(rng) < mutationProb) {
            childChrom1 = chooseMutation(childChrom1, mutationType, rng);
        }
        if (prob(rng) < mutationProb) {
            childChrom2 = chooseMutation(childChrom2, mutationType, rng);
        }

        Individual child1 = makeIndividual(childChrom1);
        Individual child2 = makeIndividual(childChrom2);
        vector<Individual> candidates = population;
        candidates.push_back(child1);
        candidates.push_back(child2);
        population = proportionalSelection(candidates, popSize, rng);
        sort(population.begin(), population.end(), [](const Individual &a, const Individual &b) {
            return a.fitness < b.fitness;
        });

        if (population.front().fitness < bestOverall.fitness) {
            bestOverall = population.front();
        }
        trace.emplace_back(population.front().x, population.front().fitness);

        cout << "Поколение " << generation << ": best x=" << population.front().x
             << ", f(x)=" << population.front().fitness << "\n";
    }

    cout << "\nНайден лучший результат: x=" << bestOverall.x << ", f(x)=" << bestOverall.fitness << "\n";
    writePlotFiles(trace, bestOverall);

    cout << "Данные для графика записаны в файлы func_data.txt, trace_data.txt, best_point.txt." << endl;
    cout << "Gnuplot-скрипт создан: plot.gnuplot" << endl;
    cout << "Запуск построения графика..." << endl << endl;

    int plotResult = system("run_plot.bat");
    
    return 0;
}