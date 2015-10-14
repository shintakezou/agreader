/** slowcat : like cat, but slow down output
*** vi:set ts=3:
**/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int nb, char *argv[])
{
	int fd;
	static char c[50], nbc;
	fd = (nb > 1 ? open(argv[1], O_RDONLY) : 0);
	if(fd != -1)
	{
		while( (nbc = read(fd, c, 50)) > 0 )
		{
			printf("%d bytes read\n", nbc);
		}
	} else perror(argv);
}

