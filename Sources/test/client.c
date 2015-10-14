/** slowcat : like cat, but slow down output
*** vi:set ts=3:
**/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int nb, char *argv[])
{
	int fd; char c;
	fd = (nb > 1 ? open(argv[1], O_RDONLY) : 0);
	if(fd != -1)
	{
		while( read(fd, &c, 1) == 1 )
		{
			write(1, &c, 1);
			if(c == '\n') sleep(1);
		}
	} else perror(argv);
}

