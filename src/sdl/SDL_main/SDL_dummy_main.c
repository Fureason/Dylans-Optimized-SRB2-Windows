 
#include "SDL_main.h"

#ifdef main
#undef main
int main(int argc, char *argv[])
{
	return(SDL_main(argc, argv));
}
#else
 ;
#endif
