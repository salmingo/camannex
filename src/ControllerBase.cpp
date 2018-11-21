/*
 * @file ControllerBase.cpp 定义文件, 温控与真空共用控制接口
 * @version 0.1
 * @date 2017-11-16
 */

#include <boost/make_shared.hpp>
#include <boost/format.hpp>
#include "ControllerBase.h"
#include "GLog.h"

using namespace boost::posix_time;

ControllerBase::ControllerBase() {
	nhead_ = ntail_ = 0;
	bufrcv_.reset(new char[SERIAL_BUFF_SIZE]);
	ascproto_ = make_ascproto();
}

ControllerBase::~ControllerBase() {
	Stop();
}

void ControllerBase::RegisterResult(const CBSlot &slot) {
	if (!cbrslt_.empty()) cbrslt_.disconnect_all_slots();
	cbrslt_.connect(slot);
}

int ControllerBase::Start(string portname, int baudrate) {
	if (portname.empty()) return -1;
	const SerialComm::CBSlot& slot1 = boost::bind(&ControllerBase::serial_read,  this, _1, _2);
	const SerialComm::CBSlot& slot2 = boost::bind(&ControllerBase::serial_write, this, _1, _2);
	serial_ = make_serial();
	serial_->RegisterRead (slot1);
	serial_->RegisterWrite(slot2);
	if (!serial_->Open(portname, baudrate)) {
		_gLog.Write(LOG_FAULT, NULL, "%s", serial_->GetErrdesc());
		return -2;
	}
	
	portname_ = portname;
	thrdCycle_.reset(new boost::thread(boost::bind(&ControllerBase::thread_cycle, this)));
	thrdRespond_.reset(new boost::thread(boost::bind(&ControllerBase::thread_respond, this)));
	thrdHB_.reset(new boost::thread(boost::bind(&ControllerBase::thread_heartbeat, this)));

	return 0;
}

void ControllerBase::Stop() {
	interrupt_thread(thrdHB_);
	interrupt_thread(thrdRespond_);
	interrupt_thread(thrdCycle_);
}

void ControllerBase::CoupleNetwork(TcpCPtr session, string grpid) {
	if (!tcp_.use_count()) tcp_ = session;
	grpid_ = grpid;
}

void ControllerBase::DecoupleNetwork() {
	mutex_lock lck(mtxNet_);
	tcp_.reset();
}

void ControllerBase::SetDatabase(const string& url) {
	if (url.empty()) db_.reset();
	else db_ = boost::make_shared<DataTransfer>(url.c_str());
}

void ControllerBase::AddDevice(uint8_t idd) {
	int n = allDev_.size(), n1 = drct_.size(), i;
	for (i = 0; i < n && idd != allDev_[i]; ++i);
	if (i == n) allDev_.push_back(idd);
}

void ControllerBase::Write(uint8_t idd, uint8_t idf) {
	Directive one(idf);
	one.len = encode_data(idd, idf, one.msg);
	append_directive(one);
}

void ControllerBase::Write(uint8_t idd, uint8_t idf, int value) {
	Directive one(idf);
	one.len = encode_data(idd, idf, value, one.msg);
	append_directive(one);
}

void ControllerBase::Write(uint8_t idd, uint8_t idf, double value) {
	Directive one(idf);
	one.len = encode_data(idd, idf, value, one.msg);
	append_directive(one);
}

const char *ControllerBase::GetPortname() {
	return portname_.c_str();
}

void ControllerBase::append_directive(Directive &drct) {
	mutex_lock lck(mtxDrct_);
	drct_.push_back(drct);
}

void ControllerBase::remove_directive() {
	mutex_lock lck(mtxDrct_);
	drct_.pop_front();
}

int ControllerBase::encode_data(uint8_t idd, uint8_t idf, char *output) {
	return encode_data(idd, idf, NULL, 0, output);
}

