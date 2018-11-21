/**
 * @file AnnexControl.cpp 软件控制接口定义文件
 * @version 0.1
 * @date 2017-10-15
 */

#include <boost/algorithm/string.hpp>
#include "AnnexControl.h"
#include "globaldef.h"
#include "GLog.h"

AnnexControl::AnnexControl(io_service* iomain) {
	iomain_ = iomain;
	param_.LoadFile(gConfigPath);
}

AnnexControl::~AnnexControl() {
	StopService();
}

bool AnnexControl::StartService() {
	string mqname = "msgque_";
	mqname += DAEMON_NAME;
	register_messages();
	if (!Start(mqname.c_str())) {
		_gLog.Write(LOG_FAULT, NULL, "failed to create message queue<%s>", mqname.c_str());
		return false;
	}
	if (!connect_server(false)) {
		_gLog.Write(LOG_FAULT, NULL, "failed to connect server");
		return false;
	}
	if (!connect_cooler(true)) {
		_gLog.Write(LOG_FAULT, NULL, "failed to connect cooler");
		return false;
	}
//	if (!connect_vacuum(true)) {
//		_gLog.Write(LOG_FAULT, NULL, "failed to connect vacuum");
//		return false;
//	}
//	if (param_.enableNTP) {
//		ntp_ = make_ntp(param_.hostNTP.c_str(), 123, param_.maxDiffNTP);
//		ntp_->EnableAutoSynch(true);
//	}

	return true;
}

void AnnexControl::StopService() {
	interrupt_thread(thrdnetwork_);
    Stop();
}

void AnnexControl::register_messages() {
	const CBSlot& slot1 = boost::bind(&AnnexControl::on_connect_network, this, _1, _2);
	const CBSlot& slot2 = boost::bind(&AnnexControl::on_receive_network, this, _1, _2);
	const CBSlot& slot3 = boost::bind(&AnnexControl::on_close_network,   this, _1, _2);
	const CBSlot& slot4 = boost::bind(&AnnexControl::on_close_cooler,    this, _1, _2);
	const CBSlot& slot5 = boost::bind(&AnnexControl::on_close_vacuum,    this, _1, _2);

	RegisterMessage(MSG_CONNECT_NETWORK, slot1);
	RegisterMessage(MSG_RECEIVE_NETWORK, slot2);
	RegisterMessage(MSG_CLOSE_NETWORK,   slot3);
	RegisterMessage(MSG_CLOSE_COOLER,    slot4);
	RegisterMessage(MSG_CLOSE_VACUUM,    slot5);
}

bool AnnexControl::connect_server(bool async) {
	if (!param_.bServer) return true;

	const CBSlot& slot1 = boost::bind(&AnnexControl::network_receive, this, _1, _2);
	const CBSlot& slot2 = boost::bind(&AnnexControl::network_connect, this, _1, _2);
	tcp_ = maketcp_client();
	tcp_->RegisterRead(slot1);
	if (!async) {
		if (!tcp_->Connect(param_.ipServer, param_.portServer)) {
			_gLog.Write(LOG_WARN, NULL, "failed to connect server<%s:%d>",
					param_.ipServer.c_str(), param_.portServer);
			return false;
		}
		else _gLog.Write("SUCCEED: connection with server");
	}
	else {
		tcp_->RegisterConnect(slot2);
		tcp_->AsyncConnect(param_.ipServer, param_.portServer);
	}
	return true;
}

bool AnnexControl::connect_serial(int devtype, Annex *device) {
	if (!(devtype == 1 || devtype == 2)) return false;

	string portname = device->portName;
	int baudrate = device->baudRate;
	if (devtype == 1) {// 温控
		const CoolerCtl::CBSlot& slot = boost::bind(&AnnexControl::cooler_receive, this, _1, _2);
		CoolCPtr one = make_cooler();
		one->RegisterResult(slot);
		if (one->Start(portname, baudrate)) {
			_gLog.Write(LOG_WARN, NULL, "failed to connect COOLER<%s>", portname.c_str());
			return false;
		}
		else {
			_gLog.Write("SUCCED: connection with COOLER<%s>", portname.c_str());
			one->CoupleNetwork(tcp_, param_.groupid);
			one->SetDatabase(param_.urlDB);
			cctl_.push_back(one);

			for (vector<uint8_t>::iterator it = device->idd.begin(); it != device->idd.end(); ++it) {
				one->AddDevice(*it);
			}
		}
	}
	else {// 真空
		const VacuumCtl::CBSlot& slot = boost::bind(&AnnexControl::vacuum_receive, this, _1, _2);
		VacuumCPtr one = make_vacuum();
		one->RegisterResult(slot);
		if (one->Start(portname, baudrate)) {
			_gLog.Write(LOG_WARN, NULL, "failed to connect VACUUM<%s>", portname.c_str());
			return false;
		}
		else {
			_gLog.Write("SUCCED: connection with VACUUM<%s>", portname.c_str());
			one->CoupleNetwork(tcp_, param_.groupid);
			vctl_.push_back(one);

			for (vector<uint8_t>::iterator it = device->idd.begin(); it != device->idd.end(); ++it) {
				one->AddDevice(*it);
			}
		}
	}
	return true;
}

