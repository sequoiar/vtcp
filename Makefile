all: vtcpd bind connect
vtcpd: vtcpd.cpp
	g++ -o vtcpd vtcpd.cpp
bind: bind.cpp
	g++ -o bind bind.cpp vtcp.cpp
connect: connect.cpp
	g++ -o connect connect.cpp vtcp.cpp
clean:
	rm -rf vtcpd bind connect
