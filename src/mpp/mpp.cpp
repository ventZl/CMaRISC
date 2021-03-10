#include <iostream>
#include "preprocess.h"

int main(int argc, char ** argv) {
	Preprocessor * pp = new Preprocessor(argv[1]);
	pp->addIncludePath(".");
	std::cout << pp->preprocess();
	return 0;
}