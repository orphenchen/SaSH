﻿/*
				GNU GENERAL PUBLIC LICENSE
				   Version 2, June 1991
COPYRIGHT (C) Bestkakkoii 2023 All Rights Reserved.
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/

#include "stdafx.h"
#include "clua.h"

#include "signaldispatcher.h"
#include <gamedevice.h>
#include "../script/parser.h"


#ifdef _WIN64
#ifdef _DEBUG
#pragma comment(lib, "lua-5.4.4_x64d.lib")
#else
#pragma comment(lib, "lua-5.4.4_x64.lib")
#endif
#else
#ifdef _DEBUG
#pragma comment(lib, "lua-5.4.4d.lib")
#else
#pragma comment(lib, "lua-5.4.4.lib")
#endif
#endif

#define OPEN_HOOK

void __fastcall luadebug::tryPopCustomErrorMsg(const sol::this_state& s, const LUA_ERROR_TYPE element, const QVariant& p1, const QVariant& p2, const QVariant& p3, const QVariant& p4)
{
	std::ignore = p4;//reserved
	lua_State* L = s;

	switch (element)
	{
	case ERROR_FLAG_DETECT_STOP:
	case ERROR_REQUEST_STOP_FROM_USER:
	case ERROR_REQUEST_STOP_FROM_SCRIP:
	case ERROR_REQUEST_STOP_FROM_DISTRUCTOR:
	case ERROR_REQUEST_STOP_FROM_PARENT_SCRIP:
	{
		//用戶請求停止
		const QString qmsgstr(errormsg_str.value(element));
		const std::string str(util::toConstData(qmsgstr));
		luaL_error(L, str.c_str());
		break;
	}
	case ERROR_PARAM_SIZE://固定參數數量錯誤
	{
		long long topsize = lua_gettop(L);
		const QString qmsgstr(QString(errormsg_str.value(element)).arg(p1.toLongLong()).arg(topsize));
		const std::string str(util::toConstData(qmsgstr));
		luaL_argcheck(L, topsize == p1.toLongLong(), topsize, str.c_str());
		break;
	}
	case ERROR_PARAM_SIZE_NONE://無參數數量錯誤
	{
		long long topsize = lua_gettop(L);
		const QString qmsgstr(QString(errormsg_str.value(element)).arg(topsize));
		const std::string str(util::toConstData(qmsgstr));
		luaL_argcheck(L, topsize == 0, 1, str.c_str());
		break;
	}
	case ERROR_PARAM_SIZE_RANGE://範圍參數數量錯誤
	{
		long long topsize = lua_gettop(L);
		const QString qmsgstr(QString(errormsg_str.value(element)).arg(p1.toLongLong()).arg(p2.toLongLong()).arg(topsize));
		const std::string str(util::toConstData(qmsgstr));
		luaL_argcheck(L, topsize >= p1.toLongLong() && topsize <= p2.toLongLong(), 1, str.c_str());
		break;
	}
	case ERROR_PARAM_VALUE://數值錯誤 p1 第幾個參數 p2邏輯 str訊息
	{
		const QString qmsgstr(p3.toString());
		const std::string str(util::toConstData(qmsgstr));
		luaL_argcheck(L, p2.toBool(), p1.toLongLong(), str.c_str());
		break;
	}
	case ERROR_PARAM_TYPE://參數預期錯誤 p1 第幾個參數 p2邏輯 str訊息
	{
		const QString qmsgstr(p3.toString());
		const std::string str(util::toConstData(qmsgstr));
		luaL_argexpected(L, p2.toBool(), p1.toLongLong(), str.c_str());
		break;
	}
	default:
	{
		luaL_error(L, "UNKNOWN ERROR TYPE");
		break;
	}
	}
}

QString __fastcall luadebug::getErrorMsgLocatedLine(const QString& str, long long* retline)
{
	const QString cmpstr(str.simplified());

	QRegularExpressionMatch match = rexGetLine.match(cmpstr);
	QRegularExpressionMatch match2 = reGetLineEx.match(cmpstr);
	static const auto matchies = [](const QRegularExpressionMatch& m, long long* retline)->void
		{
			long long size = m.capturedTexts().size();
			if (size > 1)
			{
				for (long long i = (size - 1); i >= 1; --i)
				{
					const QString s = m.captured(i).simplified();
					if (!s.isEmpty())
					{
						bool ok = false;
						long long row = s.simplified().toLongLong(&ok);
						if (ok)
						{
							if (retline)
								*retline = row - 1;

							break;
						}
					}
				}
			}
		};

	if (match.hasMatch())
	{
		matchies(match, retline);
	}
	else if (match2.hasMatch())
	{
		matchies(match2, retline);
	}

	return cmpstr;
}

QString __fastcall luadebug::getTableVars(lua_State*& L, long long si, long long& depth)
{
	if (!L)
		return "\0";

	QPair<QString, QVariant> pair;
	long long pos_si = si > 0 ? si : (si - 1);
	QString ret;
	long long top = lua_gettop(L);
	lua_pushnil(L);
	long long empty = 1;
	QStringList varList;

	--depth;

	while (lua_next(L, pos_si) != 0)
	{
		if (empty)
			empty = 0;

		QString key;
		pair = getVars(L, -2, depth);
		if (pair.second.toString().startsWith("_"))
		{
			lua_pop(L, 1);
			continue;
		}

		if (pair.first == "(string)")
			ret += QString("%1=").arg(pair.second.toString());
		else if (pair.first == "(integer)")
		{
			pair.second = pair.second.toString();
			ret += QString("[%1]=").arg(pair.second.toString());
		}
		else
			continue;

		if (depth < 0)
		{
			ret += ("{}");
		}
		else
		{
			pair = getVars(L, -1, depth);
			if (pair.first == "(string)")
				ret += QString("[[%1]]").arg(pair.second.toString());
			else
				ret += pair.second.toString();
		}
		lua_pop(L, 1);
		varList.append(ret);
		ret.clear();
	}

	lua_settop(L, top);

	std::sort(varList.begin(), varList.end());
	ret = QString("{%1}").arg(varList.join(","));
	return ret.simplified();
}

QPair<QString, QVariant> __fastcall luadebug::getVars(lua_State*& L, long long si, long long& depth)
{
	switch (lua_type(L, si))
	{
	case LUA_TNIL:
	{
		return qMakePair(QString("(nil)"), QString("nil"));
	}
	case LUA_TBOOLEAN:
	{
		return qMakePair(QString("(boolean)"), util::toQString(lua_toboolean(L, si) > 0));
	}
	case LUA_TSTRING:
	{
		return qMakePair(QString("(string)"), util::toQString(luaL_checklstring(L, si, nullptr)));
	}
	case LUA_TNUMBER:
	{
		double d = static_cast<double>(luaL_checknumber(L, si));
		if (d == static_cast<long long>(d))
			return qMakePair(QString("(integer)"), util::toQString(static_cast<long long>(luaL_checkinteger(L, si))));
		else
			return qMakePair(QString("(number)"), util::toQString(d));

	}
	case LUA_TFUNCTION:
	{
		lua_CFunction func = lua_tocfunction(L, si);
		if (func != nullptr)
		{
			return qMakePair(QString("(C function)"), QString("0x%1").arg(util::toQString(reinterpret_cast<long long>(func), 16)));
		}
		else
		{
			return qMakePair(QString("(function)"), QString("0x%1").arg(util::toQString(reinterpret_cast<long long>(func), 16)));
		}
		break;
	}
	case LUA_TUSERDATA:
	{
		return qMakePair(QString("(user data)"), QString("0x%1").arg(util::toQString(reinterpret_cast<long long>(lua_touserdata(L, si)), 16)));
	}
	case LUA_TTABLE:
	{
		return qMakePair(QString("(table)"), getTableVars(L, si, depth));
	}
	default:
		break;
	}

	return qMakePair(QString("(nil)"), QString("nil"));
}

bool __fastcall luadebug::isInterruptionRequested(const sol::this_state& s)
{
	sol::state_view lua(s.lua_state());
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.IS_SCRIPT_INTERRUPT.get())
		return true;

	return false;
}

bool __fastcall luadebug::checkStopAndPause(const sol::this_state& s)
{
	sol::state_view lua(s.lua_state());

	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	gamedevice.checkScriptPause();

	bool isStop = gamedevice.IS_SCRIPT_INTERRUPT.get();
	if (isStop)
	{
		tryPopCustomErrorMsg(s, ERROR_FLAG_DETECT_STOP);
	}

	return isStop;
}

bool __fastcall luadebug::checkOnlineThenWait(const sol::this_state& s)
{
	checkStopAndPause(s);
	sol::state_view lua(s.lua_state());
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	bool bret = false;
	if (!gamedevice.worker->getOnlineFlag())
	{
		util::timer timer;
		bret = true;
		for (;;)
		{
			if (checkStopAndPause(s))
				break;

			if (!gamedevice.worker.isNull())
				break;

			if (gamedevice.worker->getOnlineFlag())
				break;

			if (timer.hasExpired(180000))
				break;

			QThread::msleep(100);
		}

		QThread::msleep(1000UL);
	}
	return bret;
}

bool __fastcall luadebug::checkBattleThenWait(const sol::this_state& s)
{
	checkStopAndPause(s);

	sol::state_view lua(s.lua_state());
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	bool bret = false;
	if (gamedevice.worker->getBattleFlag())
	{
		util::timer timer;
		bret = true;
		for (;;)
		{
			if (checkStopAndPause(s))
				break;

			if (!gamedevice.worker.isNull())
				break;

			if (!gamedevice.worker->getBattleFlag())
				break;
			if (timer.hasExpired(180000))
				break;

			QThread::msleep(100);
		}

		QThread::msleep(1000UL);
	}
	return bret;
}

void __fastcall luadebug::processDelay(const sol::this_state& s)
{
	sol::state_view lua(s.lua_state());
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	long long extraDelay = gamedevice.getValueHash(util::kScriptSpeedValue);
	if (extraDelay > 1000ll)
	{
		//將超過1秒的延時分段
		long long i = 0ll;
		long long size = extraDelay / 1000ll;
		for (i = 0; i < size; ++i)
		{
			if (checkStopAndPause(s))
				return;

			QThread::msleep(1000L);
		}
		if (extraDelay % 1000ll > 0ll)
			QThread::msleep(extraDelay % 1000ll);
	}
	else if (extraDelay > 0ll)
	{
		QThread::msleep(extraDelay);
	}
}

//遞歸獲取每一層目錄
void __fastcall luadebug::getPackagePath(const QString base, QStringList* result)
{
	QDir dir(base);
	if (!dir.exists())
		return;
	dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
	dir.setSorting(QDir::DirsFirst);
	QFileInfoList list = dir.entryInfoList();
	for (long long i = 0; i < list.size(); ++i)
	{
		QFileInfo fileInfo = list.value(i);
		result->append(fileInfo.filePath());
		getPackagePath(fileInfo.filePath(), result);
	}
}

void __fastcall luadebug::logExport(const sol::this_state& s, const QStringList& datas, long long color, bool doNotAnnounce)
{
	for (const QString& data : datas)
	{
		logExport(s, data, color, doNotAnnounce);
	}
}

void __fastcall luadebug::logExport(const sol::this_state& s, const QString& data, long long color, bool doNotAnnounce)
{

	//打印當前時間
	const QDateTime time(QDateTime::currentDateTime());
	const QString timeStr(time.toString("hh:mm:ss:zzz"));
	QString msg = "\0";
	QString src = "\0";
	sol::state_view lua(s);
	long long currentline = lua["LINE"].get<long long>();//getCurrentLine(s);

	msg = (QString("[%1 | @%2]: %3\0") \
		.arg(timeStr)
		.arg(currentline, 3, 10, QLatin1Char(' ')).arg(data));


	long long currentIndex = lua["__INDEX"].get<long long>();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.appendScriptLog(msg, color);
	GameDevice& gamedevice = GameDevice::getInstance(currentIndex);
	if (!gamedevice.worker.isNull() && !doNotAnnounce)
	{
		gamedevice.worker->announce(data, color);
	}

	if (gamedevice.log.isOpen())
		gamedevice.log.write(data, currentline);
}

void __fastcall luadebug::showErrorMsg(const sol::this_state& s, long long level, const QString& data)
{
	QString newText = QString("%1%2").arg(level == 0 ? QObject::tr("[warn]") : QObject::tr("[error]")).arg(data);
	logExport(s, newText, 0, true);
}

//根據傳入function的循環執行結果等待超時或條件滿足提早結束
bool __fastcall luadebug::waitfor(const sol::this_state& s, long long timeout, std::function<bool()> exprfun)
{
	if (timeout < 0)
		timeout = std::numeric_limits<long long>::max();
	else if (timeout == 0)
	{
		return exprfun();
	}

	sol::state_view lua(s.lua_state());
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	bool bret = false;
	util::timer timer;
	for (;;)
	{
		if (checkStopAndPause(s))
			break;

		if (timer.hasExpired(timeout))
			break;

		if (gamedevice.worker.isNull())
			break;

		if (exprfun())
		{
			bret = true;
			break;
		}

		QThread::msleep(100);
	}
	return bret;
}

//lua函數鉤子 這裡主要用於控制 暫停、終止腳本、獲取棧數據、變量數據...或其他操作
void luadebug::hookProc(lua_State* L, lua_Debug* ar)
{
	if (!L)
		return;

	if (!ar)
		return;

	sol::this_state s = L;
	sol::state_view lua(L);

	switch (ar->event)
	{
	case LUA_HOOKCALL:
	{
		//函數入口
		break;
	}
	case LUA_MASKCOUNT:
	{
		//每 n 個函數執行一次
		break;
	}
	case LUA_HOOKRET:
	{
		//函數返回
		break;
	}
	case LUA_HOOKLINE:
	{
		if (lua["__HOOKFORSTOP"].is<bool>() && lua["__HOOKFORSTOP"].get<bool>())
		{
			luadebug::checkStopAndPause(s);
			break;
		}

		long long currentLine = ar->currentline;
		SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(lua["__INDEX"].get<long long>());
		long long max = lua["ROWCOUNT"];

		GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());

		emit signalDispatcher.scriptLabelRowTextChanged(currentLine, max, false);

		processDelay(s);
		if (gamedevice.IS_SCRIPT_DEBUG_ENABLE.get())
		{
			QThread::msleep(1);
		}
		else
		{
			luadebug::checkStopAndPause(s);
			return;
		}

		luadebug::checkStopAndPause(s);

		CLua* pLua = lua["__THIS_CLUA"].get<CLua*>();
		if (pLua == nullptr)
			return;

		QString scriptFileName = gamedevice.currentScriptFileName.get();

		safe::hash<long long, break_marker_t> breakMarkers = gamedevice.break_markers.value(scriptFileName);
		const safe::hash<long long, break_marker_t> stepMarkers = gamedevice.step_markers.value(scriptFileName);
		if (!(breakMarkers.contains(currentLine) || stepMarkers.contains(currentLine)))
		{
			return;//檢查是否有中斷點
		}

		gamedevice.paused();

		if (breakMarkers.contains(currentLine))
		{
			//叫用次數+1
			break_marker_t mark = breakMarkers.value(currentLine);
			++mark.count;

			//重新插入斷下的紀錄
			breakMarkers.insert(currentLine, mark);
			gamedevice.break_markers.insert(scriptFileName, breakMarkers);
			//所有行插入隱式斷點(用於單步)
			emit signalDispatcher.addStepMarker(currentLine, true);
		}

		emit signalDispatcher.addForwardMarker(currentLine, true);

		//獲取區域變量數值
		long long i;
		const char* name = nullptr;
		QVariantHash varhash;
		for (i = 1; (name = lua_getlocal(L, ar, i)) != NULL; ++i)
		{
			long long depth = 1024;
			QPair<QString, QVariant> vs = getVars(L, i, depth);

			QString key = QString("local|%1").arg(util::toQString(name));
			varhash.insert(key, vs.second.toString());
			lua_pop(L, 1);// no match, then pop out the var's value
		}

		Parser parser(lua["__INDEX"].get<long long>());

		emit signalDispatcher.varInfoImported(&parser, varhash, QStringList{});

		luadebug::checkStopAndPause(s);

		break;
	}
	default:
		break;
	}
}

bool luatool::checkRange(sol::object o, long long& min, long long& max, QVector<long long>* pindexs)
{
	if (o == sol::lua_nil)
		return true;

	if (o.is<std::string>())
	{
		QString str = util::toQString(o.as<std::string>());
		if (str.isEmpty() || str == "?")
		{
			return true;
		}

		bool ok = false;
		long long tmp = str.toLongLong(&ok) - 1;
		if (ok)
		{
			if (tmp < 0)
				return false;

			min = tmp;
			max = tmp;
			return true;
		}

		QStringList range = str.split("-", Qt::SkipEmptyParts);
		if (range.isEmpty())
		{
			return true;
		}

		if (range.size() == 2)
		{
			bool ok1, ok2;
			long long tmp1 = range.value(0).toLongLong(&ok1);
			long long tmp2 = range.value(1).toLongLong(&ok2);
			if (ok1 && ok2)
			{
				if (tmp1 < 0 || tmp2 < 0)
					return false;

				min = tmp1 - 1;
				max = tmp2 - 1;
				return true;
			}
		}
		else
		{
			if (pindexs != nullptr)
			{
				for (const QString& str : range)
				{
					bool ok;
					long long tmp = str.toLongLong(&ok) - 1;
					if (ok && tmp >= 0)
						pindexs->append(tmp);
				}

				return pindexs->size() > 0;
			}
		}
	}
	else if (o.is<long long>())
	{
		long long tmp = o.as<long long>() - 1;
		if (tmp < 0)
			return false;

		min = tmp;
		max = tmp;
		return true;
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CLua::CLua(long long index, QObject* parent)
	: Indexer(index)
{
	std::ignore = parent;
	qDebug() << "CLua 1";
}

CLua::CLua(long long index, const QString& content, QObject* parent)
	: Indexer(index)
	, scriptContent_(content)
{
	std::ignore = parent;
	qDebug() << "CLua 2";
}

CLua::~CLua()
{
	qDebug() << "~CLua";
}

void CLua::start()
{
	moveToThread(&thread_);

	connect(this, &CLua::finished, &thread_, &QThread::quit);
	connect(&thread_, &QThread::finished, &thread_, &QThread::deleteLater);
	connect(&thread_, &QThread::started, this, &CLua::proc);
	connect(this, &CLua::finished, this, [this]()
		{
			thread_.requestInterruption();
			thread_.quit();
			thread_.wait();
			qDebug() << "CLua::finished";
		});

	thread_.start();
}

void CLua::wait()
{
	thread_.wait();
}

void CLua::open_enumlibs()
{
	/*
	//添加lua枚舉值示範
	enum TemplateEnum
	{
		EnumValue1,
		EnumValue2,
		EnumValue3
	};

	lua_.new_enum<TemplateEnum>("TEST",
		{
			{ "EnumValue1", TemplateEnum::EnumValue1 },
			{ "EnumValue2", TemplateEnum::EnumValue2 },
			{ "EnumValue3", TemplateEnum::EnumValue3 }
		}
	);

	在lua中的使用方法: TEST.EnumValue1
	*/

	//kSelectSelf = 0x1,            // 己 (Self)
	//kSelectPet = 0x2,             // 寵 (Pet)
	//kSelectAllieAny = 0x4,        // 我任 (Any ally)
	//kSelectAllieAll = 0x8,        // 我全 (All allies)
	//kSelectEnemyAny = 0x10,       // 敵任 (Any enemy)
	//kSelectEnemyAll = 0x20,       // 敵全 (All enemies)
	//kSelectEnemyFront = 0x40,     // 敵前 (Front enemy)
	//kSelectEnemyBack = 0x80,      // 敵後 (Back enemy)
	//kSelectLeader = 0x100,        // 隊 (Leader)
	//kSelectLeaderPet = 0x200,     // 隊寵 (Leader's pet)
	//kSelectTeammate1 = 0x400,     // 隊1 (Teammate 1)
	//kSelectTeammate1Pet = 0x800,  // 隊1寵 (Teammate 1's pet)
	//kSelectTeammate2 = 0x1000,    // 隊2 (Teammate 2)
	//kSelectTeammate2Pet = 0x2000, // 隊2寵 (Teammate 2's pet)
	//kSelectTeammate3 = 0x4000,    // 隊3 (Teammate 3)
	//kSelectTeammate3Pet = 0x8000, // 隊3寵 (Teammate 3's pet)
	//kSelectTeammate4 = 0x10000,   // 隊4 (Teammate 4)
	//kSelectTeammate4Pet = 0x20000 // 隊4寵 (Teammate 4's pet)

	lua_.new_enum <util::SelectTarget>("TARGET",
		{
			{ "SELF", util::kSelectSelf },
			{ "PET", util::kSelectPet },
			{ "ALLIE_ANY", util::kSelectAllieAny },
			{ "ALLIE_ALL", util::kSelectAllieAll },
			{ "ENEMY_ANY", util::kSelectEnemyAny },
			{ "ENEMY_ALL", util::kSelectEnemyAll },
			{ "ENEMY_FRONT", util::kSelectEnemyFront },
			{ "ENEMY_BACK", util::kSelectEnemyBack },
			{ "LEADER", util::kSelectLeader },
			{ "LEADER_PET", util::kSelectLeaderPet },
			{ "TEAM", util::kSelectTeammate1 },
			{ "TEAM1_PET", util::kSelectTeammate1Pet },
			{ "TEAM2", util::kSelectTeammate2 },
			{ "TEAM2_PET", util::kSelectTeammate2Pet },
			{ "TEAM3", util::kSelectTeammate3 },
			{ "TEAM3_PET", util::kSelectTeammate3Pet },
			{ "TEAM4", util::kSelectTeammate4 },
			{ "TEAM4_PET", util::kSelectTeammate4Pet }
		}
	);
	//在腳本中的使用方法: Target.Self Target.Pet
}

