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
#include <gamedevice.h>
#include "signaldispatcher.h"

long long CLuaPet::setState(long long petIndex, long long state, sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	return gamedevice.worker->setPetState(--petIndex, static_cast<sa::PetState>(state));
}

long long CLuaPet::drop(long long petIndex, sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	return gamedevice.worker->dropPet(--petIndex);
}

long long CLuaPet::rename(long long petIndex, std::string name, sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	QString qname = util::toQString(name);

	return gamedevice.worker->setPetFreeName(--petIndex, qname);
}

long long CLuaPet::learn(long long petIndex, long long fromSkillIndex, long long toSkillIndex, sol::object ounitid, sol::object odialogid, sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	long long modelid = -1;
	long long unitid = -1;
	if (ounitid.is<long long>())
		modelid = ounitid.as<long long>();

	QString npcname = "";
	if (ounitid.is<std::string>())
		npcname = util::toQString(ounitid);

	if (modelid != -1 || !npcname.isEmpty())
	{
		sa::map_unit_t unit = {};
		if (gamedevice.worker->findUnit(npcname, sa::kObjectNPC, &unit, "", modelid))
		{
			unitid = unit.id;
		}
	}

	long long dialogid = 0;
	if (odialogid.is<long long>())
		dialogid = odialogid.as<long long>();

	return gamedevice.worker->learn(--petIndex, --fromSkillIndex, --toSkillIndex, dialogid, unitid);
}

long long CLuaPet::swap(long long petIndex, long long from, long long to, sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	return gamedevice.worker->petitemswap(--petIndex, --from, --to);
}

long long CLuaPet::deposit(long long petIndex, long long unitid, long long dialogid, sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	return gamedevice.worker->depositPet(--petIndex, dialogid, unitid);
}

long long CLuaPet::withdraw(long long petIndex, long long unitid, long long dialogid, sol::this_state s)
{
	sol::state_view lua(s);
	GameDevice& gamedevice = GameDevice::getInstance(lua["__INDEX"].get<long long>());
	if (gamedevice.worker.isNull())
		return FALSE;

	luadebug::checkOnlineThenWait(s);
	luadebug::checkBattleThenWait(s);

	return gamedevice.worker->withdrawPet(--petIndex, dialogid, unitid);
}