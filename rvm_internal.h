#ifndef RVM_INTERNAL
#define RVM_INTERNAL

#include <stddef.h>

typedef struct rvm_t {
	const char *directory;
} rvm_t;

typedef int trans_t;

typedef struct segment_t {
	const char* seg_name;
	int seg_size;
	void *data;
	trans_t trans_id;
} segment_t;

typedef struct region_t {
	segment_t *segment;
	int offset;
	int size;
	void *undo_data;
} region_t;

typedef struct logrecordheader_t {
	char segmentname[256];
	int region_offset;
	int region_size;
	void *data;
} logrecordheader_t;

typedef struct trans_header_t {
	trans_t tid;
	size_t length;
	int recordnum;
} trans_header_t;

typedef struct trans_end_t {
	trans_t tid;
	size_t length;
	int recordnum;
} trans_end_t;

#endif