//以Metatable方式註冊函數 支援函數多載、運算符重載，面向對象，常量
void CLua::open_testlibs()
{
	/*
	sol::usertype<CLuaTest> test = lua_.new_usertype<CLuaTest>("Test",
		sol::call_constructor,
		sol::constructors<
		//建構函數多載
		CLuaTest(sol::this_state),
		CLuaTest(long long a, sol::this_state)>(),

		//成員函數多載
		"add", sol::overload(
			sol::resolve<long long(long long a, long long b)>(&CLuaTest::add),
			sol::resolve<long long(long long b)>(&CLuaTest::add)
		),

		//成員函數聲明
		"sub", &CLuaTest::sub
	);


	在lua中的使用方法:

	local test = Test(1)
	print(test:add(1, 2))
	print(test:add(2))
	print(test:sub(1, 2))

	*/
}

void CLua::open_utillibs(sol::state&)
{

}

//luasystem.cpp
void CLua::open_syslibs(sol::state& lua)
{
	lua.set_function("capture", &CLuaSystem::capture, &luaSystem_);

	lua.set_function("setglobal", &CLuaSystem::setglobal, &luaSystem_);
	lua.set_function("getglobal", &CLuaSystem::getglobal, &luaSystem_);
	lua.set_function("clearglobal", &CLuaSystem::clearglobal, &luaSystem_);

	lua.set_function("send", &CLuaSystem::send, &luaSystem_);
	lua.set_function("sleep", &CLuaSystem::sleep, &luaSystem_);
	lua.set_function("openlog", &CLuaSystem::openlog, &luaSystem_);
	lua.set_function("__print", &CLuaSystem::print, &luaSystem_);

	lua.set_function("printf", [](sol::object ovalue, sol::object ocolor, sol::this_state s)->long long
		{
			sol::state_view lua(s);
			if (!ocolor.is<long long>() || ocolor.as<long long>() < -2 || ocolor.as<long long>() > 11)
			{
				ocolor = sol::make_object(lua, sol::lua_nil);
			}

			sol::protected_function print = lua["__print"];
			if (!print.valid())
				return 0;

			if (ovalue.is<std::string>())
			{
				sol::protected_function format = lua["format"];
				if (!format.valid())
					return 0;

				sol::object o = format.call(ovalue.as<std::string>());
				if (!o.valid())
					return 0;

				return print.call(o, ocolor).get<long long>();
			}

			return print.call(ovalue, ocolor).get<long long>();
		});

	//直接覆蓋print會無效,改成在腳本內中轉覆蓋
	lua.safe_script(R"(
		print = printf;
	)", sol::script_pass_on_error);

	lua.set_function("msg", &CLuaSystem::messagebox, &luaSystem_);
	lua.set_function("saveset", &CLuaSystem::savesetting, &luaSystem_);
	lua.set_function("loadset", &CLuaSystem::loadsetting, &luaSystem_);
	lua.set_function("set", &CLuaSystem::set, &luaSystem_);
	lua.set_function("lclick", &CLuaSystem::leftclick, &luaSystem_);
	lua.set_function("rclick", &CLuaSystem::rightclick, &luaSystem_);
	lua.set_function("dbclick", &CLuaSystem::leftdoubleclick, &luaSystem_);
	lua.set_function("dragto", &CLuaSystem::mousedragto, &luaSystem_);

	lua.set_function("createch", &CLuaSystem::createch, &luaSystem_);
	lua.set_function("delch", &CLuaSystem::delch, &luaSystem_);
	lua.set_function("menu", &CLuaSystem::menu, &luaSystem_);

	lua.new_usertype<util::timer>("Timer",
		sol::call_constructor,
		sol::constructors<util::timer()>(),

		"restart", &util::timer::restart,
		"expired", &util::timer::hasExpired,
		"get", &util::timer::cost
	);

	lua.set_function("say", &CLuaSystem::talk, &luaSystem_);
	lua.set_function("cls", &CLuaSystem::cleanchat, &luaSystem_);
	lua.set_function("logout", &CLuaSystem::logout, &luaSystem_);
	lua.set_function("logback", &CLuaSystem::logback, &luaSystem_);
	lua.set_function("eo", &CLuaSystem::eo, &luaSystem_);
	lua.set_function("button", &CLuaSystem::press, &luaSystem_);

	lua.set_function("chname", &CLuaSystem::chname, &luaSystem_);
	lua.set_function("chpetname", &CLuaSystem::chpetname, &luaSystem_);
	lua.set_function("chpet", &CLuaSystem::chpet, &luaSystem_);

	lua.set_function("waitpos", &CLuaSystem::waitpos, &luaSystem_);
	lua.set_function("waitmap", &CLuaSystem::waitmap, &luaSystem_);
	lua.set_function("waitpet", &CLuaSystem::waitpet, &luaSystem_);
	lua.set_function("waititem", &CLuaSystem::waititem, &luaSystem_);
	lua.set_function("waitteam", &CLuaSystem::waitteam, &luaSystem_);
	lua.set_function("waitdlg", &CLuaSystem::waitdlg, &luaSystem_);
	lua.set_function("waitsay", &CLuaSystem::waitsay, &luaSystem_);
}

