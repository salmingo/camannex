/**
 Name        : camannex.cpp GWAC-GY相机配套软件, 监测温度和真空度
 Author      : Xiaomeng Lu
 Version     : 0.1
 Copyright   : SVOM Group, NAOC
 Description : GWAC系统定制相机配套软件, 监测温度和真空度
 */
#include <boost/asio.hpp>
#include "globaldef.h"
#include "GLog.h"
#include "AnnexControl.h"
#include "daemon.h"

GLog _gLog;

/*!
 * @brief 主程序
 * @param argc 参数数量
 * @param argv 参数列表
 * @note
 * - 无参数, 以服务形式启动
 * - 有参数, 仅接受-d, 用于生成初始化配置文件
 */
int main(int argc, char** argv) {
	if (argc >= 2) {// 处理命令行参数
		if (strcmp(argv[1], "-d") == 0) {
			param_config param;
			param.InitFile("camannex.xml");
		}
		else printf("Usage: camannex <-d>\n");
	}
	else {// 常规工作模式
		io_service ios;
		boost::asio::signal_set signals(ios, SIGINT, SIGTERM);  // interrupt signal
		signals.async_wait(boost::bind(&io_service::stop, &ios));

		if (!MakeItDaemon(ios)) return 1;
		if (!isProcSingleton(gPIDPath)) {
			_gLog.Write("%s is already running or failed to access PID file", DAEMON_NAME);
			return 2;
		}

		_gLog.Write("Try to launch %s %s %s as daemon", DAEMON_NAME, DAEMON_VERSION, DAEMON_AUTHORITY);
		// 主程序入口
		AnnexControl ac(&ios);
		if (ac.StartService()) {
			_gLog.Write("Daemon goes running");
			ios.run();
			ac.StopService();
		}
		else {
			_gLog.Write(LOG_FAULT, NULL, "Fail to launch %s", DAEMON_NAME);
		}
		_gLog.Write("Daemon stopped");
	}

	return 0;
}
