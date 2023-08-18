exe: client-phase1.cpp client-phase2.cpp client-phase3.cpp client-phase4.cpp client-phase5.cpp
	g++ -pthread client-phase1.cpp -o client-phase1
	g++ -pthread client-phase2.cpp -o client-phase2
	g++ -pthread client-phase3.cpp -lcrypto -o client-phase3
	g++ -pthread client-phase4.cpp -o client-phase4
	g++ -pthread client-phase5.cpp -o client-phase5