void CLua::open_itemlibs(sol::state& lua)
{
	lua.set_function("swapitem", &CLuaItem::swapitem, &luaItem_);
	lua.set_function("make", &CLuaItem::make, &luaItem_);
	lua.set_function("cook", &CLuaItem::cook, &luaItem_);
	lua.set_function("getstone", &CLuaItem::withdrawgold, &luaItem_);
	lua.set_function("putstone", &CLuaItem::depositgold, &luaItem_);
	lua.set_function("doffstone", &CLuaItem::dropgold, &luaItem_);
	lua.set_function("buy", &CLuaItem::buy, &luaItem_);
	lua.set_function("sell", &CLuaItem::sell, &luaItem_);
	lua.set_function("sellpet", &CLuaItem::sellpet, &luaItem_);
	lua.set_function("doffpet", &CLuaItem::droppet, &luaItem_);

	lua.set_function("pickup", &CLuaItem::pickitem, &luaItem_);
	lua.set_function("putitem", &CLuaItem::deposititem, &luaItem_);
	lua.set_function("getitem", &CLuaItem::withdrawitem, &luaItem_);
	lua.set_function("putpet", &CLuaItem::depositpet, &luaItem_);
	lua.set_function("getpet", &CLuaItem::withdrawpet, &luaItem_);
	lua.set_function("requip", &CLuaItem::recordequip, &luaItem_);
	lua.set_function("wequip", &CLuaItem::wearrecordedequip, &luaItem_);
	lua.set_function("uequip", &CLuaItem::unwearequip, &luaItem_);
	lua.set_function("pequip", &CLuaItem::petequip, &luaItem_);
	lua.set_function("puequip", &CLuaItem::petunequip, &luaItem_);

	lua.new_usertype<sa::item_t>("ItemStruct",
		"valid", sol::readonly(&sa::item_t::valid),
		"color", sol::readonly(&sa::item_t::color),
		"modelid", sol::readonly(&sa::item_t::modelid),
		"lv", sol::readonly(&sa::item_t::level),
		"stack", sol::readonly(&sa::item_t::stack),
		"type", sol::readonly(&sa::item_t::type),
		"field", sol::readonly(&sa::item_t::field),
		"target", sol::readonly(&sa::item_t::target),
		"flag", sol::readonly(&sa::item_t::deadTargetFlag),
		"sflag", sol::readonly(&sa::item_t::sendFlag),
		"itemup", sol::readonly(&sa::item_t::itemup),
		"counttime", sol::readonly(&sa::item_t::counttime),
		"dura", sol::readonly(&sa::item_t::damage),

		"name", sol::property(&sa::item_t::getName),
		"name2", sol::property(&sa::item_t::getName2),
		"memo", sol::property(&sa::item_t::getMemo),

		/*custom*/
		"max", sol::readonly(&sa::item_t::maxStack),
		"count", sol::readonly(&sa::item_t::count)
	);

	lua.new_usertype<CLuaItem>("ItemClass",
		sol::call_constructor,
		sol::constructors<CLuaItem(long long)>(),
		sol::meta_function::index, &CLuaItem::operator[],
		"space", sol::property(&CLuaItem::getSpace),
		"isfull", sol::property(&CLuaItem::getIsFull),
		"count", &CLuaItem::count,
		"indexof", &CLuaItem::indexof,
		"find", &CLuaItem::find
	);

	lua.safe_script("item = ItemClass(__INDEX);", sol::script_pass_on_error);
	lua.collect_garbage();
}

