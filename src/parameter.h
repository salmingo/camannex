/*!
 * @file parameter.h 使用XML文件格式管理配置参数
 */

#ifndef PARAMETER_H_
#define PARAMETER_H_

#include <string>
#include <vector>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>

using std::string;
using std::vector;

struct Annex {// 附件串口配置信息
	string portName;		//< 串口名称
	int baudRate;		//< 波特率
	int n;				//< 设备数量
	vector<uint8_t> idd;	//< 设备ID
};
typedef vector<Annex> AnnexVec;

struct param_config {// 软件配置参数
	string groupid;			//< 组标志
	bool bServer;			//< 是否启用网络通信
	string ipServer;		//< 服务器IP地址
	uint16_t portServer;	//< 服务器端口
	bool enableDB;			//< 数据库启用标志
	string urlDB;			//< 数据库访问地址
	AnnexVec cooler;		//< 温控参数
	AnnexVec vacuum;		//< 真空度参数
	bool enableNTP;			//< NTP启用标志
	string hostNTP;			//< NTP服务器IP地址
	uint16_t portNTP;		//< NTP服务器端口
	int  maxDiffNTP;		//< 采用自动校正时钟策略时, 本机时钟与NTP时钟所允许的最大偏差, 量纲: 毫秒

public:
	/*!
	 * @brief 初始化文件filepath, 存储缺省配置参数
	 * @param filepath 文件路径
	 */
	void InitFile(const std::string& filepath) {
		using namespace boost::posix_time;
		using boost::property_tree::ptree;

		ptree pt;
		int i;
		boost::format idkey("Device_%d.<xmlattr>.ID");

		if (!cooler.empty()) cooler.clear();
		if (!vacuum.empty()) vacuum.clear();

		pt.add("version", "0.1");
		pt.add("Description", "config parameters of annex software for GWAC-GY camera");
		pt.add("date", to_iso_string(second_clock::universal_time()));
		pt.add("GroupID", groupid = "001");
		pt.add("Server.<xmlattr>.Enable", bServer = false);
		pt.add("Server.<xmlattr>.IP", ipServer = "172.28.1.11");
		pt.add("Server.<xmlattr>.Port", portServer = 4016);
		pt.add("Database.<xmlattr>.Enable", enableDB = true);
		pt.add("Database.<xmlattr>.URL",    urlDB    = "http://172.28.8.8:8080/gwebend/");
		pt.add("NTP.<xmlattr>.Enable",  enableNTP = false);
		pt.add("NTP.<xmlattr>.IP",      hostNTP = "172.28.1.3");
		pt.add("NTP.<xmlattr>.Port",    portNTP = 123);
		pt.add("NTP.<xmlattr>.MaxDiff", maxDiffNTP = 5);

		ptree& node1 = pt.add("Cooler", "");
		Annex acool;
		node1.add("SerialPort.<xmlattr>.Name",     acool.portName = "/dev/ttyS0");
		node1.add("SerialPort.<xmlattr>.BaudRate", acool.baudRate = 9600);
		node1.add("DeviceNumber", acool.n = 2);
		for (i = 1; i <= acool.n; ++i) {
			uint8_t idd = uint8_t(i);
			idkey % i;
			node1.add(idkey.str(), idd);
			acool.idd.push_back(idd);
		}
		cooler.push_back(acool);

		ptree& node2 = pt.add("Vacuum", "");
		Annex avacuum;
		node2.add("SerialPort.<xmlattr>.Name",     avacuum.portName = "/dev/ttyS1");
		node2.add("SerialPort.<xmlattr>.BaudRate", avacuum.baudRate = 9600);
		node2.add("DeviceNumber", avacuum.n = 1);
		for (i = 1; i <= avacuum.n; ++i) {
			uint8_t idd = uint8_t(i);
			idkey % i;
			node2.add(idkey.str(), idd);
			avacuum.idd.push_back(idd);
		}
		vacuum.push_back(avacuum);

		ptree& node3 = pt.add("Vacuum", "");
		Annex bvacuum;
		node3.add("SerialPort.<xmlattr>.Name",     avacuum.portName = "/dev/ttyS2");
		node3.add("SerialPort.<xmlattr>.BaudRate", avacuum.baudRate = 9600);
		node3.add("DeviceNumber", bvacuum.n = 1);
		for (i = 1; i <= bvacuum.n; ++i) {
			uint8_t idd = uint8_t(i);
			idkey % i;
			node3.add(idkey.str(), idd);
			bvacuum.idd.push_back(idd);
		}
		vacuum.push_back(bvacuum);

		boost::property_tree::xml_writer_settings<std::string> settings(' ', 4);
		write_xml(filepath, pt, std::locale(), settings);
	}

	/*!
	 * @brief 从文件filepath加载配置参数
	 * @param filepath 文件路径
	 */
	void LoadFile(const std::string& filepath) {
		try {
			using boost::property_tree::ptree;

			// 避免重复加载配置文件时多次缓存
			if (cooler.size()) cooler.clear();
			if (vacuum.size()) vacuum.clear();

			std::string value;
			ptree pt;
			bool bc, bv;
			int i;
			boost::format idkey("Device_%d.<xmlattr>.ID");
			read_xml(filepath, pt, boost::property_tree::xml_parser::trim_whitespace);

			groupid    = pt.get("GroupID", "001");
			bServer    = pt.get("Server.<xmlattr>.Enable", false);
			ipServer   = pt.get("Server.<xmlattr>.IP",   "172.28.1.11");
			portServer = pt.get("Server.<xmlattr>.Port", 4016);
			enableDB   = pt.get("Database.<xmlattr>.Enable",  true);
			urlDB      = pt.get("Database.<xmlattr>.URL",     "http://172.28.8.8:8080/gwebend/");
			enableNTP  = pt.get("NTP.<xmlattr>.Enable",  false);
			hostNTP    = pt.get("NTP.<xmlattr>.IP",      "172.28.1.3");
			portNTP    = pt.get("NTP.<xmlattr>.Port",    123);
			maxDiffNTP = pt.get("NTP.<xmlattr>.MaxDiff", 5);

			BOOST_FOREACH(ptree::value_type const &child, pt.get_child("")) {
				bc = boost::iequals(child.first, "Cooler");
				bv = boost::iequals(child.first, "Vacuum");

				if (bc || bv) {
					Annex one;

					one.portName = child.second.get("SerialPort.<xmlattr>.Name", "/dev/ttyS0");
					one.baudRate = child.second.get("SerialPort.<xmlattr>.BaudRate", 9600);
					one.n        = child.second.get("DeviceNumber", 1);
					for (i = 1; i <= one.n; ++i) {
						idkey % i;
						uint8_t idd = child.second.get(idkey.str(), i);
						one.idd.push_back(idd);
					}
					if (bc) cooler.push_back(one);
					else    vacuum.push_back(one);
				}
			}
		}
		catch(boost::property_tree::xml_parser_error &ex) {
			InitFile(filepath);
		}
	}
};

#endif // PARAMETER_H_
