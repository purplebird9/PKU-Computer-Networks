#include <cstdio>
#include <cstring>
#include <algorithm>
#include <iostream>

int main(int argc, char **argv) {
    int option = 0;
    if (argc == 2)
        option = atoi(argv[1]);

    switch (option) {
        case 0:
            std::cout << "TA is saying hello\n";
            break;
        case 1:
            std::cout << "Hello Network Hello PKU\n"; 
            break;
        case 2:
            std::cout << "Oops.\n";
            break;
        default:
            std::cerr << "No such option\n";
            return 1;
    }
    return 0;
}