void CLua::open_charlibs(sol::state& lua)
{
	lua.set_function("join", &CLuaChar::join, &luaChar_);
	lua.set_function("leave", &CLuaChar::leave, &luaChar_);
	lua.set_function("kick", &CLuaChar::kick, &luaChar_);
	lua.set_function("skup", &CLuaChar::skup, &luaChar_);
}

void CLua::open_petlibs(sol::state& lua)
{
	lua.set_function("learn", &CLuaPet::learn, &luaPet_);
	lua.new_usertype<CLuaPet>("PetClass",
		sol::call_constructor,
		sol::constructors<CLuaPet()>(),
		"setState", &CLuaPet::setState,
		"drop", &CLuaPet::drop,
		"rename", &CLuaPet::rename,
		"learn", &CLuaPet::learn,
		"swap", &CLuaPet::swap,
		"deposit", &CLuaPet::deposit,
		"withdraw", &CLuaPet::withdraw
	);
}

void CLua::open_maplibs(sol::state& lua)
{
	lua.set_function("findpath", &CLuaMap::findPath, &luaMap_);
	lua.set_function("move", &CLuaMap::move, &luaMap_);
	lua.set_function("w", &CLuaMap::packetMove, &luaMap_);
	lua.set_function("chmap", &CLuaMap::teleport, &luaMap_);
	lua.set_function("download", &CLuaMap::downLoad, &luaMap_);
	lua.set_function("findnpc", &CLuaMap::findNPC, &luaMap_);
	lua.set_function("dir", &CLuaMap::setdir, &luaMap_);
	lua.set_function("walkpos", &CLuaMap::walkpos, &luaMap_);
}

