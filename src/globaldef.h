/*
 * @file globaldef.h 声明全局唯一参数
 */

#ifndef GLOBALDEF_H_
#define GLOBALDEF_H_

// 软件名称、版本与版权
#define DAEMON_NAME			"camannex"
#define DAEMON_VERSION		"v0.1 @ Oct, 2017"
#define DAEMON_AUTHORITY	"© SVOM Group, NAOC"

// 日志文件路径与文件名前缀
const char gLogDir[]    = "/var/log/camannex";
const char gLogPrefix[] = "camannex_";

// 软件配置文件
const char gConfigPath[] = "/usr/local/etc/camannex.xml";

// 文件锁位置
const char gPIDPath[] = "/var/run/camannex.pid";

#endif /* GLOBALDEF_H_ */
