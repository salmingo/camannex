/**
 * @file CoolerCtl.cpp 温度控制器接口定义文件
 * @date 2017-10-15
 * @version 0.1
 **/

#include <boost/make_shared.hpp>
#include <boost/format.hpp>
#include <stdlib.h>
#include "CoolerCtl.h"
#include "AMath.h"
#include "GLog.h"

using namespace AstroUtil;
using namespace boost::posix_time;

//////////////////////////////////////////////////////////////////////////////
static uint8_t COOLER_MONITOR[] = { CFID_READ_VOL, CFID_READ_CUR, CFID_READ_T1, CFID_READ_T2, CFID_READ_COOLSET };

CoolCPtr make_cooler() {
	return boost::make_shared<CoolerCtl>();
}

CoolerCtl::CoolerCtl() {
	head_ = ":";
	tail_ = "\r\n";
	nhead_ = head_.size();
	ntail_ = tail_.size();
}

CoolerCtl::~CoolerCtl() {
}

void CoolerCtl::check_data(int nDev) {
	if (nDev != data_.size()) {
		data_.resize(nDev);
		for (int i = 0; i < nDev; ++i) data_[i].idd = allDev_[i];
	}
}

void CoolerCtl::generate_directive(uint8_t idd) {
	int n = sizeof(COOLER_MONITOR) / sizeof(uint8_t);
	uint8_t idf;
	for (int i = 0; i < n; ++i) {
		Directive one(idf = COOLER_MONITOR[i]);
		one.len = ControllerBase::encode_data(idd, idf, one.msg);
		append_directive(one);
	}
}

int CoolerCtl::encode_data(uint8_t idd, uint8_t idf, const char* value, int len, char *output) {
	int i(-1);
	uint8_t lrc, t, base_0(0x30), base_1(0x37);

	output[++i] = ':';	// 引导符
	// 设备编号
	output[++i] = ((t = idd / 16) >= 0x0A) ? (t + base_1) : (t + base_0);
	output[++i] = ((t = idd % 16) >= 0x0A) ? (t + base_1) : (t + base_0);
	// 功能编码
	output[++i] = ((t = idf / 16) >= 0x0A) ? (t + base_1) : (t + base_0);
	output[++i] = ((t = idf % 16) >= 0x0A) ? (t + base_1) : (t + base_0);
	// 数据
	for (int j = 0; j < len; ++j) {
		output[++i] = ((t = value[j] / 16) >= 0x0A) ? (t + base_1) : (t + base_0);
		output[++i] = ((t = value[j] % 16) >= 0x0A) ? (t + base_1) : (t + base_0);
	}
	output[++i] = 0;		// 检验码
	output[++i] = 0;		// 校验码
	output[++i] = 0x0D;	// 回车
	output[++i] = 0x0A;	// 换行
	output[++i] = 0;	// 结束符

	lrc = LRC(output, i);
	output[i - 4] = ((t = lrc / 16) >= 0x0A) ? (t + base_1) : (t + base_0);
	output[i - 3] = ((t = lrc % 16) >= 0x0A) ? (t + base_1) : (t + base_0);

	return i;
}

int CoolerCtl::decode_data(int len) {
	if ((len < 9)) return -1;	// 格式错误
	if ((len - 9) % 2) return -2;	// 格式错误

	char *ptr = bufrcv_.get();
	int first(5), last(len - 5), i, j;
	string strval;
	uint8_t idd, idf;
	double value;

	idd = decode_uint8(ptr[1], ptr[2]);
	idf = decode_uint8(ptr[3], ptr[4]);
	if (len > 9) {
		for (i = first, j = 0; i <= last; i += 2, ++j) strval += decode_uint8(ptr[i], ptr[i + 1]);
		value = atof(strval.c_str());
	}

	CoolerData *data = find_device(idd);
	if (data) {// 分类处理
		if      (idf == CFID_READ_VOL)     data->set_voltage(value);
		else if (idf == CFID_READ_CUR)     data->set_current(value);
		else if (idf == CFID_READ_T1)      data->set_coolget(value);
		else if (idf == CFID_READ_T2)      data->set_thot(value);
		else if (idf == CFID_READ_COOLSET) data->set_coolset(value);
	}

	return 0;
}

void CoolerCtl::write_log() {
	int n = data_.size();
	for (int i = 0; i < n; ++i) {
		CoolerData& data = data_[i];
		if (!data.dirty) continue;
		data.dirty = false;
		_gLog.Write("Cooler<%d>: (Voltage, Current, Hot, Coolget, Coolset) = %.1f  %.1f  %.1f  %.1f  %.1f",
				data.idd, data.vol, data.cur, data.thot, data.coolget, data.coolset);
	}
}

void CoolerCtl::upload_database() {
//	  int uploadTemperature(const char *groupId, const char *unitId, const char *camId,
//	          float voltage, float current, float thot, float coolget, float coolset, const char *time, char statusstr[]);
	string now = to_iso_extended_string(second_clock::universal_time());
	int n = data_.size();
	char uid[10], cid[10], status[200];
	for (int i = 0; i < n; ++i) {
		CoolerData& x = data_[i];
		sprintf(uid, "%03d", x.idd / 10);
		sprintf(cid, "%03d", x.idd);
		if (db_->uploadTemperature(grpid_.c_str(), uid, cid, x.vol, x.cur, x.thot, x.coolget,
				x.coolset, now.c_str(), status)) {
			_gLog.Write(LOG_WARN, NULL, "cooler[%s] upload to database failed: %s", cid, status);
		}
	}
}

void CoolerCtl::network_respond() {
	int n = data_.size(), len;
	boost::format fmt("%03d");
	apcooler proto = boost::make_shared<ascii_proto_cooler>();
	uint8_t idd;
	const char *tosend;

	for (int i = 0; i < n; ++i) {
		CoolerData& data = data_[i];
		proto->gid = grpid_;
		proto->uid = (fmt % (data.idd / 10)).str();
		proto->cid = (fmt % data.idd).str();
		proto->voltage = data.vol;
		proto->current = data.cur;
		proto->hotend  = data.thot;
		proto->coolget = data.coolget;
		proto->coolset = data.coolset;
		tosend = ascproto_->CompactCooler(proto, len);
		tcp_->Write(tosend, len);
	}
}

CoolerData* CoolerCtl::find_device(uint8_t idd) {
	int n = data_.size(), i;
	for (i = 0; i < n && data_[i].idd != idd; ++i);
	return i == n ? NULL : &data_[i];
}