void CLua::open_battlelibs(sol::state& lua)
{
	//registerFunction("bwait", &Interpreter::bwait);
	//registerFunction("bend", &Interpreter::bend);

	lua.set_function("bh", &CLuaBattle::charUseAttack, &luaBattle_);
	lua.set_function("bj", &CLuaBattle::charUseMagic, &luaBattle_);
	lua.set_function("bp", &CLuaBattle::charUseSkill, &luaBattle_);
	lua.set_function("bs", &CLuaBattle::switchPet, &luaBattle_);
	lua.set_function("be", &CLuaBattle::escape, &luaBattle_);
	lua.set_function("bd", &CLuaBattle::defense, &luaBattle_);
	lua.set_function("bi", &CLuaBattle::useItem, &luaBattle_);
	lua.set_function("bt", &CLuaBattle::catchPet, &luaBattle_);
	lua.set_function("bn", &CLuaBattle::nothing, &luaBattle_);
	lua.set_function("bw", &CLuaBattle::petUseSkill, &luaBattle_);
	lua.set_function("bwn", &CLuaBattle::petNothing, &luaBattle_);
	lua.set_function("bwait", &CLuaBattle::bwait, &luaBattle_);
	lua.set_function("bend", &CLuaBattle::bend, &luaBattle_);
}

