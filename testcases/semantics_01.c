/* Involve a segment in multiple transactions, each transaction modifying a
 * region and either committing or aborting.  Check the final state. */

#include "rvm.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include <sys/wait.h>

/* create the situation where multiple transactions are started and stopped
 * (either aborted or committed) on a particular region, each transaction
 * modifying regions of the segment */
void create() 
{
     
  // rvm_t rvm = rvm_init(__FILE__ ".d");
  rvm_t rvm = rvm_init("rvm_segments");
     rvm_destroy(rvm, "testseg");
     char* segs[1];
     segs[0] = (char *) rvm_map(rvm, "testseg", 6);
     
     trans_t tid;

     /* zero out */
     tid = rvm_begin_trans(rvm, 1, (void **) segs);
     rvm_about_to_modify(tid, segs[0], 0, 5);
     sprintf(segs[0], "    ");
     assert(strcmp("    ", segs[0]) == 0);
     rvm_commit_trans(tid);
     rvm_unmap(rvm, segs[0]);

     segs[0] = (char *) rvm_map(rvm, "testseg", 10);
     tid = rvm_begin_trans(rvm, 1, (void **) segs);
     rvm_about_to_modify(tid, segs[0], 0, 1);
     segs[0][0] = 'a';
     assert(strcmp("a   ", segs[0]) == 0);
     rvm_about_to_modify(tid, segs[0], 3, 1);
     segs[0][3] = 'z';
     assert(strcmp("a  z", segs[0]) == 0);
     rvm_commit_trans(tid);

     /* abort an attempt to overwrite */
     tid = rvm_begin_trans(rvm, 1, (void **) segs);
     rvm_about_to_modify(tid, segs[0], 0, 5);
     sprintf(segs[0], "bbb ");
     assert(strcmp("bbb ", segs[0]) == 0);
     rvm_abort_trans(tid);
     assert(strcmp("a  z", segs[0]) == 0);
     rvm_unmap(rvm, segs[0]);

     abort();
}


/* test the resulting segment */
void test() 
{
     char* segs[1];
     rvm_t rvm;

     rvm = rvm_init("rvm_segments");

     segs[0] = (char *) rvm_map(rvm, "testseg", 10000);
     assert(strcmp("a  z", segs[0]) == 0);

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
