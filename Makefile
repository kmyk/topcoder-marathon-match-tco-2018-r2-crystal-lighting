.PHONY: build
.DEFAULT: build

PROBLEM := CrystalLighting
CXX := g++
CXXFLAGS := -std=c++11 -Wall -O2 -g -DLOCAL

build: a.out tester.jar

run: a.out tester.jar
	java -jar tester.jar -exec ./a.out

a.out: main.cpp ${PROBLEM}.cpp
	${CXX} ${CXXFLAGS} $<

tester.jar: ${PROBLEM}Vis.java
	javac $<
	jar cvfe tester.jar ${PROBLEM}Vis *.class

URL := https://community.topcoder.com/longcontest/?module=ViewProblemStatement&compid=64279&rd=17179
submit:
	oj submit '${URL}' --language C++ ${PROBLEM}.cpp -y --open
submit/full:
	oj submit '${URL}' --language C++ ${PROBLEM}.cpp -y --open --full

standings:
	oj get-standings '${URL}' --format=csv | \
		sed 's/\w\w\.\w\w\.\w\w\w\w \w\w:\w\w:\w\w/& EST/ ; y/ /~/ ; :1 ; s/,,/,-,/ ; t1 ; s/$$/,|/' | \
		column -t -s , | \
		sed 's/\( \+\)|\?/\1| /g ; s/^/| / ; s/.$$// ; y/~/ / ; 1 { p ; s/[^|]/-/g }'

image/11: a.out tester.jar
	java -jar tester.jar -exec ./a.out -debug -plain -mark -rays -seed 11 -save -size 32
	optipng 11.png

timestamp := $(shell date +%s)
size := 100
score: a.out tester.jar
	-mkdir log
	cp a.out log/${timestamp}.bin
	parallel -- bash score.sh ${timestamp} {} ::: $$(seq ${size})
	python3 stats.py table log/${timestamp}.json
	python3 stats.py summary log/${timestamp}.json

requirements:  # for new cloud environments
	apt update
	apt upgrade -y
	apt install -y build-essential g++ default-jre default-jdk python3 python3-pip
	LANG=en_US.UTF-8 LC_ALL=en_US.UTF-8 pip3 install --requirement requirements.txt
