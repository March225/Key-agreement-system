src = $(wildcard ./src/*.cpp)
objs = $(patsubst %.cpp,%.o,$(src))
target1 = ./lib/libencrypt.so
target2 = ./lib/libdecrypt.so
target3 = test
INCLUDE = -I./include
LIBS = -lstdc++ -lcrypto -lssl
all: $(target1) $(target2) $(target3)
.PHONY : all 
$(target1):$(objs)
	g++ -shared $(objs) -o $@ $(LIBS) 

$(target2):$(objs)
	g++ -shared $(objs) -o $@ $(LIBS) 

$(target3):$(addprefix $(target3), .cpp)
	g++ -o $@ $< $(INCLUDE) -L./lib -lencrypt -ldecrypt 

%.o:%.cpp
	g++ -std=c++11 -fpic -c -o $@ $< $(INCLUDE) 

.PHONY:clean
clean:
	rm -f $(target1) $(target2) $(target3) $(objs)

