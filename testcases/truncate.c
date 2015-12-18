/* Test rvm_truncate_log() correctly removes log and applies
changes to segments. */

#include "rvm.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_STRING1 "hello, world"
#define TEST_STRING2 "bleg!"
#define OFFSET2 1000


int main(int argc, char **argv)
{
  rvm_t rvm;
  char *seg;
  void *segs[1];
  trans_t trans;
     
  // rvm = rvm_init(__FILE__ ".d");
  rvm = rvm_init("rvm_segments");
     
  rvm_destroy(rvm, "testseg");
     
  segs[0] = (char *) rvm_map(rvm, "testseg", 10000);
  seg = (char *) segs[0];

  trans = rvm_begin_trans(rvm, 1, segs);
  rvm_about_to_modify(trans, seg, 0, 100);
  sprintf(seg, TEST_STRING1);
     
  rvm_about_to_modify(trans, seg, OFFSET2, 100);
  sprintf(seg+OFFSET2, TEST_STRING2);
     
  rvm_commit_trans(trans);


  printf("Before Truncation:\n");
  // system("ls -l " __FILE__ ".d");
  system("ls -l rvm_segments");

  printf("\n\n");

  rvm_truncate_log(rvm);
	 
  printf("\nAfter Truncation:\n");
  // system("ls -l " __FILE__ ".d");
  system("ls -l rvm_segments");

  rvm_unmap(rvm, seg);
  exit(0);
}

