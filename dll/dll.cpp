#include "dll.h" 
#include <iostream>
#include <string>

#define TRY_AND_DO(ptr, func) do { if (ptr == NULL) {func;  return; }} while (0)
#define TRY(ptr) do { if (ptr == NULL) { return; }} while (0)

// This function will run when the shared object is loaded
__attribute__((constructor)) void onLoad() {
	std::cout << "Shared object (.so) file loaded." << std::endl;
}

// This function will run when the shared object is unloaded
__attribute__((destructor)) void onUnload() {
	std::cout << "Shared object (.so) file unloaded." << std::endl;
}
