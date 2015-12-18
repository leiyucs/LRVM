#include "rvm.h"
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <string>
#include <list>
#include <vector>
#include <iostream>

trans_t trans_num = 0;

static const char* LOG;
static std::map<void*, segment_t *> segbasemap; 
static std::map<std::string, segment_t *> segnamemap;
static std::map < trans_t, std::list<region_t *> *> txregions;
/**
 * Check if a file exists
 * @return true if and only if the file exists, false else
 */
 bool fileExists(const char* file) {
 	struct stat buf;
 	return (stat(file, &buf) == 0);
 }
/**
 * Get the size of a file.
 * @return The filesize, or 0 if the file does not exist.
 */
 size_t getFilesize(const char* filename) {
 	struct stat st;
 	if(stat(filename, &st) != 0) {
 		return 0;
 	}
 	return st.st_size;   
 }


 rvm_t rvm_init(const char *directory) {
 	struct stat buf;
 	rvm_t rvm;
 	rvm.directory = NULL;
 	if (stat(directory, &buf)==0 && (buf.st_mode&S_IFDIR))
 	{
 		rvm.directory = directory;
 	}
 	else {
 		if(mkdir(directory, S_IRWXU | S_IRWXG )==0) {
 			rvm.directory = directory;
 		}
 	}
 	char *log_file_path = (char *)malloc(256);
 	memset(log_file_path,0,256);
 	strcat(log_file_path, directory);
 	strcat(log_file_path,"/rvm.log");
 	std::cout<<"Initilization RVM directory: " << log_file_path <<std::endl;
 	LOG = log_file_path;
 	return rvm;
 }

 void *rvm_map(rvm_t rvm, const char *segname, int size_to_create) {
 	
 	std::string seg_name;
 	std::map<std::string, segment_t*>::iterator it;

 	if (segname == NULL) 
 		return NULL;

 	seg_name = segname;
 	it = segnamemap.find(seg_name);
	if (it != segnamemap.end()) { //check whether it is already mapped
		std::cerr << "Error: " + seg_name + " already mapped" << std::endl;
		return NULL;
	}

	std::string seg_path(rvm.directory);
	seg_path = seg_path+"/"+segname+".seg";
	const char* seg_path_cstr = seg_path.c_str();

	segment_t *segment = (segment_t *) malloc (sizeof(segment_t));
	segment->seg_name = segname;
	segment->seg_size = size_to_create;
	segment->trans_id = -1;
	segment->data = (void *)malloc(size_to_create);
	memset(segment->data,0,size_to_create);

	if(fileExists(seg_path_cstr)) {
		//replay redo log and update the segment		
		int segfilesize = getFilesize(seg_path_cstr);
		//std::cout<<segfilesize << " "<< size_to_create<<std::endl;
		if (segfilesize != size_to_create)
		{
			std::cout<<"resizing the segment" + std::string(seg_name) << std::endl;
			truncate(seg_path_cstr, size_to_create);
		}

		int fileLen = 0;
		FILE *logfp = fopen(LOG, "r");
		if (logfp == NULL) {
    			printf("fopen failed%s\n",LOG);
		} else {
			fseek(logfp, 0, SEEK_END);
			fileLen = ftell(logfp);
			fseek(logfp, 0, SEEK_SET);
			fclose(logfp);
		}

		if (fileLen != 0) {
			rvm_truncate_log(rvm);
		}

		FILE *fp = fopen(seg_path_cstr, "r");
		//std::cout << "end of unmap  " << (char*)(segment->data) << " " << seg_path_cstr << std::endl;
		fread(segment->data, 1, size_to_create, fp);
		//std::cout << "end of unmap1  " << (char*)(segment->data) << " " << size_to_create << std::endl;
		fclose(fp);
	} 
	else {
		FILE *fp = fopen(seg_path_cstr, "w");
		fclose(fp);
		truncate(seg_path_cstr, size_to_create);
	}
	segnamemap.insert(std::pair<std::string,segment_t *>(std::string(segname),segment));
	segbasemap.insert(std::pair<void *,segment_t *>(segment->data,segment));
	return segment->data;
}

void rvm_unmap(rvm_t rvm, void *segbase){
	std::map<void*, segment_t*>::iterator it;
	it = segbasemap.find(segbase);
	if (it == segbasemap.end()) {
		return;
	}
	else {
		segnamemap.erase(std::string(it->second->seg_name));
		free(it->second);
		segbasemap.erase(segbase);
	}
}

