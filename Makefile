all:
	g++ -std=c++11 exchange.cpp -o Exchange.app

run:
	./Exchange.app test_cases/test2.txt -s -t -v -m

clean:
	rm Exchange.app
