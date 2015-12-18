/* ensure an unlogged modification is lost */

#include "rvm.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include <sys/wait.h>

#define ABET "abcdefghijklmnopqrstuvwxyz"

/* create the scenario */
void create() 
{
     
     rvm_t rvm = rvm_init("rvm_segments");
     char* segs[1];

     /*-- without unmapping (first half of segment) --*/
     segs[0] = (char *) rvm_map(rvm, "testseg", 100);
     trans_t tid = rvm_begin_trans(rvm, 1, (void **) segs);
     rvm_about_to_modify(tid, segs[0], 0, 50);
     sprintf(segs[0], ABET); /* write "foo" */
     rvm_commit_trans(tid);

     tid = rvm_begin_trans(rvm, 1, (void **) segs);
     rvm_about_to_modify(tid, segs[0], 0, 10);
     segs[0][17] = '0'; /* change to boo without logging */
     assert(strcmp(segs[0],  "abcdefghijklmnopq0stuvwxyz") == 0); /* ensure change */
     rvm_commit_trans(tid);

     exit(EXIT_SUCCESS);
}


/* test the resulting segment */
void test() 
{
     char* segs[1];
     rvm_t rvm;

     rvm = rvm_init("rvm_segments");

     segs[0] = (char *) rvm_map(rvm, "testseg", 100);

     assert(strcmp(segs[0], ABET) == 0);

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
	  create();
	  exit(0);
     }

     waitpid(pid, NULL, 0);

     test();

     return 0;
}
