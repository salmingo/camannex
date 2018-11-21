/**
 * @file AnnexControl.h 软件控制接口声明文件
 * @version 0.1
 * @date 2017-10-15
 */

#ifndef ANNEXCONTROL_H_
#define ANNEXCONTROL_H_

#include <boost/container/stable_vector.hpp>
#include "MessageQueue.h"
#include "parameter.h"
#include "CoolerCtl.h"
#include "VacuumCtl.h"
#include "tcpasio.h"
#include "NTPClient.h"

//////////////////////////////////////////////////////////////////////////////

class AnnexControl: public MessageQueue {
public:
	AnnexControl(io_service* iomain);
	virtual ~AnnexControl();

protected:
	/* 数据类型 */
	enum MSG_AC {// 消息代码
		MSG_CONNECT_NETWORK,		//< 连接服务器结果
		MSG_RECEIVE_NETWORK,		//< 收到网络消息
		MSG_CLOSE_NETWORK,		//< 断开网络连接
		MSG_CLOSE_COOLER,		//< 温控控制器断开连接
		MSG_CLOSE_VACUUM,		//< 真空度控制器断开连接
		MSG_LAST		//< 占位
	};

	typedef boost::container::stable_vector<CoolCPtr> CoolCVec;		//< 矢量组: 温控
	typedef boost::container::stable_vector<VacuumCPtr> VacuumCVec;	//< 矢量组: 真空

protected:
	/* 成员变量 */
	io_service* iomain_;	//< 主IO接口
	param_config param_;	//< 配置参数
	CoolCVec cctl_;		//< 控制接口: 温控
	VacuumCVec vctl_;	//< 控制接口: 真空度
	boost::mutex mtx_cctl_;	//< 互斥锁: 温控接口
	boost::mutex mtx_vctl_;	//< 互斥锁: 真空度接口
	TcpCPtr tcp_;			//< 网络接口
	NTPPtr  ntp_;			//< 时间接口
	threadptr thrdnetwork_;	//< 周期线程, 监测网络状态, 重新连接服务器

public:
	/* 接口 */
	/*!
	 * @brief 启动服务
	 * @return
	 * 服务启动结果
	 */
	bool StartService();
	/*!
	 * @brief 停止服务
	 */
	void StopService();

protected:
	/* 功能 */
	/*!
	 * @brief 注册由消息队列管理的消息及响应函数
	 */
	void register_messages();
	/*!
	 * @brief 连接服务器
	 */
	bool connect_server(bool async = true);
	/*!
	 * @brief 尝试连接串口
	 * @param devtype   设备类型. 1: 温控; 2: 真空
	 * @param device    设备配置参数
	 * @return
	 * 连接结果
	 */
	bool connect_serial(int devtype, Annex *device);
	/*!
	 * @brief 尝试连接温控器
	 * @brief init 是否初始化连接
	 * @return
	 * 连接结果
	 */
	bool connect_cooler(bool init = false);
	/*!
	 * @brief 尝试连接真空控制器
	 * @brief init 是否初始化连接
	 * @return
	 * 连接结果
	 */
	bool connect_vacuum(bool init = false);
	/*!
	 * @brief 回调函数, 处理网络连接结果
	 * @param client
	 * @param ec 0 -- 成功; 其它 - 失败
	 */
	void network_connect(const long client, const long ec);
	/*!
	 * @brief 处理收到的网络信息
	 * @param client
	 * @param ec
	 */
	void network_receive(const long client, const long ec);
	/*!
	 * @brief 处理收到温控信息
	 * @param client 控制接口
	 * @param ec     错误字. 0: 成功
	 */
	void cooler_receive(long client, long ec);
	/*!
	 * @brief 处理收到真空度信息
	 * @param client 控制接口
	 * @param ec     错误字. 0: 成功
	 */
	void vacuum_receive(long client, long ec);
	/*!
	 * @brief 与服务器异步连接结果
	 * @param client
	 * @param ec 0 - 成功; 其它 - 失败
	 */
	void on_connect_network(const long client, const long ec);
	/*!
	 * @brief 处理收到的网络信息
	 * @param client
	 * @param ec
	 */
	void on_receive_network(const long client, const long ec);
	/*!
	 * @brief 远程主机断开连接
	 * @param client
	 * @param ec
	 */
	void on_close_network(const long client, const long ec);
	/*!
	 * @brief 温控控制器断开连接
	 * @param client
	 * @param ec
	 */
	void on_close_cooler(const long client, const long ec);
	/*!
	 * @brief 真空度控制器断开连接
	 * @param client
	 * @param ec
	 */
	void on_close_vacuum(const long client, const long ec);
	/*!
	 * @brief 线程: 定时重新连接服务器
	 */
	void thread_network();
};

#endif /* ANNEXCONTROL_H_ */
