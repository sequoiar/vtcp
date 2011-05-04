vtcpd: vtcpd.cpp
	g++ -o vtcpd vtcpd.cpp
bind: bind.cpp
	g++ -o bind bind.cpp
connect: connect.cpp
	g++ -o connect connect.cpp
all: vtcpd bind connect

