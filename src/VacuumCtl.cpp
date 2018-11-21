/**
 * @file VacuumCtl.cpp 离子泵压力控制器接口定义文件
 * @date 2017-10-15
 * @version 0.1
 */

#include <boost/make_shared.hpp>
#include <boost/format.hpp>
#include <stdlib.h>
#include "AMath.h"
#include "VacuumCtl.h"
#include "GLog.h"
using namespace boost::posix_time;

//////////////////////////////////////////////////////////////////////////////
static uint8_t VACUUM_MONITOR[] = { VFID_READ_CUR, VFID_READ_PRES, VFID_READ_VOL };

VacuumCPtr make_vacuum() {
	return boost::make_shared<VacuumCtl>();
}

VacuumCtl::VacuumCtl() {
	tail_ = "\r";
	ntail_ = tail_.size();
}

VacuumCtl::~VacuumCtl() {
}

void VacuumCtl::check_data(int nDev) {
	if (nDev != data_.size()) {
		data_.resize(nDev);
		for (int i = 0; i < nDev; ++i) data_[i].idd = allDev_[i];
	}
}

void VacuumCtl::generate_directive(uint8_t idd) {
	int n = sizeof(VACUUM_MONITOR) / sizeof(uint8_t);
	uint8_t idf;
	for (int i = 0; i < n; ++i) {
		Directive one(idf = VACUUM_MONITOR[i]);
		one.len = ControllerBase::encode_data(idd, idf, one.msg);
		append_directive(one);
	}
}

int VacuumCtl::encode_data(uint8_t idd, uint8_t idf, const char *value, int n, char *output) {
	int i(-1);
	uint8_t lrc(0), t, base_0(0x30), base_1(0x37);

	output[++i] = '~';	// 引导符
	output[++i] = ' '; lrc += output[i]; // 空格
	// 设备编号
	output[++i] = ((t = idd / 16) >= 0x0A) ? (t + base_1) : (t + base_0);  lrc += output[i];
	output[++i] = ((t = idd % 16) >= 0x0A) ? (t + base_1) : (t + base_0);  lrc += output[i];
	output[++i] = ' '; lrc += output[i]; // 空格
	// 功能编码
	output[++i] = ((t = idf / 16) >= 0x0A) ? (t + base_1) : (t + base_0); lrc += output[i];
	output[++i] = ((t = idf % 16) >= 0x0A) ? (t + base_1) : (t + base_0); lrc += output[i];
	output[++i] = ' ';  lrc += output[i]; // 空格

	output[++i] = 0;		// 检验码
	output[++i] = 0;		// 校验码
	output[++i] = 0x0D;	// 回车
	output[++i] = 0;

	output[i - 3] = ((t = lrc / 16) >= 0x0A) ? (t + base_1) : (t + base_0);
	output[i - 2] = ((t = lrc % 16) >= 0x0A) ? (t + base_1) : (t + base_0);

	return i;
}

int VacuumCtl::decode_data(int len) {
	if ((len < 12)) return -1;	// 格式错误

	int first(6), last(len - 5), i, j;
	Directive one = drct_.front();
	string strval;
	char *ptr = bufrcv_.get();
	uint8_t idd, idf;

	idd = decode_uint8(ptr[0], ptr[1]);
	idf = one.idf;
	for (i = first, j = 0; i <= last; i += 2, ++j) strval += decode_uint8(ptr[i], ptr[i + 1]);

	VacuumData *data = find_device(idd);
	if (data) {// 分类处理
		if      (idf == VFID_READ_CUR)  data->set_current(atof(strval.c_str()));
		else if (idf == VFID_READ_VOL)  data->set_voltage(atof(strval.c_str()));
		else if (idf == VFID_READ_PRES) data->set_pressure(strval);
	}

	return 0;
}

void VacuumCtl::write_log() {
	int n = data_.size();
	for (int i = 0; i < n; ++i) {
		VacuumData& data = data_[i];
		if (!data.dirty) continue;
		data.dirty = false;
		_gLog.Write("Cooler<%d>: (Voltage, Current, Pressure) = %.1f  %.1f  %s",
				data.idd, data.vol, data.cur, data.pres.c_str());
	}
}

void VacuumCtl::upload_database() {

}

void VacuumCtl::network_respond() {
	int n = data_.size(), len;
	boost::format fmt("%03d");
	apvacuum proto = boost::make_shared<ascii_proto_vacuum>();
	uint8_t idd;
	const char *tosend;

	for (int i = 0; i < n; ++i) {
		VacuumData& data = data_[i];
		proto->gid = grpid_;
		proto->uid = (fmt % (data.idd / 10)).str();
		proto->cid = (fmt % data.idd).str();
		proto->voltage = data.vol;
		proto->current = data.cur;
		proto->pressure= data.pres;
		tosend = ascproto_->CompactVacuum(proto, len);
		tcp_->Write(tosend, len);
	}
}

VacuumData* VacuumCtl::find_device(uint8_t idd) {
	int n = data_.size(), i;
	for (i = 0; i < n && data_[i].idd != idd; ++i);
	return i == n ? NULL : &data_[i];
}
