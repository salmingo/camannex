/*
 * @file ControllerBase.h 声明文件, 温控与真空共用控制接口
 * @version 0.1
 * @date 2017-11-15
 * @note
 * -
 */

#ifndef CONTROLLERBASE_H_
#define CONTROLLERBASE_H_

#include <list>
#include <vector>
#include <string.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "SerialComm.h"
#include "tcpasio.h"
#include "AsciiProtocol.h"
#include "DataTransfer.h"

using std::list;
using std::vector;

class ControllerBase {
public:
	ControllerBase();
	virtual ~ControllerBase();

public:
	/* 数据结构 */
	/*!
	 * @brief 声明CoolerCtl回调函数类型
	 * @param _1 对象指针
	 * @param _2 错误代码. 0: 正确
	 */
	typedef boost::signals2::signal<void (long, long)> CallbackFunc;
	typedef CallbackFunc::slot_type CBSlot; // 插槽函数

	struct Directive {// 单条控制指令
		uint8_t idf;		//< 功能编号
		int len;			//< 指令字符串有效长度
		char msg[30];	//< 指令字符串存储区

	public:
		Directive() {
			idf = 0;
			len = 0;
		}

		Directive(uint8_t _idf) {
			idf = _idf;
			len = 0;
		}
	};

	typedef boost::unique_lock<boost::mutex> mutex_lock;	//< 互斥锁
	typedef boost::shared_ptr<boost::thread> threadptr;	//< 线程指针
	typedef boost::shared_array<char> charray;	//< 字符型数组
	typedef list<Directive> DrctList;	//< 指令列表

protected:
	/* 成员变量 */
	string portname_;	//< 串口名称
	SerialPtr serial_;	//< 串口接口
	string grpid_;		//< 网络组标志
	vector<uint8_t> allDev_;	//< 与串口关联的设备编号列表

	TcpCPtr tcp_;		//< 网络接口
	AscProtoPtr ascproto_;	//< 通信协议接口
	string head_, tail_;	//< 串口信息起始/结束标志
	int nhead_, ntail_;	//< 串口信息起始/结束标志长度, 量纲: 字节
	charray bufrcv_;		//< 串口信息接收缓冲区

	boost::condition_variable cndDrct_;	//< 完成控制指令发送-接收流程
	DrctList drct_;			//< 指令集合, 用于周期状态检测和发送临时控制指令
	CallbackFunc cbrslt_;	//< 串口访问结果, 用于通知主程序串口异常
	threadptr thrdCycle_;	//< 周期线程, 定时检测设备工作状态
	threadptr thrdRespond_;	//< 线程, 响应处理串口操作结果
	threadptr thrdHB_;		//< 心跳线程, 检测串口有效性
	boost::mutex mtxDrct_;	//< 指令互斥锁
	boost::mutex mtxNet_;	//< 网络互斥锁
	boost::posix_time::ptime tmlast_;	//< 最后一次通信时间