void CLua::openlibs()
{
	if (!isSubScript_)
	{
		GameDevice& gamedevice = GameDevice::getInstance(getIndex());
		gamedevice.scriptThreadId = reinterpret_cast<unsigned long long>(QThread::currentThreadId());
	}

	lua_.set("__THIS_CLUA", this);// 將this指針傳給lua設置全局變量
	lua_.set("__THIS_PARENT", parent_);// 將父類指針傳給lua設置全局變量
	lua_.set("__INDEX", getIndex());

	lua_.set("ROWCOUNT", max_);

	//打開lua原生庫
	lua_.open_libraries(
		sol::lib::base,
		sol::lib::package,
		sol::lib::os,
		sol::lib::string,
		sol::lib::math,
		sol::lib::table,
		sol::lib::debug,
		sol::lib::utf8,
		sol::lib::coroutine,
		sol::lib::io
	);

	open_enumlibs();
	//open_testlibs();
	open_utillibs(lua_);
	open_syslibs(lua_);
	open_itemlibs(lua_);
	open_charlibs(lua_);
	open_petlibs(lua_);
	open_maplibs(lua_);
	open_battlelibs(lua_);

	//執行短腳本
	lua_.safe_script(R"(
collectgarbage("setpause", 100)
collectgarbage("setstepmul", 100);
collectgarbage("step", 1024);
	)", sol::script_pass_on_error);

	//Add additional package path.
	QStringList paths;
	std::string package_path = lua_["package"]["path"];
	paths.append(util::toQString(package_path).replace("\\", "/"));

	QStringList dirs;
	luadebug::getPackagePath(util::applicationDirPath() + "/", &dirs);
	for (const QString& it : dirs)
	{
		QString path = it + "/?.lua";
		paths.append(path.replace("\\", "/"));
	}

	lua_["package"]["path"] = std::string(util::toConstData(paths.join(";")));

	if (isHookEnabled_)
	{
		lua_State* L = lua_.lua_state();
		lua_sethook(L, &luadebug::hookProc, LUA_MASKLINE | LUA_MASKCALL | LUA_MASKRET, NULL);// | LUA_MASKCOUNT
	}
}

