	make clean
	make
	echo ----- ./testHof 20 ------
	./testHof 20
	echo ----- ./testHof 200000 ------
	./testHof 200000
	echo ----- ./testHof temp1 20 ------
	./testHof temp1 20
	echo ----- ./testHof temp1 ------
	./testHof temp1
	rm temp1
	echo ----- ./testHof temp1 200000 ------
	./testHof temp1 200000
	echo ----- ./testHof temp1 ------
	./testHof temp1
