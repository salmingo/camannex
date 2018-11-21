/**
 * @file CoolerCtl.h 温度控制器接口声明文件
 * @note
 * - 串口通信采用modbus ASCII协议
 * - 为GWAC系统封装, 非通用modbus协议串口通信接口
 *
 * @date 2017-10-15
 * @version 0.1
 * - 维护串口连接
 * - 串行化发送编码数据
 * - 解析接收数据
 */

#ifndef COOLERCTL_H_
#define COOLERCTL_H_

#include <boost/container/stable_vector.hpp>
#include "ControllerBase.h"

//////////////////////////////////////////////////////////////////////////////
/* 数据类型 -- 温控 */
enum CoolerFuncID {// 温控功能编码
	CFID_READ_VOL = 0x21,		//< 读工作电压
	CFID_READ_CUR = 0x22,		//< 读工作电流
	CFID_READ_T1  = 0x23,		//< 读冷端温度, 即探测器温度
	CFID_READ_T2  = 0x24,		//< 读热端温度
	CFID_READ_COOLSET  = 0x15,	//< 读制冷温度
	CFID_WRITE_COOLSET = 0x16	//< 写制冷温度
};

struct CoolerData {// 单个温控器数据
	bool dirty;		//< 修改标记
	uint8_t idd;		//< 设备编号
	double  vol;		//< 实时电压
	double	cur;		//< 实时电流
	double	thot;	//< 热端温度
	double	coolset;//< 制冷温度
	double	coolget;//< 探测器温度

public:
	void set_voltage(double value) {
		if (vol != value) {
			vol = value;
			dirty = true;
		}
	}

	void set_current(double value) {
		if (cur != value) {
			cur = value;
			dirty = true;
		}
	}

	void set_thot(double value) {
		if (thot != value) {
			thot = value;
			dirty = true;
		}
	}

	void set_coolset(double value) {
		if (coolset != value) {
			coolset = value;
			dirty = true;
		}
	}

	void set_coolget(double value) {
		if (coolget != value) {
			coolget = value;
			dirty = true;
		}
	}
};
typedef boost::container::stable_vector<CoolerData> CoolDVec;	//< 温控数据集合

class CoolerCtl : public ControllerBase {
public:
	CoolerCtl();
	virtual ~CoolerCtl();

protected:
	CoolDVec data_;	//< 温控数据

protected:
	/* 功能: 数据编码与解码 */
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
	CoolerData* find_device(uint8_t idd);
};
typedef boost::shared_ptr<CoolerCtl> CoolCPtr;
extern CoolCPtr make_cooler();

#endif /* COOLERCTL_H_ */
