CXXFLAGS=-std=c++17
LDFLAGS=-loil -lpq -lsqlite3 -lcurl

ifndef release
CXXFLAGs+=-g -Wall -Wextra
else
CXXFLAGS+=-O3 -Os
endif

default: diskUsage

diskUsage: diskUsage.o color.o
	${CXX} -o $@ $^ ${LDFLAGS}

%.o: %.cpp
	${CXX} -c -o $@ ${CXXFLAGS} $^

clean:
	rm -f diskUsage ./*.o

