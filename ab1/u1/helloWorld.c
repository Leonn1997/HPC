#include <stdio.h>
#include <omp.h>

int main() {


    #pragma omp parallel //remove this line for 1 a)
    printf("Hello, World! from Thread %d of %d \n", omp_get_thread_num(), omp_get_max_threads()); //ex1b
    //1c) order of threads in the output change


    printf("-------------------------------------------------\n");

    char lang_en[] = "Hello World!.";
    char lang_de[] = "Servus Welt!.";
    char lang_it[] = "Ciao mondo!";
    char lang_fr[] = "Bonjour tout le monde!";


    #pragma omp parallel sections firstprivate(lang_en, lang_de, lang_it, lang_fr)
    {
        printf("%s from Thread %d of %d \n", lang_en, omp_get_thread_num(), omp_get_max_threads());
        #pragma omp section
        printf("%s from Thread %d of %d \n", lang_de, omp_get_thread_num(), omp_get_max_threads());
        #pragma omp section
        printf("%s from Thread %d of %d \n", lang_it, omp_get_thread_num(), omp_get_max_threads());
        #pragma omp section
        printf("%s from Thread %d of %d \n", lang_fr, omp_get_thread_num(), omp_get_max_threads());
    };
    return 0;
}
