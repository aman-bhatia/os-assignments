#include "types.h"
#include "user.h"
#include "fcntl.h"

int main(){

	int fd = open("a.txt", O_CREATE|O_WRONLY);
	if (fd < 0){
		printf(1, "dup2.c: Error occured while opening file\n");
		exit();
	}

	// dup std_out to opened file descriptor
	int ret = dup2(fd,1);
	if (ret < 0){
		printf(1, "dup2.c: Error doing dup2\n");
		exit();
	}

	// This should print to file now
	printf(1,"testing dup2\n");
	exit();
}