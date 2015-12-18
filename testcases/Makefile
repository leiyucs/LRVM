CC=g++
OPTIONS= -g 

TESTCASESDIR=.

all: basic multi abort multi-abort truncate semantics_01 semantics_02 semantics_03 basic9 fullbinary
basic: 
	$(CC) $(OPTIONS) -o basic $(TESTCASESDIR)/basic.c ../librvm.a

multi: 
	$(CC) $(OPTIONS) -o multi $(TESTCASESDIR)/multi.c ../librvm.a

abort: 
	$(CC) $(OPTIONS) -o abort $(TESTCASESDIR)/abort.c ../librvm.a

multi-abort: 
	$(CC) $(OPTIONS) -o multi-abort $(TESTCASESDIR)/multi-abort.c ../librvm.a

truncate: 
	$(CC) $(OPTIONS) -o truncate $(TESTCASESDIR)/truncate.c ../librvm.a

semantics_01:
	$(CC) $(OPTIONS) -o semantics_01  $(TESTCASESDIR)/semantics_01.c ../librvm.a

semantics_02:
	$(CC) $(OPTIONS) -o semantics_02  $(TESTCASESDIR)/semantics_02.c ../librvm.a

semantics_03:
	$(CC) $(OPTIONS) -o semantics_03  $(TESTCASESDIR)/semantics_03.c ../librvm.a

basic9: 
	$(CC) $(OPTIONS) -o basic9 $(TESTCASESDIR)/basic9.c ../librvm.a

fullbinary: 
	$(CC) $(OPTIONS) -o fullbinary $(TESTCASESDIR)/fullbinary.c ../librvm.a

clean:
	rm -rf *.o basic multi abort multi-abort truncate semantics_01 semantics_02 semantics_03 basic9 fullbinary