int ControllerBase::encode_data(uint8_t idd, uint8_t idf, int value, char *output) {
	boost::format fmt("%d");
	fmt % value;
	return encode_data(idd, idf, fmt.str().c_str(), fmt.size(), output);
}

int ControllerBase::encode_data(uint8_t idd, uint8_t idf, double value, char *output) {
	boost::format fmt("%.1f");
	fmt % value;
	return encode_data(idd, idf, fmt.str().c_str(), fmt.size(), output);
}

uint8_t ControllerBase::decode_uint8(uint8_t b1, uint8_t b2) {
	uint8_t val, t;

	if ((t = b1) >= '0' && b1 <= '9') t -= 0x30;
	else if (b1 >= 'A' && b1 <= 'F') t -= 0x37;
	val = t << 4;
	if ((t = b2) >= '0' && b2 <= '9') t -= 0x30;
	else if (b2 >= 'A' && b2 <= 'F') t -= 0x37;
	val += t;
	return val;
}

/*
 * @note 发送列表中第一条信息
 */
void ControllerBase::first_send() {
	if (serial_.unique() && serial_->IsOpen()) {
		Directive& one = drct_.front();
//		_gLog.Write("tosend: %s", one.msg);
		serial_->Write(one.msg, one.len);
	}
	else _gLog.Write("port is closed");
}

void ControllerBase::serial_read(long client, long ec) {
	uint8_t idd(0xFF), idf(0xFF);
	double value(0.0);

	if (!ec) {// 处理收到的信息
		int ihead(0), itail(0), len;
		if (nhead_) ihead = serial_->Lookup(head_.c_str(), nhead_);
		if (ihead >= 0) itail = serial_->Lookup(tail_.c_str(), ntail_, ihead);
		if (itail > ihead) {
			len = itail - ihead + ntail_;
			serial_->Read(bufrcv_.get(), len, ihead);
			if (!decode_data(len)) cndDrct_.notify_one();
		}
	}
	else if (!cbrslt_.empty()) cbrslt_((long) this, 1); // 接收时遇到错误
}

void ControllerBase::serial_write(long client, long ec) {
	if (ec && !cbrslt_.empty()) cbrslt_((long) this, 2); // 发送时遇到错误
}

/*
 * @brief 周期线程, 定时检测设备状态
 */
void ControllerBase::thread_cycle() {
	boost::chrono::seconds period(20);	// 周期: 20秒
	int n;

	boost::this_thread::sleep_for(boost::chrono::seconds(1));
	while(1) {
		if (!drct_.size()) {// 为所有设备生成监测指令
			n = allDev_.size();
			check_data(n);
			for (int i = 0; i < n; ++i) generate_directive(allDev_[i]);
			first_send();
		}

		boost::this_thread::sleep_for(period);
	}
}

/*
 * @brief 响应线程, 处理串口读出结果
 */
void ControllerBase::thread_respond() {
	boost::mutex mtxDummy;	// 互斥锁哑元
	mutex_lock lckDummy(mtxDummy);

	while(1) {
		cndDrct_.wait(lckDummy);
		boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
		tmlast_ = second_clock::local_time();
		remove_directive();
		if (drct_.size()) first_send();
		else {
			write_log();
			if (db_.unique()) upload_database();
			if (tcp_.use_count() && tcp_->IsOpen()) network_respond();
		}
	}
}

/*
 * @note 心跳线程, 监测串口有效性
 */
void ControllerBase::thread_heartbeat() {
	boost::chrono::seconds period(30);	// 周期: 30秒
	time_duration td;
	int t0 = period.count();

	tmlast_ = second_clock::local_time();
	do {
		boost::this_thread::sleep_for(period);
		td = second_clock::local_time() - tmlast_;
	} while(td.total_seconds() < t0);
	if (!cbrslt_.empty()) cbrslt_((long) this, 3); // 长时间收不到信息
}

void ControllerBase::interrupt_thread(threadptr& thrd) {
	if (thrd.unique()) {
		thrd->interrupt();
		thrd->join();
		thrd.reset();
	}
}
