#include <signal.h>
#include <stdlib.h>


#define SIGUSER1 1001
#define SIGUSER2 1002


void sig_usr(int signo) 
{
  if (signo == SIGUSER1)
    printf("recv signal user1");
  else if (signo == SIGUSER2)
    printf("recv signal user2");
  else
    printf("recv signal: %d", signo);
}


int main()
{
    if (signal(SIGUSER1, sig_usr) == SIG_ERR)
      printf("can not catch sig user1");
    if (signal(SIGUSER2, sig_usr) == SIG_ERR)
      printf("can not catch sig user2");

    for(;;)
       pause();
}