void rvm_destroy(rvm_t rvm, const char *segname){
	std::map<std::string, segment_t*>::iterator it;
	if (segnamemap.find(std::string(segname)) == segnamemap.end())
	{
		std::string seg_path(rvm.directory);
		seg_path = seg_path+"/"+segname+".seg";
		remove(seg_path.c_str());
	}
	else {
		std::cerr<< "Error: " + std::string(segname)+" is still mapped" <<std::endl;
		return;
	}
}

trans_t rvm_begin_trans(rvm_t rvm, int numsegs, void **segbases) {
	trans_t trans_id = -1;
	std::map<void*, segment_t*>::iterator it;
	for (int i=0; i< numsegs; i++) {
		//first check whether the segment is mapped or not
		it = segbasemap.find(segbases[i]);
		if (it == segbasemap.end())
			return -1;
		else {
			if (it->second->trans_id != -1) 		//second check whether the segment is being modified by a transation
				return -1;
		}
	}

	trans_id = trans_num++;
	for (int i=0; i< numsegs; i++) {
		it = segbasemap.find(segbases[i]);
		it->second->trans_id = trans_id; 
	}
	return trans_id;
}


void rvm_about_to_modify(trans_t tid, void *segbase, int offset, int size) {

	assert(tid != -1);
	std::map<void*, segment_t*>::iterator it;
	it = segbasemap.find(segbase);
	if (it != segbasemap.end()) {
		if (it->second->trans_id == tid) {
			assert(offset+size<=it->second->seg_size);

			region_t *region =(region_t *) malloc (sizeof(region_t));
			region->segment = it->second;
			region->offset = offset;
			region->size = size;
			region->undo_data = (void *)malloc(size);
			memcpy(region->undo_data, (char*)segbase+offset, size);

			std::map<trans_t, std::list<region_t *> *>::iterator itr; 
			
			itr = txregions.find(tid);
			if (itr == txregions.end()){
				std::list<region_t *> *region_list = new std::list<region_t *>;
				region_list->push_back(region);
				txregions.insert(std::pair<trans_t,std::list<region_t *> *>(tid,region_list));
			}
			else {
				itr->second->push_back(region);
			}

		}
		else {
			std::cerr<<"the segment is not being associated by this transation" <<std::endl;
			return;
		}
	}
	else {
		std::cerr<<"the segment is being modified by another transation" <<std::endl;
		return;
	}


}

void rvm_commit_trans(trans_t tid) {
	assert(tid != -1);
	std::map<trans_t, std::list<region_t *> *>::iterator itr; 

	itr = txregions.find(tid);
	if (itr != txregions.end()){
		FILE *fp = fopen(LOG, "ab");
		std::list<region_t *> *region_list;
		region_list = itr->second;

		trans_header_t txheader;
		trans_end_t txender;
		txheader.recordnum = 0;
		txheader.length = sizeof(trans_header_t) + sizeof(trans_end_t);
		txheader.tid = tid;
		for (std::list<region_t *>::iterator it = region_list->begin(); it!=region_list->end(); it++)
		{
			txheader.recordnum++;
			txheader.length += (sizeof(int)+strlen((*it)->segment->seg_name)+sizeof(int)*2+(*it)->size);
		}

		txender.recordnum = txheader.recordnum;
		txender.length = txheader. length;
		txender.tid = tid;

		fwrite(&txheader, 1, sizeof(trans_header_t), fp);
		for (std::list<region_t *>::iterator it = region_list->begin(); it!=region_list->end(); it++)
		{
			char buf[256];
			int len;
			len = strlen((*it)->segment->seg_name);
			memcpy(buf, (char*)&len, sizeof(int));
			memcpy(buf + sizeof(int), (*it)->segment->seg_name, len);
			memcpy(buf + sizeof(int)+len, &((*it)->offset), sizeof(int));
			memcpy(buf + sizeof(int)+len+ sizeof(int), &((*it)->size), sizeof(int));
			fwrite(buf, 1, sizeof(int)+len+ sizeof(int)*2, fp);
			fwrite((char *)(*it)->segment->data + (*it)->offset, 1, (*it)->size, fp);
		}
		fwrite(&txender, 1, sizeof(trans_end_t), fp);
		fclose(fp);

		//delete the information with transation id
		for (std::list<region_t *>::iterator it = region_list->begin(); it!=region_list->end(); it++) {
			(*it)->segment->trans_id = -1;
			free((*it)->undo_data);
			free(*it);
		}
		delete txregions[tid];
		txregions.erase(tid);

	}
	//must: tid<->segment it is associated with
	std::map<std::string, segment_t *>::iterator it;
	for(it=segnamemap.begin(); it!=segnamemap.end(); it++)
	{
		if(it->second->trans_id == tid)
			it->second->trans_id = -1;
	}

}

