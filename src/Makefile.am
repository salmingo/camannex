bin_PROGRAMS=camannex
camannex_SOURCES=AMath.cpp GLog.cpp IOServiceKeep.cpp SerialComm.cpp tcpasio.cpp MessageQueue.cpp NTPClient.cpp \
				 ControllerBase.cpp CoolerCtl.cpp VacuumCtl.cpp \
				 AnnexControl.cpp \
				 daemon.cpp \
				 AsciiProtocol.cpp \
				 DataTransfer.cpp \
				 camannex.cpp

camannex_LDFLAGS = -L/usr/local/lib
BOOST_LIBS = -lboost_system-mt -lboost_thread-mt -lboost_chrono-mt  -lboost_date_time-mt -lboost_filesystem-mt
camannex_LDADD = ${BOOST_LIBS} -lrt -lm -lpthread -lcurl
