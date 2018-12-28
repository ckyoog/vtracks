#include <stdlib.h>
#include <unistd.h>
#include "setproctitle.h"


int main(int argc, char **argv)
{
	init_setproctitle(&argv);
	setproctitle("whatever-you-want");
	return sleep(10);
}