	boost::shared_ptr<DataTransfer> db_;	//< 数据库访问接口

public:
	/* 接口 */
	/*!
	 * @brief 注册回调函数
	 * @param slot 函数插槽
	 * @note
	 * 通知主程序串口异常, 需要重启串口等
	 */
	void RegisterResult(const CBSlot &slot);
	/*!
	 * @brief 启动控制服务
	 * @param portname  串口名称
	 * @param baudrate  波特率
	 * @return
	 * 服务启动结果.
	 *  0: 正确
	 * -1: 未指定串口名称
	 * -2: 串口打开失败
	 */
	int Start(string portname, int baudrate);
	/*!
	 * @brief 停止控制服务
	 */
	void Stop();
	/*!
	 * @brief 关联控制器与网络通信接口
	 * @param session 网络通信接口
	 * @param grpid   网络组标志
	 */
	void CoupleNetwork(TcpCPtr session, string grpid);
	/*!
	 * @brief 解联控制器与网络通信接口
	 */
	void DecoupleNetwork();
	/*!
	 * @brief 设置数据库访问地址
	 * @param url 数据库访问地址
	 */
	void SetDatabase(const string& url);
	/*!
	 * @brief 接口: 添加与串口关联的设备编号
	 * @param idd 设备编号
	 * @note
	 * 一个串口可以关联复数设备
	 */
	void AddDevice(uint8_t idd);
	/*!
	 * @brief 向串口发送指令
	 * @param idd 设备编号
	 * @param idf 功能编号
	 */
	void Write(uint8_t idd, uint8_t idf);
	/*!
	 * @brief 向串口发送指令
	 * @param idd   设备编号
	 * @param idf   功能编号
	 * @param value 整数类型参数
	 */
	void Write(uint8_t idd, uint8_t idf, int value);
	/*!
	 * @brief 向串口发送指令
	 * @param idd   设备编号
	 * @param idf   功能编号
	 * @param value 浮点类型参数
	 */
	void Write(uint8_t idd, uint8_t idf, double value);
	/*!
	 * @brief 查看串口名称
	 * @return
	 * 串口名称
	 */
	const char *GetPortname();

protected:
	/**** 纯虚函数 -- 功能 ****/
	/*!
	 * @brief 检查并创建数据存储区
	 * @param nDev 设备数量
	 */
	virtual void check_data(int nDev) = 0;
	/*!
	 * @brief 为指定设备生成控制指令
	 * @param idd 设备编号
	 */
	virtual void generate_directive(uint8_t idd) = 0;
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
	virtual int encode_data(uint8_t idd, uint8_t idf, const char *value, int n, char *output) = 0;
	/*!
	 * @brief 解码数据串
	 * @param len    待解码数据串长度, 量纲: 字节
	 * @return
	 * 数据串解码结果
	 *  0: 成功
	 * -1: 长度不足
	 * -2: 数据长度不足
	 */
	virtual int decode_data(int len) = 0;
	/*!
	 * @brief 在日志中记录最新工作状态
	 */
	virtual void write_log() = 0;
	/*!
	 * @brief 向数据库上传监测信息
	 */
	virtual void upload_database() = 0;
	/*!
	 * @brief 通过网络发送设备状态
	 */
	virtual void network_respond() = 0;

protected:
	/* 功能 */
	/*!
	 * @brief 在指令列表中追加一条指令
	 * @param txt 指令信息
	 * @param n   指令信息长度
	 */
	void append_directive(Directive &drct);
	/*!
	 * @brief 移除指令列表首地址指令
	 */
	void remove_directive();
	/*!
	 * @brief 编码无参数协议
	 * @param idd    设备编号
	 * @param idf    功能编号
	 * @param output 编码后字符串
	 * @return
	 * 编码后字符串长度. 0表示错误
	 */
	int encode_data(uint8_t idd, uint8_t idf, char *output);
	/*!
	 * @brief 编码无参数协议
	 * @param idd    设备编号
	 * @param idf    功能编号
	 * @param value  整数类型参数
	 * @param output 编码后字符串
	 * @return
	 * 编码后字符串长度. 0表示错误
	 */
	int encode_data(uint8_t idd, uint8_t idf, int value, char *output);
	/*!
	 * @brief 编码无参数协议
	 * @param idd    设备编号
	 * @param idf    功能编号
	 * @param value  浮点类型参数
	 * @param output 编码后字符串
	 * @return
	 * 编码后字符串长度. 0表示错误
	 */
	int encode_data(uint8_t idd, uint8_t idf, double value, char *output);
	/*!
	 * @brief 解码字符
	 * @param 构成字符的高4位
	 * @param 构成字符的低4位
	 * @return
	 * 解码后字符
	 */
	uint8_t decode_uint8(uint8_t b1, uint8_t b2);
	/*!
	 * @brief 发送列表中第一条信息
	 */
	void first_send();
	/*!
	 * @brief 串口读出回调函数
	 * @param client 串口指针
	 * @param ec     错误代码. 0: 无错误
	 */
	void serial_read(long client, long ec);
	/*!
	 * @brief 串口写入回调函数
	 * @param client 串口指针
	 * @param ec     错误代码. 0: 无错误
	 */
	void serial_write(long client, long ec);
	/*!
	 * @brief 周期线程, 定时检测系统工作状态
	 */
	void thread_cycle();
	/*!
	 * @brief 线程, 响应串口反馈消息
	 */
	void thread_respond();
	/*!
	 * @brief 心跳线程, 监测串口连接有效性
	 */
	void thread_heartbeat();
	/*!
	 * @brief 中止线程
	 * @param thrd 线程指针
	 */
	void interrupt_thread(threadptr& thrd);
};
typedef boost::shared_ptr<ControllerBase> CtlBasePtr;

#endif /* CONTROLLERBASE_H_ */
