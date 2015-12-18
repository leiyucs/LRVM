/* Writes binary values and tests whether they are correctly written */

#include "rvm.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

int _PTS = 5;

const char *getfile() { return __FILE__; }

#define OFFSET2 1000


/* proc1 writes some data, commits it, then exits */
void proc1() 
{
     rvm_t rvm;
     trans_t trans;
     char* segs[1];
     unsigned char *foo;
     int i;
     
     rvm = rvm_init("rvm_segments");
     segs[0] = (char *) rvm_map(rvm, "testseg", 10000);

     
     trans = rvm_begin_trans(rvm, 1, (void **) segs);
     
     rvm_about_to_modify(trans, segs[0], 0, 600);
     foo = (unsigned char *) segs[0];
     for(i = 0; i < 256; i++) {
       foo[i] = i;
       foo[i+256] = 255 - i;
     }
     
     rvm_commit_trans(trans);

     exit(EXIT_SUCCESS);
}


/* proc2 opens the segments and reads from them */
void proc2() 
{
     char* segs[1];
     rvm_t rvm;
     unsigned char *foo;
     int i;
     
     rvm = rvm_init("rvm_segments");

     segs[0] = (char *) rvm_map(rvm, "testseg", 10000);

     foo = (unsigned char *) segs[0];
     for(i = 0; i < 256; i++) {
       if(foo[i] != i) {
         printf("ERROR: data mismatch at %d\n", i);
         exit(2);
       }
       if(foo[i+256] != 255 - i) {
         printf("ERROR: data mismatch at %d\n", i);
         exit(2);         
       }
     }

     printf("OK\n");
     exit(0);
}


int main(int argc, char **argv)
{
     int pid;

     pid = fork();
     if(pid < 0) {
	  perror("fork");
	  exit(2);
     }
     if(pid == 0) {
	  proc1();
	  exit(0);
     }

     waitpid(pid, NULL, 0);

     proc2();

     return 0;
}
