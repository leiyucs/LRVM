
we use redo logfiles to record all the changes to the memory ranges of the data segments specified by the function rvm_about_to_modify(). At the time when the transaction is committed, all the regions that are modified in this transaction are written to the rvm.log file on the disk, in a format of transaction record. In this way, all the changes in a transaction become persistent on the disk. For the recovery, the logs on the logfile is read and re-applied.
If the transaction is not committed, the changes by the transaction will be lost and will not be reflected on the data segment. 

When reading logfiles, we detect whether a transaction record on the log file is complete because the writing to the logfile may fail due to reasons like power loss, which incurs partial transaction record writing. We ignore such partial record and regard this transaction not happened.

In this way, we can provide "all-or-nothing" semantic for transaction.

The logfile contains transaction record, in which all the lastest changes by the transaction made are recorded. We have transaction header and ender to indicate a transaction record. In a transaction record, for each memoray region modified,  the name of the segment, the offset, the length, and the data image of the region are recorded.

During the recovery, the redo log is read sequentially and the changes are applied sequentially to the external data segment files, such that the external data segments can have the latest data in it. When the process restarts, it maps (read) these data segments into its memory to recover its state before crash.

We use rvm_truncate_log() to shrink the redo logfiles. This function sequentially reads the rvm.log file and merge the changes to the data segment files. After the merge is done, the redo logfile is shrinked to zero size. It can be periodically called to avoid the logfile to expand indenfinitely. When the process is recovered from the crash, rvm_truncate_log() is called to merge the latest the changes to the external data segment and the process maps the data segment into memory.        

##The API
rvm_t rvm_init(const char *directory) - Initialize the library with the specified directory as backing store.
void *rvm_map(rvm_t rvm, const char *segname, int size_to_create) - map a segment from disk into memory. If the segment does not already exist, then create it and give it size size_to_create. If the segment exists but is shorter than size_to_create, then extend it until it is long enough. It is an error to try to map the same segment twice.
void rvm_unmap(rvm_t rvm, void *segbase) - unmap a segment from memory.
void rvm_destroy(rvm_t rvm, const char *segname) - destroy a segment completely, erasing its backing store. This function should not be called on a segment that is currently mapped.
trans_t rvm_begin_trans(rvm_t rvm, int numsegs, void **segbases) - begin a transaction that will modify the segments listed in segbases. If any of the specified segments is already being modified by a transaction, then the call should fail and return (trans_t) -1. Note that trant_t needs to be able to be typecasted to an integer type.
void rvm_about_to_modify(trans_t tid, void *segbase, int offset, int size) - declare that the library is about to modify a specified range of memory in the specified segment. The segment must be one of the segments specified in the call to rvm_begin_trans. Your library needs to ensure that the old memory has been saved, in case an abort is executed. It is legal call rvm_about_to_modify multiple times on the same memory area.
void rvm_commit_trans(trans_t tid) - commit all changes that have been made within the specified transaction. When the call returns, then enough information should have been saved to disk so that, even if the program crashes, the changes will be seen by the program when it restarts.
void rvm_abort_trans(trans_t tid) - undo all changes that have happened within the specified transaction.
void rvm_truncate_log(rvm_t rvm) - play through any committed or aborted items in the log file(s) and shrink the log file(s) as much as possible.