void CLua::proc()
{
	do
	{
		if (scriptContent_.simplified().isEmpty())
			break;

		safe::auto_flag autoFlag(&isRunning_);

		max_ = scriptContent_.split("\n").size();

		lua_State* L = lua_.lua_state();
		sol::this_state s = L;

		openlibs();

		QStringList tableStrs;
		std::string luaCode(util::toConstData(scriptContent_));

		//安全模式下執行lua腳本
		sol::protected_function_result loaded_chunk = lua_.safe_script(luaCode.c_str(), sol::script_pass_on_error);
		lua_.collect_garbage();

		if (!loaded_chunk.valid())
		{
			sol::error err("\0");
			QString qstrErr("\0");
			bool errOk = false;
			try
			{
				err = loaded_chunk;
				qstrErr = util::toQString(err.what());
				errOk = true;
			}
			catch (...)
			{
				if (!isSubScript_)
					tableStrs << "[fatal]:" + tr("========== lua error result with an EXCEPTION ==========");
				else
					tableStrs << "[fatal]: SUBTHREAD | " + tr("========== lua error result with an EXCEPTION ==========");

				if (errOk)
				{
					long long retline = -1;
					QString msg(luadebug::getErrorMsgLocatedLine(qstrErr, &retline));

					if (msg.contains("FLAG_DETECT_STOP")
						|| msg.contains("REQUEST_STOP_FROM_USER")
						|| msg.contains("REQUEST_STOP_FROM_SCRIP")
						|| msg.contains("REQUEST_STOP_FROM_PARENT_SCRIP")
						|| msg.contains("REQUEST_STOP_FROM_DISTRUCTOR"))
					{
						if (msg.contains("FLAG_DETECT_STOP"))
							tableStrs << tr("> lua script stop by flag change to false");
						else if (msg.contains("REQUEST_STOP_FROM_USER"))
							tableStrs << tr("> lua script stop with user request");
						else if (msg.contains("REQUEST_STOP_FROM_SCRIP"))
							tableStrs << tr("> lua script stop from script request");
						else if (msg.contains("REQUEST_STOP_FROM_PARENT_SCRIP"))
							tableStrs << tr("> lua script stop from parent script request");
						else if (msg.contains("REQUEST_STOP_FROM_DISTRUCTOR"))
							tableStrs << tr("> lua script stop from it's distructor");
						//SPD_LOG(GLOBAL_LOG_ID, QString("%1 [index:%2] Lua Info:%3").arg(__FUNCTION__).arg(m_index).arg(msg));
						//emit this->addErrorMarker(retline, true);
						if (isDebug_)
							tableStrs << tr("> message: ");
						tableStrs << "> [info]:" + msg;
					}
					else
					{
						tableStrs << tr("========== lua script stop with an ERROR ==========");
						//SPD_LOG(GLOBAL_LOG_ID, QString("%1 [index:%2] Lua Warn:%3").arg(__FUNCTION__).arg(m_index).arg(msg), SPD_WARN);
						if (isDebug_)
							tableStrs << tr("> reason: ");
						tableStrs << "> [error]:" + msg;
					}

					//emit this->logMessageExport(s, );
					if (isDebug_ && !isSubScript_)
						tableStrs << ">";
				}
			}

			tableStrs.append(qstrErr);
		}
		else
		{
#ifdef _DEBUG
			isDebug_ = true;
#endif

			if (isDebug_)
			{
				//get script return value
				tableStrs << tr("========== lua script normally end ==========");
				tableStrs << tr("> return value:");
				sol::object retObject;

				try
				{
					retObject = loaded_chunk;
				}
				catch (...)
				{
					if (!isSubScript_)
						tableStrs << "[fatal]:" + tr("========== lua normal result with EXCEPTION ==========");
					else
						tableStrs << "[fatal]: SUBTHREAD | " + tr("========== lua normal result with EXCEPTION ==========");
				}
				if (retObject.is<bool>())
				{
					tableStrs << QString("> (boolean)%1").arg(util::toQString(retObject.as<bool>()));
				}
				else if (retObject.is<long long>())
				{
					tableStrs << "> (integer)" + util::toQString(retObject.as<long long>());

				}
				else if (retObject.is<double>())
				{
					tableStrs << "> (number)" + util::toQString(retObject.as<double>());
				}
				else if (retObject.is<std::string>())
				{
					tableStrs << "> (string)" + util::toQString(retObject);
				}
				else if (retObject == sol::lua_nil)
				{
					tableStrs << "> (nil)";
				}
				else if (retObject.is<sol::table>())
				{
					sol::table t = retObject.as<sol::table>();

					tableStrs << "> {";
					for (const std::pair<sol::object, sol::object>& it : t)
					{
						sol::object key = it.first;
						sol::object val = it.second;
						if (!key.valid() || !val.valid()) continue;
						if (!key.is<std::string>() && !key.is<long long>()) continue;
						QString qkey = key.is<std::string>() ? util::toQString(key) : util::toQString(key.as<long long>());

						if (val.is<bool>())
						{
							tableStrs << QString(R"(>     ["%1"] = (boolean)%2,)").arg(qkey).arg(util::toQString(val.as<bool>()));
						}
						else if (val.is<long long>())
						{
							tableStrs << QString(R"(>     ["%1"] = (integer)%2,)").arg(qkey).arg(val.as<long long>());
						}
						else if (val.is<double>())
						{
							tableStrs << QString(R"(>     ["%1"] = (number)%2,)").arg(qkey).arg(val.as<double>());
						}
						else if (val.is<std::string>())
						{
							tableStrs << QString(R"(>     ["%1"] = (string)%2,)").arg(qkey).arg(util::toQString(val));
						}
						else if (val.is<sol::table>())
						{
							tableStrs << QString(R"(>     ["%1"] = %2,)").arg(qkey).arg("table");
						}
						else
						{
							tableStrs << QString(R"(>     ["%1"] = %2,)").arg(qkey).arg("unknown");
						}

					}
					tableStrs << "> }";
				}
				else
				{
					tableStrs << tr("> (unknown type of data)");
				}
				tableStrs << ">";
			}
		}

		luadebug::logExport(s, tableStrs, 0);
	} while (false);

	emit finished();

	long long currentIndex = getIndex();
	SignalDispatcher& signalDispatcher = SignalDispatcher::getInstance(currentIndex);
	emit signalDispatcher.scriptFinished();
}