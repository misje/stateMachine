default: clean dist run

dist:
	mkdir bin/
	gcc -std=c99 -I src src/stateMachine.c examples/stateMachineExample.c  -o bin/example
	
run:
	./bin/example
	
clean:
	rm -rf bin