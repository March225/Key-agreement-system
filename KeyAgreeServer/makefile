src = $(wildcard *.cpp ./src/*/*.cpp)
objs = $(patsubst %.cpp,%.o,$(src))
target = svrMain
INCLUDE = -I ./include
LIBS = -lstdc++ -lssl -lcrypto -lpthread -lmysqlclient
$(target):$(objs)
	g++ -o $@ $(objs) $(LIBS) 

%.o:%.cpp
	g++ -std=c++11 -c -o $@ $< $(INCLUDE) 

.PHONY:clean
clean:
	rm -f $(target) $(objs)

