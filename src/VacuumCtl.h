/**
 * @file VacuumCtl.h 离子泵压力控制器接口声明文件
 * @date 2017-10-15
 * @version 0.1
 * - 维护串口连接
 * - 维护设备
 * - 监测压力与电压、电流
 */

#ifndef VACUUMCTL_H_
#define VACUUMCTL_H_

#include <boost/container/stable_vector.hpp>
#include "ControllerBase.h"

//////////////////////////////////////////////////////////////////////////////
/* 数据类型 -- 真空度 */
enum VacuumFuncID {// 真空度功能编码
	VFID_RESET = 0x07,		//< 软重启
	VFID_READ_CUR  = 0x0A,	//< 读真空泵流量
	VFID_READ_PRES = 0x0B,	//< 读真空泵压力
	VFID_READ_VOL  = 0x0C	//< 读工作电压
};

struct VacuumData {// 单真空控制器数据
	bool dirty;		//< 修改标志
	uint8_t idd;		//< 设备编号
	double cur;		//< 实时电流
	string pres;		//< 实时气压
	double vol;		//< 实时电压

public:
	void set_current(double value) {
		if (value != cur) {
			cur = value;
			dirty = true;
		}
	}

	void set_pressure(string value) {
		if (value != pres) {
			pres = value;
			dirty = true;
		}
	}

	void set_voltage(double value) {
		if (value != vol) {
			vol = value;
			dirty = true;
		}
	}
};
typedef boost::container::stable_vector<VacuumData> VacuumDVec;	//< 真空数据集合

class VacuumCtl : public ControllerBase {
public:
	VacuumCtl();
	virtual ~VacuumCtl();

protected:
	/* 成员变量 */
	VacuumDVec data_;	//< 温控数据

protected:
	/* 功能: 串口回调函数 */
	/*!
	 * @brief 检查并创建数据存储区
	 * @param nDev 设备数量
	 */
	void check_data(int nDev);
	/*!
	 * @brief 为指定设备生成控制指令
	 * @param idd 设备编号
	 */
	void generate_directive(uint8_t idd);
	/*!
	 * @brief 依照串口通信协议, 对数据进行编码
	 * @param idd    设备编号
	 * @param idf    功能编号
	 * @param value  待编码字符串
	 * @param n      待编码字符串长度
	 * @param output 编码后字符串
	 * @return
	 * 编码后字符串长度. 0表示错误
	 * @note
	 * 虚函数, 继承类完成编码流程后调用基类该函数, 将编码后信息存入列表
	 */
	int encode_data(uint8_t idd, uint8_t idf, const char *value, int n, char *output);
	/*!
	 * @brief 解码数据串
	 * @param len    待解码数据串长度, 量纲: 字节
	 * @return
	 * 数据串解码结果
	 *  0: 成功
	 * -1: 长度不足
	 * -2: 数据长度不足
	 */
	int decode_data(int len);
	/*!
	 * @brief 在日志中记录最新工作状态
	 */
	void write_log();
	/*!
	 * @brief 向数据库上传监测信息
	 */
	void upload_database();
	/*!
	 * @brief 通过网络发送设备状态
	 */
	void network_respond();
	/*!
	 * @brief 查找与设备编号对应的设备数据存储区
	 * @param idd 设备编号
	 * @return
	 * 与设备编号对应的数据存储区指针
	 */
	VacuumData* find_device(uint8_t idd);
};
typedef boost::shared_ptr<VacuumCtl> VacuumCPtr;
extern VacuumCPtr make_vacuum();

#endif /* VACUUMCTL_H_ */