bool AnnexControl::connect_cooler(bool init) {
	AnnexVec &cooler = param_.cooler;
	AnnexVec::iterator it1, it1end = cooler.end();
	CoolCVec::iterator it2, it2end;
	string portname;
	bool need(init);

	for (it1 = cooler.begin(); it1 != it1end; ++it1) {
		portname = (*it1).portName;
		it2end = cctl_.end();

		if (!init) {
			for (it2 = cctl_.begin(); it2 != it2end && !boost::iequals(portname, (*it2)->GetPortname()); ++it2);
			need = it2 == it2end;
		}
		if (need && !connect_serial(1, &(*it1)))
			return false;
	}

	return true;
}

bool AnnexControl::connect_vacuum(bool init) {
	AnnexVec &vacuum = param_.vacuum;
	string portname;
	AnnexVec::iterator it1, it1end = vacuum.end();
	VacuumCVec::iterator it2, it2end;
	bool need(init);

	for (it1 = vacuum.begin(); it1 != it1end; ++it1) {
		portname = (*it1).portName;
		it2end = vctl_.end();

		if (!init) {
			for (it2 = vctl_.begin(); it2 != it2end && !boost::iequals(portname, (*it2)->GetPortname()); ++it2);
			need = it2 == it2end;
		}
		if (need && !connect_serial(2, &(*it1)))
			return false;
	}

	return true;
}

void AnnexControl::network_connect(const long client, const long ec) {
	PostMessage(MSG_CONNECT_NETWORK, client, ec);
}

void AnnexControl::network_receive(const long client, const long ec) {
	PostMessage(!ec ? MSG_RECEIVE_NETWORK : MSG_CLOSE_NETWORK);
}

void AnnexControl::cooler_receive(long client, long ec) {
	PostMessage(MSG_CLOSE_COOLER, client);
}

void AnnexControl::vacuum_receive(long client, long ec) {
	PostMessage(MSG_CLOSE_VACUUM, client);
}

void AnnexControl::on_connect_network(const long client, const long ec) {
	if (!ec) {
		_gLog.Write("SUCCEED: connection with server");
		if (thrdnetwork_.unique()) {
			thrdnetwork_->interrupt();
			thrdnetwork_->join();
			thrdnetwork_.reset();
		}
		/* 关联串口设备 */
		for (CoolCVec::iterator it = cctl_.begin(); it != cctl_.end(); ++it)
			(*it)->CoupleNetwork(tcp_, param_.groupid);
		for (VacuumCVec::iterator it = vctl_.begin(); it != vctl_.end(); ++it)
			(*it)->CoupleNetwork(tcp_, param_.groupid);
	}
	else {
		tcp_.reset();
		_gLog.Write(LOG_WARN, NULL, "failed to connect server<%s:%d>",
				param_.ipServer.c_str(), param_.portServer);
	}
}

void AnnexControl::on_receive_network(const long client, const long ec) {

}

void AnnexControl::on_close_network(const long client, const long ec) {
	_gLog.Write("CLOSED: connection with server");
	for (CoolCVec::iterator it = cctl_.begin(); it != cctl_.end(); ++it) (*it)->DecoupleNetwork();
	for (VacuumCVec::iterator it = vctl_.begin(); it != vctl_.end(); ++it) (*it)->DecoupleNetwork();
	tcp_.reset();
	thrdnetwork_.reset(new boost::thread(&AnnexControl::thread_network, this));
}

void AnnexControl::on_close_cooler(const long client, const long ec) {
	CoolerCtl *ptr = (CoolerCtl*) client;
	CoolCVec::iterator it, itend = cctl_.end();

	_gLog.Write(LOG_WARN, NULL, "CLOSED: connection with cooler<%s>", ptr->GetPortname());
	for (it = cctl_.begin(); it != itend && (*it).get() != ptr; ++it);
	cctl_.erase(it);
}

void AnnexControl::on_close_vacuum(const long client, const long ec) {
	VacuumCtl *ptr = (VacuumCtl*) client;
	VacuumCVec::iterator it, itend = vctl_.end();

	_gLog.Write(LOG_WARN, NULL, "CLOSED: connection with vacuum<%s>", ptr->GetPortname());
	for (it = vctl_.begin(); it != itend && (*it).get() != ptr; ++it);
	vctl_.erase(it);
}

void AnnexControl::thread_network() {
	boost::chrono::minutes period(1);

	while(1) {
		boost::this_thread::sleep_for(period);
		if (!tcp_.unique()) connect_server();
	}
}
