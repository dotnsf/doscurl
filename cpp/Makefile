#
# DOSCurl Makefile for mTCP
#

# mTCP directories
tcp_h_dir = C:\mTCP\src\TCPINC
tcp_c_dir = C:\mTCP\src\TCPLIB
common_h_dir = C:\mTCP\src\INCLUDE

# Memory model and compile options
memory_model = -ml
compile_options = -0 $(memory_model) -DCFG_H="doscurl.cfg" -oh -ok -ot -s -oa -ei -zp2 -zpw -we -ob -ol+ -oi+
compile_options += -i=$(tcp_h_dir) -i=$(common_h_dir)

# mTCP objects (from TCPLIB)
tcpobjs = packet.obj arp.obj eth.obj ip.obj tcp.obj tcpsockm.obj udp.obj utils.obj dns.obj timer.obj ipasm.obj trace.obj

# DOSCurl objects
objs = doscurl.obj

all : clean doscurl.exe

clean : .symbolic
  @del doscurl.exe 2>nul
  @del *.obj 2>nul
  @del *.map 2>nul

.asm : $(tcp_c_dir)

.cpp : $(tcp_c_dir)

.asm.obj :
  wasm -0 $(memory_model) $[*

.cpp.obj :
  wpp $[* $(compile_options)

doscurl.exe: $(tcpobjs) $(objs)
  wlink system dos option map option eliminate option stack=8192 name $@ file *.obj

