/**
 * @file track.cpp
 *
 * Implementation of functionality tracking what the mouse cursor is pointing at.
 */
#include "track.h"

#include <SDL.h>

#include "cursor.h"
#include "engine/point.hpp"
#include "player.h"

namespace devilution {

namespace {

bool sgbIsScrolling;
Uint32 sgdwLastWalk;
bool sgbIsWalking;

} // namespace

static bool RepeatMouseAttack(bool leftButton)
{
	if (pcurs != CURSOR_HAND)
		return false;

	Uint32 *timePressed;
	MouseActionType lastAction;
	if (leftButton) {
		if (sgbMouseDown != CLICK_LEFT)
			return false;
		timePressed = &lastLeftMouseButtonTime;
		lastAction = lastLeftMouseButtonAction;
	} else {
		if (sgbMouseDown != CLICK_RIGHT)
			return false;
		timePressed = &lastRightMouseButtonTime;
		lastAction = lastRightMouseButtonAction;
	}

	if (lastAction != MouseActionType::Attack && lastAction != MouseActionType::Attack_MonsterTarget && lastAction != MouseActionType::Attack_PlayerTarget && lastAction != MouseActionType::Spell && lastAction != MouseActionType::Spell_ComplainedAboutMana)
		return false;

	if (Players[MyPlayerId]._pmode != PM_DEATH && Players[MyPlayerId]._pmode != PM_QUIT && Players[MyPlayerId].destAction == ACTION_NONE && SDL_GetTicks() - *timePressed >= (Uint32)gnTickDelay * 4) {
		bool rangedAttack = Players[MyPlayerId].UsesRangedWeapon();
		*timePressed = SDL_GetTicks();
		switch (lastAction) {
		case MouseActionType::Attack:
			if (cursmx >= 0 && cursmx < MAXDUNX && cursmy >= 0 && cursmy < MAXDUNY)
				NetSendCmdLoc(MyPlayerId, true, rangedAttack ? CMD_RATTACKXY : CMD_SATTACKXY, { cursmx, cursmy });
			break;
		case MouseActionType::Attack_MonsterTarget:
			if (pcursmonst != -1)
				NetSendCmdParam1(true, rangedAttack ? CMD_RATTACKID : CMD_ATTACKID, pcursmonst);
			break;
		case MouseActionType::Attack_PlayerTarget:
			if (pcursplr != -1 && !gbFriendlyMode)
				NetSendCmdParam1(true, rangedAttack ? CMD_RATTACKPID : CMD_ATTACKPID, pcursplr);
			break;
		case MouseActionType::Spell:
		case MouseActionType::Spell_ComplainedAboutMana:
			CheckPlrSpell(true);
			break;
		case MouseActionType::Other:
		case MouseActionType::None:
			break;
		}
	}

	return true;
}

void track_process()
{
	if (RepeatMouseAttack(true) || RepeatMouseAttack(false))
		return;

	if (!sgbIsWalking)
		return;

	if (cursmx < 0 || cursmx >= MAXDUNX - 1 || cursmy < 0 || cursmy >= MAXDUNY - 1)
		return;

	const auto &player = Players[MyPlayerId];

	if (player._pmode != PM_STAND && !(player.IsWalking() && player.AnimInfo.GetFrameToUseForRendering() > 6))
		return;

	const Point target = player.GetTargetPosition();
	if (cursmx != target.x || cursmy != target.y) {
		Uint32 tick = SDL_GetTicks();
		int tickMultiplier = 6;
		if (currlevel == 0 && sgGameInitInfo.bRunInTown != 0)
			tickMultiplier = 3;
		if ((int)(tick - sgdwLastWalk) >= gnTickDelay * tickMultiplier) {
			sgdwLastWalk = tick;
			NetSendCmdLoc(MyPlayerId, true, CMD_WALKXY, { cursmx, cursmy });
			if (!sgbIsScrolling)
				sgbIsScrolling = true;
		}
	}
}

void track_repeat_walk(bool rep)
{
	if (sgbIsWalking == rep)
		return;

	sgbIsWalking = rep;
	if (rep) {
		sgbIsScrolling = false;
		sgdwLastWalk = SDL_GetTicks() - gnTickDelay;
		NetSendCmdLoc(MyPlayerId, true, CMD_WALKXY, { cursmx, cursmy });
	} else if (sgbIsScrolling) {
		sgbIsScrolling = false;
	}
}

bool track_isscrolling()
{
	return sgbIsScrolling;
}

} // namespace devilution
