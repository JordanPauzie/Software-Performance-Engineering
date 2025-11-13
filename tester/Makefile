.PHONY: ALL
ALL: autotester bayeselo pgnstats

.PHONY: autotester
autotester:
	cd autotester && $(MAKE)

.PHONY: pgnstats
pgnstats:
	cd pgnstats && $(MAKE)

.PHONY: bayeselo
bayeselo:
	cd BayesElo && $(MAKE)

referee:
	cp ../player/leiserchess ./referee

.PHONY: clean
clean:
	cd autotester && $(MAKE) clean
	cd BayesElo && $(MAKE) clean
	cd pgnstats && $(MAKE) clean

TEST_FILE := ../tests/basic.txt
BATCH_ID := 

.PHONY: test
test: autotester bayeselo referee
	java -jar ./autotester/lauto.jar ./referee $(TEST_FILE) $(BATCH_ID)
	cd pgn/ && ./pgnrate.tcl ../$(TEST_FILE:.txt=$(if $(BATCH_ID),_$(BATCH_ID).pgn,.pgn))
