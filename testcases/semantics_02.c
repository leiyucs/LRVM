/* ensure an unlogged modification is lost */

#include "rvm.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include <sys/wait.h>

int _PTS = 5;

const char *getfile() { return __FILE__; }

/* create the scenario */
void create() 
{
     
  // rvm_t rvm = rvm_init(__FILE__ ".d");
  rvm_t rvm = rvm_init("rvm_segments");
     rvm_destroy(rvm, "testseg");
     char* segs[1];


     /*-- without unmapping (first half of segment) --*/
     segs[0] = (char *) rvm_map(rvm, "testseg", 10);
     trans_t tid = rvm_begin_trans(rvm, 1, (void **) segs);
	 printf("%d\n", tid);
     rvm_about_to_modify(tid, segs[0], 0, 5);
     sprintf(segs[0], "foo"); /* write "foo" */
     rvm_commit_trans(tid);

     tid = rvm_begin_trans(rvm, 1, (void **) segs);
	 printf("%d\n", tid);
     segs[0][0] = 'b'; /* change to boo without logging */
     assert(strcmp(segs[0], "boo") == 0); /* ensure change */
     rvm_commit_trans(tid);



     /*-- unmap and remap (second half of segment) --*/
     tid = rvm_begin_trans(rvm, 1, (void **) segs);
	 printf("%d\n", tid);
     rvm_about_to_modify(tid, segs[0], 4, 5);
     sprintf(segs[0] + 4, "bar"); /* write "bar" */
     rvm_commit_trans(tid);
     rvm_unmap(rvm, segs[0]);

     segs[0] = (char *) rvm_map(rvm, "testseg", 10);
     tid = rvm_begin_trans(rvm, 1, (void **) segs);
	 printf("%d\n", tid);
     segs[0][6] = 'z'; /* change to baz without logging */
     assert(strcmp(segs[0] + 4, "baz") == 0); /* ensure change */
     rvm_commit_trans(tid);
     abort();
}


/* test the resulting segment */
void test() 
{
     char* segs[1];
     rvm_t rvm;

     // rvm = rvm_init(__FILE__ ".d");
     rvm = rvm_init("rvm_segments");

     segs[0] = (char *) rvm_map(rvm, "testseg", 10);

     /* ensure originals */
     assert(strcmp(segs[0] + 0, "foo") == 0);
     assert(strcmp(segs[0] + 4, "bar") == 0);

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