void rvm_abort_trans(trans_t tid) {
	assert(tid != -1);
	std::map<trans_t, std::list<region_t *>* >::iterator itr; 
	itr = txregions.find(tid);
	if (itr != txregions.end()){
		std::list<region_t *> *region_list;
		region_list = itr->second;
		for (std::list<region_t *>::reverse_iterator it = region_list->rbegin(); it!=region_list->rend(); it++) {
			memcpy((char *)(*it)->segment->data + (*it)->offset, (*it)->undo_data, (*it)->size);
			(*it)->segment->trans_id = -1;
			free((*it)->undo_data);
			free(*it);
		}
		//delete the information with transation id
		delete txregions[tid];
		txregions.erase(tid);
	}
		//must: tid<->segment it is associated with
	std::map<std::string, segment_t *>::iterator it;
	for(it=segnamemap.begin(); it!=segnamemap.end(); it++)
	{
		if(it->second->trans_id == tid)
			it->second->trans_id = -1;
	}
}

void rvm_truncate_log(rvm_t rvm) {
	FILE *fp = fopen(LOG, "rb");
	size_t fileLen = 0;
	if (fp != NULL) {
		fseek(fp, 0, SEEK_END);
		fileLen = ftell(fp);
		fseek(fp, 0, SEEK_SET);
	}
	while(!feof(fp)) {
		std::vector<logrecordheader_t* > record_list(0);

		if (ftell(fp) + sizeof(trans_header_t) > fileLen) {
			break;
		}

		trans_header_t r_txheader;
		fread(&r_txheader, 1, sizeof(trans_header_t), fp);
		
		if (ftell(fp) + r_txheader.length - sizeof(trans_header_t) > fileLen) {
			break;
		}

		for (int i = 0; i < r_txheader.recordnum; i++) {
			logrecordheader_t* record = (logrecordheader_t*) malloc (sizeof(logrecordheader_t));
			memset(record->segmentname, 0, 256);
			int segnameLen;
			fread(&segnameLen,  1, sizeof(int), fp);
			fread(&(record->segmentname), 1, segnameLen, fp);
			fread(&(record->region_offset), 1, sizeof(int),  fp);
			fread(&(record->region_size), 1, sizeof(int), fp);
			record->data = (void*)malloc(record->region_size);
			fread(record->data, 1, record->region_size, fp);
			record_list.push_back(record);
			//std::cout << segnameLen << record->segmentname << " " << record->region_offset <<" " <<record->region_size<<std::endl;
		}
		trans_end_t r_txender;
		fread(&r_txender, 1, sizeof(trans_end_t),  fp);
		if (r_txender.tid != r_txheader.tid || 
			r_txender.length != r_txheader.length ||
			r_txender.recordnum != r_txheader.recordnum) {
			for (std::vector<logrecordheader_t *>::iterator it = record_list.end() - r_txheader.recordnum;it != record_list.end(); it++) {
				free((*it)->data);
				free(*it);            
          //delete a pointer in struct?
			}
			record_list.erase(record_list.end() - r_txheader.recordnum, record_list.end());
		}
		r_txheader.recordnum = 0;
		r_txender.recordnum = 0;

		for (size_t i = 0; i < record_list.size(); i++) {
			char segment_file_path[256];
			memset(segment_file_path, 0, 256);
			strcat(segment_file_path, rvm.directory);
			strcat(segment_file_path,"/");
			strcat(segment_file_path, record_list[i]->segmentname);
			strcat(segment_file_path, ".seg");
			//std::cout << segment_file_path << std::endl;
			//std::cout << strlen(segment_file_path)<< std::endl;
			FILE *backupFile = fopen(segment_file_path,"r+");
			//std::cout << (char*)record_list[i]->data << std::endl;
			fseek(backupFile, record_list[i]->region_offset, SEEK_SET);
			fwrite(record_list[i]->data, 1, record_list[i]->region_size, backupFile);
			fflush(backupFile);
			fclose(backupFile);
		}
		for (std::vector<logrecordheader_t *>::iterator it = record_list.begin(); it != record_list.end(); it++) {
			free((*it)->data);		
			free(*it);
		}
		record_list.clear();
	}
	fclose(fp);
	FILE* rfp = fopen(LOG, "w");
	fclose(rfp);
}

